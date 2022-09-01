#include "DisabledIncidents.h"
#include "DateAndTime.h"
#include "TypeOfIncident.h"
#include "Incidents.h"


/*------------------------------------------------------
**
** File: DisabledIncidents.c
** Author: Matt Weston & Richard Tam
** Created: February 18, 2016
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - This file was created in release 1.1
**                      - Many methods are copies of Incident.c methods
**                      -
**
*/

// Initiate field for DisabledIncident List
//	dil	- The disabled incident list to the created
//	return	- Void
void createDisabledIncidentList(struct DisabledIncidentList* dil) {
	dil->head = NULL;
	dil->tail = dil->head;
	dil->count = 0;
}

// Standard linked-list Queue style insert at the tail of the list
//	dil	- The disabled incident list in which the new disabled incident will be added to
//	in	- The new disabled incident
//	return	- Void
void insertIntoDisabledIncidentList(struct DisabledIncidentList* dil, struct DisabledIncident* in) {
	in->next = NULL; // just to be sure.
	if(dil->head == NULL) {
		dil->head = in;
		dil->tail = in;
		dil->count = 1;
	}
	else {
		dil->tail->next = in;
		dil->tail = in;
		dil->count++;
	}
}

// remove an incident from the list and return a pointer to it. 
// DisabledIncident will be removed from the head
//	dil	- The Disabled Incident List from which the head will be removed
//	din	- Coded incorrectly, doesn't return anything
//	return	- void
void removeFromDisabledIncidentList(struct DisabledIncidentList* dil, struct DisabledIncident* din) {
	din = dil->head;
	if(dil->count > 1) {
		dil->head = dil->head->next;
		dil->count--;
		din->next = NULL; // ensure complein sever from the list
	}
	else if(dil->count == 1) {
		dil->head = NULL;
		dil->tail = dil->head;
		dil->count = 0;
		din->next = NULL; // ensure complein sever from the list
	}
	else { // count is 0
		dil->count=0; // just a precaution
	}
}

// remove an incident from the head of the list and destroy it
//	dil	- The Disabled Incident List who's head will be removed and freed from memory
//	return	- Void
void removeAndDestroyDisabledIncident(struct DisabledIncidentList* dil) {
	struct DisabledIncident* din = dil->head;
	if(dil->count > 1) {
		dil->head = dil->head->next;
		dil->count--;
		din->next = NULL; // ensure complete sever from the list
	}
	else if(dil->count == 1) {
		dil->head = NULL;
		dil->tail = dil->head;
		dil->count = 0;
		din->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		dil->count=0; // just a precaution
	}
	free(din->location1);
	free(din->other);
	free(din);
}
//Returns the number of disabled incidents in a Disabled Incident List
//	dil	- The Disabled Incident List who's count will be returned
//	return	- The count of the Disabled Incident List
int getCountOfDisabledIncidentList(struct DisabledIncidentList* dil) {
	return dil->count;
}

// While the list is not empty, remove and destroy the head element. Lastly
// destroy the DisabledIncidentList object. 
//	dil	- The Disabled Incident List to be destroyed
//	return	- Void
void destroyDisabledIncidentList(struct DisabledIncidentList* dil) {
	while(dil->count > 0) {
		removeAndDestroyDisabledIncident(dil);
	}
	free(dil);
}

// 'incidentDuringDurationWindow' takes in an Incident and a Duration as a
// arguments. The time of the Incident is compared to the start and end time of
// the duration and a macro is returned indicating whether the incident occured
// before, during or after the duration.
//	in	- The incident to be check if it is in the duration window
//	dur	- The duration window that the incident will be checked against
//	return	- An int indicating the status of the incident with respect to the duration window
// This method was added in Revision 1.0, it implements similar functioinality to 
// the pre-existing 'checkEnabledDisabled' but uses the new DisabledIncidentList for 
// the disabled incidents instead of the standard IncidentList that 'checkEnabledDisabled'
// does. 'checkEnabledDisabled' should eventually be replaced with this method

// use for debugging purposes and to present output to the user in a friendly way
//	dil	- The Disabled Incident List to be printed
//	return	- Void
void printDisabledIncidentList(struct DisabledIncidentList* dil) {
	struct DisabledIncident* tmp = dil->head;
	if(dil->count > 0) {
		while(tmp != NULL) {
			// during debugging, fields would occasionally be NULL and cause segfaults
			// in execution. This style of printing out was done to circumvent them.
			// Left in in case problems reappear
			printTypeOfIncidentList(tmp->typeOfIncidentList);
			tmp->location1 != NULL ? printf("Loc:%s|", tmp->location1) : printf("Loc:NULL|");
			tmp->data != NULL ? printf("Data:%s|", tmp->data) : printf("Data:NULL|");
			printf("StartTime:%s|EndTime:%s|", getStringFromDate(tmp->duration->startTime->timeObj),getStringFromDate(tmp->duration->endTime->timeObj));
            printf("\n");
			tmp = tmp->next;
		}
	}
	else {
		printf("DisabledIncidentList is empty: count = %d\n", dil->count);
	}
}

// Read in file containing incidents that have been disabled due to frequency 
// or non-safety related reasons. DisabledIncidents are stored in a linked-list
// of DisabledIncident structs.
//	dil	- The Disabled Incident List in which the disabled incidents will be stored
//	return	- Void
void readInDisabledIncidents2(struct DisabledIncidentList* dil) {
  // filePath is the file path of the file which contains the extension of
  // actually file used to disabled incidents via the web interface.
  // The web interface may move this file, so an option was needed to make this
  // feature configurable. 
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  
  // filePath2 will be the extension of the interface-disabled incidents file. 
  char* filePath2 = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath, OTHER, DISABLED_INCIDENTS_FILE_PATH, DOT_TXT);
  FILE* disabledIncidentsFilePathFile = fopen(filePath, "r");
  
  if(disabledIncidentsFilePathFile==NULL) {
    printf("The file |%s| could not be found\n", filePath);
	return;
  }
  else {
    
	int result = readInLine(disabledIncidentsFilePathFile, &filePath2, STRING_LENGTH);
	
	if(result == NO_STRING_READ) {
	  printf("No filepath was read in. The DisabledIncidentList will be empty\n");
	  return;
	}
  }
  
  // open interface-disabled incidents folder
  FILE* fp = fopen(filePath2, "r");
  
  // If the file is present
  if(fp!=NULL) {
    // tmp will hold the complete line that is read in from the file
	char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
    
	// typeStr will hold the comma-separated list of incident types that have
	// have been selkected to be disabled. 
	char* typeStr = (char*)calloc(STRING_LENGTH, sizeof(char));
    
	// dateStr1 and dateStr2 will hold the start and end date respectively
	char* dateStr1 = (char*)calloc(STRING_LENGTH, sizeof(char));
    char* dateStr2 = (char*)calloc(STRING_LENGTH, sizeof(char));
   
    int lineRes = readInLine(fp, &tmp, STRING_LENGTH);
  
    while(lineRes!=END_OF_FILE) {
      if(lineRes == READ_IN_STRING || lineRes == STRANGE_END_OF_FILE) {

        //Initialize DisabledIncident for value assignment
        struct DisabledIncident* din = malloc(sizeof(struct DisabledIncident));
          din->typeOfIncidentList = malloc(sizeof(struct TypeOfIncidentList));
            createTypeOfIncidentList(din->typeOfIncidentList);
          din->duration = malloc(sizeof(struct Duration));
            din->duration->startTime = malloc(sizeof(struct TimeElement));
            din->duration->endTime = malloc(sizeof(struct TimeElement));
          din->other = NULL;
          din->location1 = (char*)calloc(STRING_LENGTH, sizeof(char));
        
		// parse string 'tmp' and store sections in small strings.
        strcpy(typeStr, strtok(tmp, ";"));
        strcpy(din->location1, strtok(NULL, ";"));
        strcpy(dateStr1, strtok(NULL, ";"));
        strcpy(dateStr2, strtok(NULL, ";"));
		
		// get date object from strings 
        getDateFromString(dateStr1, &(din->duration->startTime->timeObj));
        getDateFromString(dateStr2, &(din->duration->endTime->timeObj));
 
        // cycle through the string containing the different types and add them to
        // a list		
        int num = getCharCount(typeStr, ','); int i=1;
        struct TypeOfIncident* toi = malloc(sizeof(struct TypeOfIncident));
        toi->typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));
        strcpy(toi->typeOfIncident, strtok(typeStr, ","));
        insertIntoTypeOfIncidentList(din->typeOfIncidentList, toi);
        while(i <= num) {
          struct TypeOfIncident* tmptoi = malloc(sizeof(struct TypeOfIncident));
          tmptoi->typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));
          strcpy(tmptoi->typeOfIncident, strtok(NULL, ","));
          insertIntoTypeOfIncidentList(din->typeOfIncidentList, tmptoi);
          i++;
        }
		
		// insert into the list
        insertIntoDisabledIncidentList(dil, din);
        // read in the next line. 
		tmp = (char*)realloc(tmp, STRING_LENGTH);
      }//end of if
      lineRes = readInLine(fp, &tmp, STRING_LENGTH);
    }
	
    //clean things up
    free(dateStr1);
    free(dateStr2);
    free(typeStr);
    free(tmp);
    free(filePath);
    free(filePath2);
    fclose(disabledIncidentsFilePathFile);
    
	// close the file
    if(EOF == fclose(fp)) {
      printf("Could not close file |%s|.\n", filePath);
      printf("errno = %d, strerror is %s\n", errno, strerror(errno));
    }
  }
  // The file is not present, print an error message and return
  else {
    printf("The file: '%s' could not be found.\n", filePath);
	return;
  }
}

// The web interface from which the DisabledIncidentList is generated uses
// "conventional" station names, such as 'St George (BD)', 'Kipling' and 'Sheppard-Yonge'
// but WBSS uses the names 'St. George Lower Level', 'Kipling Interlocking', and
// 'Yonge Interlocking East' respectively.

// 'changeStationNames' is used to convert the 'conventional' station names
// used in the DisabledIncidentList to the respective WBSS station names.
// Some conventional station names will correspond to two WBSS station names,
// in which case another element will be added to the list.
//	dil	- The Disabled Incident List who's disabled incidents will have their station names changed
//	return	- Void
void changeStationNames(struct DisabledIncidentList* dil) {
  // Read in the StationPairList that will act as a key/reference-table to
  // convert between WBSS and conventional names
  printf("A\n");
  struct StationPairList* spl = malloc(sizeof(struct StationPairList));
  createStationPairList(spl);
  readInConvToWBSSList(spl);

  printf("b\n");
  struct DisabledIncident* din = dil->head;
  BOOL preMatchFound = FALSE;
  char* convName = (char*)calloc(STRING_LENGTH, sizeof(char));  

  // a while loop to cycle through all of the elements in the DisableIncidentsList
  while(din != NULL) {
    // din->location1 may be change during the execution of this while loop, but its
    // original value will still be needed. Because of this convName is used.	
	strcpy(convName, din->location1);
    preMatchFound = FALSE;
    struct StationPair* sp = spl->head;
	// a while loop to cycle through all of the elements in the StationPair list
    while(sp != NULL) {
      // if the 'conventional name' field of the StationPair matches the 
      // name of the location for the disabled incident and this 
	  // is the first time a match has beeb found, copy the
      // proper wbss name into it
      if(strcmp(sp->location1, convName)==0  && !preMatchFound) {
        strcpy(din->location1, sp->location2);
        preMatchFound = TRUE;
      }
      // if the 'conventional name' field of the StationPair matches the 
      // name of the location for the disabled incident and another match 
	  // has already been found then create a new incident and add it to the end
	  // in this case the StationPairList has a single conventional name, that
	  // incoporates two WBSS names:
	  // e.g. (conventional name,WBSS name)
	  // Bay Lower,Bay Lower BD
	  // Bay Lower,Bay Lower YUS
      else if(strcmp(sp->location1, convName)==0  && preMatchFound) {
        struct DisabledIncident* new_din = malloc(sizeof(struct DisabledIncident));
        copyDisabledIncident(new_din, din);
        strcpy(new_din->location1, sp->location2);    
        insertIntoDisabledIncidentList(dil, new_din);
      }
      sp=sp->next;
    }
    din=din->next;
  }
  
  //clean things up
  free(convName);
  destroyStationPairList(spl);
}

// create a complete copy of a DisabledIncident struct
//	new_din	- The disabled incident to be created
//	orig	- The disabled incident that will be copied
//	return	- void
void copyDisabledIncident(struct DisabledIncident* new_din, struct DisabledIncident* orig) {
   // initialize all structs and fields
   new_din->location1 = (char*)calloc(STRING_LENGTH, sizeof(char));
   new_din->location2 = (char*)calloc(STRING_LENGTH, sizeof(char));
   new_din->data = (char*)calloc(STRING_LENGTH, sizeof(char));
   new_din->other = (char*)calloc(STRING_LENGTH, sizeof(char));
   new_din->duration = malloc(sizeof(struct Duration));
     new_din->duration->startTime = malloc(sizeof(struct TimeElement));
     new_din->duration->endTime = malloc(sizeof(struct TimeElement));
   new_din->typeOfIncidentList = malloc(sizeof(struct TypeOfIncidentList));
     createTypeOfIncidentList(new_din->typeOfIncidentList);
   new_din->next = NULL;

   // copy over all strings from din to new_din
   if(orig->location1 != NULL) {
     strcpy(new_din->location1, orig->location1);
   }
   if(orig->location2 != NULL) {
     strcpy(new_din->location2, orig->location2);
   }
   if(orig->data != NULL) {
     strcpy(new_din->data, orig->data);
   }
   if(orig->other != NULL) {
     strcpy(new_din->other, orig->other);
   }
   
   // copy over time objects
   if(orig->duration->startTime != NULL) { 
     new_din->duration->startTime->timeObj = orig->duration->startTime->timeObj;
   }
   if(orig->duration->endTime != NULL) {
     new_din->duration->endTime->timeObj = orig->duration->endTime->timeObj;
   }
   
   // cycle through the TypeofIncidentList of din and copy its entries into
   // new_din one-by-one
   struct TypeOfIncident* tmp = orig->typeOfIncidentList->head;
   while(tmp != NULL) {
     struct TypeOfIncident* tmp2 = malloc(sizeof(struct TypeOfIncident));
     tmp2->typeOfIncident = (char*)calloc(STRING_LENGTH, sizeof(char));
     strcpy(tmp2->typeOfIncident, tmp->typeOfIncident);
     insertIntoTypeOfIncidentList(new_din->typeOfIncidentList, tmp2);
     tmp=tmp->next;
   }
}
