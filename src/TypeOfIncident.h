#ifndef TYPEOFINCIDENT_H
#define TYPEOFINCIDENT_H
#include "StringAndFileMethods.h"

/*
** Structures
** -----------------------------------------------------
*/

// A struct for a type of Incident, part of a linked-list

// typeOfIncident will hold the short-code for the type of incident
// next is a pointer to the next incident
struct TypeOfIncident {
	char* typeOfIncident;
  struct TypeOfIncident* next;
};

// Container for linked list of incident types
struct TypeOfIncidentList {
	struct TypeOfIncident* head;
	struct TypeOfIncident* tail;
	int count;
};

/*
** Function Prototypes
** -----------------------------------------------------
*/

// Initialize list
void createTypeOfIncidentList(struct TypeOfIncidentList* toil);

// Insert TypeOfIncident into TypeOfIncidentList at the tail
void insertIntoTypeOfIncidentList(struct TypeOfIncidentList* toil, struct TypeOfIncident* toi);

// print out TypeOfIncident list, used for debugging
void printTypeOfIncidentList(struct TypeOfIncidentList* toil);

// Remove a TypeOfIncident from a list at the head
// Returns a pointer to the removed TypeOfIncident
void removeFromTypeOfIncidentList(struct TypeOfIncidentList* toil, struct TypeOfIncident* toi);

// Remove a TypeOfIncident from the list at the head and destroy it
void removeAndDestroyTypeOfIncident(struct TypeOfIncidentList* toil);

//Returns the number of types of incidents in a Type Of Incident List
int getCountOfTypeOfIncidentList(struct TypeOfIncidentList* toil);

// While the list is not empty, call removeAndDestroyTypeOfIncident and 
// finally destroy the list
void destroyTypeOfIncidentList(struct TypeOfIncidentList* toil);
// check to see if the TypeOfIncidentList struct 'toil' contains the TypeOfIncident 'typeOfIncident'
BOOL typeOfIncidentListContain(struct TypeOfIncidentList* toil, char* typeOfIncident);

#endif
