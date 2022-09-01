#include "StringAndFileMethods.h"

#define DEFAULT_THRESHOLD_TIME 24

/*
** Structures
** -----------------------------------------------------
*/

// Threshold is a struct to hold information for the number of incidents in a 
// given time period that must occur before an email is sent about them. These
// are known as threshold conditions. Threshold will usually be part of a
// linked-list

// numOfIncidents is the number of incidents
// numOfHours is the number of minutes
// next is a pointer to the next object in a linked-list
struct Threshold {
	int numOfIncidents;
	int numOfMinutes;
  struct Threshold* next;
};

// ThresholdList is a container for a linked list of Threshold structs

// head is the first element in the linked list
// tail is the last element in the linked list
// count is the number of elements in the linked list
struct ThresholdList {
	struct Threshold* head;
	struct Threshold* tail;
	int count;
};

/*
** Function Prototypes
** -----------------------------------------------------
*/

// initialize filed for threshold list
void createThresholdList(struct ThresholdList* tl);

// Standard linked-list queue-style 
// inserts threshold struct at the tail of the list
void insertIntoThresholdList(struct ThresholdList* tl, struct Threshold* th);

// used for debugging purposes and to present 
// output to the user in a friendly way
void printThresholdList(struct ThresholdList* tl);

// remove a threshold struct from the list and return a pointer to it. 
// threshold struct will be removed from the head
void removeFromThresholdList(struct ThresholdList* tl, struct Threshold* th);

// remove a threshold struct from the head of the list and destroy it
void removeAndDestroyThreshold(struct ThresholdList* tl);
//Returns the number of thresholds in the Threshold List
int getCountOfThresholdList(struct ThresholdList* tl);

// While the count of the threshold list is greater than zero call the
// 'removeAndDestroyThreshold'. Once the list is empty, free the memory of the
// list structure 
void destroyThresholdList(struct ThresholdList* tl);
//goes through the threshold list and free all thresholds and then the threshold list
void deleteThresholdList(struct ThresholdList* thresholdList);

int getExpiringTime(struct ThresholdList* tl);
