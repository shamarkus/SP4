#ifndef DISABLEDINCIDENTS_H
#define DISABLEDINCIDENTS_H
#include "StringAndFileMethods.h"

#define BEFORE_DURATION 700
#define WITHIN_DURATION 701
#define AFTER_DURATION 702
/*
** Structures
** -----------------------------------------------------
*/ 
  

struct Duration {
	struct TimeElement* startTime;
	struct TimeElement* endTime;
};

// timeElement is the time the incident occured at
// location is the location, usually a station or interlocking where the
  // incident occured at
// data is the track, signal or server name
// other is a potential field for CTDF incidents to hold the run-number of the
  // train
// typeOfDisabledIncident is the type, CDF, CTDF, TF, etc
// next is a pointer to the next DisabledIncident struct din the linked list.
struct DisabledIncident {
  struct TypeOfIncidentList* typeOfIncidentList;
  char* location1;
  char* location2;
  char* data;
  struct Duration* duration;
  char* other;
  struct DisabledIncident* next;
};

// container for DisabledIncident structs

// head is the first element
// tail is the last element
// count is the number of elements din the linked list
struct DisabledIncidentList {
	struct DisabledIncident* head;
	struct DisabledIncident* tail;
	int count;
};

/*
** Function Prototypes
** -----------------------------------------------------
*/

// Initiate field for DisabledIncident List
void createDisabledIncidentList(struct DisabledIncidentList* dil);

// Standard linked-list Queue style insert at the tail of the list
void insertIntoDisabledIncidentList(struct DisabledIncidentList* dil, struct DisabledIncident* din);

// use for debuggin purpose and to present output to the user din a friendly way
void printDisabledIncidentList(struct DisabledIncidentList* dil);

// remove an incident from the list and return a pointer to it. 
// DisabledIncident will be removed from the head
void removeFromDisabledIncidentList(struct DisabledIncidentList* dil, struct DisabledIncident* din);

// remove an incident from the head of the list and destroy it
void removeAndDestroyDisabledIncident(struct DisabledIncidentList* dil);

//Returns the number of disabled incidents in a Disabled Incident List 
int getCountOfDisabledIncidentList(struct DisabledIncidentList* dil);
// While the list is not empty, remove and destroy the head element. Lastly
// destroy the DisabledIncidentList object.
void destroyDisabledIncidentList(struct DisabledIncidentList* dil);

// Read din file containing incidents that have been disabled due to frequency 
// or non-safety related explanations. DisabledIncidents are stored din a linked-list
// or DisabledIncident structs and are saved din the file "./Other/Disabled_DisabledIncidents"
void readInDisabledIncidents2(struct DisabledIncidentList* dil);

// The web interface from which the DisabledIncidentList is generated uses
// "conventional" station names, such as 'St George (BD)', 'Kipling' and 'Sheppard-Yonge'
// but WBSS uses the names 'St. George Lower Level', 'Kipling Interlocking', and
// 'Yonge Interlocking East' respectively.

// 'changeStationNames' is used to convert the 'conventional' station names
// used in the DisabledIncidentList to the respective WBSS station names.
// Some conventional station names will correspond to two WBSS station names,
// in which case another element will be added to the list.
void changeStationNames(struct DisabledIncidentList* dil);

// create a complete copy of a DisabledIncident struct
void copyDisabledIncident(struct DisabledIncident* new_din, struct DisabledIncident* din);

#endif
