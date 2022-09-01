#include "DatabaseRecord.h"
//#include "DateAndTime.h"
//#include "EmailInfo.h"

/*------------------------------------------------------
**
** File: DatabaseRecord.c
** Author: Matt Weston & Richard Tam
** Created: August 31, 2015
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - Changed #include files
**                      - Changed ReadInDBFile method
**                      - Changed printDatabaseToFile
**                      - Introduced addIncidentsToDatabaseList method
**                      
** 23 Mar 2016: Rev 2.1 - RTam
**						- Updated addIncidentsToDatabaseList method to include new server switchover alarm types
**                        in check for incidents where location is NULL
**
** 3 Oct 2016: Rev 2.2 RC3  - MTedesco
**                          - Added new "summary email" feature - sends out a summary email of how many incidents occurred at a location
**                              after the email time delay expires (new function sendSummaryEmails)
** 25 Nov 2016: Rev 2.2      - MTedesco
**                          - Bug fixes for rev 2.2 RC3 (mostly to summary email feature)
**                          - Created checkSummaryThresholdConditions - modification of 
**                              checkThresholdConditions, specifically for summary emails
**
** 22 Oct 2018: Rev 4.0 - IMiller
**                      - Bug fix for Incidents on-board trains (Redmine Issue #1324)
*/

// Initialize variables for DatabaseList
//	dbl	- The Database List to be created
//	return	- Void
void createDatabaseList(struct DatabaseList* dbl) {
	dbl->head = NULL;
	dbl->tail = dbl->head;
	dbl->count = 0;
  dbl->isOnBoardIncident = FALSE;
}

// Standard linked list, queue style, data is inserted at the end of the list,
// at the tail 
//	dbl	- The Database List the Database record will be added to
//	dr	- The Database record to be added
//	return	- Void
void insertIntoDatabaseList(struct DatabaseList* dbl, struct DatabaseRecord* dr) {
	dr->next = NULL; // just to be sure.
	if(dbl->head == NULL) {
		dbl->head = dr;
		dbl->tail = dr;
		dbl->count = 1;
	}
	else {
		dbl->tail->next = dr;
		dbl->tail = dr;
		dbl->count++;
	}
}

// 'updating' a databaseRecord means adding a new time to its timeList 
// (likely one from an incident list, that was read in from a logfile)
// also checks for duplicate times so the email message doesn't have 8-10 events that
// happens at the same time (Current Bandaid Solution).
//	dr	- The Database record in which the new time event will be added
//	te	- The new time event to be added
//	return	- Void
void updateDatabaseRecord(struct DatabaseRecord* dr, struct TimeElement* te) {
  	// insert new time into DatabaseRecord's TimeList
	BOOL sameTime = FALSE;
	struct TimeElement* recordedTimes = dr->timeList->head;
	while(NULL != recordedTimes && FALSE != sameTime)
	{
		if(recordedTimes->timeObj == te->timeObj)
		{
			sameTime = TRUE;
		}
		recordedTimes = recordedTimes->next;
	}
	//if an event at this time has not been recorded yet, add it to the list
	if(FALSE == sameTime)
	{
		insert(dr->timeList, te);
	}
}

// Used for debugging purposes and user-side checks.
// prints list in user-friendly way
//	dbl	- The Database List to be printed
//	return	- Void
void printDatabaseList(struct DatabaseList* dbl) {
	struct DatabaseRecord* tmp = dbl->head;
	if(dbl->count > 0)
  {
    //If onboard incident
    if(TRUE == dbl->isOnBoardIncident)
    {
      while(tmp != NULL)
      {
        if(strcmp(tmp->flag->msg,NOEMAIL)==0) {
          printf("CC: %s|msg:%s|", tmp->data, tmp->flag->msg);
        }
        else {
          printf("CC: %s|msg:%s-%s|", tmp->data, tmp->flag->msg, getStringFromDate(tmp->flag->timeOfEmail));
        }
        printOnboardTimeList(stdout, tmp->timeList);
        tmp = tmp->next;
		  }
    }
    else
    {
      while(tmp != NULL) {
        if(strcmp(tmp->flag->msg,NOEMAIL)==0) {
          printf("Loc:%s|Data:%s|msg:%s|", tmp->location, tmp->data, tmp->flag->msg);
        }
        else {
          printf("Loc:%s|Data:%s|msg:%s-%s|", tmp->location, tmp->data, tmp->flag->msg, getStringFromDate(tmp->flag->timeOfEmail));
        }
        printTimeList(stdout, tmp->timeList);
        tmp = tmp->next;
      }
    }
	}
	else {
		printf("DatabaseList is empty: count = %d\n", dbl->count);
	}
}

// entries are removed from the linked-list at the head, as is standard for
// queue-style linked lists.
// this removed method return a pointer to the removed element, but will not 
// delete it
//	dbl	- The Database List who's head will be removed
//	dr	- ??? Probably supposed to be the reference to the removed database record
//		  but isn't properly coded as such
void removeFromDatabaseList(struct DatabaseList* dbl, struct DatabaseRecord* dr) {
	dr = dbl->head;
	if(dbl->count > 1) {
		dbl->head = dbl->head->next;
		dbl->count--;
		dr->next = NULL; // ensure complete sever from the list
	}
	else if(dbl->count == 1) {
		dbl->head = NULL;
		dbl->tail = dbl->head;
		dbl->count = 0;
		dr->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		dbl->count=0; // just a precaution
	}
}

// entries are removed from the linked-list at the head, as is standard for
// queue-style linked lists.
// this method will delete the databaseRecord
//	dbl	- The database list who's head will be removed and it's memory freed
//	return	- Void
void removeAndDestroyDatabaseRecord(struct DatabaseList* dbl) {
	struct DatabaseRecord* dr = dbl->head;
	if(dbl->count > 1) {
		dbl->head = dbl->head->next;
		dbl->count--;
		dr->next = NULL; // ensure complete sever from the list
	}
	else if(dbl->count == 1) {
		dbl->head = NULL;
		dbl->tail = dbl->head;
		dbl->count = 0;
		dr->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		dbl->count=0; // just a precaution
	}
        destroyTimeList(dr->timeList);
        free(dr->flag->msg);
        free(dr->flag);
	free(dr->location);
	free(dr->data);
        free(dr->other);
        free(dr->extra);
	free(dr->lastSummaryEvent);
        free(dr->typeOfIncident);
	free(dr->subwayLine);
	free(dr);
}

int getCountOfDatabaseList(struct DatabaseList* dbl) {
return dbl->count;
}

// recursively calls removeAndDestroyDatabaseRecord until list is empty and
// then deletes the list itself
//	dbl	- The Database List to be destroyed.
void destroyDatabaseList(struct DatabaseList* dbl) {
	while(dbl->count > 0) {
		removeAndDestroyDatabaseRecord(dbl);
	}
	free(dbl);
}

// A certain type of incident's Database file will be read in and saved in a
// linked list of DatabaseRecord structs. This method has a caveat that incidents
// in the database file that occured more than X hours ago, where X is 
// 'emailDelayTimeMinutes', are not read in. These entries are considered 'expired'

// emailDelayTimeMinutes is a variable that is passed in from a function called on the 
// thresholdList for this type of incident. The function finds the max number of hours
// ago records must be kept from in order to successfully check the threshold
// conditions. E.g. if the threshold conditions are 5,1 and 10,24 records from 
// as long as 24 hours ago will be kept.
//	fileName		- The file name of the database file to be read
//	dbl			- The Database List in which all read Database Records will be stored
//	emailDelayTimeMinutes	- The amount of time, in minutes, after which database records will not be kept
//				  (ie if its 60, events older than 60 minutes will be ignored)
//	sel			- The Summary Email List that will store if any summary emails need to be sent
//				  for older incidents
//	incidentType		- The Incident Type of the database records that are being read
//	return			- Void
void readInDBFile(char* fileName, struct DatabaseList* dbl, 
  int emailDelayTimeMinutes, struct SummaryEmailList* sel, struct IncidentType* incidentType) {
  // open file
  char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
  char* tmpflag = (char*)calloc(STRING_LENGTH, sizeof(char));
  char* extension = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(extension, DATABASE_FILES, fileName, DOT_TXT);
  FILE *db = fopen(extension, "r");
  
  // If the database does not exist then return. One will be created near the
  // end of execution after new incidents have been merged with this 
  // databaseList
  if(db == NULL) { // db does not exist
    printf("Database '%s' does not exist. One will be created at output.\n", extension);
    free(tmp);
    free(tmpflag);
    free(extension);
    return;
  }
  else {
    int lineRes = readInLine(db, &tmp, STRING_LENGTH);
   
    // while the end of the file has not been, keep reading in lines and
    // storing them
    while(lineRes!=END_OF_FILE) {
      if(lineRes==READ_IN_STRING || lineRes==STRANGE_END_OF_FILE) {

        // create database record
        struct DatabaseRecord* dr = malloc(sizeof(struct DatabaseRecord));
	      dr->location = (char*)calloc(STRING_LENGTH, sizeof(char));
	      dr->data = (char*)calloc(STRING_LENGTH, sizeof(char));
              dr->other = (char*)calloc(STRING_LENGTH,sizeof(char));
	      dr->extra = calloc(STRING_LENGTH,sizeof(char));
	      dr->subwayLine = (char*)calloc(LINE_LENGTH,sizeof(char));
              dr->typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));
	      dr->timeList = malloc(sizeof(struct TimeList));
                createTimeList(dr->timeList);
	      dr->flag = malloc(sizeof(struct Flag));
              dr->flag->msg = (char*)calloc(STRING_LENGTH, sizeof(char));
	      dr->next = NULL;
              dr->lastSummaryEvent = (char*)calloc(STRING_LENGTH,sizeof(char));
	      dr->incidentType = incidentType;   
        //If onboard incident type
        if(TRUE == dbl->isOnBoardIncident)
        {
          removeFirstChars(tmp, strlen("CC: "));
          getCharsUpTo(tmp, dr->data, ";");
          removeFirstChars(tmp, strlen(dr->data)+1);
        }
        else
        {
          getCharsUpTo(tmp, dr->location, ";");
          removeFirstChars(tmp, strlen(dr->location)+1);
          
          getCharsUpTo(tmp, dr->data, ";");
          removeFirstChars(tmp, strlen(dr->data)+1);
        }
        
        getCharsUpTo(tmp, dr->other, ";");
        removeFirstChars(tmp, strlen(dr->other)+1);

	getCharsUpTo(tmp, dr->extra, ";");
	removeFirstChars(tmp, strlen(dr->extra)+1);
        
	getCharsUpTo(tmp, dr->subwayLine, ";");
	removeFirstChars(tmp, strlen(dr->subwayLine)+1);

        getCharsUpTo(tmp, dr->typeOfIncident, ";");
        removeFirstChars(tmp, strlen(dr->typeOfIncident)+1);
        
        getCharsUpTo(tmp, tmpflag, ";");
        removeFirstChars(tmp, strlen(tmpflag)+1);
        
        getCharsUpTo(tmp, dr->lastSummaryEvent, ";");
        removeFirstChars(tmp,strlen(dr->lastSummaryEvent)+1);
        
                
        // swapped c for tmp
        int tmpTimeStrLength = (strlen(tmp)/STRING_LENGTH + 1)*STRING_LENGTH;
        char* tmpTimeStr = (char*)calloc(tmpTimeStrLength, sizeof(char));
        strcpy(tmpTimeStr, tmp);
        
        //set up summary email structures
        BOOL sendSummary = FALSE;
        struct TimeList* summaryTimeList = malloc(sizeof(struct TimeList));
        createTimeList(summaryTimeList);
        
        //If onboard incident
        if(TRUE == dbl->isOnBoardIncident)
        {
          // split up timestr into a linked list of TimeElement structs. 
          while( !(tmpTimeStr==NULL || strcmp(tmpTimeStr,"")==0) ) {
            time_t tmpTime;
            char* tmp_loc = calloc(STRING_LENGTH, sizeof(char));
            getDateFromString(tmpTimeStr, &tmpTime);

            if(contains(tmpTimeStr, ","))
            {
              getCharsUpTo(tmpTimeStr, tmp_loc, ",");
              removeFirstChars(tmpTimeStr, (strlen(tmp_loc) + 1));
            }
              //Else must be last location
            else
            {
              strcpy(tmp_loc, tmpTimeStr);
              removeFirstChars(tmpTimeStr, strlen(tmp_loc));
            }

            // If the incident happened within the email delay time from the 
            // current time than it is read in and kept.
            // else it is ignored.
            if(tmpTime + emailDelayTimeMinutes*60 > time(NULL)- OFFSET*24*60*60) {
              struct TimeElement* te = malloc(sizeof(struct TimeElement));
              te->timeObj = tmpTime;
              te->location = calloc(STRING_LENGTH, sizeof(char));
              strcpy(te->location, tmp_loc);

              insert(dr->timeList, te);
              if(sendSummary) {
                  struct TimeElement* sum = malloc(sizeof(struct TimeElement));
                  sum->timeObj = tmpTime;
                  sum->location = calloc(STRING_LENGTH, sizeof(char));
                  strcpy(sum->location, tmp_loc);
                  insert(summaryTimeList, sum);
              }
            }
            //it's expired and an email has been sent about the issue
            //and the event that last triggered is over the emailDelayTimeMinutes
            //period from this event's time
            //if sumTimeStr is 0, then that indicates that no summary email has
            //ever been sent for this event
            else if(strcmp(tmpflag, NOEMAIL) != 0){
              if(!sendSummary) {
                time_t lastSumTime;
                char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
                strcpy(tmp, dr->lastSummaryEvent);
                BOOL flag = getDateFromString(tmp, &lastSumTime);
            
                free(tmp);
                //if flag is false, then a summary email has not been sent before
                //if flag is true, an email has been sent before and therefore
                //the time of the event being processed should be larger than
                //the previous event processed by at least the email delay amount
                if(!flag || lastSumTime + emailDelayTimeMinutes*60 < tmpTime) {
                  struct TimeElement* te = malloc(sizeof(struct TimeElement));
                  te->timeObj = tmpTime;
                  te->location = calloc(STRING_LENGTH, sizeof(char));
                  strcpy(te->location, tmp_loc);
                  insert(summaryTimeList, te);
                  //insert time into database record
                  char* s;
                  strcpy(dr->lastSummaryEvent,s = getStringFromDate(te->timeObj));
                  free(s);
                  sendSummary = TRUE;   
                }
              }
              else {
                  struct TimeElement* te = malloc(sizeof(struct TimeElement));
                  te->timeObj = tmpTime;
                  te->location = calloc(STRING_LENGTH, sizeof(char));
                  strcpy(te->location, tmp_loc);
                  insert(summaryTimeList, te);
              }
            }
            free(tmp_loc);
          }
        }
          //Not an onboard incident
        else
        {
          // split up timestr into a linked list of TimeElement structs.
          while( !(tmpTimeStr==NULL || strcmp(tmpTimeStr,"")==0) ) {
            time_t tmpTime;
            getDateFromString(tmpTimeStr, &tmpTime);

            // If the incident happened within the email delay time from the 
            // current time than it is read in and kept.
            // else it is ignored.
            if(tmpTime + emailDelayTimeMinutes*60 > time(NULL)- OFFSET*24*60*60) {
              struct TimeElement* te = malloc(sizeof(struct TimeElement));
              te->timeObj = tmpTime;
              te->location = calloc(STRING_LENGTH, sizeof(char));

              insert(dr->timeList, te);
              if(sendSummary) {
                  struct TimeElement* sum = malloc(sizeof(struct TimeElement));
                  sum->timeObj = tmpTime;
                  sum->location = calloc(STRING_LENGTH, sizeof(char));
                  insert(summaryTimeList, sum);
              }
            }
            //it's expired and an email has been sent about the issue
            //and the event that last triggered is over the emailDelayTimeMinutes
            //period from this event's time
            //if sumTimeStr is 0, then that indicates that no summary email has
            //ever been sent for this event
            else if(strcmp(tmpflag, NOEMAIL) != 0){
              if(!sendSummary) {
                time_t lastSumTime;
                char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
                strcpy(tmp, dr->lastSummaryEvent);
                BOOL flag = getDateFromString(tmp, &lastSumTime);
            
                free(tmp);
                //if flag is false, then a summary email has not been sent before
                //if flag is true, an email has been sent before and therefore
                //the time of the event being processed should be larger than
                //the previous event processed by at least the email delay amount
                if(!flag || lastSumTime + emailDelayTimeMinutes*60 < tmpTime) {
                  struct TimeElement* te = malloc(sizeof(struct TimeElement));
                  te->timeObj = tmpTime;
                  te->location = calloc(STRING_LENGTH, sizeof(char));
                  insert(summaryTimeList, te);
                  //insert time into database record
                  char* s;
                  strcpy(dr->lastSummaryEvent,s = getStringFromDate(te->timeObj));
                  free(s);
                  sendSummary = TRUE;   
                }
              }
              else {
                  struct TimeElement* te = malloc(sizeof(struct TimeElement));
                  te->timeObj = tmpTime;
                  te->location = calloc(STRING_LENGTH, sizeof(char));
                  insert(summaryTimeList, te);
              }
            }
          }
        }
        
        //if sendSummary is true, we want to make a summary email of this issue
        //to be sent to the appropriate parties
        if(sendSummary) {
            struct SummaryEmail* se = malloc(sizeof(struct SummaryEmail));
            se->typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));
            se->data = (char*)calloc(STRING_LENGTH, sizeof(char));
            se->other = (char*)calloc(STRING_LENGTH, sizeof(char));
	    se->extra = calloc(STRING_LENGTH,sizeof(char));
            se->location = (char*)calloc(STRING_LENGTH, sizeof(char));
            se->emailFileName = (char*)calloc(STRING_LENGTH, sizeof(char));
            
            strcpy(se->typeOfIncident, dr->typeOfIncident);
            strcpy(se->data, dr->data);
            strcpy(se->location, dr->location);
	    strcpy(se->extra,dr->extra);
            strcpy(se->other, dr->other);
        
            se->tl = summaryTimeList;
	    	//clear duplicate events form the summary list
		//struct TimeElement* timeElement = se->tl->head;
		//while(timeElement != NULL)
		//{
		//	RemoveDupTime(se->tl,timeElement->timeObj);
		//	timeElement = timeElement->next;
		//}
            se->next = NULL;
 
            
            time_t t = time(NULL) - OFFSET*24*60*60;
            char* datestamp = getSlashlessDatestampFromDate(t);
            
            if(strcmp(se->location, "") == 0)
            {
              sprintf(se->emailFileName, "EMAIL_%s_%s_%s", se->typeOfIncident, se->data, datestamp);
            }
            else
            {
              sprintf(se->emailFileName, "EMAIL_%s_%s_%s", se->typeOfIncident, se->location, datestamp);
            }
            
            char* s; char* d; 
            //remove spaces from email file name
            for(s=d=se->emailFileName; *d=*s; d+=(*s++!=' '));
            
	    //get thresholdList from incidentType
            struct ThresholdList* thresholdList;
            thresholdList = incidentType->thresholdList;         

            free(datestamp);
            
            insertIntoSummaryEmailList(sel, se);
        
        }
        //clean things up
        free(tmpTimeStr);
        
        BOOL reset = FALSE;
        
        // If the flag NO EMAIL is in the file an email has not been sent
        if( strcmp(tmpflag,NOEMAIL)==0 ) {
          strcpy(dr->flag->msg, NOEMAIL);
        }
        // An email has been sent recently, 
        // but the forbidden-period after that may have expired
        // and another email can be sent
        else {
          strcpy(dr->flag->msg, EMAIL);
          removeFirstChars(tmpflag,6); // remove "EMAIL-"
          
          // Get the time the email was sent
          getDateFromString(tmpflag, &(dr->flag->timeOfEmail)); 
          
          // forbidden period has expired, reset to "NOEMAIL"
          if(dr->flag->timeOfEmail + emailDelayTimeMinutes*60 < time(NULL) - OFFSET*24*60*60) {
             strcpy(dr->flag->msg, NOEMAIL);
             dr->flag->timeOfEmail = 0;
             reset = TRUE;
          }
        }

        // with 'expired' times not being included, the timeList could be empty
        // if so the entire DatabaseRecord may be destroy, if not, the record
        // is added to the DatabaseRecord List        
        if(dr->timeList->count > 0 && reset==FALSE) {
          insertIntoDatabaseList(dbl, dr);
        }
        else
        {
          // Kill the DatabaseRecord and free the mem
          free(dr->location);
          free(dr->data);
          free(dr->flag->msg);
          free(dr->flag);
          free(dr->other);
	  free(dr->extra);
	  free(dr->subwayLine);
          free(dr->next);
          free(dr->typeOfIncident);
          destroyTimeList(dr->timeList);
          free(dr);
        }
        tmp = (char*)realloc(tmp, STRING_LENGTH);
      
      } // end of ifSTRING_LENGTH
      lineRes = readInLine(db, &tmp, STRING_LENGTH); // tmp could be larger than STRING_LENGTH, 
      // if a previously read in line reallocated tmp to a bigger size but as
      // an insurance measure, we will assume it is the absolute shortest that it can be
    } // end of while
    if(EOF == fclose(db)) {
      printf("db file |%s| could not be closed.", extension);
    }
    free(tmp);
    free(tmpflag);
    free(extension);
  }
}

// Method to take in a linked-list of DatabaseRecords and format them into
// a structured output file. The original database file will be renamed to include
// a datastamp of the current time.
//	typeOfIncident	- The short code of the name of the incident, ie TF or CTDF. (used for file name)
//	dbl		- The Database List to be printed
//	return 		- Void
void printDatabaseToFile(char* typeOfIncident, struct DatabaseList* dbl) {
  // More than one database file for each type of incident will be kept.
  // when DatabaseRecords are 'printed out' to a file, the old file is not overwritten.
  // Instead, the old or existing file is renamed to include a datestamp of the current time.
  // The file being printed, which is the active file, will not have a datestamp.
  // This active file is what is 'read in' the new time the program executes and the process repeats.
  // While the file is being printed, its name will have an "_1" after it in case the program terminates
  // unexpectedly.

  // filePath is the temporary name for the new databasefile while it is being printed  
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  char* fileName = (char*)calloc(STRING_LENGTH, sizeof(char)); 
  strcpy(fileName, typeOfIncident);
  strcat(fileName, "_1");
  constructLocalFilepath(filePath, DATABASE_FILES, fileName, DOT_TXT);
   
  // The old or existing database file will be renamed to the string 'deprecatedFileName'
  time_t t = time(NULL) - OFFSET*24*60*60;
  char* deprecatedFileName = (char*)calloc(STRING_LENGTH, sizeof(char));
  char* deprecatedFilePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  strcpy(deprecatedFileName, typeOfIncident);
  strcat(deprecatedFileName, "_deprecated_at_");
  char* s;
  strcat(deprecatedFileName, s = getSlashlessDatestampFromDate(t));
  free(s);
  constructLocalFilepath(deprecatedFilePath, DATABASE_FILES, deprecatedFileName, DOT_TXT);
 
  // originalFilePath is the name of the old or existing database file. It will eventually be renamed 
  // to 'deprecatedFilePath'
  char* originalFilePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(originalFilePath, DATABASE_FILES, typeOfIncident, DOT_TXT);

  FILE* outputDb = fopen(filePath, "w"); // XXX_1.txt file
  if(outputDb == NULL)
  {
    printf("Output database |%s| could not be opened for write\n", filePath);
    printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
  } 
  else
  {
    // Write out database file using semi-colons to separate unique fields and 
    // commas to separate incident times
    struct DatabaseRecord* dr = dbl->head;

    //If it's an onboard incident, treat CC number as location and don't print location
    if(TRUE == dbl->isOnBoardIncident)
    {
      while(dr != NULL)
      {
        (!dr->other) ? (dr->other=" ") : dr->other ;
        if(strcmp(dr->flag->msg,NOEMAIL)==0)
        {
	//fprintf(outputDb, "CC: %s;%s;%s;%s;%s;%s;", dr->data, dr->other, dr->extra, dr->typeOfIncident, dr->flag->msg, dr->lastSummaryEvent);
          fprintf(outputDb, "CC: %s;%s;%s;%s;%s;%s;%s;", dr->data, dr->other, dr->extra, dr->subwayLine, dr->typeOfIncident, dr->flag->msg, dr->lastSummaryEvent);
          printOnboardTimeList(outputDb, dr->timeList);
        }
        else
        {
          char* s; //tmp variable for getStringFromDate
          fprintf(outputDb, "CC: %s;%s;%s;%s;%s;%s-%s;%s;", dr->data, dr->other, dr->extra, dr->subwayLine, dr->typeOfIncident, dr->flag->msg,s = getStringFromDate(dr->flag->timeOfEmail), dr->lastSummaryEvent);
	//fprintf(outputDb, "CC: %s;%s;%s;%s;%s-%s;%s;", dr->data, dr->other, dr->extra, dr->typeOfIncident, dr->flag->msg,s = getStringFromDate(dr->flag->timeOfEmail), dr->lastSummaryEvent);
          free(s);
          printOnboardTimeList(outputDb, dr->timeList);
        }
        dr=dr->next;
      }
    }
    else
    {
      while(dr != NULL)
      {
        (!dr->other) ? (dr->other=" ") : dr->other ;
        if(strcmp(dr->flag->msg,NOEMAIL)==0)
        {
          fprintf(outputDb, "%s;%s;%s;%s;%s;%s;%s;%s;", dr->location, dr->data, dr->other, dr->extra, dr->subwayLine, dr->typeOfIncident, dr->flag->msg, dr->lastSummaryEvent);
	//fprintf(outputDb, "%s;%s;%s;%s;%s;%s;%s;", dr->location, dr->data, dr->other, dr->extra, dr->typeOfIncident, dr->flag->msg, dr->lastSummaryEvent);
          printTimeList(outputDb, dr->timeList);
        }
        else
        {
          char* s; //tmp variable for getStringFromDate
          //fprintf(outputDb, "%s;%s;%s;%s;%s;%s-%s;%s;", dr->location, dr->data, dr->other, dr->extra, dr->typeOfIncident, dr->flag->msg,s = getStringFromDate(dr->flag->timeOfEmail), dr->lastSummaryEvent);
	fprintf(outputDb, "%s;%s;%s;%s;%s;%s;%s-%s;%s;", dr->location, dr->data, dr->other, dr->extra, dr->subwayLine, dr->typeOfIncident, dr->flag->msg,s = getStringFromDate(dr->flag->timeOfEmail), dr->lastSummaryEvent);
          free(s);
          printTimeList(outputDb, dr->timeList);
        }
        dr=dr->next;
      }
    }

    if(EOF == fclose(outputDb)) {
      printf("Output database |%s| could not be closed\n", filePath);
      printf("errno = %d\n, strerror is %s\n", errno, strerror(errno));
    }
    else {
	  // rename the old or existing database file to the deprecated file name
      int y = rename(originalFilePath, deprecatedFilePath);
      printf("remove result = %d, err = %d\n for type %s\n", y, errno, typeOfIncident);
      // rename the 'working copy' or 'temp copy' of the new database file to the original name
	  // of the old or existing database file
      int x = rename(filePath, originalFilePath);
      printf("rename result = %d, err = %d\n for type %s\n", x, errno, typeOfIncident);
    }
  }
  free(filePath);
  free(fileName);
  free(originalFilePath);
  free(deprecatedFileName);
  free(deprecatedFilePath);
}

// This method will cycle through a DatabaseRecord's timelist and see if any
// pattern of times satisfies a particular threshold condition of X times in
// Y hours.
//	th	- The Threshold to be checked
//	dr	- The Database Record being checked to see if it triggered a threshold
//	a	- The time list to be check
//	return	- An int that indicates whether or not the threshold condition was met
int checkThresholdCondition(struct Threshold* th, struct DatabaseRecord* dr, struct TimeElement** a) {
  // If there are not enough times then they cannot possibly meet threshold
  if( dr->timeList->count < th->numOfIncidents ) {
    return NO_CONDITION_MET;
  }
  else {
    *a = (dr->timeList->head); // a is first time in range
    struct TimeElement* b = dr->timeList->head; // b is last time in range
    int i=1;
    for(i; i<th->numOfIncidents && b!=NULL; i++) {

      b = b->next; 
    } // we now have a block with a length of 'numOfIncidents'
    
    while(b!=NULL){
      if( b->timeObj - (*a)->timeObj < (th->numOfMinutes)*60 ) {
      	return CONDITION_MET;
      }
      *a = ((*a)->next);
      b = b->next;
    }
    // If b is now the last element in the list and no condition has been met,
    // then return NO_CONDITION_MET
    return NO_CONDITION_MET;
  }
  return NO_CONDITION_MET;
}
// This method will cycle through a DatabaseRecord's timelist and see if any
// pattern of times satisfies a particular threshold condition of X times in
// Y hours for summary emails.
//	th	- The threshold to be checked for
//	dr	- The Database Record to be checked
//	a	- The Database Record's time element list's head.
//	return	- An int that indictates whether or not the threshold email summary condition has been met
int checkSummaryThresholdCondition(struct Threshold* th, struct DatabaseRecord* dr, struct TimeElement** a) {
    int condition = checkThresholdCondition(th,dr,a);
    //if threshold condition is met, also check that the ENTIRE timelist fits the timespan
    if(CONDITION_MET == condition) {
        int timeDiff = difftime(dr->timeList->tail->timeObj, dr->timeList->head->timeObj);
        printf("timediff %d", timeDiff);
        //timeDiff is in seconds, convert to minutes
        if(abs(timeDiff)/60.0 > th->numOfMinutes) {
            return INCIDENT_OVER_TIME;
        }
        else {
            return CONDITION_MET;
        }
   }
    return NO_CONDITION_MET;
}



// Check if a DatabaseRecord has been emailed about or no
//	dr	- The Database Record to be checked if it has had an email sent or not
//	return	- An int indicating whether or not an email has been sent
int checkEmailStatus(struct DatabaseRecord* dr) {
  if(strcmp(dr->flag->msg,NOEMAIL)==0) {
    return EMAIL_NOT_SENT_YET;
  }
  else {
    return EMAIL_ALREADY_SENT;
  }
}

// This method is called to add all of the incidents in the incident list of
// a certain type to the database list for that same type. The database list will
// have been read in from the database file (see ./Database_Files")
//	typeOfIncident	- The short name for the type of incident thats being looked for
//	dbl		- The Database List to have matching incidents added to
//	il		- The Incident List to be looked through to find matching incidents
void addIncidentsToDatabaseList(char* typeOfIncident, struct DatabaseList* dbl, 
  struct IncidentList* il) {
  struct Incident* in = il->head;
  
  // While loop to cycle through incident List
  while(in != NULL) 
  {
    // Only add the matching type of incident, other types are ignored and will be added to a different
	  // database list
    if(strcmp(in->incidentType->typeOfIncident,typeOfIncident)==0)
    {
        
      struct DatabaseRecord* tmp = dbl->head;
      BOOL found = FALSE;
      
      // This time element will either be added to a list or inserted alone into
      // a new db object if no matching db record exists.
      struct TimeElement* te = malloc(sizeof(struct TimeElement));
      te->timeObj = in->timeElement->timeObj;
      te->location = calloc(STRING_LENGTH, sizeof(char));
      strcpy(te->location, in->location);
      te->next = NULL;
      
      //If it's an Onboard Incident (Redmine Issue #1324) 
      if(contains(in->incidentType->processingFlags, ONBOARD_INCIDENT_FLAG))
      {
        //Loop through DB Records searching for CC Number only
        while((NULL != tmp) && (FALSE == found))
        {
          found = (strcmp(tmp->data, in->data) == 0);    //CC Number

          if(found)
          {
            updateDatabaseRecord(tmp, te);
          }

          tmp = tmp->next;
        } //end of CC searching while loop
      }
      else
      { //Not an onboard incident

        // While loop to cycle through database list, searching for pre-existing
        // records of incidents at this location.
        while((NULL != tmp) && (FALSE == found))
        {
          // Compare data to see if incident matches on already in database list
          // some incidents may have NULL location, so they must be handled
          BOOL locMatch = strcmp(tmp->location, in->location)==0;
          BOOL dataMatch = strcmp(tmp->data, in->data)==0;

          //since alot of the new incidents also use other, match for other aswell
          BOOL otherMatch = TRUE;
          if(*(in->other) != '\0')
          {
            otherMatch = strcmp(tmp->other,in->other)==0;
          }

          found = otherMatch && locMatch && dataMatch;
          
          if(found) 
          {
            // simply adds a time to the timeList object of
            // the DatabaseRecord object 'tmp'
            updateDatabaseRecord(tmp, te);
          }
          tmp = tmp->next;
        } // end of database list searching while loop
      }
      
      // If the while loop has finished searching the database list and no
      // existing database record matches the incident 'in' of incidentList 'il'
      // then a new databaseRecord is created and added to the database list with
      // the new time (see 'te')
      if(!found)
      {
        struct DatabaseRecord* dr = malloc(sizeof(struct DatabaseRecord));
        dr->timeList = malloc(sizeof(struct TimeList));
          createTimeList(dr->timeList);
        dr->location = (char*)calloc(STRING_LENGTH, sizeof(char));
        dr->data = (char*)calloc(STRING_LENGTH, sizeof(char));
        dr->typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));
        dr->other = (char*)calloc(STRING_LENGTH, sizeof(char));
	      dr->extra = calloc(STRING_LENGTH,sizeof(char));
	dr->subwayLine = (char*)calloc(LINE_LENGTH,sizeof(char));
        dr->flag = malloc(sizeof(struct Flag));
        dr->flag->msg = (char*)calloc(STRING_LENGTH, sizeof(char));
        dr->next = NULL;
        dr->lastSummaryEvent = (char*)calloc(STRING_LENGTH, sizeof(char));
    
        insert(dr->timeList, te);
        strcpy(dr->location, in->location);
        strcpy(dr->data, in->data);
        strcpy(dr->other, in->other);
	      strcpy(dr->extra, in->extra);
	strcpy(dr->subwayLine, in->subwayLine);
        strcpy(dr->typeOfIncident, in->incidentType->typeOfIncident);
        strcpy(dr->flag->msg, NOEMAIL);
        strcpy(dr->lastSummaryEvent, "NA");
        insertIntoDatabaseList(dbl, dr);
      }
    }
    in = in->next;
  } //  end of incidentList searching while loop
}
