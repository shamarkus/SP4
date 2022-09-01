#include "DatabaseRecord.h"
#include "TypeOfIncident.h"
#include "Incidents.h"
#include "main.h"
#define DATA "Data"
#define EMAIL_TIME_FOLDER "Other"
#define EMAIL_TIME_FILENAME "LastEmailTime"
#define VERSION "5.0"
#define NUM_OF_CC_PAIRS 99
#define NUM_OF_ATC_PAIRS 25
#define NUM_OF_BDS_PAIRS 14
/*------------------------------------------------------
**
** File: main.c
** Author: Matt Weston & Richard Tam
** Created: August 31, 2015
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - Changed the #include files
**                      - Moved checkEnabledDisabled method
**                      - Moved addIncidentsToDatabaseList method
**                      - Moved processInfo function
**                      - modified addIncidentToEmail method
**                      - modified main method
**
** 23 Mar 2016: Rev 2.1 - RTam
**                      - Changed version number in email message from "1.1 Beta" to "2.1"
**						- Modified addIncidentToEmail to include new server switchover alarms (PNLSO, ORSSO, COMSO, WBSSO)
**						- Added additional calls to processInfo for new server switchover alarms
**
** 15 Sep 2016: Rev 2.2 RC1 - MTedesco
**                          - Added three new incident types (FCUCF, FCUAS, RSO)
**                              - Modified addIncidentToEmail to include new types
**                              - Added additional calls to processInfo
** 16 Sep 2016: Rev 2.2 RC2 - MTedesco
**                          - Changed email formatting for server switchover and PTSLS incidents to give more information
** 3 Oct 2016: Rev 2.2 RC3  - MTedesco
**                          - Added new "summary email" feature - sends out a summary email of how many incidents occurred at a location
**                              after the email time delay expires (new function sendSummaryEmails)
**                          - Consolidated some code for sending of emails in functions getIncidentName, getEquipmentDescription, and getEventLine
** 25 11 2016: Rev 2.2      - MTedesco
**                          - Bug fixes for rev 2.2 RC3 (mostly to summary email feature)
**
** 26 Jan 2017: Rev 2.3 - RTam
**                      - Added three columns to the table output (Redmine Issue #721)
**                      - Fixed version output of all-clear email notification (Redmine Issue #759)
**                      - Added "Summary" to subject line of summary emails
**
** 16 Oct 2017: Rev 3.0 - DVasile
**						- Added two hard coded IncidentType variables for the VHLC/FCU check
**						- Added Critical Incident check to print special line in email message
**						- Added readInIncidentTypes to main function and changed hard coded incident checks such as TF or CDF with
**			  			  these new read in variables.
**						- Added a macro for the version number and changed email header to use new macro
**
** 15 Oct 2018: Rev 4.0 - IMiller
**						- Added new {CC, car #}s to ccLUT
**						- Modified subject line of Summary emails to include more info. ("Automated CSS Alarm Tool" shows at end of subject line)
**            
** 09 Nov 2018: Rev 4.1 - IMiller
**            - Added new subject line feature (shows what alarms/locations are conatained in the email)
**              - New functions add_to_subject_line(), merge_and_delete(), and print_subject_line()
**              - Modified addIncidentToEmail(), main()
**
** 04 Jun 2019: Rev 4.2 - JBerezny
**		- Added work cars to CCLUT (Redmine Issue #1827)
**		- Modified getFormattedLine() to output RT prefix for work cars
**
** 26 May 2020: Rev 4.3 - ADeng
**		- March 3, 2020 - Added function to print all found incidents to a formatted text file, for use with the LAS.
**		- Add support for reading in CC-to-TR/RT mapping from a configuration file at start up
**			- added function readInCCMapping()
**			- modified struct CCPair into an LL and moved to Incidents.h (due to a dependency on the CCPair LL)
**			- modifed function getCarNum() to search the CCPair LL for a CCID and return the car number.
**			- modified any other function that required using the ccLUT to get the car number.
**	
** 24 June 2022: Rev 5.0 - MKarlov
** 		- Created abbreviations for the incidents on the subject header line of the email (config file in ./exe/Other)
**		- Implemented object types within Incident_Types.txt 
** 		- Streamlined addIncidentToEmail() function & grouped tables based on incident type
** 		- Critical Incidents display an image on the right side
*/

//program defined const variables

//hard coded incidentTypes used for in the check to see if FCU events happened on line 4

char* abrvPath = NULL;

static const struct IncidentType VHLC_ADTSS_IncidentType =
	{
		.keywordList = NULL,
		.summaryTemplate = "Summary For : VHLC AUTO SWITCH : \\K to \\E at \\L",
		.typeOfIncident = "ADTSS",
		.emailTemplate = "There was a <b>VHLC AUTO SWITCH</b> from \\K to \\E at \\L",
		.emailTemplateLocation = "VHLC AUTO DTS SWITCHOVER - AT \\L : FROM \\K TO \\E",
		.processingFlags = "SV",
		.thresholdList = NULL,
		.next = NULL
	};
static const struct IncidentType VHLC_FEPTO_IncidentType =
	{
		.keywordList = NULL,
		.summaryTemplate = "Summary For : VHLC COMMUNICATION FAULT : \\K \\E at \\L",
		.typeOfIncident = "FEPTO",
		.emailTemplate = "There was a <b>VHLC COMMUNICATION FAULT</b> from CHANNEL \\K \\E at \\L.",
		.emailTemplateLocation = "VHLC COMMUNICATION FAULT - CHANNEL \\K \\E AT \\L",
		.processingFlags = "SV",
		.thresholdList = NULL,
		.next = NULL
	};

//2-D String array for converting ZCS Controller # to string
//E.G: 7681 --> VMC to MU
//E.G: 7682 --> QP to FI
const char* ZCS2STATION[3] = {"VMC to MU","QP to FI","MTT"};

// Recursively deletes the linked list mapping CC number to TR/RT number
//   ccPairLLHead -- Pointer to the head of the linked list
void
deleteCCPairList(struct CCPair* ccPairLLHead)
{
	struct CCPair* temp;
	while (NULL != ccPairLLHead){
		temp = ccPairLLHead;
		ccPairLLHead = ccPairLLHead->next;
		free(temp);
	}
}

// For a given CC number, retrieve the TR/RT number
//   CC -- The CC number of the train reported in the alarm
//   ccPairLLHead -- Pointer to the head of the linked list mapping CC number to TR/RT number
//   return -- Returns the TR/RT number from the linked list
int
getCarNum(int CC, struct CCPair* ccPairLLHead)
{
	// Initialize return value to invalid number
	// If this invalid value is returned, error message will be output in the email notification
	int carNum = -1;
	//Traverse the linked list
	while (NULL != ccPairLLHead) {
		if (CC == ccPairLLHead->cc) {
			carNum = ccPairLLHead->carNum;
		}
		ccPairLLHead = ccPairLLHead->next;
	}
	return carNum;
}

//  read in ./Other/CC_Mapping.txt file into an array of CCPair struct
//  
//	return		-- Linked list of struct CCPair
struct CCPair* readInCCMapping() 
{
  struct CCPair* head = NULL; //points to the head of the linked list
  struct CCPair* tail = NULL; //points to the end of the linked list
  
  char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
  
  // open file
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath, OTHER, "CC_Mapping", DOT_TXT);
  FILE *mappingFile = fopen(filePath, "r");
  
  if (NULL != mappingFile){
  	int lineRes = readInLine(mappingFile, &tmp, STRING_LENGTH); 
	while(lineRes != END_OF_FILE){
		if(READ_IN_STRING == lineRes || STRANGE_END_OF_FILE == lineRes) {
			// create CCPair struct and allocate memory
			struct CCPair* cc_pair = (struct CCPair*)malloc(sizeof(struct CCPair)) ;
			if (NULL == cc_pair)
			{
				printf("Could not allocate memory for CCPair\n");
				return NULL;
			}
		
			cc_pair->next = NULL; // just to be sure.
			//Mapping config file format:
			//CCID,CAR_NUM
			cc_pair->cc=atoi(strtok(tmp, ","));
			cc_pair->carNum=atoi(strtok(NULL, ","));
			//If the head is NULL, add the CCPair struct to the head of the linked list.
			//Otherwise, append to the linked list.
			if (NULL == head){
				head = cc_pair;
				tail = head;
			}
			else {	
				if (NULL == tail->next){
					tail->next = cc_pair; //append the newly created CCPair to the next node (after the end of the linked list)
					tail = tail->next;
				}
			}
		}
		lineRes = readInLine(mappingFile, &tmp, STRING_LENGTH); // Get the next line 
	}
	
    if(EOF == fclose(mappingFile)) {
      printf("CC-to-TR/RT mapping file cannot be closed. Mapping file should have been read-in\n");
      printf("errno = %d, strerror is %s\n", errno, strerror(errno));  
    }
  }
  else {
    printf("CC-to-TR/RT mapping file cannot be opened. Records should be null.\n");
    //clean things up before error
    free(tmp);
    free(filePath);
    return NULL;
  }
  //clean things up
  free(tmp);
  free(filePath);
  return head;
}


//checks if the incident is a critical incident, and needs to have a special line
//	incidentType	-- The incident type of the incident that is being checked
//	data		-- The ID of the object to which the incident happend to, ie; W99T
//	return		-- Returns TRUE if a match is found and the incidentType has "C" processingFlag, or
//			   Returns TRUE if the incidentType has the "M" processingFlag, otherwise returns FALSE
BOOL checkCriticalAlert(const struct IncidentType* incidentType,const char* data)
{
	BOOL isCriticalIncident = FALSE;
	//checks if the incident is labeled as mission critical
	if(contains(incidentType->processingFlags,MISSION_CRITICAL_FLAG))
	{
		isCriticalIncident = TRUE;
	}
	//checks if this incident is a possible critical incident
	else if(contains(incidentType->processingFlags,CRITICAL_INCIDENT_FLAG) && !isCriticalIncident)
	{
		char* filePath = calloc(STRING_LENGTH,sizeof(char));
		constructLocalFilepath(filePath,OTHER,CRITICAL_INCIDENTS_FILE,DOT_TXT);
		FILE* file = NULL;
		file = fopen(filePath,"r");
		//check if the file exists
		if(file == NULL)
		{
			printf("Error could not open Critical_Incidents.txt, creating one..");
			file = fopen(filePath,"w");
			fclose(file);
			free(filePath);
		}
		else
		{
			char character;
			char* line = calloc(STRING_LENGTH,sizeof(char));
			//read in the entire file or until a match is found
			while(character != EOF && !isCriticalIncident)
			{
				int i = 0;
				//get a line
				character = getc(file);
				while(character != '\n' && character != EOF)
				{
					*(line + i) = character;
					i++;
					character = getc(file);
				}
				//check if the line is a match for data
				i = 0;
				while(*(line + i) && *(data + i) && *(line + i) == *(data + i))
				{
					if(*(line + i + 1) == '\0' && *(data + i + 1) == '\0')
					{
						isCriticalIncident = TRUE;
					}
					i++;
				}
				//clear line
				i = 0;
				while(*(line + i))
				{
					*(line + i) = '\0';
					i++;
				}
			}
			//free used variables and close file
			free(line);
			fclose(file);
			free(filePath);
		}
	}
	//if none of these conditions are met : incident is not mission critical, and incident was not
	//found on the list of critical incidents, then this function returns false 
	
	return isCriticalIncident;
}

//checks if the incident is an auto dts switch or comunication failure that happened
//on the Sheppard line, and changes the name accordingly
//	typeOfIncident	-- The type of incident has is currently being processed, ie; CDF or ADTSS
//	location	-- The location of the incident
//	return		-- Returns a pointer to a hardcoded IncidentType if a location and typeOfIncident match is found
//			   Otherwise it returns NULL
const struct IncidentType* checkIncidentName(char* typeOfIncident, char* location) {
	const struct IncidentType* incidentType = NULL;
              //if incident occurs on the Sheppard line, it is technically
              //a VHLC fault/switch, not a FCU fault/switch
              //store internally as FCU, but display as VHLC for the emails
	if(strcmp(typeOfIncident, "FCUAS") == 0) {
	        if(strcmp(location, "YongeWest") == 0
	            || strcmp(location, "YongeEast") == 0
	            || strcmp(location, "Bayview") == 0
	            || strcmp(location, "Don Mills Interlocking") == 0
	            || strcmp(location, "DonMills") == 0) 
		{
	            incidentType = &VHLC_ADTSS_IncidentType;
	        }
	}
	else if(strcmp(typeOfIncident, "FCUCF") == 0) {
        
	        if(strcmp(location, "YongeWest") == 0
	            || strcmp(location, "YongeEast") == 0
	            || strcmp(location, "Bayview") == 0
	            || strcmp(location, "Don Mills Interlocking") == 0
	            || strcmp(location, "DonMills") == 0) 
		{
	            incidentType = &VHLC_FEPTO_IncidentType;
	        }
	}
	return incidentType;
}

//Gets the header line for tables that hold incidents
//Header string is found within <b> and </b> of the IncidentTypes templates
//Used in addIncidentToEmail
//If not found, '\0' is returned to avoid SIGSEGV
char* getHeaderLine(const struct IncidentType* incidentType){
  char* headerLine = NULL;  
  char* template = (char*)calloc(LONG_STRING_LENGTH,sizeof(char));
  sprintf(template,"%s%s%s", incidentType->emailTemplate,incidentType->emailTemplateLocation,incidentType->summaryTemplate);
  
  //patterns to look for within the templates
  const char* P1 = "<b>";
  const char* P2 = "</b>";
  char *start, *end;
  
  //After finding the 1st pat, the headerLine takes on all characters before the 2nd pat
  start = strstr(template, P1);
  if (start)
  {
    start += strlen( P1 );
    end = strstr(start, P2);
    if (end)
    {
      headerLine = (char *)malloc(end-start+1);
      memcpy(headerLine,start,end-start);
      headerLine[end-start]='\0';
    }
    else
    {
        ;//do nothing
    }
  }
  else
  {
        ;//do nothing
  }
  free(template);
  return headerLine;
}

//All caps string to normal caps string
//e.g: SECONDARY B --> Secondary B
void normalCaps(char *str){
  char *temp = str;
  for(;*temp!='\0';temp++){
    if(temp==str)
    {
	 continue;
    }
    else if(' '==*temp)
    {
	temp++;
    }
    else
    {
	 *temp = *temp + 32;   
    }
  }
}

//returns 0 if word is specific
//returns 1 if word is generic

//depending on the variable whichLine, this function formats either the eventline template 
//or the location template, replacing all \K,\L,\E,\X's with their subsitute.
//Returns a pointer to the formated line.
//	IncidentType	-- The IncidentType of the incident that is currently being processed
//	data		-- The ID of the object the incident is happening to (matches to the data variable in the incident struct)
//	location	-- The location of the incident (matches to the location variable in the incident struct)
//	other		-- any other data (matches to the other variable in the incident struct)
//	extra		-- any extra data (matches to the extra variable in the incident struct)
//	whichLine	-- An int that tells the function which line it should format and return, either the event or location line.
//	return		-- Returns a char pointer to the beginning of the formated string
char* getFormatedLine(struct CCPair* ccPairLLHead, const struct IncidentType* incidentType,char* data,char* location,char* other,char* extra, int whichLine) {
	char* eventLine = (char*)calloc(LONG_STRING_LENGTH,sizeof(char));
	char* template;
	//gets either the eventline template or location template depending on whichline
	if(whichLine == EVENT_LINE)
	{
		template = incidentType->emailTemplate;
	}
	else if(whichLine == LOCATION_LINE)
	{
		template = incidentType->emailTemplateLocation;
	}
	else if(whichLine == SUMMARY_LINE)
	{
		template = incidentType->summaryTemplate;
	}
	else
	{
		free(eventLine);
		return NULL;
	}

	//tokenize the template delimited by spaces
	int templateOffset = 0;
	int eventLineOffset = 0;
	//goes through entire template
	while(*(template + templateOffset))
	{
		//if a '\' has been found
		if(*(template + templateOffset) == '\\')
		{
			int i = 0;
			char* wordToCopy;
			//find out which special keyword was indicated,
			//and copy the special keyword to replace the '\*'
			templateOffset++;
			if(*(template + templateOffset) == 'K')
			{
				wordToCopy = data;
				while(wordToCopy != NULL && *(wordToCopy + i))
				{
					*(eventLine + eventLineOffset) = *(wordToCopy + i);
					i++;
					eventLineOffset++;
				}
			}
			else if(*(template + templateOffset) == 'L')
			{
				wordToCopy = location;
				while(wordToCopy != NULL && *(wordToCopy + i))
				{
					*(eventLine + eventLineOffset) = *(wordToCopy + i);
					i++;
					eventLineOffset++;
				}
			}
			else if(*(template + templateOffset) == 'E')
			{
				wordToCopy = other;
				while(wordToCopy != NULL && *(wordToCopy + i))
				{
					*(eventLine + eventLineOffset) = *(wordToCopy + i);
					i++;
					eventLineOffset++;
				}
			}
			else if(*(template + templateOffset) == 'X')
			{
				wordToCopy = extra;
				while(wordToCopy != NULL && *(wordToCopy + i))
				{
					*(eventLine + eventLineOffset) = *(wordToCopy + i);
					i++;
					eventLineOffset++;
				}
			}
			else
			{
				printf("Error : An Unidentified Special Keyword %c%c Was encountered",*(template + templateOffset - 1),*(template + templateOffset)); 
				free(eventLine);
				return NULL;
			}
			templateOffset++;
		}
		else
		{
			*(eventLine + eventLineOffset) = *(template + templateOffset);
			eventLineOffset++;
			templateOffset++;
		}
	}

	if(contains(incidentType->processingFlags,GET_CAR_NUMBER_FLAG) == TRUE)
	{
		int carNum = getCarNum(atoi(data),ccPairLLHead);
		
		//append the car number or error message if the LUT search failed
		char* temp = calloc(LONG_STRING_LENGTH,sizeof(char));
		if(carNum == -1)
		{
			//appends an error msg
			sprintf(temp,"%s ERROR : CC # %d does not match any Car #",eventLine,atoi(data));
		}
		else
		{
			//appends the car #
			if (atoi(data) < 100)
			{
				sprintf(temp,"%s (CAR NUMBER : %d).",eventLine,carNum);
			}
			else if ((atoi(data) < 119) && (atoi(data) > 109))
			{
				sprintf(temp,"%s (CAR NUMBER : RT-%d/%d).",eventLine,carNum,carNum+1);
			}
			else
			{
				sprintf(temp,"%s (CAR NUMBER : RT-%d).", eventLine,carNum);
			}
			}
		//copy the string over to event line
		int q = 0;
		while(*(temp + q))
		{
			*(eventLine + q) = *(temp + q);
			++q;
		}
		free(temp);
	}

	return eventLine;
}

//Converts CC number to corresponding Car Number
//String includes distinction based on work-car OR revenue-car
//RT-X/X VS RT-X VS TRX
//Where X represents the car number
//Used in addIncidentToEmail
char* getCarNumString(char* data,struct CCPair* ccPairLLHead){
	char* carNumString  = (char*)calloc(STRING_LENGTH, sizeof(char)); 
	int carNum = getCarNum(atoi(data),ccPairLLHead);
	if( -1==carNum ) 
	{
		//appends an error msg
		sprintf(carNumString,"ERROR : CC # %d does not match any Car #",atoi(data));
	}
	else 
	{
		//appends the car #
		if (atoi(data) < 100) 
		{
			sprintf(carNumString,"TR%d",carNum);
		}
		else if ((atoi(data) < 119) && (atoi(data) > 109)) 
		{
			sprintf(carNumString,"RT-%d/%d",carNum,carNum+1);
		}
		else 
		{
			sprintf(carNumString,"RT-%d",carNum);
		}
	}
	return carNumString;
}

//Converts a CSS location to an Abbreviation from Abrvs.txt in ./Other
//Designed to keep an email's subject line concise
//Server name Incidents use both data & loc parameters
//loc="ZCS" data="7683" -->    ZCS_7683 ; ZC3
//Other Incidents use just the loc parameter
//loc="Kipling" data=NULL -->   Kipling ; KP
char* abrv(char* loc,char* data){
  char* command = (char*)calloc(STRING_LENGTH, sizeof(char)); 
  char* output = (char*)calloc(STRING_LENGTH, sizeof(char)); 

  //If both parameters are non-empty, a consensual, non-identical, search will be done
  //Otherwise an identical search will be conducted based on the non-empty parameter
  if(NULL == data || '\0' == *data) 
  {
	sprintf(command,"awk -F\";\" -v pat=\"%s\" \'$1==pat{print $2;exit}\' \"%s\"",loc,abrvPath); 
  }
  else if(NULL == loc || '\0'  == *loc) 
  {
	sprintf(command,"awk -F\";\" -v pat=\"%s\" \'$1==pat{print $2;exit}\' \"%s\"",data,abrvPath); 
  }
  else 
  {
	sprintf(command,"awk -F\";\" -v loc=\"%s\" -v data=\"%s\" \'/^[^//]/ && $1~loc && $1~data{print $2;exit}\' \"%s\"",loc,data,abrvPath);  
  }

  FILE* file = popen(command,"r");
  fgets(output,STRING_LENGTH, file);
  pclose(file);
  free(command);

  //No result found : awk returns a null-terminating char
  if('\0' == output[0] || '\n' == output[0]) 
  {
  	if(NULL == data || '\0' == *data)
	{
		 strcpy(output,loc);
	}
  	else 
	{
		strcpy(output,data);
	}
  }
  else
  {
	 output[strlen(output)-1] = '\0';
  }

  return output;
}

//The equipment could be templated as GND_\K or AC_\K
//This algorithm splits the formated line & attemps to find \K
//If successful, returns the whole token with \K
char* fullNameFromPartial(char* data,char* formatedLine){
	char *equipment = strtok(formatedLine," ");
        while(equipment != NULL){
          if(strstr(equipment,data)!=NULL) 
	  {
		return equipment;   
	  }
	  else
	  {
		  equipment = strtok(NULL," ");
	  }
	}
	return data;
}
/* Takes in an incident that will be emailed about and adds it to the subject line linked-list, if not already added.
  Gets the actual name of the incident and the actual location (CC# for onboard alarms, server name for server alarms)
  sl - the SubjectList linked-list containing all elements of the subject line
  dr - the DatabaseRecord to be emailed about
  it - the IncidentType matching the type of incident of dr */

void
add_to_subject_line(struct SubjectList* sl, struct DatabaseRecord* dr, const struct IncidentType* it, struct CCPair* ccPairLLHead)
{
  char* name_of_incident = calloc(STRING_LENGTH, sizeof(char));

  //get actual name of incident
  if(contains(it->emailTemplate, "<b>") && contains(it->emailTemplate, "</b>"))  //Some incidents do not have <b> tags...
  {
    char* email_template = calloc(STRING_LENGTH, sizeof(char));
    strcpy(email_template, it->emailTemplate);

    strcpy(name_of_incident, strtok(email_template, "<b>"));
    strcpy(name_of_incident, strtok(NULL, "</b>"));
    free(email_template);
  }
  else
  {
    char* summary_template = calloc(STRING_LENGTH, sizeof(char));
    strcpy(summary_template, it->summaryTemplate);

    strcpy(name_of_incident, strtok(summary_template, ":"));
    strcpy(name_of_incident, strtok(NULL, ":"));
    free(summary_template);
  }

  //if SubjectList is empty OR last incident added is different than current incident (dr)
  if((NULL == sl->head) || (strcmp(sl->tail->type_of_incident, it->typeOfIncident) != 0))
  {
    //create new SubjectIncident struct
    struct SubjectIncident* si = malloc(sizeof(struct SubjectIncident));
    si->type_of_incident = it->typeOfIncident;
    si->incident_name = (char*)calloc(STRING_LENGTH, sizeof(char));
    strcpy(si->incident_name, name_of_incident);
    si->location_list = malloc(sizeof(struct LocationList));
    create_location_list(si->location_list);
    si->next = NULL;
    
    //create new Location struct
    struct Location* loc = malloc(sizeof(struct Location));
    loc->location = (char*)calloc(STRING_LENGTH, sizeof(char));
    
    //If onboard incident, use CC #-Car # as location
    if(contains(it->processingFlags, ONBOARD_INCIDENT_FLAG))
    {
      int car = getCarNum(atoi(dr->data),ccPairLLHead);

      if (atoi(dr->data) < 100)
      {
        sprintf(loc->location, "CC %s-TR%d", dr->data, car);
      }
      else if (((atoi(dr->data) < 119) && (atoi(dr->data) > 109)) || atoi(dr->data) == 165)
      {
        sprintf(loc->location, "CC %s-RT%d/%d", dr->data, car, car+1);
      }
	  else if (atoi(dr->data) == 160)
	  {
		sprintf(loc->location, "CC %s-RT%d/C1", dr->data, car);
	  }
      else
      {
        sprintf(loc->location, "CC %s-RT%d", dr->data, car);
      }
	
    }
      //If it's a server-related incident, use server name as location
    else if(contains(it->processingFlags, SERVER_RELATED_INCIDENT))
    {
      char* loc2abrv = abrv(dr->location,dr->data);
      strcpy(loc->location, loc2abrv);
      free(loc2abrv);
    }
    else
    {
      char* loc2abrv = abrv(dr->location,NULL);
      strcpy(loc->location, loc2abrv);
      free(loc2abrv);
    }
    loc->next = NULL;

    //add nodes to linked lists
    insert_into_location_list(si->location_list, loc);
    insert_into_subject_list(sl, si);
  }
  else
  { //else, incidents match but there may be a new location to add
    BOOL found = FALSE;
    struct Location* temp_loc = sl->tail->location_list->head;

    while((NULL != temp_loc) && (FALSE == found))
    {
      //If onboard incident
      if(contains(it->processingFlags, ONBOARD_INCIDENT_FLAG))
      {
        char* on_board_loc = calloc(STRING_LENGTH, sizeof(char));
        sprintf(on_board_loc, "CC %s-", dr->data);

        if(contains(temp_loc->location, on_board_loc))
        {
          found = TRUE;
        }
        free(on_board_loc);
      }
      else{
       char* loc2abrv = abrv(dr->location,dr->data); 

       //If server-related incident
       if(!strcmp(temp_loc->location, loc2abrv)) 
       	{
		found = TRUE;
	}
       
       //If other incident type, compare just location
       else{ 
      	free(loc2abrv); 
      	loc2abrv = abrv(dr->location,NULL);
      	if(!strcmp(temp_loc->location,loc2abrv))
		{
		found = TRUE;
	}
       }  
       free(loc2abrv);
      }  
      temp_loc = temp_loc->next;
    }

    //if current location has not been added before, add it to LocationList
    if(FALSE == found)
    {
      //create new Location struct
      struct Location* loc = malloc(sizeof(struct Location));
      loc->location = (char*)calloc(STRING_LENGTH, sizeof(char));
      
      //If onboard incident, use CC #-Car # as location
      if(contains(it->processingFlags, ONBOARD_INCIDENT_FLAG))
      {
        int car = getCarNum(atoi(dr->data),ccPairLLHead);
        if (atoi(dr->data) < 100)
        {
          sprintf(loc->location, "CC %s-TR%d", dr->data, car);
        }
        else if (atoi(dr->data) < 117 && atoi(dr->data) > 109)
        {
          sprintf(loc->location, "CC %s-RT%d/%d", dr->data, car, car+1);
        }
        else
        {
          sprintf(loc->location, "CC %s-RT%d", dr->data, car);
        }
      }
        //If it's a server-related incident, use server name as location
      else if(contains(it->processingFlags, SERVER_RELATED_INCIDENT))
      {
      	char* loc2abrv = abrv(dr->location,dr->data);
        strcpy(loc->location, loc2abrv);
      	free(loc2abrv);
      }
      else
      {
      	char* loc2abrv = abrv(dr->location,NULL);
        strcpy(loc->location, loc2abrv);
      	free(loc2abrv);
      }
      loc->next = NULL;

      //add location to latest SubjectIncident
      insert_into_location_list(sl->tail->location_list, loc);
    }
  }
  free(name_of_incident);
}

/* Appends the body part of an email to the Header, then deletes the body file.
   ei - the EmailInfo struct containing the filepath names for the body and header */
void
merge_and_delete(struct EmailInfo* ei)
{
  char* filePath = calloc(STRING_LENGTH, sizeof(char));
  char* body_filePath = calloc(STRING_LENGTH, sizeof(char));
  char buffer[STRING_LENGTH];

  constructLocalFilepath(filePath, EMAIL_INFO, ei->emailFileName, DOT_HTML);
  constructLocalFilepath(body_filePath, EMAIL_INFO, ei->bodyFileName, DOT_HTML);

  FILE* subject = fopen(filePath, "a");
  FILE* body = fopen(body_filePath, "r");

  if(NULL == subject)
  {
    printf("Cannot open email |%s| for merging. Skipping.\n", filePath);
    printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
    free(filePath);
    return;
  }
  if(NULL == body)
  {
    printf("Cannot open email |%s| for merging. Skipping.\n", body_filePath);
    printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
    free(body_filePath);
    return;
  }

  //Read body and append to subject line-by-line
  while(fgets(buffer, sizeof(buffer), body) != NULL)
  {
    fputs(buffer, subject);
  }

  if(EOF == fclose(subject))
  {
    printf("There was an error with file: %s\n The file could not be closed by the program\n", ei->emailFileName);
    printf("Cannot close emai0timediff l |%s| from read. Skipping.\n", filePath);
    printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
  }
  if(EOF == fclose(body))
  {
    printf("There was an error with file: %s\n The file could not be closed by the program\n", ei->bodyFileName);
    printf("Cannot close emai0timediff l |%s| from read. Skipping.\n", body_filePath);
    printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
  }

  //Subject is now the complete email file, so delete body
  remove(body_filePath);
  free(filePath);
  free(body_filePath);
}


//prints subject line to email file. Format is:
// [Name of Alarm Type 1] ([Station 1]/[Station 2]/.../[Station N]), 
// [Name of Alarm Type 2] ([Station 1]/[Station 2]/.../[Station N]) – Automated CSS Alarm Tool
// ei - EmailInfo struct containing SubjectList
void
print_subject_line(struct EmailInfo* ei)
{
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));

  constructLocalFilepath(filePath, EMAIL_INFO, ei->emailFileName, DOT_HTML);
  FILE* emailMsg = fopen(filePath, "a");

  if(NULL == emailMsg)
  {
    printf("Cannot open email |%s| for appending. Skipping.\n", filePath);
    printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
    free(filePath);
    return;
  }

  fprintf(emailMsg, "From: WBSS-noreply@ttc.ca\n"); 
  fprintf(emailMsg, "to: %s\n", ei->personsEmail);
  fprintf(emailMsg, "Subject: ");
  
  struct SubjectIncident* si = ei->subject_list->head;
  struct Location* loc = si->location_list->head;

  char* subjectMessage = (char*)calloc(LONG_SUBJECT_LENGTH, sizeof(char));
  char* tempIncident = (char*)calloc(STRING_LENGTH, sizeof(char));

  //print first incident and location(s) to subject line, to handle formatting issues
  if(contains(si->type_of_incident, "FEP"))
  {
    sprintf(tempIncident, "%s (FEP Server %s", si->incident_name, loc->location);
	strcat(subjectMessage,tempIncident);
  }
  else
  {
    sprintf(tempIncident, "%s (%s", si->incident_name, loc->location);
    strcat(subjectMessage,tempIncident);
  }
  loc = loc->next;
  while(NULL != loc)
  {
    if(contains(si->type_of_incident, "FEP"))
    {
      sprintf(tempIncident, "/FEP Server %s", loc->location);
      strcat(subjectMessage,tempIncident);
    }
    else
    {
      sprintf(tempIncident, "/%s", loc->location);
      strcat(subjectMessage,tempIncident);
    }
    loc = loc->next;
  }

  sprintf(tempIncident, ")");
  strcat(subjectMessage,tempIncident);

  si = si->next;

  //prints rest of incidents to subject line
  while(NULL != si)
  {
    struct Location* loc = si->location_list->head;
    if(contains(si->type_of_incident, "FEP"))
    {
      sprintf(tempIncident, ", %s (FEP Server %s", si->incident_name, loc->location);
      strcat(subjectMessage,tempIncident);
    }
    else
    {
      sprintf(tempIncident, ", %s (%s", si->incident_name, loc->location);
      strcat(subjectMessage,tempIncident);
    }
    loc = loc->next;
    while(NULL != loc)
    {
      if(contains(si->type_of_incident, "FEP"))
      {
        sprintf(tempIncident, "/FEP Server %s", loc->location);
	strcat(subjectMessage,tempIncident);
      }
      else
      {
        sprintf(tempIncident, "/%s", loc->location);
	strcat(subjectMessage,tempIncident);
      }
      loc = loc->next;
    }
    sprintf(tempIncident, ")");
    strcat(subjectMessage,tempIncident);

    si = si->next;
  }
  
  //print end of subject line
  sprintf(tempIncident, " - Automated CSS Alarm Tool\n");
  strcat(subjectMessage,tempIncident);

  if(strlen(subjectMessage) > (EMAIL_SUBJECT_CHAR_LIMIT))
  {
	  subjectMessage[EMAIL_SUBJECT_CHAR_LIMIT-EMAIL_SUBJECT_SUFFIX_CHAR_LIMIT]='\0';
	  fprintf(emailMsg,"%s",subjectMessage);
	  fprintf(emailMsg,"%s\n",EMAIL_SUBJECT_SUFFIX);
  }
  else
  {
	  fprintf(emailMsg,"%s",subjectMessage);
  }
  free(subjectMessage);
  free(tempIncident);

  //close email file from appending
  if(EOF == fclose(emailMsg))
  {
    printf("There was an error with file: %s\n The file could not be closed by the program\n", ei->emailFileName);
    printf("Cannot close emai0timediff l |%s| from read. Skipping.\n", filePath);
    printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
  }

  //append email body to header
  merge_and_delete(ei);

  free(filePath);
}

// Add new incident to file stream to be added to the output csv file for the Log Aggregation System
//   incidentLogs -- file pointer to the output csv file for the Log Aggregation System
//   isoString -- timestamp of incident in ISO8601 format
//   incidentType -- pointer to incident type linked list
//   data -- pointer to string in incident linked list node for incident data
//   other -- pointer to string in incident linked list node for other data
//   extra -- pointer to string in incident linked list node for extra data
//   subwayLine -- pointer to string in incident linked list node for subway line (YUS, BDS, CYUS)
//   locationString -- pointer to formatted string to be printed to notification email
//   ccPairLLHead -- pointer to head of linked list for mapping CC number to TR/RT number
void addIncidentToLogs(FILE* incidentLogs, char* isoString, struct IncidentType* incidentType, char* data, char* location, char* other, char* extra, char* subwayLine, char* locationString, struct CCPair* ccPairLLHead){
	
	
	//Format of an incident (alarm) for telegraf to parse (grok and logfmt) and output to influxDB: ts=2020-02-28T11:00:05Z,incident_type=TF,...
	fprintf(incidentLogs, "timestamp=%s,line=%s,", isoString, subwayLine);
	
	if (NULL != incidentType) {
		fprintf(incidentLogs, "incident_type=%s", incidentType->typeOfIncident);
	}
	
	fprintf(incidentLogs, ",message=\"%s\"", locationString);

	if (NULL != data && strcmp(data,"") != 0)
	{
		//If there is a processing flag for finding the car number (F), then the data variable (\K) contains the CC ID. 
		if(contains(incidentType->processingFlags,GET_CAR_NUMBER_FLAG) == TRUE)
		{
			fprintf(incidentLogs, " CCID=\"%s\" car_number=\"%d\"", data, getCarNum(atoi(data),ccPairLLHead));
		}
		else
		{
			//Otherwise, the data may be for a track circuit/switch/server/etc
			//For now, just print as is
			fprintf(incidentLogs, " data=\"%s\"", data);
		}

	}

	if (NULL != location &&  strcmp(location,"") != 0)
	{
		fprintf(incidentLogs, " location=\"%s\"", location); 
	}
	
	char* train_data = NULL;
	if (NULL != other &&  strcmp(other,"") != 0)
	{
		char train_other[TRAIN_STRING_LENGTH];
		if(snprintf(train_other, TRAIN_STRING_LENGTH, "TRAIN %s", other) >=  TRAIN_STRING_LENGTH){
			printf("Not enough space.\n");
		}
		if (contains(locationString, train_other))
		{
			//if true, other is a string storing train data
			train_data = other;
		}
		else
		{
			//Print the raw other variable to the log
			fprintf(incidentLogs, " other=%s",other);
		}
	}

	if (NULL != extra && strcmp(extra,"") != 0)
	{
		char train_extra[TRAIN_STRING_LENGTH];
		if(snprintf(train_extra,  TRAIN_STRING_LENGTH, "TRAIN %s", extra) >=  TRAIN_STRING_LENGTH){
			printf("Not enough space.\n");
		}
		if (contains(locationString, train_extra))
		{
			train_data = extra;
		}
		else
		{
			//Print the raw extra variable to the log
			fprintf(incidentLogs, " extra=\"%s\"", extra);
		}
	}
	//Check if it is a long berth identifier (ex: X111HK000000M)
	if (NULL != train_data && strlen(train_data) == LONG_BERTH_LENGTH)
	{
		fprintf(incidentLogs," train=\"%s\"", train_data);
		int offset = 0;
		while(*(train_data+offset))
		{
			if (offset == TRAIN_CLASS) //if (offset == 0)
			{
				//Need to add LUT for train class representations (R for revenue, etc..)
				fprintf(incidentLogs, " train_class=\"%c\"", *(train_data+offset));					
			}
			else if (offset == TRAIN_RUN) //if (offset == 1)
			{
				fprintf(incidentLogs, " run_number=\"%c%c%c\"", *(train_data+offset), *(train_data+offset+1), *(train_data+offset+2));
				offset+= 2;
			}
			else if (offset == TRAIN_ORIGIN) //if (offset == 4)
			{
				fprintf(incidentLogs, " origin=\"%c\"", *(train_data+offset));
			}
			else if (offset == TRAIN_DESTINATION) //if (offset == 5)
			{
				fprintf(incidentLogs, " destination=\"%c\"", *(train_data+offset));
			
			}
			else if (offset == TRAIN_DISPATCH_TIME) //if (offset == 6)
			{
				fprintf(incidentLogs, " dispatch_time=\"%c%c:%c%c:%c%c\"", *(train_data+offset), *(train_data+offset+1),*(train_data+offset+2),*(train_data+offset+3),*(train_data+offset+4),*(train_data+offset+5));
				offset+= 5;
			}
			else if (offset == TRAIN_SCHEDULE_MODE) //if (offset == 12)
			{	//Manual (M) or automatic (A)
				fprintf(incidentLogs, " schedule_mode=\"%c\"", *(train_data+offset));
			}
			offset++;
		}
	}
	fprintf(incidentLogs, "\n");//end of influx line
}

// Print list of incidents to csv file for import into Log Aggregation System
//   il -- Pointer to linked list containing incidents
//   ccPairLLHead - Pointer to linked list mapping CC number to TR/RT number
void printIncidentsToLogs(struct IncidentList* il, struct CCPair* ccPairLLHead){
	char* trackedIncidentsFilePath = (char*)calloc(STRING_LENGTH, sizeof(char));
	constructLocalFilepath(trackedIncidentsFilePath, INCIDENT_LOGS, INCIDENT_LOGS_FILE, DOT_CSV);
	FILE* incidentLogs = fopen(trackedIncidentsFilePath, "a");
     // check if incidentLogs file is NULL, if so, create it.
      if(incidentLogs == NULL) {   // first entry, must create file 
        
       incidentLogs = fopen(trackedIncidentsFilePath, "w+");
        if(NULL == incidentLogs) {
          printf("Cannot open incident alarm logs |%s| for write. Skipping.\n", trackedIncidentsFilePath);
          printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
	        free(trackedIncidentsFilePath);
          return;
        }
      }
      // if the file is not open, open it in 'append' mode
      else {
        if(EOF == fclose(incidentLogs)) {
          printf("Cannot close incident alarm logs |%s| from read. Skipping.\n", trackedIncidentsFilePath);
          printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
	        free(trackedIncidentsFilePath);
          return;
        }
        incidentLogs = fopen(trackedIncidentsFilePath, "a");
        if(NULL == incidentLogs) {
          printf("Cannot open incident alarm logs |%s| for appending. Skipping.\n", trackedIncidentsFilePath);
          printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
	        free(trackedIncidentsFilePath);
          return;
        }
      }

	struct Incident* tmp = il->head;
	if(il->count > 0) {
		while(tmp != NULL) {

			//ISO-8601: YYYY-MM-DD HH:mm:ss
			char* isoString = getISOStringFromDate(tmp->timeElement->timeObj);
			char* locationString = (char*)getFormatedLine(ccPairLLHead, tmp->incidentType, tmp->data,tmp->location,tmp->other,tmp->extra, LOCATION_LINE);
			addIncidentToLogs(incidentLogs, isoString, tmp->incidentType, tmp->data, tmp->location, tmp->other, tmp->extra, tmp->subwayLine, locationString, ccPairLLHead);
			free(isoString);
			free(locationString);
      tmp = tmp->next;
    		}
	}
	else {
		printf("IncidentList is empty: count = %d\n", il->count);
	}

      fclose(incidentLogs);
      free(trackedIncidentsFilePath);
}

// If a database record has met threshold conditions, is not disabled and has 
// not already been emailed about this method will take the information from a
// database record and format it into a table in a html file associated
// with each person is EmailInfoList 'el' who is supposed to receive emails 
// about this type of incident
//	dr		-- A record of an incident
//	th		-- The threshold that was triggered to send the email
//	el		-- The list of all possible email recipients
//	start		-- The start time of the program
//	incidentType	-- The type of incident that dr holds
//	return		-- void
void addIncidentToEmail(struct DatabaseRecord* dr, struct Threshold* th, 
  struct EmailInfoList* el, struct TimeElement* start, const struct IncidentType* incidentType, struct CCPair* ccPairLLHead, bool headerExists) {
  struct EmailInfo* tmp = el->head;
  char* filePath = calloc(STRING_LENGTH, sizeof(char));
  const struct IncidentType* incidentTypeCheck = NULL;
  //checks to see if a FCU event happened on line 3
  if(contains(incidentType->processingFlags,CHECK_TCS_VHLC_SERVER_FLAG) == TRUE)
  {
  	incidentTypeCheck = checkIncidentName(dr->typeOfIncident, dr->location);
  }
  //if the FCU event did happen on line 3, change incidentTypes
  if(incidentTypeCheck != NULL)
  {
	incidentType = incidentTypeCheck;
  }
  // while loop for EmailInfoList 'el' this list contains the information of
  // recipients who are supposed to receive emails 
  while( tmp!=NULL) {
    // check if this person is supposed to receive emails about this type of
    // incident
    if( shouldReceiveEmail(tmp,dr->typeOfIncident) == TRUE) {
      constructLocalFilepath(filePath, EMAIL_INFO, tmp->bodyFileName, DOT_HTML);
      
      FILE* emailMsg = fopen(filePath, "r");

      // check if emailMsg file is NULL, if so, create it.
      if(emailMsg == NULL) {   // first entry, must create file 
        tmp->sendEmail = TRUE;
        
        emailMsg = fopen(filePath, "w+");
        if(NULL == emailMsg) {
          printf("Cannot open email |%s| for write. Skipping.\n", filePath);
          printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
	        free(filePath);
          return;
        } 

        add_to_subject_line(tmp->subject_list, dr, incidentType, ccPairLLHead);

	fprintf(emailMsg, "Content-Type: text/html\n");
	fprintf(emailMsg, "MIME-Version: 1.0\n\n");
        fprintf(emailMsg, "<!DOCTYPE html>\n");
        fprintf(emailMsg, "<style>body { font-family: \"Arial\", sans-serif; font-size: 14px; }\n table, th, td { border: 1px solid black; border-collapse: collapse; }\n td, th { padding-left: 0.625em; padding-right: 0.625em; line-height: 120%; text-align: center;}\nth { font-weight: bold; }\nsl { font-weight: bold; font-size:12pt;}\nimg {max-width: 17%%; height: auto;}</style><html><body>\n");
	//fprintf(emailMsg, "<h1><br></h1>");
      }
      // if the file is not open, open it in 'append' mode
      else {
        tmp->sendEmail = TRUE;
        if(EOF == fclose(emailMsg)) {
          printf("Cannot close email |%s| from read. Skipping.\n", filePath);
          printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
	        free(filePath);
          return;
        }
        emailMsg = fopen(filePath, "a");
        if(NULL == emailMsg) {
          printf("Cannot open email |%s| for appending. Skipping.\n", filePath);
          printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
	        free(filePath);
          return;
        }
      }


      add_to_subject_line(tmp->subject_list, dr, incidentType, ccPairLLHead);

      // Brief descriptive message and table headings
      // <th> is table heading
      // <td> is table data
      //gets the event line, depending on if checkIncidentName returned something or not
      //this is due to incidents happening on different lines are acutally different incidents

//The first time an incident type appears, it is created a header line & a table
      if(!headerExists)
      {

   	 char *headerLine = getHeaderLine(incidentType);

	 //If the incident is Mission Critical, it will have an additional header
   	 if(contains(incidentType->processingFlags,"M"))
	 {
		 fprintf(emailMsg,"<font color=#bf0000><sl>CRITICAL INCIDENT</sl></font>\n<br>\n<sl style=\"color:#f20000\">%s</sl>\n<table style=\"color:#f20000\">",headerLine);
	 }
   	 else 
	 {
		 fprintf(emailMsg,"<sl>%s</sl>\n<table>",headerLine);			   
	 }

   	 free(headerLine);
   	 //Tables have rows organized by the processing flags they carry
   	 //More specific rows are differentiated by non-empty/empty databaseRecord members 
   	 //Priority of flags is: TRVSMQFC -- based on incident types with colluding flags
   	 if(contains(incidentType->processingFlags,"T"))
	 {
		 fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Location</th> <th>%s</th> <th>Fault Cause</th> <th>Engineering Review</th> <th>Signals Remarks</th> </tr>\n",incidentType->object);
	 }

   	 else if(contains(incidentType->processingFlags,"R"))
	 {
		 fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Location</th> </tr>\n");
	 }

   	 else if(contains(incidentType->processingFlags,"V"))
	 {
		 fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th>  <th>Location</th> <th>Channel</th> </tr>\n");
	 }

   	 else if(contains(incidentType->processingFlags,"S"))
	 {
		 fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Server</th> <th>Line</th> </tr>\n");
	 }

   	 else if(contains(incidentType->processingFlags,"M")) 
	 {
	
	  //Non-empty/empty incident member variables deduce the table header
		  if('\0' == *(dr->location) && '\0' == *(dr->data))
		  {
			  fprintf(emailMsg,"<tr> <th>Date</th> <th>Time</th> <th>Statement</th> </tr>\n");
		  }
		  else if('\0' == *(dr->location))
		  {
			  fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>%s</th> </tr>\n",incidentType->object);
		  }
		  else
		  {
			   fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Location</th> <th>%s</th></tr>\n",strtok(incidentType->object," - "));
			   strcpy(incidentType->object,strtok(NULL," - "));
		  }
   	 }

   	 else if (contains(incidentType->processingFlags,"Q")) 
	 {

    	   //Non-empty/empty incident member variables deduce the table header
    	   fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>%s</th> <th>Line</th> ",incidentType->object);
    	   
    	   //If the controller is of ZCS type, then extra column is added to header
    	   if('7'==*(dr->data))
	   {
    	   	fprintf(emailMsg,"<th>Station</th> </tr> \n");
    	   }
    	   else
	   {
    	   	fprintf(emailMsg,"</tr> \n");
    	   }
        }

        else if (contains(incidentType->processingFlags,"F")) 
	{

	  //Differentiation for UCCR incident type
          if('\0'==*(dr->extra))
	  {
		  fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Train</th> <th>Run Number</th> <th>Location</th> </tr>\n");
	  }
          else
	  {
		  fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>User</th> <th>Train</th> <th>Run Number</th> <th>Location</th> </tr>\n"); 
	  }	
        }

        else if (contains(incidentType->processingFlags,"C")) 
	{
	
       	//Table header may have a specific Equipment type (based on object) 
            if('\0'==*(incidentType->object))
	    {
		    fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Location</th> <th>Equipment</th> <td style=\'border-color:white;\'></td></tr>\n"); 
	    }
            else
	    {
		    fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Location</th> <th>%s</th> <td style=\'border-color:white;\'></td></tr>\n", incidentType->object);
	    }
        }

	//Table headers for incidents with no processing flags
        else
	{

	  //Non-empty/empty incident member variables deduce the table header
	  //Either just a location column, or a location column with an equipment type
          if('\0'==*(incidentType->object) && '\0'==*(dr->data))
	  {
		  fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Location</th> </tr>\n");
	  }
          else if('\0'==*(incidentType->object) || '\0'==*(dr->data))
	  {
		  fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Location</th> <th>Equipment</th></tr>\n");
	  }
          else
	  {
		  fprintf(emailMsg, "<tr> <th>Date</th> <th>Time</th> <th>Location</th> <th>%s</th> </tr>\n",incidentType->object);
	  }
	}
      }
      else
      {
	      ;//do nothing
      }

	  
	//Every incident is appended to the table of their corresponding type
	struct TimeElement* te = start;
	char* remoteLink;
        int i;
	printf("numIncidents: %d\n", th->numOfIncidents);
	for(i=0; i < th->numOfIncidents; i++) {
		char* s;
		char* timeString = strtok(s = getStringFromDate(te->timeObj), " ");
		char* dateString = strtok(NULL, " ");
		char* formatedLine = getFormatedLine(ccPairLLHead, incidentType, dr->data,dr->location,dr->other,dr->extra, LOCATION_LINE);

	     //Tables have rows organized by the processing flags they carry
	     //More specific rows are differentiated by non-empty/empty databaseRecord members 
	     //Contents of incidents are appended based on priority ordering
	     if (contains(incidentType->processingFlags,"T")) 
	     {
	      char* Track = dr->data;
	      char* loc = dr->location;
	      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> <td></td> <td></td> <td></td> </tr>\n",dateString,timeString,loc,Track);
	     }

	     else if (contains(incidentType->processingFlags,"R"))
	     {
	      char* loc = dr->location;
	      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,loc);
	     }

	     else if (contains(incidentType->processingFlags,"V")){ 
	      char* loc = dr->location;
	      char* channel = (char*)calloc(STRING_LENGTH, sizeof(char));

	      //VHLC Check may contain data for 2 channels -- " to " keyword necessary
	      //If only one channel, then the other keyword will be channel specification (e.g: A,B,C,D,E)
	      if(1==strlen(dr->other))
	      {
		sprintf(channel,"%s %s",dr->data,dr->other);
		normalCaps(channel);
	      }
	      else 
	      {
		      sprintf(channel,"%s to %s", dr->data,dr->other);
	      }
	      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,loc,channel);
	      free(channel);
	      }

	     else if (contains(incidentType->processingFlags,"S")) 
	     {
	      char* Server = (char*)calloc(STRING_LENGTH, sizeof(char));

	      //Server Name Check may contain data for 2 servers -- " to " keyword necessary
	      if(NULL==dr->other  || '\0' == *(dr->other)) 
	      {
		      strcpy(Server,dr->data);
	      }
	      else 
	      {
		      sprintf(Server,"%s to %s",dr->other,dr->data);
	      }
	      char* SubLine = dr->location;
	      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,Server,SubLine);
	      free(Server);
	     }

	     else if (contains(incidentType->processingFlags,"M")) 
	     {
	      char* loc = dr->location;
		
	      //If incident has all empty members, the template string is printed; Otherwise
	      //If incident has an empty location member, the data member is printed; Otherwise
	      //If incident has an empty data member, the location & object is printed; Otherwise
	      //The location, and (object together with data) members are printed
	      if('\0'==*loc && '\0'==*(dr->data))
	      {
		      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,formatedLine);
	      }
	      else if('\0'==*loc)
	      {
		      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,dr->data);
	      }
	      else if('\0'==*(dr->data))
	      {
		      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td></tr>\n",dateString,timeString,loc,incidentType->object);
	      }
	      else
	      {
		      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s%s</td> </tr>\n",dateString,timeString,loc,incidentType->object,dr->data);
	      }
	     }

	     else if (contains(incidentType->processingFlags,"Q")) 
	     {
	      char* Controller = (char*)calloc(STRING_LENGTH, sizeof(char));
	      char* Line = dr->other;
	      
	      //Controller name with # may be solely in data OR a combination of location & data
	      if( NULL==dr->location || '\0' == *(dr->location))
	      {
		      strcpy(Controller,dr->data);
	      }
	      else
	      {
		      sprintf(Controller,"%s_%s",dr->location,dr->data);
	      }

	      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> ",dateString,timeString,Controller,Line);
	      
	      //ZCS controller will have an extra column for the stations it governs
	      if('7'==*(dr->data) && atoi(dr->data)%7681<3)
	      {
		      fprintf(emailMsg,"<td>%s</td> </tr> \n",ZCS2STATION[atoi(dr->data)%7681]); 
	      }
	      else
	      {
		      fprintf(emailMsg,"</tr> \n");
	      }
	      free(Controller);
	     }
	     else if (contains(incidentType->processingFlags,"F")) 
	     {
	      char* CC = dr->data;
	      char* loc = dr->location;
	      char* TrainNum = getCarNumString(dr->data,ccPairLLHead);
	      char* RunNum;
	      if('\0'==*(dr->extra)) 
	      {
		RunNum = dr->other;
		fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>CC%02d-%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,atoi(CC),TrainNum,RunNum,loc);
	      }
	      //Necessary condition for UCCR incidents
	      else 
	      {
		RunNum = dr->extra;
		char* user = dr->other;
		fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>CC%02d-%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,user,atoi(CC),TrainNum,RunNum,loc);
	      }
	      free(TrainNum);
	     }
	     else if (contains(incidentType->processingFlags,"C"))
	     {
	      char* loc = dr->location;
	      char* critIncident = (char*)calloc(STRING_LENGTH, sizeof(char)); 
	      
	      //The incident may be critical
	      //If so, an image aligned to the row is pasted signifying importance
	      if( TRUE==checkCriticalAlert(incidentType,dr->data) ) 
	      {
		fprintf(emailMsg, "<tr style=\"color:#f20000\">");
		char* imgSrc = (char*)calloc(STRING_LENGTH, sizeof(char)); 
		constructLocalFilepath(imgSrc,EMAIL_TIME_FOLDER,"Critical_Incident_Image",".jpg");
		sprintf(critIncident,"<img align=\"left\" src=\".%s\" alt=\"CRITICAL INCIDENT\">",imgSrc);
		free(imgSrc);
	      }
	      else
	      {
		      fprintf(emailMsg,"<tr>");
	      }
	      
	      //The 'border-color:white' is necessary for alignment of an image in the case that there is a critical incident
	      if('\0'==*(dr->data))
	      {
		      fprintf(emailMsg, "<td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> <td style=\'border-color:white;\'>%s</td></tr>\n",dateString,timeString,loc,incidentType->object,critIncident);
	      }
	      else{ 
		char* equipment = fullNameFromPartial(dr->data,formatedLine);
		fprintf(emailMsg, "<td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> <td style=\'border-color:white;\'>%s</td></tr>\n",dateString,timeString,loc,equipment,critIncident);
	      }
	      free(critIncident);
	     }
	      //header for all incidents without ProcessingFlags
	    else
	    {
		    char* loc = dr->location;

		     //If extra member is non-empty, it is RLSC OR CLSC Type; Otherwise
		     //If data member is empty & object is empty, print just location; Otherwise
		     //If data member is empty & object is nonempty, print location & object; Otherwise
		     //Print both location & data members
	     if(*(dr->extra)!='\0')
	     {
	       if('\0'==*(dr->other))
	       {
		       fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,loc,dr->data);
	       }
	       else
	       {
		       fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s - %s</td> </tr>\n",dateString,timeString,loc,dr->other,dr->data);
	       }
	     }
	     else if('\0'==*(dr->data)) 
	     {
		if('\0'==*(incidentType->object))
		{
			fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,loc);
		}
		else
		{
			fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,loc,incidentType->object); 
		}
	}
	     else 
	     {
		char* equipment = fullNameFromPartial(dr->data,formatedLine);
		fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n",dateString,timeString,loc,equipment);
	     } 
	   }

	te = te->next;
	free(formatedLine);
	free(s);
      } 
      //If incident is the last incident of List, then close the table
      if(dr->lastRecord){ 
        fprintf(emailMsg, "</table>");
        fprintf(emailMsg,"<br>"); 
        fprintf(emailMsg,"<br>");
      }
      else
      {
	      ;//do nothing
      }
      if( fclose(emailMsg) == EOF )
      {
        printf("There was an error with file: %s\n The file could not be closed by the program\n", tmp->bodyFileName);
        printf("Cannot close emailtimediff l |%s| from read. Skipping.\n", filePath);
        printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
      }
      
    }
    tmp=tmp->next;

  }
  free(filePath);
}


//When a summary email needs to be sent, this function will check who needs to be emailed
//and create one email to send to all of them, that contains an event line detailing the 
//incident and a table containing an entry for each time the incident happened.
//	sel		-- A list of summary emails. summary email struct is very similar to incident struct and database struct.
//	el		-- Email list of all possible email recipients
//	incidnetTypeList-- A list with all incidentTypes that were read in.
//	return		-- void
void sendSummaryEmails(struct SummaryEmailList* sel, struct EmailInfoList* el, struct IncidentTypeList* incidentTypeList, struct CCPair* ccPairLLHead) {
    struct SummaryEmail* se = sel->head;
    struct EmailInfo* ei = el->head;
    const struct IncidentType* incidentTypeListTraveller;
    char* command;
    
    while(NULL != se) {
        char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
        constructLocalFilepath(filePath, EMAIL_INFO, se->emailFileName, DOT_HTML);
        FILE* emailMsg = fopen(filePath, "w");

        if(NULL == emailMsg) {
            printf("Cannot open email |%s| for write. Skipping.\n", filePath);
            printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
            return;
        }
        else {
            //make threshold list
	    BOOL found = FALSE;
	    incidentTypeListTraveller = incidentTypeList->head;
            struct ThresholdList* thresholdList;
	    const struct IncidentType* incidentType;
	    //find the incidentType that coresponds to the typeOfIncident string
            while(!found && incidentTypeListTraveller != NULL)
	    {
		if(strcmp(se->typeOfIncident,incidentTypeListTraveller->typeOfIncident) == 0)
		{
			//get threshold from indcident type
			thresholdList = incidentTypeListTraveller->thresholdList;
			incidentType = incidentTypeListTraveller;
			found = TRUE;
		}
		incidentTypeListTraveller = incidentTypeListTraveller->next;
	    }
	    //if the incidentType could not be found
	    if(!found)
	    {
		printf("Error : Could not match typeOfIncident :%s: for Summary Incident Email \n",se->typeOfIncident);
		return;
	    }
            
            //discover which threshold condition has been met
            struct DatabaseRecord* dr = malloc(sizeof(struct DatabaseRecord));
            dr->data = se->data;
            dr->location = se->location;
            dr->other = se->other;
	    dr->subwayLine = NULL;
            dr->typeOfIncident = se->typeOfIncident;
            dr->timeList = se->tl;
            
            struct Threshold* th_tmp = thresholdList->head;
            struct TimeElement* start;
            int numOfIncidents = -1;
            int numOfMinutes = -1;
            int largestThresholdNumMinutes = th_tmp->numOfMinutes;
            while(NULL != th_tmp) {
                int thresholdResult = checkSummaryThresholdCondition(th_tmp, dr, &start);
                if(CONDITION_MET == thresholdResult) {
                    numOfIncidents = th_tmp->numOfIncidents;
                    numOfMinutes = th_tmp->numOfMinutes;
                }
                if(INCIDENT_OVER_TIME == thresholdResult) {
                    numOfIncidents = th_tmp->numOfIncidents;
                }
                if(th_tmp->numOfMinutes > largestThresholdNumMinutes) {
                    largestThresholdNumMinutes = th_tmp->numOfMinutes;
                }
               th_tmp=th_tmp->next;
            }
            if(-1 == numOfMinutes) {
                numOfMinutes = largestThresholdNumMinutes;
            }
            
            //if no events have been added since sending out initial email, do
            //not send email
            if(se->tl->count > numOfIncidents) {

		const struct IncidentType* incidentTypeCheck = NULL;

		//checks to see if a FCU event happened on line 3
		if(contains(incidentType->processingFlags,CHECK_TCS_VHLC_SERVER_FLAG) == TRUE)
		{
			incidentTypeCheck = checkIncidentName(dr->typeOfIncident, dr->location);
		}
		//if the FCU event did happen on line 3, change incidentTypes
		if(incidentTypeCheck != NULL)
		{
			incidentType = incidentTypeCheck;
		}
            
                fprintf(emailMsg, "From: WBSS-noreply@ttc.ca\n");
                fprintf(emailMsg, "to: ");

                //print destination email addresses to file
                 while(NULL != ei ) {
                     if(typeOfIncidentListContain(ei->typeOfIncidentList, se->typeOfIncident)) {
                         fprintf(emailMsg, "%s, ", ei->personsEmail);
                     }
                     ei = ei->next;
                 }
                 ei = el->head;
		//get subject line
		char* subjectLine = getFormatedLine(ccPairLLHead, incidentType,se->data,se->location,se->other,se->extra,SUMMARY_LINE);
		//print subject line
		fprintf(emailMsg,"\nSubject: %s - Automated CSS Alarm Tool\n",subjectLine);
                fprintf(emailMsg, "Content-Type: text/html\n");
		fprintf(emailMsg, "MIME-Version: 1.0\n\n");
		fprintf(emailMsg, "<!DOCTYPE html>\n");
                fprintf(emailMsg, "<html><head><style>body { font-family: \"Arial\", sans-serif; font-size: 14px; }\n table, th, td { border: 1px solid black; border-collapse: collapse; }\n td, th { padding-left: 0.625em; padding-right: 0.625em; line-height: 120%; text-align: center; }\n th { font-weight: bold; }</style></head><body>");
		//fprintf(emailMsg, "</h1>\n");
                char* eventLine = getFormatedLine(ccPairLLHead, incidentType,se->data,se->location,se->other,se->extra,EVENT_LINE);

                if(contains(incidentType->processingFlags, ONBOARD_INCIDENT_FLAG))
                {
                  removeFirstChars(subjectLine, strlen("Summary For : "));
                  fprintf(emailMsg, subjectLine);
                }
                else
                {
                  fprintf(emailMsg,eventLine);
                }
    free(subjectLine);
		free(eventLine);

                fprintf(emailMsg, " %d incident(s) in %d hour(s) and %d minute(s).\n",
                        se->tl->count, (int)numOfMinutes / (int)60,(int)numOfMinutes % 60);

                if(contains(incidentType->processingFlags, ONBOARD_INCIDENT_FLAG))
                {
                  fprintf(emailMsg, "<table><tr> <th>Date(MM/DD/YY)</th> <th>Time(24Hr)</th> <th>Location</th> </tr></b>\n");
                }
                else
                {
                  fprintf(emailMsg, "<table><tr> <th>Date(MM/DD/YY)</th> <th>Time(24Hr)</th> </tr></b>\n");
                }

                struct TimeElement* te = se->tl->head;
                while(NULL != te) {
                    char* s;
                    char* timeString = strtok(s = getStringFromDate(te->timeObj), " ");
                    char* dateString = strtok(NULL, " ");
                    if(contains(incidentType->processingFlags, ONBOARD_INCIDENT_FLAG))
                    {
                      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> <td>%s</td> </tr>\n", dateString, timeString, te->location);
                    }
                    else
                    {
                      fprintf(emailMsg, "<tr> <td>%s</td> <td>%s</td> </tr>\n", dateString, timeString);
                    }
                    
                    free(s);
                    te = te->next;
                }

                fprintf(emailMsg, "</table><hr><p>This is an automated message from the CSS Alarm Tool ");
		fprintf(emailMsg, VERSION);
		fprintf(emailMsg, "<br>\n");
                fprintf(emailMsg, "Do not reply; this email account is not monitored.</p></body></html>\n");
                fclose(emailMsg);

                char* sum_command = (char*)calloc(STRING_LENGTH, sizeof(char));
                printf("Sending summary email.\n");
                sprintf(sum_command, "cat %s | /usr/sbin/sendmail -Ac -t -v", filePath);
                system(sum_command);
                free(sum_command);
            }
            else {
                fclose(emailMsg);
                remove(filePath);
            }
            se = se->next;
        }
    }
}
//If no incidents happened in the last 24 hours, ACAT will send an email to all admins that
//says no incidents happened in the last 24 hours, so they know it hasnt crashed.
//	ei	-- EmailInfo struct of the admin that will be emailed
//	return	-- Void
void addAllClearMessage(struct EmailInfo* ei) {
	char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
	constructLocalFilepath(filePath, EMAIL_INFO, ei->emailFileName, DOT_HTML);
	FILE* emailMsg = fopen(filePath, "w");
	
	ei->sendEmail = TRUE;
	//Email Header
	fprintf(emailMsg, "From: WBSS-noreply@ttc.ca\n");
	fprintf(emailMsg, "To: %s\n", ei->personsEmail);
	fprintf(emailMsg, "Subject: Automated CSS Alarm Tool: 24-hour All-Clear\n");
	fprintf(emailMsg, "Mime-Version: 1.0\n");
	fprintf(emailMsg, "Content-Type: text/html\n\n");
	fprintf(emailMsg, "<!DOCTYPE html>\n");
	fprintf(emailMsg, "<html><head><style>table, th, td { border: 1px solid black; border-collapse: collapse; }</style></head><body>\n");
	fprintf(emailMsg, VERSION);
	fprintf(emailMsg, "</h1>\n");
		
	//All-clear message
	fprintf(emailMsg, "<p>There have been no tracked alarms in the past 24 hours.</p>\n");
	
	if (EOF == fclose(emailMsg)) {
		printf("There was an error with file: %s\n The file could not be closed by the program\n", ei->emailFileName);
		printf("Cannot close email |%s| from read. Skipping. \n", filePath);
		printf("errorno = %d\n strerror is %s\n", errno, strerror(errno));
	}
}
//Adds the closing tags to emails
//	ei	-- EmailInfo of the recipient who's email the closing tags will be written to
//	return	-- void
void addClosingTagsToEmail(struct EmailInfo* ei) {
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath, EMAIL_INFO, ei->emailFileName, DOT_HTML);
  FILE* emailMsg = fopen(filePath, "a");
  
  if(NULL == emailMsg) {
    printf("Cannot open email |%s| for appending\n", filePath);
    printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
    free(filePath);
    return;
  }
  else {
    fprintf(emailMsg, "<hr>\n<p>This is an automated message from the CSS Alarm Tool ");
    fprintf(emailMsg, VERSION);
    fprintf(emailMsg, "<br>Do not reply, this email account is not monitored.</p></body></html>");
    if(EOF == fclose(emailMsg)) {
      printf("Cannot close email |%s| after appending tags\n", filePath);
      printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
    }
  }
  free(filePath);
}
//Reads the time of when the last email was sent to keep track of when the 24 Hours no incident
//email should be sent. If there is no file with a previous time, it creates one and writes the current time. 
//	current_time	-- The time of when the program started executing
//	return		-- The time of when the last email was sent.
time_t readTimeOfLastEmail(time_t current_time) {
	char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
	constructLocalFilepath(filePath, EMAIL_TIME_FOLDER, EMAIL_TIME_FILENAME, DOT_TXT);
	FILE* lastEmailTimeFile = fopen(filePath, "r");
	time_t lastEmailTime;
	char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
	
	if (NULL == lastEmailTimeFile) {
		//There is no file containing the last time of a sent email
		//Create the file, and return the current time
		lastEmailTimeFile = fopen(filePath, "w");
		fprintf(lastEmailTimeFile, "%s", getStringFromDate(current_time));
		if (EOF == fclose(lastEmailTimeFile)) {
			printf("Output database |%s| could not be closed\n", filePath);
			printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
		}
		lastEmailTime = current_time;
	}
	else {
		int lineRes = readInLine(lastEmailTimeFile, &tmp, STRING_LENGTH);
		BOOL time_success = getDateFromString(tmp, &lastEmailTime);
    if (!time_success)
    {
      lastEmailTime = current_time;
      printf("[ERROR] Could not interpret content of %s.\n", EMAIL_TIME_FILENAME);
    }
	}
        //free things
	free(filePath);
        free(tmp);
        fclose(lastEmailTimeFile);
	return lastEmailTime;
}
//writes the time of the last email to the last email time file.
//	current_time	-- The time of when the last email was sent
//	return		-- void
void writeTimeOfLastEmail(time_t current_time) {
	char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
	constructLocalFilepath(filePath, EMAIL_TIME_FOLDER, EMAIL_TIME_FILENAME, DOT_TXT);
	FILE* lastEmailTimeFile = fopen(filePath, "w");
	
  char* s = getStringFromDate(current_time); //tmp variable for getStringFromDate
  if (NULL == s || strlen(s) < 8)
  {
    printf("[ERROR] Could not get current time. (writeTimeOfLastEmail)\n");
  }
  else
  {
	  fprintf(lastEmailTimeFile, "%s", s );
  }
	free(s);
        
	if (EOF == fclose(lastEmailTimeFile)) {
		printf("Output database |%s| could not be closed\n", filePath);
		printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
	}
	free(filePath);
	return;
}

int main() {
  
  // INCIDENT TYPE LIST
  // Programmer defined linked-list of structs
  // Incident types are defined in ./Other/Incident_Types.txt, and
  // the program uses these to know what incidents to look for.
  // Also used to format the emails that this program sends.
  struct IncidentTypeList* incidentTypeList = (struct IncidentTypeList*)malloc(sizeof(struct IncidentTypeList));
  createIncidentTypeList(incidentTypeList);
  int result = readInIncidentTypes(incidentTypeList);

  abrvPath = calloc(STRING_LENGTH, sizeof(char));
	constructLocalFilepath(abrvPath, EMAIL_TIME_FOLDER,"Abrvs", DOT_TXT);
  if(result == ERROR)
  {
      exit(0);
  }
  printf("%d Incident Types have been read in\n",incidentTypeList->count);
  printf("\nReading in CC mapping.\n");
  struct CCPair* ccPairLLHead = readInCCMapping();
  printf("CC mapping has been read in\n");
  // Current Time
  // Initially put in for debugging purposed to set date to June 14 - 16th, 
  // as sample data was from this time
  // left in to allow for easier debugging or checking when code last ran.
  time_t t = time(NULL) - OFFSET*24*60*60;
  char*s; //variable for string of date
  printf("Current time is: %s\n\n", s = getStringFromDate(t));
  free(s);
  
  // Time when last email was sent
  time_t lastEmailSent = readTimeOfLastEmail(t);
  printf("Last email was sent at: %s\n\n", s = getStringFromDate(lastEmailSent));
  free(s);
  
  // Was the last email sent more than 24 hours ago?
  BOOL flag24Hours;
  if (t - lastEmailSent > 24*60*60) {
	  flag24Hours = TRUE;
  }
  else {
	  flag24Hours = FALSE;
  }
  printf("More than 24 hours ago? %d\n", flag24Hours);
  
  // EMAIL RECIPIENTS LIST
  // Programmer-defined linked list of structs,
  // file is read in from "./Email_Info/email_recipients.txt" whcih contains
  // employees email addresses as well as the incidents that they should be 
  // emailed about. Each struct in the list also has an html file associated 
  // with it that is updated with occurrences that meet threshold conditions,
  // Each html file is emailed to the associated recipients, and then deleted.
  struct EmailInfoList* emailInfoList = malloc(sizeof(struct EmailInfoList));
  createEmailInfoList(emailInfoList);
  readInEmailInfoFile(emailInfoList);

  printf("Email Recipients List\n");
  
  printEmailInfoList(emailInfoList);
  printf("\n");
  
  

  // SUMMARY EMAIL LIST
  // As database records expire, events that have hit threshold conditions
  // are sent out in a summary email, in order to track severity of a problem
  // This list consists of every summary email that has to be sent out
  struct SummaryEmailList* sel = malloc(sizeof(struct SummaryEmailList));
  createSummaryEmailList(sel);
  
  
  // INCIDENTS LIST
  // Programmer defined linked-list of structs
  // Incidents are read in from constantly changing, constantly updating log files
  // The extensions for these files (which could change) are stored in the
  // file "./Other/LogFolderPath.txt"
  struct IncidentList* incidentList = malloc(sizeof(struct IncidentList));
  createIncidentList(incidentList);
  result = readInFiles(incidentList,incidentTypeList); 
  if(result == ERROR) {
    printf("There was an error in reading in the list of incidents. Program terminating\n");
    return 0;
  }
  
  printf("Incident List\n");
  printIncidentList(incidentList, NULL);
  printIncidentsToLogs(incidentList, ccPairLLHead);
  printf("\n");
   
  
  // PROCESS INFORMATION
  // Information is processed individually for each incident. Each incident's
  // database is read in and stored in a linked list and new incidents, from 
  // the incident list, are added to this database list as well. Once this is
  // complete the database list is check against threshold conditions and if they meet or
  // exceed a threshold condition the information from the database is added
  // to the .html email file of each person in 'emailInfoList' (see above) who
  // is supposed to get email about this type of incident
  struct IncidentType* incidentTypeListTraveller = incidentTypeList->head;


  while(incidentTypeListTraveller != NULL)
  {  
      processInfo(incidentTypeListTraveller->typeOfIncident,incidentList,emailInfoList,sel,incidentTypeListTraveller,ccPairLLHead);
  	incidentTypeListTraveller = incidentTypeListTraveller->next;
  }

  // Finish emails
  struct EmailInfo* ei = emailInfoList->head;
  char* command = (char*)calloc(STRING_LENGTH, sizeof(char));
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  
  BOOL emailSentFlag = FALSE;
  
  // Cycle through emailInfoList to see if any info has been written to the
  // associated files and if it has, add closing tags to the email and send it.
  while(ei!=NULL) {
    if(ei->sendEmail == TRUE) { // variable updated when file is first created
      print_subject_line(ei);

      addClosingTagsToEmail(ei);
      constructLocalFilepath(filePath, EMAIL_INFO, ei->emailFileName, DOT_HTML);
      sprintf(command, "cat %s | /usr/sbin/sendmail -Ac -t -v", filePath);
      // debugging purposes
      printf("My command = '%s'\n", command);
      system(command);
      emailSentFlag = TRUE;
    }
    else {
      //If there have been no emails sent for any alarm in the past 24 hours, and if the current
      //email recipient is a member of the admin group, send an all-clear message
      if (flag24Hours == TRUE && ei->isAdmin == TRUE) {
        addAllClearMessage(ei);
		printf("Sent all-clear message to %s\n", ei->personsEmail);
		addClosingTagsToEmail(ei);
		constructLocalFilepath(filePath, EMAIL_INFO, ei->emailFileName, DOT_HTML);
		sprintf(command, "cat %s | /usr/sbin/sendmail -Ac -t -v", filePath);
		// debugging purposes
		printf("My command = '%s'\n", command);
		system(command);
		emailSentFlag = TRUE;
      }
    }
    ei = ei->next;
  }
  //send out summary emails now
  printf("Summary list count: %d\n", getCountOfSummaryEmailList(sel));
  sendSummaryEmails(sel, emailInfoList,incidentTypeList, ccPairLLHead);
  //free things
  destroySummaryEmailList(sel);
  destroyEmailInfoList(emailInfoList);  
  destroyIncidentList(incidentList);
  if (emailSentFlag == TRUE) {
	  writeTimeOfLastEmail(t);
  }
  deleteIncidentTypeList(incidentTypeList);
  deleteCCPairList(ccPairLLHead);
  free(command);
  free(filePath);
  free(abrvPath);
  return 0;
}
