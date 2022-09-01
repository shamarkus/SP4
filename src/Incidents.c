#include "DatabaseRecord.h"
#include "DisabledIncidents.h"
#include "Incidents.h"
#include "main.h"
#include "TypeOfIncident.h"
//#include "Threshold.h"
/*------------------------------------------------------
**
** File: Incidents.c
** Author: Matt Weston & Richard Tam
** Created: August 31, 2015
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - Changed #include files
**                      - Introduced checkEnabledDisabled method
**                      - created parseTCSSwitchover method
**                      - created parseRemoteLinkStatusChanged method
**                      - modified readInLogFile method
**                      - removed getDateFromFileName method
**                      - created reassignTrackCircuitLocations method
**                      - introduced processInfo method
**                      - modified readInFiles method
**
** 23 Mar 2016: Rev 2.1 - RTam
**                      - Modified containsErrorMessage method for detecting TCS Switchover alarms
**                      - TCSSO now requires both "PRIMARY -- SWITCHOVER COMPLETED" and "TCS" to be present
**                      - Added new incident types: PNLSO - Switchover of panel server
**													ORSSO - Switchover of oracle server
**													COMSO - Switchover of communication server
**													WBSSO - Switchover of WBSS server
**													SERVSO - Switchover of any other type of server
**						- Renamed parseTCSSwitchover method to parseServerSwitchover method - now used for all server switchover alarms
**						- Modified parseServerSwitchover method to be insensitive to the length of server type (i.e. "PNL" vs "WBSS")
**
** 15 Sep 2016: Rev 2.2 RC1 - MTedesco
**                          - Added new incident types: FCUAS - FCU channel fail
**                                                  FCUCF - FCU communication fault
**                                                  RSO - red signal overrun
** 16 Sep 2016: Rev 2.2 RC2 - MTedesco
**                          - Added feature to report subway line and hostname of server switchover and PTSLS incidents
 * 3 Oct 2016: Rev 2.2 RC3  - MTedesco
 *                          - Added new "summary email" feature - sends out a summary email of how many incidents occurred at a location
 *                              after the email time delay expires (new function sendSummaryEmails)
 *                          - ProcessInfo is no longer passed incident name
**
**
** 10 Oct 2017: Rev 3.0		-DVasile
**				Put all incidents into a config file that is now read
**				in when the program runs so it knows what to look for.
**				Added/changed the follwing functions to deal with this change
**
**				-getPrevServer
**				-checkEnabledDisabled
**				-containsErrorMessage
**				-switchServerNames
**				-getSpecialKeyword
**				-containsKeywords
**				-parseIncident
**				-parseKeywords
**				-processInfo
**
**				Updated HostnameLUT to inculde the new cbtc portion of
**				the YUS line
**
** 15 Oct 2018: Rev 4.0 - IMiller
**                      - Fixed function readInRecords() to include alarms with commas (Redmine Issue #1579)
**                      - Bug fix to only email first occurence on an on-train incident (Redmine Issue #1324)
**
** 29 Jan 2019: Rev 4.1 - IMiller
**                      - Renamed function incidentDuringRevenueHours() to within_revenue_hours()
** 26 June 2022: Rev 5.0 - MKarlov
** 			- Implemented objects for Incident_Types
**
*/

//lookup table for connecting uip to hostname
static const struct HostnameLUT lookup[] = 
{	
	{ "TCS_1A", "TCCYAS11", "TCCAS01", "TCCYATC11" },
	{ "TCS_1B", "INCYAS11", "INCAS01", "INCYATC11" },
	{ "ORS_1A", "TCCYAS12", "TCCAS02", "TCYATC12" },
	{ "ORS_1B", "INCYAS12", "INCAS02", "INYATC12" },
	{ "COM_1A", "TCCYAS13", "TCCAS03", "COM_1A" },
	{ "COM_1B", "INCYAS13", "INCAS03", "COM_1B" },
	{ "PNL_1A", "TCCYAS26", "TCCYAS26", "TCCYAS26" },
	{ "PNL_1B", "TCCYAS27", "TCCYAS27", "TCCYAS27" },
	{ "WBSS_1A", "TCCWBSS01", "TCCWBSS01", "TCCWBSS01" },
	{ "WBSS_1B", "INCWBSS01", "INCWBSS01", "INCWBSS01" },
};

//figures out the previous server that the server was on, and saves it in the other variable.
//	incident	-- The incident who's previous server will be figured out and stored in the other variable
//	return		-- void
void getPrevServer(struct Incident* incident)
{
	//if the new server is ends with "1A"
	if(contains(incident->data,"1A"))
	{
		int i = 0;
		while(*(incident->data + i) != '1')
		{
			*(incident->other + i) = *(incident->data + i);
			i++;
		} 
		//append "1B" to the end of the previous server
		*(incident->other + i) = '1';
		*(incident->other + i + 1) = 'B';
	}
	//otherwise if the current server had _1B
	else if(contains(incident->data,"1B"))
	{
		int i = 0;
		while(*(incident->data + i) != '1')
		{
			*(incident->other + i) = *(incident->data + i);
			i++;
		} 
		//append 1A to the end of the new server
		*(incident->other + i) = '1';
		*(incident->other + i + 1) = 'A';
	}
	else
	{
		incident->other = NULL;
		printf("Error : Could not identify server for finding previous server");
	}
}

// Initiate field for Incident List
//	il	-- Incident List to be created
//	return	-- void
void createIncidentList(struct IncidentList* il) {
	il->head = NULL;
	il->tail = il->head;
	il->count = 0;
}

// Standard linked-list Queue style insert at the tail of the list
//	il	-- The incidentList to have the new incident inserted into
//	in	-- The incident to be inserted
//	return	-- void
void insertIntoIncidentList(struct IncidentList* il, struct Incident* in) {
	in->next = NULL; // just to be sure.
	if(il->head == NULL) {
		il->head = in;
		il->tail = in;
		il->count = 1;
	}
	else {
		il->tail->next = in;
		il->tail = in;
		il->count++;
	}
}

// Incidents (failures at certain tracks, switches or servers) can be disabled 
// via the file "./Other/Disabled_Incidents.txt"
// This method will check if a databaseRecord (a struct within a DatabaseList)
// stores the times of an incident that has been disabled and returns a value
// indicating whether or not it is disabled
//	dil	-- The list of disabled incidents
//	in	-- The incident that is being check against the list of disabled incidents
//	return	-- An int macro which states emails disabled or enabled (why isn't this a BOOL?)
int checkEnabledDisabled(struct IncidentList* dil, struct Incident* in) {
	struct Incident* din = dil->head;
	char* location;
	char* data;
	if(in->location == NULL)
	{
		location = "NULL";
	}
	else
	{
		location = in->location;
	}
	if(in->data == NULL)
	{
		data = "NULL";
	}
	else
	{
		data = in->data;
	}
	while(din != NULL)
	{
		//check to see the incidentTypes are the same
		if(strcmp(din->typeOfIncident,in->typeOfIncident) == 0)
		{
			//if the location is null and data is null, then the entire
			//incident is disabled.
			if(din->data == NULL && din->location == NULL)
			{
				return EMAILS_DISABLED;
			}
			//if data is null but location isn't then all incidents at that location
			//of that incidentType are disabled
			else if(din->data == NULL && din->location != NULL)
			{
				if(strcmp(din->location,location) == 0)
				{
					return EMAILS_DISABLED;
				}
			}
			//if the incident has no location
			else if(din->data != NULL && din->location == NULL)
			{
				if(strcmp(din->data,data) == 0)
				{
					return EMAILS_DISABLED;
				}
			}
			//if the disabled incident has both a location and data
			else if(din->data != NULL && din->location != NULL && (strcmp(in->typeOfIncident,din->typeOfIncident) == 0))
			{
				if(strcmp(din->data,in->data) == 0 && strcmp(din->location, location) == 0)
				{
					return EMAILS_DISABLED;
				}
			}
		}
		din=din->next;
	} // end of while loop for incidentList
	return EMAILS_ENABLED;
}

// use for debugging purpose and to present output to the user in a friendly way
//	il		-- The incident list to be printed
//	typeOfIncident	-- The string description of the incident list. ie; TF or CDF
//	return		-- void
void printIncidentList(struct IncidentList* il, char* typeOfIncident) {
	struct Incident* tmp = il->head;
	if(il->count > 0) {
		while(tmp != NULL) {
      if(typeOfIncident==NULL || strcmp(typeOfIncident,tmp->typeOfIncident)==0) {
        // during debugging, fields would occasionally be NULL and cause segfaults
        // in execution. This style of printing out was done to circumvent them.
        // Left in in case problems reappear
        tmp->typeOfIncident != NULL ? printf("Type:%s|", tmp->typeOfIncident) : printf("Type:NULL");
        tmp->location != NULL ? printf("Loc:%s|", tmp->location) : printf("Loc:NULL|");
        tmp->data != NULL ? printf("Data:%s|", tmp->data) : printf("Data:NULL|");
        tmp->other != NULL ? printf("Other:%s|", tmp->other) : printf("Other:NULL|");

        if(tmp->timeElement != NULL) {
            printf(ctime(&(tmp->timeElement->timeObj)));
        }
        else {
            printf("\n");
        }
      }	
      tmp = tmp->next;
    }
	}
	else {
		printf("IncidentList is empty: count = %d\n", il->count);
	}
}

// remove an incident from the list and return a pointer to it. 
// Incident will be removed from the head
//	il	-- The incident list which will have it's head item removed from list
//	in	-- Supposed to be the return (probably???) but it doesn't seem whoever wrote
//		   This function realized that the changes made to in don't persist after
//		   the function is exited. Doesn't affect the program so im not changing it - DVasile
//	return	-- void
void removeFromIncidentList(struct IncidentList* il, struct Incident* in) {
	in = il->head;
	if(il->count > 1) {
		il->head = il->head->next;
		il->count--;
		in->next = NULL; // ensure complein sever from the list
	}
	else if(il->count == 1) {
		il->head = NULL;
		il->tail = il->head;
		il->count = 0;
		in->next = NULL; // ensure complein sever from the list
	}
	else { // count is 0
		il->count=0; // just a precaution
	}
}

// remove an incident from the head of the list and destroy it
//	il	-- The incident List that will lose it's head
//	return	-- void
void removeAndDestroyIncident(struct IncidentList* il) {
	struct Incident* in = il->head;
	if(il->count > 1) {
		il->head = il->head->next;
		il->count--;
		in->next = NULL; // ensure complete sever from the list
	}
	else if(il->count == 1) {
		il->head = NULL;
		il->tail = il->head;
		il->count = 0;
		in->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		il->count=0; // just a precaution
	}
        if(in->timeElement != NULL) {
            free(in->timeElement);
        }
        if(in->location != NULL) {
            free(in->location);
        }
        if(in->data != NULL) {
            free(in->data);   
        }	
        if(in->other != NULL) {
            free(in->other);
        }
	if(in->extra != NULL)
	{
		free(in->extra);
	}
	if(in->subwayLine != NULL)
	{
		free(in->subwayLine);
	}
	free(in->incidentType);
   	free(in->typeOfIncident);
	free(in);
}
//returns the count of the incident list
//	il	-- The incident list who's count will be returned
//	retunr 	-- The count of the incident list
int getCountOfIncidentList(struct IncidentList* il) {
	return il->count;
}

// While the list is not empty, remove and destroy the head element. Lastly
// destroy the IncidentList object. 
//	il	-- The incident list which will be deleted
//	return	-- void
void destroyIncidentList(struct IncidentList* il) {
	while(il->count > 0) {
		removeAndDestroyIncident(il);
	}
	free(il);
}

// lines read in from the logfiles are prefaced by some number of linux characters that are
// not relevant to the info in the line. This method is design ONLY for reading
// in lines from the log files as it automatically removes those characters.
//	fl	-- The file from which the line will be read
//	line	-- a double pointer string that will be rewritten to hold the line that is read in
//	size	-- the size of the string in line
//	return	-- An int indicating success or failure
int readInLineAndErase(FILE* fl, char** line, size_t size) {
	// Initialize line
	char tempLine[STRING_LENGTH];
	int i = 0;
	//clear both tempLine and the line read in
	while(*((*line) + i))
	{
		*((*line) + i) = '\0';
		i++;
	}
	i = 0;
	for(i = 0; i < STRING_LENGTH; i++)
	{
		tempLine[i] = '\0';
	}
	i = 0;
	int character = getc(fl);
  
/*   while the characters being read in are not end of line characters or EOF
     marker keep reading. A 'proper' file will have an '\n' character at the end
     of the last last, followed by an EOF marker.
     therefore, if this method is called again after the last line has been read
     in it should return EOF right away with a counter value of 0.
     HOWEVER, due to currently unclear circumstances (old technology is believed
     to be the culprit) ocassionally the last line of an 'active' file (one that 
     is still being written to by CSS) may only be half complete before writing 
     stops.
  
   EX:
     00:00:00 06/14/15 TRAIN R109FV235118A TRIGGERED APPROACH AT York Mills
     00:00:00 06/14/15 LOCATION York Mills INDICATION - SIGNAL 38 STOP
     00:00:02 06/14/15 TRAIN R114VF231157A TRIGGERED APPROACH AT Eglinton
     00:00:02 06/14/15 LOCATION Eglinton INDICATION - SIGNAL 40 STOP
     00:00:02 06/14/15 TRAIN R107FV234541A TRIGGERED DEPARTURE AT Eglinton
     00:00:02 06/14/15 LOCATION Eglinton INDICATION - SIGNAL 1 STOP
     00:00:02 06/14/15 TRAIN R128FV230409A TRIGGERED DEPARTURE AT Glencairn
     00:00:02 06/14/15 TRAIN R128FV230409A TRIGGERED APPROACH AT Glencairn
     00:00:02 06/14/15 LOCATION Glencairn INDICATION - SIGNAL 2 STOP
     00:00:03 06/14/15 LOCATION St. Clair West INDICATION - SPECIAL SSCW6TATE ON
     00:00:04 06/14/15 LOCATION St. Clair West INDICATION - SPECIAL SSCW6TATE OFF
     00:00:04 06/14/15 TRAIN R112VF230705A TRIGGERED ARRIVAL AT Lawrence
     00:00:04 06/14/15 LOCATION Lawrence INDICATION - SIGNAL 4 CLEAR
     00:00:06 06/14/15 LO

     In this case, the last line will not have and '\n' char before the EOF and
     is considered a 'suspicious entry' because the counter would not be 0. 
     Because it may complete once writing to the file recommences it is not a 
     reliable marker as the 'last read line' in the searching and reading method 
     so it is ignored.
     
     Regardless of where the EOF follows a '\n' character or not, END_OF_FILE is
     returned by this function when it is found.
*/
  
	while(character!='\n' && character!=EOF) {
		// add char to array of chars
            	tempLine[i] = (char)character;
		i++;
		// array may not be big enough to handle line,
		// if so it is reallocated to twice its size
		if(i == size-1) {
			*line = (char*)realloc(*line, size*2);
			size*=2;
		}
		character = getc(fl);
	}

	// if the end of line character was reached, determine what kind of ending it
	// was. e.g. suspicious of regular
	if(character == EOF) {
		
		// if the counter, i, is 0, then the file was ended normally, if it was not 0
		// the last line may only be a partial one or one that was not written out
		// properly
		if(i > 0) {	
			printf("Suspicious entry from readInLineAndErase, line read in and EOF character not prefaced by endofline character. This will be ignored for this iteration\n");
			printf("%s\n", (*line));
			return STRANGE_END_OF_FILE;
		}
		else {
			return END_OF_FILE;
		}
	}
	
	char* cleanedLine = tempLine;
	
	//move tempLine pointer to the first digit to get rid of junk characters
	while(isdigit((int)(*cleanedLine)) == 0 && i > 0)
	{
		cleanedLine++;
	}

	// Most of the time code will follow this execution path
	if(i > 0) {
	    //copy string over
		i = 0;
		while(*(cleanedLine + i))
		{
			*((*line) + i) = *(cleanedLine + i);
			i++;
		}		
		return READ_IN_STRING;
	}
	else {
		printf("No string read\n");
		return NO_STRING_READ;
	}
}

// checks if the char* str, contains a incident that the program is looking for, and if
// it does, then the program will fill out the incidentType variable with the revelant info
// (the keywords, incident name short code (ie TF), email templates, etc)
//	str			-- The string read in from the log file
//	msg			-- A string that will hold the short name for the type of incident that was found
//	incidentTypeList	-- A list of all the incidentTypes that the program read in
//	incidentType		-- A double pointer to an incidentType, this incidentType will be filled
//				   If an incident is found in the string str
//	return			-- TRUE if an incident was found, FALSE otherwise
BOOL containsErrorMessage(char* str, char* msg,struct IncidentTypeList* incidentTypeList,struct IncidentType** incidentType) {
	
    struct IncidentType* incidentTypeListTraveller = incidentTypeList->head;
    BOOL found;
    
    
    while(incidentTypeListTraveller != NULL)
    {
	//check if the line contains all the keywords for the incidentType
        found = containsKeywords(incidentTypeListTraveller,str);
        if(found)
        {
	    printf("Found Error %s in message %s\n",incidentTypeListTraveller->typeOfIncident,str);
            //clear msg
            int i = 0;
            while(*(msg + i))
            {
                *(msg + i) = '\0';
                i++;
            }
            //copy incident code into msg
            i = 0;
            while(*(incidentTypeListTraveller->typeOfIncident + i))
            {
                *(msg + i) = *(incidentTypeListTraveller->typeOfIncident + i);
                i++;
            }
            //copy incidentType
	    *incidentType = (struct IncidentType*)malloc(sizeof(struct IncidentType));
            **incidentType = *incidentTypeListTraveller;
            //end loop
            incidentTypeListTraveller = NULL;
        }
        else
        {
            incidentTypeListTraveller = incidentTypeListTraveller->next;
        }
    }
    
    return found;
    
}

// Reads in a CSS log file and adds relevant incidents to the IncidentList, il,
// that is passed in. Depending on the value of preLastReadLine, the function
// may skip some entries that have already been read in and processed.
//	filePath	-- The file path to the log file to be read
//	preLastReadLine -- A string of the last line that was read last time the program ran, can be NULL
//	il		-- Incident List to be filled with all the newly found incidents
//	newLastReadLine -- A string that will hold the new last read line in this log file
//	filterTimes	-- An array of filter times that specify when revenue hours end/begin
//	disabledList	-- A list of all disabled incidents
//	disabledIncidentList	-- A second list of more disabled incidents
//	spl		-- A list of station names that all incidents will be checked against to change station names
//	subwayLine	-- The name of the subway line, ie; YUS
//	incidentTypeList-- A list of all incident types that the program read in, and which will be looked for
//	return		-- An int that indicates a sucessful reading or a failed reading
int readInLogFile(char* filePath, char* preLastReadLine, 
  struct IncidentList* il, char* newLastReadLine, 
  struct FilterTime* filterTimes[7], struct IncidentList* disabledList,
  struct DisabledIncidentList* disabledIncidentList,
  struct StationPairList* spl, char* subwayLine,
  struct IncidentTypeList* incidentTypeList) {
	
  FILE *logFile;
  logFile = fopen(filePath, "r");

  if(logFile != NULL) {
    // tmp will hold the lne that is read in from the file such as:
    // " �    02:19:06 06/14/15 LOCATION Warden SWITCH 15A CRITICAL DETECTION FAILURE"
    char* tmp  = (char*)calloc(STRING_LENGTH, sizeof(char));
		
    // errorMsg is the type of error that has been found in the log files
    // CDF, CTDF, TF or PTSLS
    char* errorMsg = (char*)calloc(STRING_LENGTH, sizeof(char));
		
    // result of readin in a line
    int lineRes;
    
       
    // If the previously last read line for this file is "" or NULL then there
    // is no previously last read line and we should begin reading in from the 
    // very start of the file.
    // If it is not then we must search until we find the previously last read
    // line and begin reading in from the line after that.
    if( !(preLastReadLine==NULL || strcmp(preLastReadLine,"")==0) ) {
      // lineRes will be the result of reading in the line, while
      // tmp will hold the string that was read in.
      lineRes = readInLineAndErase(logFile, &tmp, STRING_LENGTH);
			
      // While the searching function has not found the previously last read
      // string and has not reached the end of the file, keep searching
      while( lineRes!=END_OF_FILE && 
        lineRes!=STRANGE_END_OF_FILE && 
        strcmp(tmp,preLastReadLine)!=0 ) {
        
        lineRes = readInLineAndErase(logFile, &tmp, STRING_LENGTH);
      }
      
      // if the searching algorithm has completed, but the string read in is
      // not a match for the previously last read line, then the searching
      // algorithm must have reached the end of the file and not found it.
      if( strcmp(tmp, preLastReadLine) != 0 ) { 
        // lineRes must be END_OF_FILE or STRANGE_END_OF_FILE, 
        // line was not found
        return LINE_NOT_FOUND; 
      }
    }
    
    // this will be the first 'new' line.
    // tmp could be longer then STRING_LENGTH if a previous array reallocated it 
    // to be double the old size, but for insurance this will ensure arrays do 
    // not go outside their bounds.
    lineRes = readInLineAndErase(logFile, &tmp, STRING_LENGTH); 
    //printf("First 'new' line is: %s\n", tmp);
		
    if(lineRes==END_OF_FILE || lineRes==STRANGE_END_OF_FILE) {
      // 2 case as this point
      
      // 1 the file HAS NOT been read from before (this is the first time) and 
      // the file is simply blank. The first line read from it will return
      // END_OF_FILE
      // In this case the previously last read line from this file should
      // logically be NULL or "" and the new lastReadLine from the file is also
      // set to "" to indiciate nothing was ever read from it.
      if( preLastReadLine==NULL || strcmp(preLastReadLine,"")==0 ) {
        strcpy(newLastReadLine, "");
      }
      
      // 2 If the file HAS NOT been written to at all since the last time it was
      // read from then lineRes will be END_OF_FILE as there are no lines after
      // the previously last read16 line
      // However, we still have A lastReadLine that we must begin from next time
      // so the new last read line will be the same as the old last read line
      else { // a file exists, but we are at the end, so keep a reference to the last entry.
        strcpy(newLastReadLine, preLastReadLine); 
      }
    }

    // If the file HAS been written to since the last time it was read from or 
    // if this is a new file that is NOT EMPTY then lineRes will be 
    // READ_IN_STRING
    while(lineRes!=END_OF_FILE && lineRes!=STRANGE_END_OF_FILE) {
      
      if(lineRes==READ_IN_STRING) { // STRANGE_END_OF_FILE will not be handled because they are susceptible to changes
        // if lineRes is READ_IN_STRING then we can be sure the string is NOT
        // one of the partial lines that is considered 'suspsicious' by the 
        // function readInLineAndErase, so it is a reliable lastReadLine marker
        // to save for the next time this tool is ran.
        strcpy(newLastReadLine, tmp);
        
        struct IncidentType* incidentType;
        
        if(containsErrorMessage(tmp, errorMsg, incidentTypeList,&incidentType)) {
          // construct incident
          struct Incident* in = malloc(sizeof(struct Incident));
          in->timeElement = malloc(sizeof(struct TimeElement));
          in->location = (char*)calloc(STRING_LENGTH, sizeof(char));
          in->data = (char*)calloc(STRING_LENGTH, sizeof(char));
          in->typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));
          strcpy(in->typeOfIncident, incidentType->typeOfIncident);
          in->other = (char*)calloc(STRING_LENGTH, sizeof(char));
      	  in->extra = (char*)calloc(STRING_LENGTH, sizeof(char));
      	  in->subwayLine = (char*)calloc(LINE_LENGTH, sizeof(char));
      	  strcpy(in->subwayLine, subwayLine);
      	  in->incidentType = incidentType;
          in->next = NULL;
	
          //get the data from the incident line
          parseIncident(incidentType,in,tmp);
          //reassign track locations from WBSS ---> Conventinal
      	  reassignTrackCircuitLocations(in, spl);
      	  //checks if this incident needs to figure out the previous server it was on
      	  if(contains(incidentType->processingFlags,GET_PREVIOUS_SERVER_FLAG))
      	  {
      		getPrevServer(in);
      	  }
      	  //checks flag if this is a server event, changes name and saves subwayLine as location
      	  if(contains(incidentType->processingFlags,SERVER_NAME_SWITCH_FLAG))
      	  {
      		//copy subwayLine into location for the server incident
      		int i = 0;
      		if(*in->location == '\0')
      		{		
      			while(*(subwayLine + i))
      			{
      				*(in->location + i) = *(subwayLine + i);
      				i++;
      			}
      		}
      		//switches server names depending on the subwayLine
      		  switchServerNames(in,subwayLine);
      	  }
          else if(contains(incidentType->processingFlags,SERVER_RELATED_INCIDENT)) 
	  {
		strcpy(in->other,subwayLine);
	  }
	  else 
	  {
		  ;//do nothing
	  }
      	  //if statment is :
      	  //( (RevenueHours OR NOT-containsRevenueCheck) AND (checkDisabled1 AND checkDisabled2 ) )
      	  if( ( within_revenue_hours(in, filterTimes) || !contains(incidentType->processingFlags,REVENUE_HOUR_TIME_CHECK_FLAG) ) && (checkEnabledDisabled(disabledList, in)==EMAILS_ENABLED) && (checkEnabledDisabled2(disabledIncidentList, in) == EMAILS_ENABLED) )
      	  {
      	  	insertIntoIncidentList(il,in);
      	  }
      	  else
      	  {
         	//MEMORY
         		free(in->data);
         		free(in->location);
         		free(in->other);
			free(in->extra);
			free(in->subwayLine);
         		free(in->typeOfIncident);
         		free(in->timeElement);
			free(in);
			free(in->incidentType);
      	  }
        }	// end of containsErrorMessage(tmp, errorMsg) check
      } // end of lineRes==READ_IN_STRING check
      else {
        printf("ERROR in logs - lineRes is %d, tmp is %s\n", lineRes, tmp);
      }
      // reallocate tmp to the 'small' size
      // it could have been made larger for a previous line, but no longer needs
      // to be that big
      free(tmp);
      tmp = (char*)calloc(STRING_LENGTH,sizeof(char)); 
      // read in next line and repeat
      lineRes = readInLineAndErase(logFile, &tmp, STRING_LENGTH);
	
    }    // end of while loop
    free(tmp);
    free(errorMsg);
    if(EOF == fclose(logFile)) {
      printf("Cannot close logFile |%s|. Program will continue.\n", filePath);
      printf("errno = %d\n and strerror is %s\n", errno, strerror(errno));
      return ERROR;
    }
    else {
      return lineRes;
    }
  }
  else {
    printf("Cannot open log file |%s|. File will be skipped", filePath);
    printf("errno = %d\n and strerror is %s\n", errno, strerror(errno));
    return ERROR;
  }
}

// Determine the mode of operation that should be used when opening and reading
// a file. 
//	rc	-- A record that stores the last read line and which file it was read from
//	return	-- An int that indicates the mode the program should read the log file in
int getMode(struct Record* rc) {
  // if there is no file that was previously read from
  // then the software has likely never run before and should operate in
  // START_UP mode
  if( rc->fileName == NULL && rc->lastReadLine == NULL) {
    return START_UP;
  }

  // If there is a file that was previously read from but the lastReadLine is
  // empty, then the file was empty and should operate in that mode
  else if( rc->fileName != NULL && rc->lastReadLine == NULL) {
    return PREVIOUS_FILE_WAS_EMPTY;
  }
  // if there is a previous file and a previous last read line then that mode
  // be operated in.
  else if( rc->fileName != NULL && rc->lastReadLine != NULL) {
    return MIDDLE_OF_PREVIOUS_FILE;
  } 
  else {
    return START_UP; // No fileName available to start from
  }
}

// function to read in ALL necessary log files. 
// If code has ran previously and is in the middle of an hour then likely only 
// 1 file will be read from
// If it is the start of a new hour then 2 files, last hour's and this hour's
// will both be handled.
// Lastly, if the tool is running for the first time, the last 24 hours worth 
// of files will be handled.
//	il		-- The incident list all new incidents will be stored in
//	incidentTypeList-- The list of all incidents to be looked for
int readInFiles(struct IncidentList* il,struct IncidentTypeList* incidentTypeList) {
  // Overhead stuff
  char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
  int i=0;
  
  // filterTimes are read in from the file "./Other/filterTimes.txt" and are
  // used to filter our incidents that occur during non-revenue hours as a 
  // result of track work and are not relevant to safety concerns.
  struct FilterTime* filterTimes[DAYS_OF_WEEK];
  readInFilterTimes(filterTimes);
 
  // user-side help and debugging
  printf("filter times\n"); 
  for(i=0; i<7; i++) {
    printf("%d - %d:%d,%d:%d\n", i, filterTimes[i]->startTime.tm_hour, filterTimes[i]->startTime.tm_min, 
      filterTimes[i]->endTime.tm_hour, filterTimes[i]->endTime.tm_min);
  }
  
  // DISABLED INCIDENTS LIST
  // Any incident can be disabled, either at a specific location such as
  // a Critical Detection Failure at 99T at Warden or broadly such as disabling
  // all Track Failures. Disabled Incidents are read in from the file 
  // ".Other/Disabled_Incidents.txt" and stored in a linked-list of structs.
  // This is the same structure as 'incidentList' is stored in.
  struct IncidentList* disabledList = malloc(sizeof(struct IncidentList));
  createIncidentList(disabledList);
  readInDisabledIncidents(disabledList);
  
  printf("Disabled List\n");
  printIncidentList(disabledList, NULL);
  printf("\n");

  // DISABLED LIST for temporary incidents
  struct DisabledIncidentList* disabledIncidentList = malloc(sizeof(struct DisabledIncidentList));
  createDisabledIncidentList(disabledIncidentList);
  readInDisabledIncidents2(disabledIncidentList);
 
  printf("Disabled List (Temporary incidents, before name change)\n");
  printDisabledIncidentList(disabledIncidentList);
  printf("\n");

  changeStationNames(disabledIncidentList);
   
  printf("Disabled List (Temporary incidents, after name change)\n");
  printDisabledIncidentList(disabledIncidentList);
  printf("\n");


  //Track Circuit Reassign List
  struct StationPairList* spl = malloc(sizeof(struct StationPairList));
  createStationPairList(spl);
  readInTrackCircuitLocationReassignmentList(spl);
  printf("Track Circuit Reassign List\n");
  printStationPairList(spl);
  printf("\n");

  // Get Records (i.e. the last read lines of each file and the file name)
  // reads are read in from the file ./Other/records.txt"
  struct Record* recordsList[NUM_OF_FOLDERS];
  readInRecords(recordsList);
  
  printf("records\n");
  for(i=0; i<NUM_OF_FOLDERS; i++) {
    printf("%d - %s,%s\n", i, recordsList[i]->fileName, recordsList[i]->lastReadLine); 
  }
  printf("\n");

  // Database file for new records. This file will eventually be used to 
  // overwrite the original database of records
  char* filePath2 = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath2, OTHER, RECORDS2, DOT_TXT);
  FILE *newRecords = fopen(filePath2, "w");
  if(NULL == newRecords) {
    printf("There was an error trying to open a new records folders. filepath is |%s|\n", filePath2);
    printf("errno = %d, strerror is %s\n", errno, strerror(errno));
    //clean up before returning error
    free(tmp);
    destroyStationPairList(spl);
    destroyDisabledIncidentList(disabledIncidentList);
    destroyIncidentList(disabledList);
    for(i = 0; i<NUM_OF_FOLDERS;i++) {
      free(recordsList[i]->fileName);
      free(recordsList[i]->lastReadLine);
      free(recordsList[i]);
    }
    for(i = 0;i<DAYS_OF_WEEK;i++) {
      free(filterTimes[i]);
    }
    return ERROR;
  } 
 
  // The location of the TCS-A, TCS-B, etc folders on the TTC's server could
  // chnage,. to prepare for this, the folders extensions are not hard-code
  // but are read in from the folder "./Other/LogFolderPaths"
  // these folders must correspond (i.e. 2 to 2, 1 to 1, etc) with the numbers
  // in the records.txt file for succesfuly execution.
  // the folders are read in and stored in an array of strings
  char* filePath3 = (char*)calloc(STRING_LENGTH, sizeof(char)); 
  constructLocalFilepath(filePath3, OTHER, LOG_FOLDER_PATHS, DOT_TXT);
  FILE *logFolderPathsFile = fopen(filePath3, "r");
  if(NULL == logFolderPathsFile) {
    printf("There was an error trying to open the log folder paths folder. filepath is |%s|\n", filePath3);
    printf("errno = %d, strerror is %s\n", errno, strerror(errno));
    //clean up before returning error
    free(tmp);
    destroyStationPairList(spl);
    destroyDisabledIncidentList(disabledIncidentList);
    destroyIncidentList(disabledList);
    for(i = 0; i<NUM_OF_FOLDERS;i++) {
      free(recordsList[i]->fileName);
      free(recordsList[i]->lastReadLine);
      free(recordsList[i]);
    }
    for(i = 0;i<DAYS_OF_WEEK;i++) {
      free(filterTimes[i]);
    }
    free(filePath3);
    return ERROR;
  }
 
  char logFolderPathsList[NUM_OF_FOLDERS][STRING_LENGTH];
  char logFolderSubwayLine[NUM_OF_FOLDERS][5];
  i=0;
  
  //null terminate all strings since 4 of the six subwaylines names are only 3 characters long
  //and the other 2 are 4 characters long
  for(i = 0; i < NUM_OF_FOLDERS; i++)
  {
      logFolderSubwayLine[i][5] = '\0';
      logFolderSubwayLine[i][4] = '\0';
  }
  
  i = 0;
  
  while(i<NUM_OF_FOLDERS) {
    int lineRes = readInLine(logFolderPathsFile, &tmp, STRING_LENGTH);
    if(lineRes == READ_IN_STRING || lineRes == STRANGE_END_OF_FILE) {
      int index = atoi(strtok(tmp,","))-1;
      strcpy(logFolderSubwayLine[index], strtok(NULL,","));
      strcpy(logFolderPathsList[index], strtok(NULL, ","));
      i++;
    }
  } 
  if(EOF == fclose(logFolderPathsFile)) {
    printf("log folder paths file cannot be closed filepath is |%s|\n", filePath3);
    printf("errno = %d, strerror is %s\n", errno, strerror(errno));
    //clean up before returning error
    free(tmp);
      destroyStationPairList(spl);
    destroyDisabledIncidentList(disabledIncidentList);
    destroyIncidentList(disabledList);
    for(i = 0; i<NUM_OF_FOLDERS;i++) {
      free(recordsList[i]->fileName);
      free(recordsList[i]->lastReadLine);
      free(recordsList[i]);
    }
    for(i = 0;i<DAYS_OF_WEEK;i++) {
      free(filterTimes[i]);
    }
    free(filePath3);
    return ERROR;
  }
  
  // Temporary vars to hold the complete extension of the folder that is being
  // read in by the software and to hold that information about where in each
  // folder (file and line) the software stopped reading at.
  char* completeFolder = (char*)calloc(STRING_LENGTH, sizeof(char));
  struct Record* newLastReadLine = malloc(sizeof(struct Record));
  newLastReadLine->fileName = (char*)calloc(DATE_STRING_LENGTH, sizeof(char));
  newLastReadLine->lastReadLine = (char*)calloc(STRING_LENGTH, sizeof(char));
  
  // a string that is passed to system() to copy log files to a temporary folder
  // so this tool does not interfere with them being written to
  char* copyCommand = (char*)calloc(STRING_LENGTH, sizeof(char));
  
  // Loop should run 6 times
  for(i=0; i<NUM_OF_FOLDERS; i++) {
    strcpy(newLastReadLine->fileName, ""); // clear
    strcpy(newLastReadLine->lastReadLine, ""); // clear
    
    if(!(logFolderPathsList[i]==NULL || strcmp(logFolderPathsList[i],"")==0)) {
      
      // based on what was stored for fileName and lastReadLine in
      // recordsList[i], executioin will be different.
      strcpy(tmp,"");
      int mode = getMode(recordsList[i]);
  
      time_t fileDate;
      
      time_t currentTime = time(NULL) - OFFSET*24*60*60;
      //printf("The current time is: "); printf(ctime(&currentTime));
      
      // Set fileDate to 24 hours ago and begin reading in information.
      // this will be done the first time the code runs
      if(mode == START_UP) {
        // start with Data from 24 hours ago
        fileDate = time(NULL) - 1*24*60*60 - OFFSET*24*60*60;
      }
      else {
        // There is a previously read from file that execution can begin with
        // Copy the fileName from the recordsList's index 'i' into the fileName
        // field for the newlastReadLine struct of type Record
        strcpy(newLastReadLine->fileName, recordsList[i]->fileName);
        // get the date from the filename for easier arithmetic and comparing
        getDateFromFileName(newLastReadLine->fileName, &fileDate);
        
        if(mode == MIDDLE_OF_PREVIOUS_FILE) {
          // there is a previously last read line that execution should begin
          // from. Lines before this will have already been read in.
          // copy this line into the lastReadLine field of the struct 
          // newLastReadLine 
          strcpy(newLastReadLine->lastReadLine, recordsList[i]->lastReadLine);
         
          sprintf(completeFolder, "%s%s%s", logFolderPathsList[i], 
            newLastReadLine->fileName, DOT_LOG);	
          sprintf(copyCommand, "cp %s %s", completeFolder, TEMP_FILE);
          system(copyCommand);	
          
          // Attempt to read in the log file and store incidents that are
          // being searched for in the incident, 'il'
          // parsing of lines that contain the error messages that are being
          // searched for will begin after the line recordsList[i]->lastReadLine
          // is found
          // the last lines read in this file will be stored in 
          // newLastReadLine->lastReadLine. This could be the same as 
          // recordsList[i]->lastReadLine if not new entries are present.
          int res = readInLogFile(TEMP_FILE, recordsList[i]->lastReadLine, il, 
            newLastReadLine->lastReadLine, filterTimes, disabledList, disabledIncidentList, spl, logFolderSubwayLine[i],incidentTypeList);
          
          // If the previously last read line for this file cannot be found for
          // some reason, the default behaviour will be to begin at the start of
          // file and treat all lines that contain error messages as though they
          // have not be handled yet.
          // This may cause errors to accidentally appear twice, but none will
          // be missed.
          if(res == LINE_NOT_FOUND) {
            printf("The line, %s, could not be found, beginning at the start of %s\n", 
            recordsList[i]->lastReadLine, completeFolder);
            res = readInLogFile(TEMP_FILE, NULL, il, 
            newLastReadLine->lastReadLine, filterTimes, disabledList, disabledIncidentList, spl, logFolderSubwayLine[i],incidentTypeList);
          }
          // increment fileDate by 1 hour and prepare to try and read the next
          // log file
          fileDate+=1*60*60;
        }
        else if(mode == PREVIOUS_FILE_WAS_EMPTY) {
          // No unique functionality needed. Can immediately enter while loop
        }
      }

      while(fileDate <= currentTime) {
        char* s; //variable for filename - so it can be freed
        sprintf(completeFolder, "%s%s%s", logFolderPathsList[i], s = getFilenameFromDate(fileDate), DOT_LOG);	
        free(s);
        FILE* check = fopen(completeFolder, "r");
        
        // If this folder exists, then make a copy of it, and begin reading
        // if it does not exist, do nothing and increment fileDate
        if(check != NULL) {
          fclose(check);
          sprintf(copyCommand, "cp %s %s", completeFolder, TEMP_FILE);
          system(copyCommand);	
          // read in the next log file and update the newLastReadLine object
          // if ERROR is returned, newLastReadLine->lastReadLine will not have
          // been updated or changed and newLastReadLine->fileName should not be
          // updated either
          int res = readInLogFile(TEMP_FILE, NULL, il, newLastReadLine->lastReadLine, filterTimes, disabledList, disabledIncidentList, spl, logFolderSubwayLine[i],incidentTypeList);
          if(res != ERROR) {
            char* s; //tmp var for getFilenameFromDate
            strcpy(newLastReadLine->fileName, s = getFilenameFromDate(fileDate));
            free(s);
          }
        }
        // increment fileDate by 1 hour and attempt to read the next file
        fileDate += 1*60*60;
      }
      printf("%s\n", logFolderPathsList[i]);
      printf("newLastReadLine->lastReadLine = %s\n", newLastReadLine->lastReadLine);
      printf("newLastReadLine->fileName = %s\n\n", newLastReadLine->fileName);
      
      // Save the new lastReadLine and new last read fileName to records2.txt
      // records.txt is not overwirtten to preserve information in the event of
      // a crash or execution being halted by something 
      if( newLastReadLine->fileName==NULL || (strcmp(newLastReadLine->fileName, "")==0) ) {
        fprintf(newRecords, "%d\n", i+1);
      }
      else if( newLastReadLine->lastReadLine==NULL || (strcmp(newLastReadLine->lastReadLine, "")==0) ) {
        fprintf(newRecords, "%d,%s\n", i+1, newLastReadLine->fileName);
      }
      else {
        fprintf(newRecords, "%d,%s,%s\n", i+1, newLastReadLine->fileName, newLastReadLine->lastReadLine);
      }

      system("rm _temp.log");
    }
    else {
      printf("There appears to be a missing entry in the LogFolderPaths files.\n");
      printf("No entry found corresponding to number '%d'.\n", i+1);
      printf("Please check LogFolderPaths file\n");
    }
  }
    
  // close the records2.txt and rename it records.txt
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath, OTHER, RECORDS, DOT_TXT);
  if(EOF == fclose(newRecords)) {
    printf("New records file cannot be closed. Previous records file will be kept.");
    printf("errno = %d, strerror is %s\n", errno, strerror(errno));  
  } else {
    int y = remove(filePath);
    int x = rename(filePath2, filePath);
    if(x < 0 || y < 0) {
      printf("x = %d, y = %d", x, y);
      printf("errno n = %d, strerror is %s\n", errno, strerror(errno));
    }    
  }
  //free things
  free(tmp);
  free(copyCommand); 
  free(completeFolder);
  free(filePath2);
  free(filePath3);
  destroyStationPairList(spl);
  destroyDisabledIncidentList(disabledIncidentList);
  destroyIncidentList(disabledList);
  for(i = 0; i<NUM_OF_FOLDERS;i++) {
      free(recordsList[i]->fileName);
      free(recordsList[i]->lastReadLine);
      free(recordsList[i]);
  }
  for(i = 0;i<DAYS_OF_WEEK;i++) {
      free(filterTimes[i]);
  }
  free(newLastReadLine->fileName);
  free(newLastReadLine->lastReadLine);
  free(newLastReadLine);
  free(filePath);
}

// Read in file containing incidents that have been disabled due to frequency 
// or non-safety related explanations. Incidents are stored in a linked-list
// or Incident structs and are saved in the file "./Other/Disabled_Incidents"
//	il	-- The incidentList that the disabled incidents will be stored in
//	return	-- void
void readInDisabledIncidents(struct IncidentList* il) {
printf("read in disabled\n");
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath, OTHER, DISABLED_INCIDENTS, DOT_TXT);
  FILE* fp = fopen(filePath, "r");
	
  if(fp != NULL) {
		// tmp is a variable used to hold the lines as they are read in and parsed
    char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
		
    int lineRes = readInLine(fp, &tmp, STRING_LENGTH);
    while(lineRes != END_OF_FILE) {
      // If a string wasn't read in it cannot be parsed. Skip and continue
      if(lineRes == READ_IN_STRING || lineRes == STRANGE_END_OF_FILE) {
        // create Incident struct
        struct Incident* in = malloc(sizeof(struct Incident));
        in->typeOfIncident = calloc(STRING_LENGTH, sizeof(char));
	in->location = calloc(STRING_LENGTH,sizeof(char));
	in->data = calloc(STRING_LENGTH,sizeof(char));
	char* wordHolder;
        in->other = NULL;
	in->extra = NULL;
	in->subwayLine = NULL;
        in->timeElement = NULL;
	wordHolder = strtok(tmp,COMMA);
	if(wordHolder == NULL)
	{
		printf("Error while reading disabled events\n");
	}        
	else
	{
		strcpy(in->typeOfIncident,wordHolder);
		wordHolder = strtok(NULL,COMMA);
		if(wordHolder != NULL)
		{
			strcpy(in->location,wordHolder);
			wordHolder = strtok(NULL,COMMA);
			if(wordHolder != NULL)
			{
				strcpy(in->data,wordHolder);
			}
			else
			{
				free(in->data);
				in->data = NULL;
			}
		}
		else
		{
			free(in->location);
			in->location = NULL;
			free(in->data);
			in->data = NULL;
		}
		insertIntoIncidentList(il,in);
	}

        
	free(tmp);
        tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
      } // end of if
      // read in next line
      lineRes = readInLine(fp, &tmp, STRING_LENGTH);
    }
    if(EOF == fclose(fp)) {
      printf("Could not close file |%s|.\n", filePath);
      printf("errno = %d, strerror is %s\n", errno, strerror(errno));
    }
    free(tmp);
  }
  else {
    printf("The file: '%s' could not be found. No Incidents will be disabled, All incident types will be emailed about\n", filePath);
  }
  //clean things up
  free(filePath);
}

// read in filter times for every day of the week
// these times will be non-revenue hours during which faults and failures
// are ignored because they are do to work and not passneger trains failing
//	ft	-- An array that will hold all the filter times
//	return	-- TRUE if filter times were read in secessfully, FALSE otherwise
BOOL readInFilterTimes(struct FilterTime* ft[7]) {
	int i;
	// initialize them all to having no filter times
	for(i=0; i<7; i++) {
		ft[i]  = malloc(sizeof(struct FilterTime));
		ft[i]->timesExist = FALSE;
	}

	// open file ./Other/filterTimes.txt
	char* filterTimesExtension = (char*)calloc(STRING_LENGTH, sizeof(char));
	constructLocalFilepath(filterTimesExtension, OTHER, FILTER_TIMES, DOT_TXT);
	FILE* filterTimeFile = fopen(filterTimesExtension, "r");

	// string to hold lines as they are read in.
	char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));

	char dayOfWeek[4];
	char startTime[8];
	char endTime[8];

	if(filterTimeFile != NULL) {
		int lineRes = readInLine(filterTimeFile, &tmp, STRING_LENGTH);
	
		while(lineRes != END_OF_FILE) {
			if(lineRes == READ_IN_STRING || lineRes == STRANGE_END_OF_FILE) {
				// lines are stored in the form "0,2:30,5:30" where 0 is Sunday,
				// 1 is Monday, etc
				strcpy(dayOfWeek, strtok(tmp, ","));
				strcpy(startTime, strtok(NULL, ","));			
				strcpy(endTime,   strtok(NULL, ","));
				
				// start time comes first
				struct tm start;
				start.tm_wday = atoi(dayOfWeek);
				start.tm_hour = atoi( strtok(startTime, ":"));
				start.tm_min  = atoi( strtok(NULL, ":"));
				//start.tm_min  = atoi( strtok(NULL, ";"));

				// end time comes second
				struct tm end;
				end.tm_wday = atoi(dayOfWeek);
				end.tm_hour = atoi(strtok(endTime, ":"));
				end.tm_min  = atoi(strtok(NULL, ":"));
				//end.tm_min  = atoi(strtok(NULL, ";"));

				ft[start.tm_wday]->startTime = start;
				ft[start.tm_wday]->endTime = end;
				ft[start.tm_wday]->timesExist = TRUE;
			}
			lineRes = readInLine(filterTimeFile, &tmp, STRING_LENGTH);
		} // end of while loop
		if(EOF == fclose(filterTimeFile)) {
		  printf("Could not close file for filter times. |%s|\n", filterTimesExtension);
		  printf("errno = %d, strerror is %s\n", errno, strerror(errno));
		}
	}
  else {
    printf("could not open file for filter times. |%s|\n", filterTimesExtension);
    printf("errno = %d, strerror is %s\n", errno, strerror(errno));
  }
  //clean things up
  free(filterTimesExtension);
  free(tmp);
}

// read in ./Other/records.txt file and save the last read and last read line
// (if they exist) for each log folder into an array of Record structs
//	recordsList	-- An array in which all the records will be stored
//	return		-- TRUE if the reading was secessful, FALSE otherwise
BOOL readInRecords(struct Record* recordsList[NUM_OF_FOLDERS]) 
{
  char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
  
  // open file
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath, OTHER, RECORDS, DOT_TXT);
  FILE *records = fopen(filePath, "r");
  
  // Initialially assume there is no previously read file or folders.
  // This will assure the tool will start in startup mode if there is indeed no
  // previous files or folders.
  int i=0;
  for(i=0; i < NUM_OF_FOLDERS; i++) {
    recordsList[i] = malloc(sizeof(struct Record));
    recordsList[i]->fileName=NULL;
    recordsList[i]->lastReadLine=NULL;
  }
  
  // check if records.txt even exists, if not, it will be created later in the
  // execution of this tool
  if(records != NULL) {
    for(i=0;i<NUM_OF_FOLDERS;i++) {
      // if string is successfully read in
      int lineRes = readInLine(records, &tmp, STRING_LENGTH); 
      if(READ_IN_STRING == lineRes || STRANGE_END_OF_FILE == lineRes) {
        // records will have an index 1, 2, 3, 4, 5, or 6. This corresponds to 
        // the indexes seen in the LogFolderPaths.txt file
        // the index will be the first character in the line
        int index;
        
        // if there is only 1 comma in the line than there is only an index
        // and a fileName (the file was empty)
        if(getCharCount(tmp, ',') == 1) {
          index = atoi(strtok(tmp,","))-1;
          recordsList[index]->fileName = (char*)calloc(STRING_LENGTH, sizeof(char));
          strcpy(recordsList[index]->fileName, strtok(NULL,","));
         
          recordsList[index]->lastReadLine=NULL;
        }
        
        // if the comma count is 2 than there is an index, a fileName and a 
        // lastReadLine (the program last read the file midway through its an
        // hour while it was being written to
        else if(getCharCount(tmp, ',') >= 2) {
          index = atoi(strtok(tmp,","))-1;
          recordsList[index]->fileName = (char*)calloc(STRING_LENGTH, sizeof(char));
          strcpy(recordsList[index]->fileName, strtok(NULL,","));
          
          recordsList[index]->lastReadLine = (char*)calloc(STRING_LENGTH, sizeof(char));
          strcpy(recordsList[index]->lastReadLine, strtok(NULL,"\n"));                  //delimiter chaged to '\n' (Redmine Issue #1579)
        }
        else {
          // there is no comma, only an index, and there is no previous
          // fileName or lastReadLine.
          index = atoi(tmp)-1;
          recordsList[index]->fileName=NULL;
          recordsList[index]->lastReadLine=NULL;
        }
      }
    }
  
    
    if(EOF == fclose(records)) {
      printf("Records file cannot be closed. Records should have been read-in\n");
      printf("errno = %d, strerror is %s\n", errno, strerror(errno));  
    }
  }
  else {
    printf("Records file cannot be open. Records should be null.\n");
    //clean things up before error
    free(tmp);
    free(filePath);
    return FALSE;
  }
  //clean things up
  free(tmp);
  free(filePath);
}

// check to see if an incident occured during revenue hours or working hours 
// if it was during working hours, then it is ignored.
//	in	-- The incident being checked.
//	ft	-- An array with the beginning and end of revenue hours for each day.
//	return	-- FALSE if outside of revenue Hours, TRUE if within revenue hours (most incidents).
BOOL within_revenue_hours(struct Incident* in, struct FilterTime* ft[7]) {
  struct tm* inTime = localtime(&(in->timeElement->timeObj));

  if( ft[inTime->tm_wday]->timesExist == TRUE ) {
    BOOL afterStartTime = (ft[inTime->tm_wday]->startTime.tm_hour < inTime->tm_hour) ||
      ((ft[inTime->tm_wday]->startTime.tm_hour == inTime->tm_hour) && 
      (ft[inTime->tm_wday]->startTime.tm_min <=  inTime->tm_min));
		
    BOOL beforeEndTime  = (ft[inTime->tm_wday]->endTime.tm_hour > inTime->tm_hour) ||
      ((ft[inTime->tm_wday]->endTime.tm_hour == inTime->tm_hour) && 
      (ft[inTime->tm_wday]->endTime.tm_min >=  inTime->tm_min));

		return !(afterStartTime && beforeEndTime);
	}
	else {
		return TRUE;
	}
}

// Certain tracks sections or switches are considered to be in work yards from the point
// of view WBSS. However, practically, they are on the mainline and revenue trains
// use them daily.

// 'reassignTrackCircuitLocations' will change the location and data field of certain
// tracks and switches to fit better with practical circumstances. It will use a .txt file
// which contains a list of track sections and switches in pairs to rename the location and
// data fields.
//	in	-- The incident who's location will be checked against spl to be changed
//	spl	-- A list of all stations who's names need to be change if found
//	return	-- TRUE if the name was changed, FALSE otherwise
BOOL reassignTrackCircuitLocations(struct Incident* in, struct StationPairList* spl) { 
  // return FALSE if the name was not changed
  // return TRUE if it was changed
  
  // If the data or location field is NULL or empty
  // than it cannot be reassigned. Immediately return FALSE
  if(in->location==NULL || strcmp(in->location, "")==0) {
    return FALSE;
  }
   if(in->data== NULL || strcmp(in->data, "")==0) {
    return FALSE;
  }

  
  BOOL locMatch;
  BOOL dataMatch;
  struct StationPair* sp = spl->head;
  // cycle through the StationPairList and check if
  // it contains the Incident 'in'. If it does, change
  // the Incident's fields and return true.  
  while(sp != NULL) {
    // if the 'convName' field of the StationPair matches the 
    // name of the location for the disbled incident, copy the
    // proper wbss name into it
    locMatch = strcmp(in->location, sp->location2)==0;
    dataMatch = strcmp(in->data, sp->data2)==0;
    if(locMatch && dataMatch) {
      strcpy(in->location, sp->location1);
      strcpy(in->data, sp->data1);
      return TRUE;
    }
    sp=sp->next;
  }
  
  // Location and data matching 'in' was not found in 
  // the StationPairList. Do not modify 'in'. return FALSE.
  return FALSE;
}

//Determines which Incidents are head incidents, and which incidents are tail incidents
//Accounting only for eligible incidents
//Necessary for determing the start and end of HTML tables
void determineBgnEndIncidents(struct DatabaseRecord* dr,struct Threshold* th,struct DatabaseList* databaseList, struct ThresholdList* thresholdList){

    struct DatabaseRecord* prevdr=databaseList->head; 

    while(dr != NULL) {
      while(th != NULL) {
        struct TimeElement* start;
        int thresholdResult = checkThresholdCondition(th, dr, &start);
        int emailStatus = checkEmailStatus(dr);

        if(thresholdResult==CONDITION_MET && emailStatus==EMAIL_NOT_SENT_YET)
        {
		dr->lastRecord = TRUE;
		if( prevdr != dr){
			prevdr->lastRecord = FALSE;
			prevdr=dr;
		}
	}
	th=th->next;
      }
      th = thresholdList->head;
      dr=dr->next;
   }
}
// Read in ThresholdList and DatabaseList associated with this type of incident
// and merge the new incidents that have been read in from the log files into
// the databaseList. Check Threshold, disabled status and if this incident has 
// been emailed about before and if these conditions are all met, prepare an
// email to send.
//	typeOfIncident	-- A stirng that contains the short name of the incident
//	incidentList	-- A list with all the found incidents
//	emailInfoList	-- A list with the email info of all possible recipients
//	sel		-- A list for summary emails (incidents added to this list if they need a summary email)
//	incidentType	-- The type of incident that will be checked this run through process info
//	return		-- void 
struct DatabaseList* processInfo(char* typeOfIncident, struct IncidentList* incidentList, struct EmailInfoList* emailInfoList, struct SummaryEmailList* sel, struct IncidentType* incidentType, struct CCPair* ccPairLLHead){
	printf("--------------------START--------------------\n\n");
    printf("Type: %s\n", typeOfIncident);

    // THRESHOLD LIST
    // gets unique threshold list for the type of incident.
    struct ThresholdList* thresholdList = incidentType->thresholdList;
    printf("Threshold List\n");
    printThresholdList(thresholdList);
    printf("\n");
    
    // DATABASE LISTS
    // Database list files are read in from the folder "./Database_Files/" and
    // are unique for each type of incident
    struct DatabaseList* databaseList = malloc(sizeof(struct DatabaseList)); 
    createDatabaseList(databaseList);

    //If onboard incident
    if(contains(incidentType->processingFlags, ONBOARD_INCIDENT_FLAG))
    {
      databaseList->isOnBoardIncident = TRUE;
    }
    else
    {
      databaseList->isOnBoardIncident = FALSE;
    }
    readInDBFile(typeOfIncident, databaseList, getExpiringTime(thresholdList), sel, incidentType);

    // Incidents of the same type as 'databaseList' are added to database list.
    // databaseList will later be printed out to a file and replace the old
    // database file.
    addIncidentsToDatabaseList(typeOfIncident, databaseList, incidentList);
    
    printf("Database List after merge, before sending emails\n");
    printDatabaseList(databaseList);
    printf("\n");
 
    // Check Thresholds and Email Status and if Disabled
    // Prepare .html documents
    struct DatabaseRecord* dr = databaseList->head;
    struct Threshold* th = thresholdList->head;
    
    determineBgnEndIncidents(dr,th,databaseList,thresholdList);

    dr = databaseList->head;
    th = thresholdList->head;
    databaseList->headerExists = FALSE; 

    while(dr != NULL) {
      while(th != NULL) {
        struct TimeElement* start;
        int thresholdResult = checkThresholdCondition(th, dr, &start);
        int emailStatus = checkEmailStatus(dr);

        // if the database record has met a threshold condition, is not disabled 
        // and has not already been emailed about then it is added to the
        // html files of each person is 'emailInfoList' who is supposed to
        // receive emails about this type of incident
        if(thresholdResult==CONDITION_MET && emailStatus==EMAIL_NOT_SENT_YET)
        {
         addIncidentToEmail(dr, th, emailInfoList, start, incidentType, ccPairLLHead, databaseList->headerExists);
	 databaseList->headerExists = TRUE;
         strcpy(dr->flag->msg, EMAIL);
         dr->flag->timeOfEmail = time(NULL) - OFFSET*24*60*60;
        }
        th=th->next;
      } // end of threshold conditions while loop
      th = thresholdList->head;
      dr=dr->next;
    } // end of databaseList while loop
    printDatabaseToFile(typeOfIncident, databaseList);
    destroyDatabaseList(databaseList);
    printf("--------------------FINISH-------------------\n\n\n");
    return databaseList;
} 

// sets the linked list to its default setting - empty
//	incidentTypeList	- The Incident Type List to be created
//	return			- Void
void createIncidentTypeList(struct IncidentTypeList* incidentTypeList)
{
    incidentTypeList->head = NULL;
    incidentTypeList->tail = NULL;
    incidentTypeList->count = 0;
}

//Alteration of the gcc strtok function
//Strtoke will return a null-terminating '\0' character if 2 delimeters exist side by side
//Necessary for incident types without objects 
//E.G: ACPO;;LOCATION ...
char* strtoke(char *str, const char *delim){
      static char *start = NULL; /* stores string str for consecutive calls */
      char *token = NULL; /* found token */
      /* assign new start in case */
      if (str)
      {
	      start = str;
      }
      /* check whether text to parse left */
      if (!start)
      {
	      return NULL;
      }
       /* remember current start as found token */
      token = start;
      /* find next occurrence of delim */
      start = strpbrk(start, delim);
      /* replace delim with terminator and move start to follower */
      if (start)
      {
	      *start++ = '\0';
      }
      /* done */
      return token;
}

// In the folder "Other/" the "Incident_Types.txt" config file exists with all the incident
// types that the user wishes to look for. This function first checks the file by calling
// the CSS_Tool_Validation_Program to check the file for validity, and if it passes the
// check, it then reads in the entire file and saves the incidentTypes in the incident type list
//	incidentTypeList	-- The list in which all read incidentsTypes will be saved
//	return			-- An int indicating success or failure
int readInIncidentTypes(struct IncidentTypeList* incidentTypeList)
{
    char* incidentTypeFilePath = (char*)calloc(STRING_LENGTH,sizeof(char));
    char* lineRead = (char*)calloc(LONG_STRING_LENGTH,sizeof(char));
    int returnCode = NO_ERROR;
    //construct file path and open file
    constructLocalFilepath(incidentTypeFilePath,OTHER,INCIDENT_TYPES_FILE,DOT_TXT);
    FILE* incidentTypeFile = fopen(incidentTypeFilePath, "r");
    
    char* validationCommand = (char*)calloc(STRING_LENGTH,sizeof(char));
    sprintf(validationCommand,"./CSS_Tool_Validation_Program");
    int validationFlag = -1;
    
    validationFlag = system(validationCommand);
    
    free(validationCommand);
    
    if(validationFlag != 0)
    {
        printf("Error : Validation Program Could Not Validate Incident Type Config File\n");
        return ERROR;
    }
    
    //check to see if the file is there
    if(incidentTypeFile != NULL)
    {
        int i = 0;
		char read = getc(incidentTypeFile);
		while(read != '\n' && read != '\0')
		{
			*(lineRead + i) = read;
			i++;
			read = getc(incidentTypeFile);
		}
		

        while(*lineRead != '\0')
        {
            if(!(lineRead[0] == '/' && lineRead[1] == '/') && read != '\0' && read != EOF) {
                
                struct IncidentType* incidentType = (struct IncidentType*)malloc(sizeof(struct IncidentType));
                printf("%s\n",lineRead);
                //allocate memory for the incident
                incidentType->typeOfIncident = (char*)calloc(STRING_LENGTH,sizeof(char));
                incidentType->object = (char*)calloc(STRING_LENGTH,sizeof(char));
                incidentType->emailTemplate = (char*)calloc(STRING_LENGTH,sizeof(char));
                incidentType->processingFlags = (char*)calloc(STRING_LENGTH,sizeof(char));
                incidentType->keywordList = (struct KeywordList*)malloc(sizeof(struct KeywordList));
                incidentType->next = NULL;
			        	incidentType->summaryTemplate = calloc(STRING_LENGTH,sizeof(char));
                incidentType->thresholdList = (struct ThresholdList*)malloc(sizeof(struct ThresholdList));
                incidentType->emailTemplateLocation = (char*)calloc(STRING_LENGTH,sizeof(char));
                
                char* parseToolKeywords;
                char* parseToolThresholds;
                //gets the incident type code; ie: CTDF
              //  int delimCount = 0;
              //  for(int i=0;i<strlen(lineRead);i++){
              //    if(lineRead[i]== ';') delimCount++;
              //  }

                strcpy(incidentType->typeOfIncident, strtoke(lineRead,SEMI_COLON));
                
               // sprintf(command,"awk -F\";\" -v pat=\"%s\" \'$1~pat{print $2;exit}\' \"%s\"",incidentType->typeOfIncident,objectFilePath); //include exit
               // FILE* file = popen(command,"r");
               // fgets(incidentType->object,20,file);
               // pclose(file);

                strcpy(incidentType->object, strtoke(NULL,SEMI_COLON));

                //ie: CRITIAL TRAIN DETECTION FALIURE,LOCATION,TRACK 
                parseToolKeywords = strtoke(NULL,SEMI_COLON);
                //gets the summary email header template that; ie: Summary for critical train detection failure at \L switch \K
	        strcpy(incidentType->summaryTemplate,strtoke(NULL,SEMI_COLON));
                //gets the email message main line structure; ie: "A CRITIAL TRAIN DETECTION FAILURE happened at \L on track \I 
                strcpy(incidentType->emailTemplate, strtoke(NULL,SEMI_COLON));
                //gets the email message template for the LOCATION coloum in the table
                strcpy(incidentType->emailTemplateLocation,strtoke(NULL,SEMI_COLON));
                //gets the thresholds for the incident type; ie: [1,20] (1 time for 20 minutes)
                parseToolThresholds = strtoke(NULL,SEMI_COLON);
                //gets the processing flags; ie: R - check for revenue hours
	       	//but since processingFlags can be empty, and be the end of the line,
	       	//we need to first check that the char* returned from strtok is not null
	       	char* null_check = strtoke(NULL,SEMI_COLON);
	       	if(null_check != NULL)
	       	{
	       		strcpy(incidentType->processingFlags,null_check);
	       	}
                //parses the keywords and inserts them into the linked list;
                //ie: CRITIAL TRAIN DETECTION FALIURE,LOCATION,TRACK ->
                //are put into three different elements in the linked list
                parseKeywords(incidentType,parseToolKeywords);
                //parses the thresholds string and puts them into the linked list
                parseThresholds(incidentType,parseToolThresholds);
                
                
                //used for debugging
                printIncidentType(incidentType);

				addToIncidentTypeList(incidentTypeList,incidentType);
            }
			//clear and reallocate space for the line and read in the next line
			if(!(read == '\0' || read == EOF) )
			{
				free(lineRead);
				lineRead = (char*)calloc(LONG_STRING_LENGTH,sizeof(char));
				i = 0;
				read = (char)getc(incidentTypeFile);
				while(read != '\n' && read != '\0' && read != EOF)
				{
					*(lineRead + i) = read;
					i++;
					read = getc(incidentTypeFile);
				}
			}
        }
		
    } else {
        printf("Error:No \"Incident_Types.txt\" File exits, cannot run program; Exiting...");
        returnCode = ERROR;
    }
    
    //free used memory and return
	if(incidentTypeFile != NULL)
	{
		fclose(incidentTypeFile);
    }
	if(incidentTypeFilePath != NULL)
	{
		free(incidentTypeFilePath);
    }
	if(lineRead != NULL)
	{
		free(lineRead);
    }
	return returnCode;
}

// adds the incidentType to the end of the incidentTypeList linked list
//	incidentTypeList	-- The list the incidentType will be added to
//	incidentType		-- The incident to be added
//	return			-- void
void addToIncidentTypeList(struct IncidentTypeList* incidentTypeList, struct IncidentType* incidentType)
{
    if(incidentTypeList->count == 0)
    {
        incidentTypeList->head = incidentType;
        incidentTypeList->tail = incidentType;
        incidentType->next = NULL;
        incidentTypeList->count++;
    } else {
        incidentTypeList->tail->next = incidentType;
        incidentTypeList->tail = incidentType;
        incidentType->next = NULL;
        incidentTypeList->count++;
    }
}
//prints the incidentType to the console, in a meaningful fashion
//	incidentType	-- The incident type to be printed
//	return		-- void
void printIncidentType(struct IncidentType* incidentType)
{
    printf("IncidentType: %s\n",incidentType->typeOfIncident);
    printf("and has proccessing flags : %s and keywords : \n", incidentType->processingFlags);
    struct Keyword* keyword = incidentType->keywordList->head;
    while(keyword != NULL)
    {
        printf("%s\n", keyword->word);
        keyword = keyword->next;
    }
    printThresholdList(incidentType->thresholdList);
    printf("The email structure is :\n%s\nThe email location structure is:\n%s\n",incidentType->emailTemplate,incidentType->emailTemplateLocation);
    printf("--End of IncidentType--\n");
}

//removes all incidentTypes and frees the allocated memory
//	incidentTypeList	-- The incident type list to be deleted
//	return			-- void
void deleteIncidentTypeList(struct IncidentTypeList* incidentTypeList)
{
    struct IncidentType* incidentTypeListTraveller = incidentTypeList->head;
    while(incidentTypeListTraveller != NULL)
    {
        struct IncidentType* incidentType = incidentTypeListTraveller;
	//free pointers that are part of incidentType
        free(incidentTypeListTraveller->emailTemplate);
        free(incidentTypeListTraveller->object);
        free(incidentTypeListTraveller->summaryTemplate);
        free(incidentTypeListTraveller->typeOfIncident);
        free(incidentTypeListTraveller->emailTemplateLocation);
        free(incidentTypeListTraveller->processingFlags);
	//delete the lists (threshold and keyword)
        deleteThresholdList(incidentTypeListTraveller->thresholdList);
        deleteKeywordList(incidentTypeListTraveller->keywordList);
	//go to next incidentType
        incidentTypeListTraveller = incidentTypeListTraveller->next;
	//free the incidentType pointer
        free(incidentType);
    }
    //free the threshold list pointer
    free(incidentTypeList);
    incidentTypeList = NULL;
}
//initalizes a keywordList
//	keywordList	-- The keyword list to be created
//	return		-- void
void createKeywordList(struct KeywordList* keywordList)
{
    keywordList->head = NULL;
    keywordList->tail = NULL;
    keywordList->count = 0;
}
//adds a keyword to the keyword list
//	keywordList	-- The list to have the keyword added to
//	keyword		-- The keyword to add
//	return		-- void
void addToKeywordList(struct KeywordList* keywordList, struct Keyword* keyword)
{
    //if the keyword list has no elements yet
    if(keywordList->count == 0)
    {
	//set the keyword as the head and tail
        keywordList->head = keyword;
        keywordList->tail = keyword;
        keywordList->count++;
        keyword->next = NULL;
    } else {
	//add the keyword in as the new tail element
        keywordList->tail->next = keyword;
	//change the tail ponter to point to the new keyword
        keywordList->tail = keyword;
        keyword->next = NULL;
        keywordList->count++;
    }
}
//deletes the keyword list
//	keywordList	-- The keyword list to delete
//	return		-- void
void deleteKeywordList(struct KeywordList* keywordList)
{
    struct Keyword* keywordListTraveller = keywordList->head;
    struct Keyword* keyword;    

    while(keywordListTraveller != NULL)
    {
        keyword = keywordListTraveller;
        free(keywordListTraveller->word);
        keywordListTraveller = keywordListTraveller->next;
        free(keyword);
    }
    
    free(keywordList);
    keywordList = NULL;
}
//splits up all the keywords from the keyword line, and saves them in the incidentType's
//keyword list.
//	incidentType	-- The incident type in which the keywords will be stored
//	keywords	-- The string that contains all the keywords
int parseKeywords(struct IncidentType* incidentType, char* keywords)
{
    char* keywordHolder;
    keywordHolder = (char*)strtok(keywords,KEYWORD_SEPERATOR_TOKEN);
    //while there are still keywords to be saved
    while(keywordHolder != NULL)
    {
        struct Keyword* keyword = (struct Keyword*)malloc(sizeof(struct Keyword));
        keyword->word = (char*)calloc(STRING_LENGTH,sizeof(char));
	//copy keyword
        strcpy(keyword->word,keywordHolder);
        //save keyword
        addToKeywordList(incidentType->keywordList,keyword);
        //get next keyword
        keywordHolder = (char*)strtok(NULL,KEYWORD_SEPERATOR_TOKEN);
    }
}
//parses the thresholds from a string, and saves them in a linked list that is then
//stored in the incidentType struct.
//	incidentType	-- The incident type in which the thresholds will be stored
//	thresholds	-- The string that contains all the thresholds
int parseThresholds(struct IncidentType* incidentType, const char* thresholds)
{
    //char used to check string character by character
    char  parser;
    BOOL  EndOfLine = FALSE;
    int   parserOffset = 0;
    //reads the entire line
    while(EndOfLine == FALSE)
    {
        struct Threshold* threshold = (struct Threshold*)malloc(sizeof(struct Threshold));
        //finds the beginning of the threshold : formated [#,#] ie [1,250]
        parser = *(thresholds + parserOffset);
        parserOffset++;
	//end of line check
        if(parser == '\0')
        {
	    free(threshold);
            EndOfLine = TRUE;
        } else {
            
            char* numHolder = (char*)calloc(STRING_LENGTH,sizeof(char));
            int   numHolderOffset = 0;
            //gets the first # (runs untill it reaches a comma
            while(parser != COMMA_CHAR)
            {
		//saves the character in the num holder string
                *(numHolder + numHolderOffset) = *(thresholds + parserOffset);
                parserOffset++;
                numHolderOffset++;
                parser = *(thresholds + parserOffset);
            }
            //turns the number from a char array to an int and reallocates the memory for numHolder
            threshold->numOfIncidents = atoi(numHolder);
            free(numHolder);
            numHolder = (char*)calloc(STRING_LENGTH,sizeof(char));
            numHolderOffset = 0;
            //since that search ends with the pointer pointing to the comma, move 1 over
            parserOffset++;
            //gets the second # (runs untill it sees a closing bracket)
            while(parser != ']')
            {
		//saves the character in numHolder
                *(numHolder + numHolderOffset) = *(thresholds + parserOffset);
                parserOffset++;
                numHolderOffset++;
                parser = *(thresholds + parserOffset);
            }
	    //since the search ends with the pointer pointing to a closing bracket,
	    //move to the next character
            parserOffset++;
	    //get the # from the string
            threshold->numOfMinutes = atoi(numHolder);
	    //add threshold to the list
            insertIntoThresholdList(incidentType->thresholdList,threshold);
            free(numHolder);
        }
    }   
}
//a function that takes in an incidentType, and incident and a incident message line
//from the log file, and reads in any keywords it needs to, as defined by
//the incidentType.
//	incidentType	-- The incident type of the error message that is contained in line
//	incident	-- The incident in which all parsed info will be stored
//	line		-- The line to be parsed
//	return		-- void
void parseIncident(struct IncidentType* incidentType,struct Incident* incident,char* line)
{
    //for some reson, the function that trims the first 4 useless characters 
    //doesnt work, it only trims 3... so maunal trimming
    //declare variables
    struct Keyword* prev = NULL;
    struct Keyword* current;
    struct Keyword* next;
    //set initial variable states 
    current = incidentType->keywordList->head;
    next = current->next;
    getDateFromString(line,&(incident->timeElement->timeObj));
    
    while(current != NULL)
    {
        //checks that the current keyword is a special keyword
        if(current->word[0] == '\\')
        {
            //if the special keyword indicates "data"
            if(current->word[1] == 'K')
            {
		//check to see if the keyword pointer is, so it doesn't try
		//to derefence a null pointer
                if(prev == NULL)
		{
			getSpecialKeyword(line,NULL,next->word,incident->data);
		}
		else if (next == NULL)
		{
			getSpecialKeyword(line,prev->word,NULL,incident->data);
		}
		else
		{
			getSpecialKeyword(line,prev->word,next->word,incident->data);
		}
            }
            //if the special keyword indicates "location"
            else if (current->word[1] == 'L')
            {
		//check to see if the keyword pointer is, so it doesn't try
		//to dereference a null pointer
                if(prev == NULL)
		{
			getSpecialKeyword(line,NULL,next->word,incident->location);
		}
		else if (next == NULL)
		{
			getSpecialKeyword(line,prev->word,NULL,incident->location);
		}
		else
		{
			getSpecialKeyword(line,prev->word,next->word,incident->location);
		}
            }
            //if the special keyword indicates "other"
            else if (current->word[1] == 'E')
            {
		//check to see if the keyword pointer is, so it doesn't try
		//to derefence a null pointer
		if(prev == NULL)
		{
			getSpecialKeyword(line,NULL,next->word,incident->other);
		}
		else if (next == NULL)
		{
			getSpecialKeyword(line,prev->word,NULL,incident->other);
		}
		else
		{
			getSpecialKeyword(line,prev->word,next->word,incident->other);
		}
            }
            //if the special keyword indicates "extra"
            else if (current->word[1] == 'X')
            {
		//check to see if the keyword pointer is, so it doesn't try
		//to derefence a null pointer
                if(prev == NULL)
		{
			getSpecialKeyword(line,NULL,next->word,incident->extra);
		}
		else if (next == NULL)
		{
			getSpecialKeyword(line,prev->word,NULL,incident->extra);
		}
		else
		{
			getSpecialKeyword(line,prev->word,next->word,incident->extra);
		}
            }
        }
        //get next keywords
        prev = current;
        current = current->next;
	//since next will become null 1 iteration sooner than current, and the 
	//loop is based off of current, this check is needed
        if(next != NULL)
        {
            next = next->next;
        }
    }
    
    
}


// find the number of occurences of a single character in a string
// mainly used for commas and semi-colons when parsing
//	str	-- The string to be searched in
//	ch	-- The character to be looked for
//	return	-- The number of times ch was found in str
int getCharCount(char* str, char ch) {
	char* s = str;
	int i;
	int count=0;
	for(i=0; i < strlen(str); i+=sizeof(char))
	{
		s[i] == ch ? count++ : count;
	}
	return count;
}
//functions that checks if an incident message from the log file
//contains all the keywords of an incident type
//	incidentType	-- The incident type who's keywords are being checked for
//	msg		-- The string which is being checked
//	return		-- TRUE if all keywords are found, FALSE otherwise
BOOL containsKeywords(struct IncidentType* incidentType,const char* msg)
{
    BOOL found = TRUE;
    struct Keyword* keyword = incidentType->keywordList->head;
    //if the message isn't null
    if(!(msg == NULL || *msg == '\0'))
    {
	//travel through the keyword linked list untill a keyword
	//the end of the list is reached or a keyword was not found in the message
        while(found && keyword != NULL)
        {
	    //if the keyword is not a special keyword
	    if(keyword->word[0] != '\\')
	    {  
		    //check that the keyword exists
	            found = contains(msg,keyword->word);
	    }
	    //go to the next keyword
	    keyword = keyword->next;
        }
    }
    else
    {
        found = FALSE;
    }
    
    return found;
}
//returns a bool that indicates if the substring was found within the string
//	string		-- The string in which the substring will be searched for
//	substring	-- The substring to be searched for
//	return		-- TRUE if the substring was found, FALSE otherwise
BOOL contains(const char* string,const char* substring)
{
    BOOL found = FALSE;
    int offset = 0;
    //while the end of the string was not reached and the substring was not found
    while(!found && *(string + offset) != '\0')
    {
	//if the characters are the same
        if(*(string + offset) == *substring)
        {
            int i = 0;
	    //while the characters are the same and the substring has not been found
            while(*(string + offset + i) == *(substring + i) && !found)
            {
		i++;
		//if the substring's next character is null
                if(*(substring + i) == '\0')
                {
		    //the string has been found
                    found = TRUE;
                }
            }
        }
        offset++;
    }
    
    return found;
}
//given a line, the previous keyword, the next keyword, and a container,
//this function will save the keyword between the prev and next keywords into
//container 
//	line		-- The string that contains the keyword that is being searched for
//	prev		-- The keyword that comes before the one that's being searched for
//	next		-- The keyword that comes after the one that's being searched for
//	container	-- The string in which the keyword will be written in
//	return		-- A BOOL that indicates if the keyword was found (TRUE), or not (FALSE)
BOOL getSpecialKeyword(const char* line,const char* prev,const char* next,char* container)
{
    char* keywordHolder = (char*)calloc(STRING_LENGTH,sizeof(char));
    //clear container just to be safe
    int i = 0;
    while(*(container + i))
    {
        *(container + i) = '\0';
        i++;
    }
    BOOL found = FALSE;
    //case if the special keyword comes right after the date
    if(prev == NULL)
    {
        //gets the start index of the next keyword
        int endIndex;
        int temp;
	//set the startIndex to the first character of the next keyword
        found = getIndexOfStr(line,next,&endIndex,&temp);
        //safety check that the keyword index was found
        if(found == FALSE)
        {
            printf("Error While looking for %s in string %s\n",next,line);
        }
        else
        {
            i = 0;
            //saves the special keyword in the keywordHolder char*
            while(i <= endIndex)
            {
                *(keywordHolder + i) = *(line + i);
                i++;
            }
        }
        
    }
    //case if the special keyword comes at the end of the line
    else if(next == NULL)
    {
      //gets the end index of the keyword that comes before the special keyword
      int startIndex;
      int temp;
    //set the end index to the last character of the previous keyword 
      found = getIndexOfStr(line,prev,&temp,&startIndex);
      //safety check that the keyword and index were found
      if(found == FALSE)
      {
          printf("Error While looking for %s in string %s\n",prev,line);
      }
      else
      {
        //save the special keyword in the keywordHolder char*
        i = 0;
        while(*(line + i + startIndex) != '\n' && *(line + i + startIndex) != '\0')
        {
            *(keywordHolder + i) = *(line + i + startIndex);
            i++;
        }

        //if incident is a UCCR, accomodate in order to read CC #
        if(contains(keywordHolder, "Reset for Train"))
        {
          char* tmp = calloc(STRING_LENGTH,sizeof(char));
          strcpy(tmp, keywordHolder);
          free(keywordHolder);
          keywordHolder = (char*)calloc(STRING_LENGTH,sizeof(char));
          
          found = getIndexOfStr(tmp,prev,&temp,&startIndex);

          if(found == FALSE)
          {
              printf("Error While looking for %s in string %s\n",prev,line);
          }
          else
          {
            //save the special keyword in the keywordHolder char*
            i = 0;
            while(*(tmp + i + startIndex) != '\n' && *(tmp + i + startIndex) != '\0')
            {
                *(keywordHolder + i) = *(tmp + i + startIndex);
                i++;
            }
          }
          free(tmp);
        }
      }
    }
    //when the special keyword is surrounded by two other keywords
    else
    {
      int startIndex;
      int endIndex;
      int temp;
      //set the endIndex to the first character of the next keyword
      found = getIndexOfStr(line,next,&endIndex,&temp);

      if(found == FALSE)
      {
        printf("Error While looking for %s in string %s\n",next,line);
      }
      //if incident is a UCCR, accomodate in order to read train #
      else if(contains(prev, "CMD CC Reset"))
      {
        char* tmp = calloc(STRING_LENGTH,sizeof(char));

        i = 0;
        while(*(line + i + temp) != '\n' && *(line + i + temp) != '\0')
        {
          *(tmp + i) = *(line + i + temp);
          i++;
        }

        found = getIndexOfStr(tmp,next,&endIndex,&temp);
        
        //set the startIndex to the last character of the previous word
        found = found & getIndexOfStr(tmp,"Train ",&temp,&startIndex);

        if(found == FALSE)
        {
          printf("Error While looking for %s in string %s\n","Train ",tmp);
        }
        else
        {
          i = 0;
          //saves the special keyword in the keywordHolder char*
          while((startIndex + i) <= endIndex)
          {
            *(keywordHolder + i) = *(tmp + startIndex + i);
            i++;
          }
        }
        free(tmp);
      }
      else
      {
        //set the startIndex to the last character of the previous word
        found = found & getIndexOfStr(line,prev,&temp,&startIndex);
        
        if(found == FALSE)
        {
            printf("Error While looking for %s in string %s\n",prev,line);
        }
        else
        {
            i = 0;
            //saves the special keyword in the keywordHolder char*
            while(startIndex + i <= endIndex)
            {
                *(keywordHolder + i) = *(line + startIndex + i);
                i++;
            }
        }
      }
    }
    
    if(found)
    {
    
        i = 0;
        int starting_space_offset = 1;
        //get rid of the first space if there is one
        if(*keywordHolder != ' ')
        {
            starting_space_offset = 0;
        }
	//copy word into the container
        while( *(keywordHolder + i + starting_space_offset) )
        {
            *(container + i) = *(keywordHolder + i + starting_space_offset);
            i++;
        }
	//remove an ending space if there is one
	if(*(container + i - 1) == ' ')
	{
		*(container + i - 1) = '\0';
	}
    
    }
    
    
    
    free(keywordHolder);
    
    return found;
}

//A function that returns the starting index and ending index of a keyword.
//This function will shave off any starting or ending spaces
//	string		-- Line that contains the keyword who's start and end indices are being searched for
//	substring	-- The keyword who's start and end indices are being searched for
//	startIndex	-- A pointer to a variable in which the start index will be stored
//	endIndex	-- A pointer to a variable in which the end index will be stored
//	return		-- A BOOL that indicates whether or not the keyword who's indices 
//			   are being looked for was found(TRUE) or not(FALSE)
BOOL getIndexOfStr(const char* string,const char* substring,int* startIndex,int* endIndex)
{
    BOOL found = FALSE;
    
    int offset = 0;
    
    while(!found && *(string + offset) != '\0')
    {
        if(*(string + offset) == *substring)
        {
	    //if statment to deal with a space or no space:
	    if(*(substring) == ' ')
	    {
            	*startIndex = offset;
	    }
	    else
	    {
		*startIndex = offset - 1;
	    }
            int i = 0;
            while(*(string + offset + i) == *(substring + i) && !found)
            {
                if(*(substring + i + 1) == '\0')
                {
			//if statment to deal with a space or no space:
	    		if(*(substring + i) == ' ')
	    		{
            			*endIndex = offset + i;
	    		}
	    		else
	    		{
				*endIndex = offset + i + 1;
	   		}
			found = TRUE;                
		}
                i++;
            }
        }
        offset++;
    }
    
    return found;
    
}
//fucntion that looks at server names and locations and switches them with
//predefined names in the LUT
//	incident	-- The incident who's server names will be checked against the LUT
//	subwayLine	-- The name of the subway line on which the incident occured
//	return		-- Void
void switchServerNames(struct Incident* incident,char* subwayLine)
{
	//in this function there is an int 'q' that is used for clearing and copying strings.
	//It acts as the index counter for clearing the string that will be copied to, 
	//then it is reset back to 0, and is used as
	//another index counter to copy a string over into the one that was just cleared.
	int i;
	for(i=0 ;i<(sizeof(lookup)/sizeof(lookup[0])); i++) {
		if(strcmp(incident->data,lookup[i].uip) == 0) {
			if(strcmp(incident->location, "YUS") == 0) {
				//clears the data string
				int q = 0;			
				while(*(incident->data + q))
				{
					*(incident->data + q) = '\0';
					q++;
				}
				q = 0;
				//copies string over
				while(*(lookup[i].yusHostname + q))
				{
					*(incident->data + q) = *(lookup[i].yusHostname + q);
					q++;
				}
			}
			else if(strcmp(incident->location, "BDS") == 0) {
				//clears the data string
				int q = 0;			
				while(*(incident->data + q))
				{
					*(incident->data + q) = '\0';
					q++;
				}
				q = 0;
				//copies string over
				while(*(lookup[i].bdsHostname + q))
				{
					*(incident->data + q) = *(lookup[i].bdsHostname + q);
					q++;
				}
			}
			//CBTC is the CBTC controlled portion of the YUS line 
			else if(strcmp(incident->location, "CYUS") == 0) {
				//clears the data string
				int q = 0;			
				while(*(incident->data + q))
				{
					*(incident->data + q) = '\0';
					q++;
				}
				q = 0;
				//copies string over
				while(*(lookup[i].cyusHostname + q))
				{
					*(incident->data + q) = *(lookup[i].cyusHostname + q);
					q++;
				}
			}
		}
	}//end of if for data variable
	i = 0;
	if(*(incident->other) != '\0')//if there is a second server name to change in other variable
	{
		for(i=0 ;i<(sizeof(lookup)/sizeof(lookup[0])); i++) {
            if(strcmp(incident->other,lookup[i].uip) == 0) {
                if(strcmp(incident->location, "YUS") == 0) {
					//clears the data string
					int q = 0;			
					while(*(incident->other + q))
					{
						*(incident->other + q) = '\0';
						q++;
					}
					q = 0;
					//copies string over
					while(*(lookup[i].yusHostname + q))
					{
						*(incident->other + q) = *(lookup[i].yusHostname + q);
						q++;
					}
                }
                else if(strcmp(incident->location, "BDS") == 0) {
					//clears the data string
					int q = 0;			
					while(*(incident->other + q))
					{
						*(incident->other + q) = '\0';
						q++;
					}
					q = 0;
					//copies string over
					while(*(lookup[i].bdsHostname + q))
					{
						*(incident->other + q) = *(lookup[i].bdsHostname + q);
						q++;
					}
                }
				//CBTC is the CBTC controlled portion of the YUS line
				else if(strcmp(incident->location, "CYUS") == 0) {
					//clears the data string
					int q = 0;			
					while(*(incident->other + q))
					{
						*(incident->other + q) = '\0';
						q++;
					}
					q = 0;
					//copies string over
					while(*(lookup[i].cyusHostname + q))
					{
						*(incident->other + q) = *(lookup[i].cyusHostname + q);
						q++;
					}
				}
           	}
		}//end of for loop for other variable
	}	
}


int checkEnabledDisabled2(struct DisabledIncidentList* dil, struct Incident* in) {
  struct DisabledIncident* din = dil->head;
  while(din != NULL) {
    // locmatch will be used to see if in->location is geographically between 
    // din->location1 and din->location2 in later releases, but for Rev 1.0
	// this 'inbetween' functionality does not exist. Only a simple 1-1 check is done.
    BOOL locMatch = strcmp(in->location, din->location1)==0;
    
	// does the time of incident fall during the duration of this disablement?
    BOOL timeMatch = incidentDuringDurationWindow(in, din->duration) == WITHIN_DURATION;
    
	// Is this type of incident disabled by this disablement?
    BOOL typeOfIncMatch = typeOfIncidentListContain(din->typeOfIncidentList, in->incidentType->typeOfIncident);
   
    if(locMatch && timeMatch && typeOfIncMatch ) {
      return EMAILS_DISABLED;
    }
    din=din->next;
  }
  return EMAILS_ENABLED;
}

int incidentDuringDurationWindow(struct Incident* in, struct Duration* dur) {
  if(in->timeElement->timeObj < dur->startTime->timeObj) {
    return BEFORE_DURATION;
  } 
  else if(in->timeElement->timeObj > dur->endTime->timeObj) {
    return AFTER_DURATION;
  }
  else {
    return WITHIN_DURATION;
  }
}

