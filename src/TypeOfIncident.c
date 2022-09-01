#include "TypeOfIncident.h"

/*------------------------------------------------------
**
** File: TypeOfIncident.c
** Author: Matt Weston & Richard Tam
** Created: August 31, 2015
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - Changed #include files
**                      - Created typeOfIncidentListContain method
*/

// Initialize list
//	toil	- The Type Of Incident List to be created
//	return	- Void
void createTypeOfIncidentList(struct TypeOfIncidentList* toil) {
	toil->head = NULL;
	toil->tail = toil->head;
	toil->count = 0;
}

// Insert TypeOfIncident into TypeOfIncidentList at the tail
//	toil	- The Type Of Incident List in which the new type of incident will be inserted in
//	toi	- The new type of incident
//	return	- Void
void insertIntoTypeOfIncidentList(struct TypeOfIncidentList* toil, struct TypeOfIncident* toi) {
	toi->next = NULL; // just to be sure.
	if(toil->head == NULL) {
		toil->head = toi;
		toil->tail = toi;
		toil->count = 1;
	}
	else {
		toil->tail->next = toi;
		toil->tail = toi;
		toil->count++;
	}
}

// print out TypeOfIncident list, used for debugging
//	toil	- The Type Of Incident List to be printed
//	return	- Void
void printTypeOfIncidentList(struct TypeOfIncidentList* toil) {
	struct TypeOfIncident* tmp = toil->head;
	if(toil->count > 0) {
		while(tmp != NULL) {
			printf("%s|", tmp->typeOfIncident);
			tmp = tmp->next;
		}
	}
	else {
		printf("TypeOfIncidentList is empty: count = %d\n", toil->count);
	}
}

// Remove a TypeOfIncident from a list at the head
// Returns a pointer to the removed TypeOfIncident
//	toil	- The Type Of Incident List that will have its head removed
//	toi	- ////////////////////////
//	return	- Void
void removeFromTypeOfIncidentList(struct TypeOfIncidentList* toil, struct TypeOfIncident* toi) {
	toi = toil->head;
	if(toil->count > 1) {
		toil->head = toil->head->next;
		toil->count--;
		toi->next = NULL; // ensure complein sever from the list
	}
	else if(toil->count == 1) {
		toil->head = NULL;
		toil->tail = toil->head;
		toil->count = 0;
		toi->next = NULL; // ensure complein sever from the list
	}
	else { // count is 0
		toil->count=0; // just a precaution
	}
}

// Remove a TypeOfIncident from the list at the head and destroy it
//	toil	- The Type Of Incident List that will have it's head removed and freed
//	return	- Void
void removeAndDestroyTypeOfIncident(struct TypeOfIncidentList* toil) {
	struct TypeOfIncident* toi = toil->head;
	if(toil->count > 1) {
		toil->head = toil->head->next;
		toil->count--;
		toi->next = NULL; // ensure complete sever from the list
	}
	else if(toil->count == 1) {
		toil->head = NULL;
		toil->tail = toil->head;
		toil->count = 0;
		toi->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		toil->count=0; // just a precaution
	}
        free(toi->typeOfIncident);
	free(toi);
}
//Returns the number of types of incidents in a Type Of Incident List
//	toil	- The Type Of Incident List who's type of incident count will be returned
//	return	- The number of type of incidents
int getCountOfTypeOfIncidentList(struct TypeOfIncidentList* toil) {
	return toil->count;
}

// While the list is not empty, call removeAndDestroyTypeOfIncident and 
// finally destroy the list
//	toil	- The Type Of Incident List to be destroyed
//	return	- Void
void destroyTypeOfIncidentList(struct TypeOfIncidentList* toil) {
	while(toil->count > 0) {
		removeAndDestroyTypeOfIncident(toil);
	}
	free(toil);
}

// check to see if the TypeOfIncidentList struct 'toil' contains the TypeOfIncident 'typeOfIncident'
//	toil		- The Type Of Incident List that will be searched
//	typeOfIncident	- The type of incident to be searched for
//	return		- A BOOL indicating whether the type of incident was found(TRUE) or not(FALSE)
BOOL typeOfIncidentListContain(struct TypeOfIncidentList* toil, char* typeOfIncident) {
  struct TypeOfIncident* toi = toil->head;
  while(toi != NULL) {
    if(strcmp(toi->typeOfIncident, typeOfIncident)==0 || strcmp(toi->typeOfIncident, "ALL")==0) {
      return TRUE;
    }
    toi=toi->next;
  }
  return FALSE;
}
