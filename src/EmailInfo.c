#include "EmailInfo.h"
#include "TypeOfIncident.h"
#include "DateAndTime.h"
#include "Incidents.h"
/*------------------------------------------------------
**
** File: EmailInfo.c
** Author: Matt Weston & Richard Tam
** Created: August 31, 2015
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - Changed #include files
**
** 08 Nov 2018: Rev 4.0 - IMiller
**                      - created functions to support linked-list structures for new subject line
**                      - Modified removeAndDestroyEmailInfo() in order to free SubjectList
**                      - created Linked-List structs in EmailInfo.h to support new subject line
**                        - Added bodyFileName and SubjectList* fields to EmailInfo struct
*/

// Initiate linked list of EmailInfo objects
//	el	- The EmailInfoList to be created
//	return	- Void
void createEmailInfoList(struct EmailInfoList* el) {
	el->head = NULL;
	el->tail = el->head;
	el->count = 0;
}

// Standard linked-list queue-style
// Inserts a EmailInfo Object at the tail of the list
//	el	- The EmailInfoList in which the new EmailInfo will be inserted
//	ei	- The new EmailInfo
//	return	- Void
void insertIntoEmailInfoList(struct EmailInfoList* el, struct EmailInfo* ei) {
	ei->next = NULL; // just to be sure.
	if(el->head == NULL) {
		el->head = ei;
		el->tail = ei;
		el->count = 1;
	}
	else {
		el->tail->next = ei;
		el->tail = ei;
		el->count++;
	}
}

// Used for debugging purposes
// Prints out the email info of the recipients during execution
//	el	- The EmailInfoList to be printed
//	return	- Void
void printEmailInfoList(struct EmailInfoList* el) {
  struct EmailInfo* ei = el->head;
  while(ei != NULL)
  {
    printf("Email:%s|FileName:%s|", ei->personsEmail, ei->emailFileName);
    printTypeOfIncidentList(ei->typeOfIncidentList);
    printf("\n");
    ei=ei->next;
  }
}

// Remove an email from the EmailInfoList and returns a pointer
// elements are removed from the head of the list
//	el	- The Email Info List who's head will be removed
//	ei	- /////////
//	return	- Void
void removeFromEmailInfoList(struct EmailInfoList* el, struct EmailInfo* ei) {
	ei = el->head;
	if(el->count > 1) {
		el->head = el->head->next;
		el->count--;
		ei->next = NULL; // ensure complete sever from the list
	}
	else if(el->count == 1) {
		el->head = NULL;
		el->tail = el->head;
		el->count = 0;
		ei->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		el->count=0; // just a precaution
	}
}

// removes an an EmailInfo element from the EmailInfoList and destroys it
//	el	- The EmailInfoList who's head will be removed and freed
//	return	- Void
void removeAndDestroyEmailInfo(struct EmailInfoList* el) {
	struct EmailInfo* ei = el->head;
	if(el->count > 1) {
		el->head = el->head->next;
		el->count--;
		ei->next = NULL; // ensure complete sever from the list
	}
	else if(el->count == 1) {
		el->head = NULL;
		el->tail = el->head;
		el->count = 0;
		ei->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		el->count=0; // just a precaution
	}
  destroy_subject_list(ei->subject_list);
  destroyTypeOfIncidentList(ei->typeOfIncidentList);
  free(ei->emailFileName);
  free(ei->personsEmail);
  free(ei->bodyFileName);
	free(ei);
}
//Returns the number of Email Infos in the Email Info List
//	el	- The Email Info List whos count will be returned
//	return	- The count of Email Infos in the EmailInfoList
int getCountOfEmailInfoList(struct EmailInfoList* el) {
	return el->count;
}

// While the list is not empty, removeAndDestroyEmailInfoList is called
// repeated. Finally the EmailInfoList struct is freed
//	el	- The Email Info List to be deleted.
//	return	- Void
void destroyEmailInfoList(struct EmailInfoList* el) {
	while(el->count > 0) {
		removeAndDestroyEmailInfo(el);
	}
	free(el);
}


// Initiate linked list of SubjectList objects
//  sl - SubjectList to be created
//	return	- Void
void 
create_subject_list(struct SubjectList* sl)
{
	sl->head = NULL;
	sl->tail = sl->head;
	sl->count = 0;
}

// Standard linked-list queue-style
// Inserts a SubjectIncident struct at the tail of the list
//	sl	- The SubjectList in which the new SubjectIncident will be inserted
//	si	- The new SubjectIncident
//	return	- Void
void 
insert_into_subject_list(struct SubjectList* sl, struct SubjectIncident* si) 
{
	si->next = NULL; // just to be sure.
	if(sl->head == NULL) 
  {
		sl->head = si;
		sl->tail = si;
		sl->count = 1;
	}
	else 
  {
		sl->tail->next = si;
		sl->tail = si;
		sl->count++;
	}
}

// Initiate linked list of LocationList objects
//  list - LocationList to be created
//	return	- Void
void 
create_location_list(struct LocationList* list)
{
	list->head = NULL;
	list->tail = list->head;
	list->count = 0;
}

// Standard linked-list queue-style
// Inserts a Location struct at the tail of the list
//	sl	- The LocationList in which the new Location will be inserted
//	si	- The new Location
//	return	- Void
void 
insert_into_location_list(struct LocationList* list, struct Location* loc) 
{
	loc->next = NULL; // just to be sure.
	if(list->head == NULL)
  {
		list->head = loc;
		list->tail = loc;
		list->count = 1;
	}
	else 
  {
		list->tail->next = loc;
		list->tail = loc;
		list->count++;
	}
}

// removes a SubjectInfo element from the SubjectList and destroys it
//	sl	- The SubjectList who's head will be removed and freed
//	return	- Void
void 
remove_and_destroy_subject_incident(struct SubjectList* sl)
{
	struct SubjectIncident* si = sl->head;
  
	if(sl->count > 1) 
  {
		sl->head = sl->head->next;
		sl->count--;
		si->next = NULL; // ensure complete sever from the list
	}
	else if(sl->count == 1) 
  {
		sl->head = NULL;
		sl->tail = sl->head;
		sl->count = 0;
		si->next = NULL; // ensure complete sever from the list
	}
	else 
  { // count is 0
		sl->count=0; // just a precaution
	}

  destroy_location_list(si->location_list);
  free(si->incident_name);
	free(si);
}

void 
remove_and_destroy_location(struct LocationList* list)
{
	struct Location* loc = list->head;
	if(list->count > 1) 
  {
		list->head = list->head->next;
		list->count--;
		loc->next = NULL; // ensure complete sever from the list
	}
	else if(list->count == 1) 
  {
		list->head = NULL;
		list->tail = list->head;
		list->count = 0;
		loc->next = NULL; // ensure complete sever from the list
	}
	else 
  { // count is 0
		list->count=0; // just a precaution
	}
  free(loc->location);
	free(loc);
}

// While the list is not empty, remove_and_destroy_subject_incident is called
// repeatedly. Finally the SubjectList struct is freed
//	sl	- The SubjectList to be deleted.
//	return	- Void
void
destroy_subject_list(struct SubjectList* sl)
{
	while(sl->count > 0)
  {
		remove_and_destroy_subject_incident(sl);
	}
	free(sl);
}

void
destroy_location_list(struct LocationList* list)
{
	while(list->count > 0)
  {
		remove_and_destroy_location(list);
	}
	free(list);
}


// Readin the list of emailRecipients from the file 
// "./Email_Info/email_recipients.txt" and stored them in a linked-list of
// EmailInfo structs

// the email_recipients.txt file is similarly stuctured to the 4 database files
// Individual fields are separated by semicolons (;) while the list of types of
// incidents the person is supposed to be emailed about are separated by 
// commas (,) ex. "Matthew.Weston@ttc.ca;CDF,TF,CTDF"
//	el	- The Email Info List in which all read Email Infos will be stored
//	return	- Void
void readInEmailInfoFile(struct EmailInfoList* el) {
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath, EMAIL_INFO, EMAIL_RECIPIENTS, DOT_TXT);
  FILE* emailRecipients = fopen(filePath, "r");
  
  // tmp will be read in and stored the entire line
  // ex. "Matthew.Weston@ttc.ca;CDF,TF,CTDF" or "Matthew.Weston@ttc.ca;CDF,TF,CTDF-admin"
  char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
  // tmp2 will store everything after the semicolon.
  // ex "CDF,TF,CTDF" or "CDF,TF,CTDF-admin"
  char* tmp2 = (char*)calloc(STRING_LENGTH, sizeof(char));
  // alarm_list will store the list of alarms the user is signed up to receive
  char* alarm_list = (char*)calloc(STRING_LENGTH, sizeof(char));
  // groups will store the list of usergroups after the hyphen
  char* group_list = (char*)calloc(STRING_LENGTH, sizeof(char));
  
  char* typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));

  if(emailRecipients != NULL) {
    // tmp will now stored a line like:
    // ex "Matthew.Weston@ttc.ca;CDF,TF,CTDF-admin"
	// The "-admin" is optional and denotes whether user should receive the 24-hour all-clear message
    int lineRes = readInLine(emailRecipients, &tmp, STRING_LENGTH);
    int counter = 1;
    while(lineRes != END_OF_FILE) {
     
      if(lineRes==READ_IN_STRING || lineRes==STRANGE_END_OF_FILE) {
        // create struct
        struct EmailInfo* ei = malloc(sizeof(struct EmailInfo));
        ei->next = NULL;
        ei->typeOfIncidentList = malloc(sizeof(struct TypeOfIncidentList));
        createTypeOfIncidentList(ei->typeOfIncidentList);
        ei->subject_list = malloc(sizeof(struct SubjectList));
        create_subject_list(ei->subject_list);
        ei->personsEmail = (char*)calloc(STRING_LENGTH, sizeof(char));
        ei->emailFileName = (char*)calloc(STRING_LENGTH, sizeof(char));
        ei->bodyFileName = (char*)calloc(STRING_LENGTH, sizeof(char));
        
        // Assume no email to be sent unless changed later.
        ei->sendEmail = FALSE; 
		
		// Assume person is not an admin unless changed later
		ei->isAdmin = FALSE;

        // Each recipient will have their own email file (html format)
        // this will be its file name and unique identifier
        strcpy(ei->personsEmail,strtok(tmp, ";"));
		
        // tmp2 will now hold something like: ex. "CDF,TF,CTDF" or "CDF,TF,CTDF-admin"
		// This is all the text after the semicolon
        strcpy(tmp2, strtok(NULL,";"));
		int tmp2_length = strlen(tmp2);
		
		// Get all text before the hyphen - these are the alarms the user is signed up to receive
		strcpy(alarm_list, strtok(tmp2, "-"));
		
		// If the length of the alarm list is less than the length of tmp2,
		// there is text after the hyphen. Copy that text into group_list
		// 1 is subtracted from the length of tmp2 in case the hyphen is the
		// last character of the string. If 1 is not subtracted, strtok generates
		// a runtime error (null reference)
		if (strlen(alarm_list) < tmp2_length - 1) {
			strcpy(group_list, strtok(NULL, "-"));
		}
		// Otherwise copy an empty string into group_list
		else {
			strcpy(group_list, "");
		}
		
		// There could be multiple usergroups, and they could appear in any order
		char* group = strtok(group_list, ",");
		while (group != NULL) {
			// If one of the groups is admin, set the isAdmin flag to be true
			if (strcmp(group, ADMIN_GROUP_IDENTIFIER) == 0) {
				ei->isAdmin = TRUE;
			}
			// Go to the next group
			group = strtok(NULL, ",");
		}
		
        // Number of commas remaining in time list string will be one less than
        // the number of types remaining in the string.
        while( getCharCount(alarm_list, ',') > 0 ) {
          // getCharsUpTo will copy all chars in alarm_list up to "," into 
          // typeOfIncident. The first time this loop runs typeOfIncident will
          // hold "CDF"
          getCharsUpTo(alarm_list,typeOfIncident,",");
          removeFirstChars(alarm_list, strlen(typeOfIncident)+1);
        
          // create struct
          struct TypeOfIncident* toi = malloc(sizeof(struct TypeOfIncident));
          toi->typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));
          strcpy(toi->typeOfIncident, typeOfIncident);
          insertIntoTypeOfIncidentList(ei->typeOfIncidentList,toi);
        }
        
        time_t t = time(NULL) - OFFSET*24*60*60;
        
        char* datestamp = getSlashlessDatestampFromDate(t);
        sprintf(ei->emailFileName, "EMAIL_%d_%s_%s", counter, ei->personsEmail, datestamp);
        sprintf(ei->bodyFileName, "EMAIL_%d_%s_%s_body", counter, ei->personsEmail, datestamp);
        free(datestamp);
        char* s; char* d;
        for(s=d=ei->emailFileName; *d=*s; d+=(*s++!='.'));
        for(s=d=ei->emailFileName; *d=*s; d+=(*s++!='@'));

        for(s=d=ei->bodyFileName; *d=*s; d+=(*s++!='.'));
        for(s=d=ei->bodyFileName; *d=*s; d+=(*s++!='@'));

        // create struct for last typeOfIncident in list, in the case of the 
        // exmaple, "CTDF"
        struct TypeOfIncident* toi = malloc(sizeof(struct TypeOfIncident));
        toi->typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));
        strcpy(toi->typeOfIncident, alarm_list);
        insertIntoTypeOfIncidentList(ei->typeOfIncidentList,toi);

        insertIntoEmailInfoList(el, ei);
        counter++;
      }
      lineRes = readInLine(emailRecipients, &tmp, STRING_LENGTH);
    }
    
    if(EOF == fclose(emailRecipients)) {
      printf("email_recipients |%s| could not be closed\n", filePath);
      printf("errno = $d\n, strerror is %s\n", errno, strerror(errno));
    } 
  }
  else {
    printf("No email file exists\n");
  }
  //clean up
  free(filePath);
  free(tmp);
  free(tmp2);
  free(alarm_list);
  free(group_list);
  free(typeOfIncident);
}

// Check if a person should receive an email give their emailInfoList field.
//	ei		- The Email info to be checked if it requires and email to be sent to.
//	typeOfIncident	- The short name of the incident being checked for
//	return		- TRUE if an email should be sent FALSE otherwise
BOOL shouldReceiveEmail(struct EmailInfo* ei, char* typeOfIncident) {
  struct TypeOfIncident* tmp = ei->typeOfIncidentList->head;
  while(tmp != NULL) {
    if(strcmp(tmp->typeOfIncident,typeOfIncident)==0 || strcmp(tmp->typeOfIncident,ALL)==0 ) {
      return TRUE; // was found
    }
    tmp=tmp->next;
  }
  return FALSE; // was not found
}
//Sets the Summary Email List to its inital state
//	sel	- The SummaryEmailList to be created
//	return	- Void
void createSummaryEmailList(struct SummaryEmailList* sel) {
	sel->head = NULL;
	sel->tail = sel->head;
	sel->count = 0;    
}
//Removes and frees the memory of the head of the Email Summary List
//	sel	- The Email Summary List who's head will be removed and freed
//	return	- Void
void removeAndDestroySummaryEmail(struct SummaryEmailList* sel) {
    	struct SummaryEmail* se = sel->head;
	if(sel->count > 1) {
		sel->head = sel->head->next;
		sel->count--;
		se->next = NULL; // ensure complete sever from the list
	}
	else if(sel->count == 1) {
		sel->head = NULL;
		sel->tail = sel->head;
		sel->count = 0;
		se->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		sel->count=0; // just a precaution
	}
        free(se->emailFileName);
        free(se->typeOfIncident);
	free(se);
    
}
//Removes and frees every summary email in the Summary Email List
//	sel	- The Summary Email List to be destroyed
//	return	- Void
void destroySummaryEmailList(struct SummaryEmailList* sel) {
    	while(sel->count > 0) {
		removeAndDestroySummaryEmail(sel);
	}
	free(sel);
    
}
//Inserts a new summary email to the end of the Summary Email List
//	sel	- The Summary Email List in which the new summary email will be inserted
//	se	- The new summary email
//	return	- Void
void insertIntoSummaryEmailList(struct SummaryEmailList* sel, struct SummaryEmail* se) {
	se->next = NULL; // just to be sure.
	//if its not a duplicate, add it
	if(sel->head == NULL) {
		sel->head = se;
		sel->tail = se;
		sel->count = 1;
	}
	else {
		sel->tail->next = se;
		sel->tail = se;
		sel->count++;
	}
}
//Returns the number of summary emails in a Summary Email List
//	sel	- The Summary Email List who's count will be returned
//	return	- The count of summary emails
int getCountOfSummaryEmailList(struct SummaryEmailList* sel) {
    return sel->count;
}
