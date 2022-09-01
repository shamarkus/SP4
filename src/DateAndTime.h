#include "StringAndFileMethods.h"

#define DATE_STRING_LENGTH 24 // Length of the char array used to hold the date
  // when formatted into a style to match the CSS log files: 
  // "HH:mm:SS MM/DD/YY" 
#define ISODATE_STRING_LENGTH 24 // Length of the char array used to hold the date
  // when formatted into ISO-8601 date for feeding into influxDB
  // "YYYY-MM-DDTHH:mm:SSZ"
#define FILTER_TIMES "filterTimes" // name of the file which will contain the
  // non-revenue hours

/*
** Structures
** -----------------------------------------------------
*/  
  
// timeObj will be the time that an incident occurs
// next will be a pointer to the next TimeElement struct
struct TimeElement	{
	time_t timeObj;
  char* location;
	struct TimeElement* next;
};

// container for a linked list of TimeElement structs

// head is the first element
// tail is the last element
// count is the number of elements
struct TimeList	{
	struct TimeElement* head;
	struct TimeElement* tail;
	int count;
};

/*
** Function Prototypes
** -----------------------------------------------------
*/

// form a time_t object from the string representing the date that is 
// read in from the log files.
// read in from the log files.
BOOL getDateFromString(char* s, time_t* tt );

// Initialize variables for TimeList
void createTimeList(struct TimeList* tl);

// In order for readInFile methods and methods that check Threshold Conditions
// to work the list most be order oldest to newest. The method will insert in 
// order.
// Other linked-linked in other files will not share this proper, they will
// be standard queues.
void insert(struct TimeList* tl, struct TimeElement* te);

// This method will print out TimeList as comma separated list.
// This is used to write out database file.
// see "printDatabaseToFile"
void printTimeList(FILE* stream, struct TimeList* tl);

// remove element from the head of the list
// returns a pointer to the removed element
void Remove(struct TimeList* tl, struct TimeElement* te);

// removes all but one of the timeElement structs with time time.
void RemoveDupTime(struct TimeList* timeList, time_t time);

// removes a TimeElement from the list at the head and then destroys it
void removeAndDestroy(struct TimeList* tl);

int getCount(struct TimeList* tl);

// Repeated calls removeAndDestroy on the list until the list is
// empty, then destroy the TimeList object
void destroyTimeList(struct TimeList* tl);

// Takes a time_t object and converts it into a tm struct,
// values are then formatted into a CSS friendly string format
char* getStringFromDate(time_t timeObj);

// Takes a time_t object and converts it into a tm struct,
// values are then formatted into a ISO-8601 datetime format
char* getISOStringFromDate(time_t timeObj);

// Similar to getStringFromDate, but the string is formatted in a different
// order and the minutes are seconds are not relevant.
// logfile names sare only about year, month, day and hour.
char* getFilenameFromDate(time_t timeObj);

// take in a  filename of the form "2015081811" for August 18, 2015 at 11:00
// and turn it into a time_t object for easier comparisons and arithmetic
BOOL getDateFromFileName(char* s, time_t* tt);

// The 'date stamp' that is added to database files when they are deprecated
// cannot contain slashes because they interfere with filepaths / extensions.
// Therefore, 'getStringFromdate' cannot be used. This method will implement
// similar functionality, with the same order and format, but without slashes.
// "HHmmSS_MMDDYY" 
char* getSlashlessDatestampFromDate(time_t timeObj);

//A function that will go through a time list and remove any duplicate times as specficed by time
void RemoveDupTime(struct TimeList* timeList, time_t time);

void printOnboardTimeList(FILE* stream, struct TimeList* tl);
