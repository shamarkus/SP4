#include "DateAndTime.h"
#include "Threshold.h"
#include "Incidents.h"

#define EMAIL "EMAIL" // A string used when printing information to the database
#define NOEMAIL "NOEMAIL" // A string used when printing information to the database

#define NO_CONDITION_MET 100 // Threshold condition was not met
#define CONDITION_MET 101 // Threshold condition was met
#define INCIDENT_OVER_TIME 101 //threshold condition met but all incidents exceed threshold num of hours
//(for summary email only)

#define EMAIL_NOT_SENT_YET 200 // An email has not been sent about this record
#define EMAIL_ALREADY_SENT 201 // An email has already been sent about this record

/*
** Structures
** -----------------------------------------------------
*/  
  
// A flag for if an email has been sent regarding this DatabaseRecord's
// incident.

// msg will be either "EMAIL" or "NOEMAIL"
// timeOfEmail will be the time it was sent
struct Flag {
  char* msg;
  time_t timeOfEmail;
};

// The struct representing a single record in the database.

// location is the location of the incident, usually a station or interlocking
// data is the track name, or switch name, or sever name
// other contains additional information - for CTDF, this is the run number of the train
// flag will indicate if an email has been sent and if so, when
// timeList will be a list of all times that this incident occurred as
// next is a pointer to the next element in the linked  list
struct DatabaseRecord {
	bool lastRecord;
	char* location;
	char* data;
        char* other;
	char* extra;
	char* subwayLine;
        char* typeOfIncident;
        char* lastSummaryEvent;
        struct IncidentType* incidentType;
	struct Flag* flag; 
	struct TimeList* timeList;
	struct DatabaseRecord* next;
};

// contain for a linked-list of DatabaseRecords

// head is the first element
// tail is the last element
// count will be the number of elements
struct DatabaseList {
	struct DatabaseRecord* head;
	struct DatabaseRecord* tail;
	int count;
	bool headerExists;
  	BOOL isOnBoardIncident;
};

/*
** Function Prototypes
** -----------------------------------------------------
*/

// Initialize variables for DatabaseList
void createDatabaseList(struct DatabaseList* dbl);

// Standard linked list, queue style, data is inserted at the end of the list,
// at the tail 
void insertIntoDatabaseList(struct DatabaseList* dbl, 
  struct DatabaseRecord* dr);
  
// 'updating' a databaseRecord means adding a new time to its timeList 
// (likely one from an incident list, that was read in from a logfile)
void updateDatabaseRecord(struct DatabaseRecord* dr, struct TimeElement* te);

// Used for debugging purposes and user-side checks.
// prints list in user-friendly way
void printDatabaseList(struct DatabaseList* dbl);

// entries are removed from the linked-list at the head, as is standard for
// queue-style linked lists.
// this removed method return a pointer to the removed element, but will not 
// delete it
void removeFromDatabaseList(struct DatabaseList* dbl, struct DatabaseRecord* dr);

// entries are removed from the linked-list at the head, as is standard for
// queue-style linked lists.
// this method will delete the databaseRecord
void removeAndDestroyDatabaseRecord(struct DatabaseList* dbl);

int getCountOfDatabaseList(struct DatabaseList* dbl);

// recursively calls removeAndDestroyDatabaseRecord until list is empty and
// then deletes the list itself
void destroyDatabaseList(struct DatabaseList* dbl);

// A cetain type of incident's Database file will be read in and saved in a
// linked list of DatabaseRecord structs. This method has a caveat at incidents
// in the database file that occured more than X hours ago, where X is 
// emailDelayTimeHours, are not read in. These entries are considered 'expired'

// emailDelayTimeHours is a variable that is passed in from a function on the 
// thresholdList for this type of incident that finds the max number of hours
// ago records must be kept from in order to successful check the threshold
// conditions. E.g. if the threshold conditions are 5,1 and 10,24 records from 
// 24 hours ago will be kept.
void readInDBFile(char* filePath, struct DatabaseList* dbl, 
  int emailDelayTimeHours, struct SummaryEmailList* sel,struct IncidentType* incidentType);

// Method to take in a linked-list of DatabaseRecords and format them into
// a structured output file.
void printDatabaseToFile(char* typeOfIncident, struct DatabaseList* dbl);

// This method will cycle through a DatabaseRecord's timelist and see if any
// pattern of times satisfies a particular threshold condition of X times in
// Y hours.
int checkThresholdCondition(struct Threshold* th, struct DatabaseRecord* dr, struct TimeElement** a);

//for checking threshold for a summary email - additional restrictions apply
int checkSummaryThresholdCondition(struct Threshold* th, struct DatabaseRecord* dr, struct TimeElement** a);

// Check if a DatabaseRecord has been emailed about or no
int checkEmailStatus(struct DatabaseRecord* dr);
// This method is called to add all of the incidents in the incident list of
// a certain type to the database list for that same type. The database list will
// have been read in from the database file (see ./Database_Files")
void addIncidentsToDatabaseList(char* typeOfIncident, struct DatabaseList* dbl, struct IncidentList* il);
