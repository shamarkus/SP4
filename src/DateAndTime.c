#include "DateAndTime.h"

/*------------------------------------------------------
**
** File: DateAndTime.c
** Author: Matt Weston & Richard Tam
** Created: August 31, 2015
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - Changed #include files
**                      - Introduced getDateFromFileName method
**                      - Created getSlashlessDatestampFromDate method
**                      -
** 04 Oct 2016: Rev 2.2 - MTedesco
 *                      - Changed getSlashlessDatestampFromString to output
 *                          YYMMDD_HHmmSS
 *                      - Changed getDateFromString to return FALSE if
 *                          string is equal to NA
*/

// form a time_t object from the string representing the date that is 
// read in from the log files.
// read in from the log files.
//	s	- The string to find the time from
//	tt	- A pointer to the time_t variable that will hold the parsed time
//	return	- A BOOL indicating whether or not a time was parsed 
BOOL getDateFromString(char* s, time_t* tt)  {	
	// Overheard
        if (strcmp(s, "NA") == 0) {
            return FALSE;
        }
	if (s == NULL || strlen(s) < 8)
	{
		return FALSE;
	}
	struct tm newDate;
	newDate.tm_isdst = -1;
	char tmp[9];

	// Get time portion
	strncpy(tmp, s, 8);
	tmp[8] = '\0';
	newDate.tm_hour = atoi(strtok(tmp, ":"));
	newDate.tm_min = atoi(strtok(NULL, ":"));
	newDate.tm_sec = atoi(strtok(NULL, ":"));
	removeFirstChars(s, 9);
	
	// Get date portion
	strncpy(tmp, s, 8);
	tmp[8] = '\0';
	newDate.tm_mon = atoi(strtok(tmp, "/")) - 1; // months begin at zero
	newDate.tm_mday = atoi(strtok(NULL, "/"));
	newDate.tm_year = atoi(strtok(NULL, "/")) + 2000 - 1900;
	removeFirstChars(s, 8);
  
	if((s[0]==' ') || (s[0]=='|') || (s[0]==',')) {
		removeFirstChars(s, 1);
	}
	
	// create TimeElement object
	*tt = mktime(&newDate);
	
	return TRUE;
}

// Initialize variables for TimeList
//	tl	- The Time List to be initalized
//	return	- Void
void createTimeList(struct TimeList* tl) {
	tl->head = NULL;
	tl->tail = tl->head;
	tl->count = 0;
}

// In order for readInFile methods and methods that check Threshold Conditions
// to work the list must be ordered oldest to newest. The method will insert in 
// order.
// Other linked-linked in other files will not share this property, they will
// be standard queues.
//	tl	- The Time List to have the new TimeElement added into
//	te	- The Time Element to be added
//	return	- Void
void insert(struct TimeList* tl, struct TimeElement* te) {
    te->next = NULL; // just to be sure, avoid conflating lists.
    if(tl->head == NULL) { // blank list
		tl->head = te;
		tl->tail = tl->head;
		tl->tail->next = NULL;
	}
	else {
		if(tl->head->timeObj >= te->timeObj ) { // insert at beginning
			te->next = tl->head;
			tl->head = te;
		}
		else if(tl->tail->timeObj <= te->timeObj) { // insert at the end
			tl->tail->next = te;
			tl->tail = te;
			tl->tail->next = NULL;
		}
		else { // cycle through list and find proper location
			struct TimeElement* tmp = tl->head;
			while(tmp->next->timeObj < te->timeObj) {
				tmp = tmp->next;
			}
			// tmp is now smaller than te,
			// tmp->next is larger.
			// insert te
			te->next = tmp->next;
			tmp->next = te;
		}
	}
	tl->count++;
}

// This method will print out TimeList as comma separatevoid RemoveDupTime(struct TimeList* tl, t_time time);d list.
// This is used to write out database file.
// see "printDatabaseToFile"
//	stream	- The file in which the Time List will be written
//	tl	- The Time List to be written
//	return	- Void
void printTimeList(FILE* stream, struct TimeList* tl) {
	struct TimeElement* tmp = tl->head;
	if(tl->count > 0) {
                char *s; //tmp var for getStringFromDate
		while(tmp->next != NULL) {
                    fprintf(stream, "%s,", s = getStringFromDate(tmp->timeObj));
                    free(s);
                    tmp = tmp->next;
		}
		fprintf(stream, "%s\n", s = getStringFromDate(tmp->timeObj));
                free(s);
        }
}

// This method will print out TimeList as comma separatevoid RemoveDupTime(struct TimeList* tl, t_time time);d list.
// This is used to write out database file.
// see "printDatabaseToFile"
//	stream	- The file in which the Time List will be written
//	tl	- The Time List to be written
//	return	- Void
void printOnboardTimeList(FILE* stream, struct TimeList* tl) {
	struct TimeElement* tmp = tl->head;
	if(tl->count > 0) {
                char *s; //tmp var for getStringFromDate
		while(tmp->next != NULL) {
                    fprintf(stream, "%s|%s,", s = getStringFromDate(tmp->timeObj), tmp->location);
                    free(s);
                    tmp = tmp->next;
		}
		fprintf(stream, "%s|%s\n", s = getStringFromDate(tmp->timeObj), tmp->location);
                free(s);
        }
}

// remove element from the head of the list
// returns a pointer to the removed element
//	tl	- The Time List to have it's head removed
//	te	- (see all previous remove functions)
//	return	- Void
void Remove(struct TimeList* tl, struct TimeElement* te) {
	te = tl->head;
	if(tl->count > 1) {
		tl->head = tl->head->next;
		tl->count--;
		te->next = NULL; // ensure complete sever from the list
	}
	else if(tl->count == 1) {
		tl->head = NULL;
		tl->tail = tl->head;
		tl->count = 0;
		te->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		tl->count=0; // just a precaution
	}
}

// removes a TimeElement from the list at the head and then destroys it
//	tl	- The Time List to have it's head removed and the head's allocated memory freed
//	return	- Void
void removeAndDestroy(struct TimeList* tl) {
	struct TimeElement* te = tl->head;
	if(tl->count > 1) {
		tl->head = tl->head->next;
		tl->count--;
		te->next = NULL; // ensure complete sever from the list
	}
	else if(tl->count == 1) {
		tl->head = NULL;
		tl->tail = tl->head;
		tl->count = 0;
		te->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		tl->count=0; // just a precaution
	}
  free(te->location);
	free(te);
}
//returns the count of a Time List
//	tl	- The Time List who's count will be returned
//	return	- The count of the Time List
int getCount(struct TimeList* tl) {
	return tl->count;
}

// Repeated calls removeAndDestroy on the list until the list is
// empty, then destroy the TimeList object
//	tl	- The Time List which will be deleted
//	return	- Void
void destroyTimeList(struct TimeList* tl) {
	while(tl->count > 0)
	{
		removeAndDestroy(tl);
	}
	free(tl);
}

// Takes a time_t object and converts it into a tm struct,
// values are then formatted into a CSS friendly string format
// returned format is "HH:mm:SS MM/DD/YY" 
//	timeObj	- The time object's who time will be turned into a date string
//	return	- An allocated string that hold's the date string
char* getStringFromDate(time_t timeObj) {
  char* s = (char*)calloc(DATE_STRING_LENGTH, sizeof(char));
	strcpy(s, "");
	struct tm *timeStructure	= localtime(&timeObj);
	char* tmp = (char*)calloc(6, sizeof(char));
	
  // Hour
	sprintf(tmp, "%d", timeStructure->tm_hour);
	if(timeStructure->tm_hour < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
  	strcat(s, ":");
  // Minute
  	sprintf(tmp, "%d", timeStructure->tm_min);
	if(timeStructure->tm_min < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
  	strcat(s, ":");
  // Second
  	sprintf(tmp, "%d", timeStructure->tm_sec);
	if(timeStructure->tm_sec < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
  	strcat(s, " ");
  // Month
  	sprintf(tmp, "%d", timeStructure->tm_mon + 1);
	if(timeStructure->tm_mon + 1 < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
  	strcat(s, "/");
  // Day
	sprintf(tmp, "%d", timeStructure->tm_mday);
	if(timeStructure->tm_mday < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
  	strcat(s, "/");
  // Year
  	sprintf(tmp, "%d", timeStructure->tm_year - 100);
	strcat(s, tmp);
        free(tmp);
	return s;
}

// Takes a time_t object and converts it into a tm struct,
// values are then formatted into a CSS friendly string format
// returned format is "YYYY-MM-DD HH:mm:SS" in local time
//	timeObj	- The time object's who time will be turned into a date string
//	return	- An allocated string that hold's the date string
char* getISOStringFromDate(time_t timeObj) {
  char* s = (char*)calloc(ISODATE_STRING_LENGTH, sizeof(char));
	strcpy(s, "");
	struct tm *timeStructure	= localtime(&timeObj);
	char* tmp = (char*)calloc(6, sizeof(char));
	

	// Year YYYY
  	sprintf(tmp, "%d", timeStructure->tm_year + 1900);
	strcat(s, tmp);
	  strcat(s, "-");
  // Month
  	sprintf(tmp, "%d", timeStructure->tm_mon + 1);
	if(timeStructure->tm_mon + 1 < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
  	strcat(s, "-");
  // Day
	sprintf(tmp, "%d", timeStructure->tm_mday);
	if(timeStructure->tm_mday < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);

		strcat(s, " ");

  // Hour
	sprintf(tmp, "%d", timeStructure->tm_hour);
	if(timeStructure->tm_hour < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
  	strcat(s, ":");
  // Minute
  	sprintf(tmp, "%d", timeStructure->tm_min);
	if(timeStructure->tm_min < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
  	strcat(s, ":");
  // Second
  	sprintf(tmp, "%d", timeStructure->tm_sec);
	if(timeStructure->tm_sec < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
 
        free(tmp);
	return s;
}



// Similar to getStringFromDate, but the string is formatted in a different
// order and the minutes are seconds are not relevant.
// logfile names sare only about year, month, day and hour.
//	timeObj	- The time object who's time will be used to formate the File Name
//	return	- An allocated string that holds the file's name
char* getFilenameFromDate(time_t timeObj) {
	char* s = (char*)calloc(DATE_STRING_LENGTH, sizeof(char));
	strcpy(s, "");
	struct tm *timeStructure	= localtime(&timeObj);
	char* tmp = (char*)calloc(6, sizeof(char));
	
	sprintf(tmp, "%d", timeStructure->tm_year + 1900);
	strcat(s, tmp);
	
	sprintf(tmp, "%d", timeStructure->tm_mon + 1);
	if(timeStructure->tm_mon + 1 < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
	
	sprintf(tmp, "%d", timeStructure->tm_mday);
	if(timeStructure->tm_mday < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
	
	sprintf(tmp, "%d", timeStructure->tm_hour);
	if(timeStructure->tm_hour < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
	free(tmp);
	return s;
}

// take in a  filename of the form "2015081811" for August 18, 2015 at 11:00
// and turn it into a time_t object for easier comparisons and arithmetic
//	s	- The file's name
//	tt	- A pointer to the variable that will store the file's date once parsed from the name
//	return	- A BOOL that is always true
BOOL getDateFromFileName(char* s, time_t* tt)  {	
	char s2[DATE_STRING_LENGTH];
	strcpy(s2,s);
  
	// Overhead
	struct tm newDate;
	newDate.tm_isdst = -1;
	char tmp[5];
	
	// Get time portion
	strncpy(tmp, s2, 4);
	tmp[4] = '\0';
	newDate.tm_year = atoi(tmp) - 1900; // years begin at 1900
	removeFirstChars(s2,4);

	strncpy(tmp, s2, 2);
	tmp[2] = '\0';
	newDate.tm_mon = atoi(tmp) - 1; // months begin at zero
	removeFirstChars(s2,2);
  
	strncpy(tmp, s2, 2);
	tmp[2] = '\0';
	newDate.tm_mday = atoi(tmp);
	removeFirstChars(s2,2);
  
	strncpy(tmp, s2, 2);
	tmp[2] = '\0';
	newDate.tm_hour = atoi(tmp);
	removeFirstChars(s2,2);

	newDate.tm_sec=0;
	newDate.tm_min=0;
  
	// create TimeElement object
	*tt = mktime(&newDate);

	return TRUE;
}

// The 'date stamp' that is added to database files when they are deprecated
// cannot contain slashes because they interfere with filepaths / extensions.
// Therefore, 'getStringFromdate' cannot be used. This method will implement
// similar functionality, with the same order and format, but without slashes.
// "YYMMDD_HHmmSS" 
//	timeObj	- A time_t variable who's time will be used to create the date stamp
//	return	- An allocated string which contains the slashless date stamp
char* getSlashlessDatestampFromDate(time_t timeObj) {
        char* s = (char*)calloc(STRING_LENGTH,sizeof(char));
	strcpy(s, "");
        
	struct tm *timeStructure;
        timeStructure  = localtime(&timeObj);
	char* tmp = (char*)calloc(6, sizeof(char));

        
        
  // Year
        sprintf(tmp, "%d", timeStructure->tm_year - 100);
	strcat(s, tmp);
        
  // Month
  	sprintf(tmp, "%d", timeStructure->tm_mon + 1);
	if(timeStructure->tm_mon + 1 < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
  
  // Day
	sprintf(tmp, "%d", timeStructure->tm_mday);
	if(timeStructure->tm_mday < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
        strcat(s, "_");
        
  // Hour
	sprintf(tmp, "%d", timeStructure->tm_hour);
	if(timeStructure->tm_hour < 10) {
		strcat(s, "0");
        }
	strcat(s, tmp);
  	
  // Minute
  	sprintf(tmp, "%d", timeStructure->tm_min);
	if(timeStructure->tm_min < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);

  // Second
  	sprintf(tmp, "%d", timeStructure->tm_sec);
	if(timeStructure->tm_sec < 10) {
		strcat(s, "0");
	}
	strcat(s, tmp);
        
        free(tmp);
	return s;
}
//A function that will go through a time list and remove any duplicate times as specficed by time
//	timeList	- The Time List from which duplicate times will be removed
//	time		- The time to be searched for
//	return		- Void
void RemoveDupTime(struct TimeList* timeList, time_t time)
{
	int count = 0;
	struct TimeElement* timeElement = timeList->head;
	struct TimeElement* prevElement = timeList->head;
	while(timeElement != NULL)
	{	
		//if they are the same time
		if(timeElement->timeObj == time)
		{
			//if count is not 0, remove the duplicate time
			if(count != 0)
			{
				//sever the next from the dup
				struct TimeElement* tmp = timeElement->next;
				timeElement->next = NULL;
				//move to next element
				timeElement = tmp;
				//point previous element to the new next
				prevElement->next = timeElement;
			}
			count++;
		}
		else
		{
			//move prev element to current element, and move to next element
			prevElement = timeElement;
			timeElement = timeElement->next;
		}
	}
}


