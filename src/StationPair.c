#include "StationPair.h"

/*------------------------------------------------------
**
** File: StationPair.c
** Author: Matt Weston & Richard Tam
** Created: February 18, 2016
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - This file was introduced in this release
**                      - Many methods are copied from Incidents.c file and adapted
**
*/

// Initiate fields for StationPair List
//	spl	- The StationPairList to be created
//	return	- Void
void createStationPairList(struct StationPairList* spl) {
	spl->head = NULL;
	spl->tail = spl->head;
	spl->count = 0;
}

// Standard linked-list Queue style insert at the tail of the list
//	spl	- The StationPairList in which the new station pair will be inserted
//	sp	- The station pair which will be inserted
//	return	- Void
void insertIntoStationPairList(struct StationPairList* spl, struct StationPair* sp) {
	sp->next = NULL; // just to be sure.
	if(spl->head == NULL) {
		spl->head = sp;
		spl->tail = sp;
		spl->count = 1;
	}
	else {
		spl->tail->next = sp;
		spl->tail = sp;
		spl->count++;
	}
}

// use for debugging purpose and to present output to the user in a friendly way
//	spl	- The StationPairList to be printed
//	return	- Void
void printStationPairList(struct StationPairList* spl) {
	struct StationPair* tmp = spl->head;
	if(spl->count > 0) {
	  while(tmp != NULL) {
        tmp->location1 != NULL ? printf("Loc1:%s|", tmp->location1) : printf("Loc1:NULL|");
        tmp->data1 != NULL ? printf("Data1:%s|", tmp->data1) : printf("Data1:NULL|");
        tmp->location2 != NULL ? printf("Loc2:%s|", tmp->location2) : printf("Loc1:NULL|");
        tmp->data2 != NULL ? printf("Data2:%s\n", tmp->data2) : printf("Data1:NULL\n");

        tmp = tmp->next;
      }
	}
	else {
		printf("StationPairList is empty: count = %d\n", spl->count);
	}
}

// remove an incident from the list and return a pointer to it. 
// The StationPair will be removed from the head
//	spl	- The StationPairList who's head will be removed
//	sp	- //////////////////////////
//	return	- void
void removeFromStationPairList(struct StationPairList* spl, struct StationPair* sp) {
	sp = spl->head;
	if(spl->count > 1) {
		spl->head = spl->head->next;
		spl->count--;
		sp->next = NULL; // ensure complete sever from the list
	}
	else if(spl->count == 1) {
		spl->head = NULL;
		spl->tail = spl->head;
		spl->count = 0;
		sp->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		spl->count=0; // just a precaution
	}
}

// remove an incident from the head of the list and destroy it
//	spl	- The StationPairList who's head will be removed and destroyed
//	return	- Void
void removeAndDestroyStationPair(struct StationPairList* spl) {
	struct StationPair* sp = spl->head;
	if(spl->count > 1) {
		spl->head = spl->head->next;
		spl->count--;
		sp->next = NULL; // ensure complete sever from the list
	}
	else if(spl->count == 1) {
		spl->head = NULL;
		spl->tail = spl->head;
		spl->count = 0;
		sp->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		spl->count=0; // just a precaution
	}
	free(sp->location1);
	free(sp->data1);
	free(sp->location2);
	free(sp->data2);
	free(sp);
}
//Returns the number of stations pairs in a StationPairList
//	spl	- The StationPairList who's count will be returned
//	return	- The count of station pairs in the StationPairList
int getCountOfStationPairList(struct StationPairList* spl) {
	return spl->count;
}

// While the list is not empty, remove and destroy the head element. Lastly
// destroy the StationPairList object. 
//	spl	- The StationPairList which will be destroyed
//	return	- Void
void destroyStationPairList(struct StationPairList* spl) {
	while(spl->count > 0) {
		removeAndDestroyStationPair(spl);
	}
	free(spl);
}

// The web interface from which the DisabledIncidentList is generated uses
// "conventional" station names, such as 'St George (BD)', 'Kipling' and 'Sheppard-Yonge'
// but WBSS uses the names 'St. George Lower Level', 'Kipling Interlocking', and
// 'Yonge Interlocking East' respectively.

// 'readInConvToWBSSList' reads in a .txt file which contains pairs of 
// stations/locations. These stations/locations are stored in StationPair
// structs.
//	spl	- The StationPairList in which the read station pairs will be stored
//	return	- Void
void readInConvToWBSSList(struct StationPairList* spl) {
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath, OTHER, CONVENTIONAL_TO_WBSS_LIST, DOT_TXT);
  FILE* fp = fopen(filePath, "r");
  
  if(fp != NULL) {
    // tmp is a variable used to hold the lines as they are read in and parsed
    char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
		
    int lineRes = readInLine(fp, &tmp, STRING_LENGTH);
    
    while(lineRes != END_OF_FILE) {
      if(lineRes==READ_IN_STRING || lineRes==STRANGE_END_OF_FILE) {
		// Initialize StationPair struct
		struct StationPair* sp = malloc(sizeof(struct StationPair));
		sp->location1 = (char*)calloc(STRING_LENGTH, sizeof(char));
		sp->location2 = (char*)calloc(STRING_LENGTH, sizeof(char));
		sp->data1 = NULL;
		sp->data2 = NULL;
		sp->next = NULL;

		// copy each station/location name into the two location fields
		// the first field be the 'conventional' name
		// the second field will be the WBSS name	  
		strcpy(sp->location1, strtok(tmp, ";"));
		strcpy(sp->location2, strtok(NULL, ";")); 
		  
		// insert the struct into the list
        insertIntoStationPairList(spl, sp);
      }
      else {
        printf("Unable to parse the line %s\n", tmp);
        printf("Please see source-code documentation for proper style and ordering\n");
        printf("This line is being ignored\n");
      }
	  // reallocate the variable tmp to be STRING_LENGTH long.
      tmp = (char*)realloc(tmp, STRING_LENGTH);
      // read sp next line
      lineRes = readInLine(fp, &tmp, STRING_LENGTH);
      
   }
    
    if(EOF == fclose(fp)) {
      printf("Could not close file |%s|.\n", filePath);
      printf("errno = %d, strerror is %s\n", errno, strerror(errno));
    }
    
    //clean things up
    free(tmp);
  }
  else {
    printf("The file: '%s' could not be found. No Incidents will be disabled, All incident types will be emailed about\n", filePath);
  }
  
  //clean things up
  free(filePath);
}

// Certain tracks sections or switches are considered to be in work yards from the point
// of view WBSS. However, practically, they are on the mainline and revenue trains
// use them daily.

// 'readInTrackCircuitLocationReassignmentList' reads in a list of track sections/switches
// in pairs. One of the switch names/locations will be a genuine WBSS station/location
// and name and the other will be a user-defined alternate station/location and name.
// E.g.
// "Greenwood,108T_wye;Greenwood Wye,108T"
//	spl	- The StationPairList in which the read station pairs will be stored
//	return	- Void
void readInTrackCircuitLocationReassignmentList(struct StationPairList* spl) {
  char* filePath = (char*)calloc(STRING_LENGTH, sizeof(char));
  constructLocalFilepath(filePath, OTHER, TRACK_CIRCUIT_LOCATION_REASSIGNMENT_LIST, DOT_TXT);
  FILE* fp = fopen(filePath, "r");
  
  if(fp != NULL) {
	// tmp is a variable used to hold the lines as they are read sp and parsed
    char* tmp = (char*)calloc(STRING_LENGTH, sizeof(char));
    
	// info1 and info2 are used to hold the user-defined 
	// and WBSS info respectively
	char* info1 = (char*)calloc(STRING_LENGTH, sizeof(char));
    char* info2 = (char*)calloc(STRING_LENGTH, sizeof(char));
    
	// read in the first line
    int lineRes = readInLine(fp, &tmp, STRING_LENGTH);
    while(lineRes != END_OF_FILE) {
	  // create StationPair struct
	  if(lineRes==READ_IN_STRING || lineRes==STRANGE_END_OF_FILE) { 
		struct StationPair* sp = malloc(sizeof(struct StationPair));
		sp->location1 = (char*)calloc(STRING_LENGTH, sizeof(char));
		sp->location2 = (char*)calloc(STRING_LENGTH, sizeof(char));
		sp->data1 = (char*)calloc(STRING_LENGTH, sizeof(char));
		sp->data2 = (char*)calloc(STRING_LENGTH, sizeof(char));
		sp->next = NULL; 

		// store the user-defined track circuit info in info1		
		strcpy(info1, strtok(tmp, ";"));
		// store the WBSS track circuit info in info2.
		// Incidents with this data will be modified to have the info
		// from info1
		strcpy(info2, strtok(NULL, ";"));

		// split the info inside info1 into the location portion
		// and the track/switch name portion
		// Store each in their respective field in the struct
		strcpy(sp->location1, strtok(info1, ","));
		strcpy(sp->data1, strtok(NULL, ","));
		// split the info inside info2 into the location portion
		// and the track/switch name portion
		// Store each in their respective field in the struct
		strcpy(sp->location2, strtok(info2, ","));
		strcpy(sp->data2, strtok(NULL, ","));

		insertIntoStationPairList(spl, sp);
      }
      else {
        printf("Unable to parse the line %s\n", tmp);
        printf("Please see source-code documentation for proper style and ordering\n");
        printf("This line is being ignored\n");
      }
      tmp = (char*)realloc(tmp, STRING_LENGTH);
      // read sp next line
      lineRes = readInLine(fp, &tmp, STRING_LENGTH);
    }
    
    if(EOF == fclose(fp)) {
      printf("Could not close file |%s|.\n", filePath);
      printf("errno = %d, strerror is %s\n", errno, strerror(errno));
    }
    
    //clean things up
    free(tmp);
    free(info2);
    free(info1);
  }
  else {
    printf("The file: '%s' could not be found. No Incidents will be disabled, All incident types will be emailed about\n", filePath);
  }
  
  //clean things up
  free(filePath);
  
}
