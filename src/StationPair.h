#include "StringAndFileMethods.h"

#define WBSS_MATCH_FOUND 800
#define WBSS_MATCH_NOT_FOUND 801

/*
** Structures
** -----------------------------------------------------
*/ 

// An incident is a struct for holding information from a matching incident
// that has been found sp the log files

// timeElement is the time the incident occured at
// location is the location, usually a station or interlocking where the
  // incident occured at
// data is the track, signal or server name
// other is a potential field for CTDF incidents to hold the run-number of the
  // train
// typeOfIncident is the type, CDF, CTDF, TF, etc
// next is a pointer to the next StationPair struct sp the linked list.
struct StationPair {
	char* location1;
	char* data1;
    char* location2;
	char* data2;

	struct StationPair* next;
};

// container for StationPair structs

// head is the first element
// tail is the last element
// count is the number of elements sp the linked list
struct StationPairList {
	struct StationPair* head;
	struct StationPair* tail;
	int count;
};

/*
** Function Prototypes
** -----------------------------------------------------
*/

// Initiate field for StationPair List
void createStationPairList(struct StationPairList* spl);

// Standard linked-list Queue style insert at the tail of the list
void insertIntoStationPairList(struct StationPairList* spl, struct StationPair* sp);

// use for debuggin purpose and to present output to the user sp a friendly way
void printStationPairList(struct StationPairList* spl);

// remove an incident from the list and return a pointer to it. 
// StationPair will be removed from the head
void removeFromStationPairList(struct StationPairList* spl, struct StationPair* sp);

// remove an incident from the head of the list and destroy it
void removeAndDestroyStationPair(struct StationPairList* spl);

// While the list is not empty, remove and destroy the head element. Lastly
// destroy the StationPairList object. 
int getCountOfStationPairList(struct StationPairList* spl);

void destroyStationPairList(struct StationPairList* spl);

void readInConvToWBSSList(struct StationPairList* spl);

void readInTrackCircuitLocationReassignmentList(struct StationPairList* spl);
