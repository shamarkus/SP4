#ifndef INCIDENTS_H
#define INCIDENTS_H
#include "DisabledIncidents.h"
#include "StationPair.h"
#include "EmailInfo.h"


#define START_UP 600 // The tool has never run before, there are no previous
  // lines/files to start from, so start from 24 hours ago.
#define PREVIOUS_FILE_WAS_EMPTY 601 // the last file that was attempted to be 
  // read was empty, so start from beginning, it may have info now.
#define MIDDLE_OF_PREVIOUS_FILE 602 // the last file that was read had 
  // incidents in it.

#define EMAILS_DISABLED 300 // emails have been disabled for this record
#define EMAILS_ENABLED 301 // emails have noit been disabled for this record
#define ENABLED_DISABLED_ERROR 302 // there was an error checking if the record
  // had emails disabled or not
 
#define LOCATION "LOCATION" // constant for the word "LOCATION"
#define SWITCH_LITERAL " SWITCH" // constant for the word " SWITCH"
#define CRITICAL " CRITICAL" // constant for the word " CRITICAL"
#define AT " AT" // constant for the word " AT"
#define FAILED_LITERAL " FAILED" // constant for the word " FAILED"
#define IS " IS" // constant for the word " IS"
#define SINGLE_HYPHEN " -" // constant for a single hyphen
#define SPECIAL_LITERAL " SPECIAL " // constant for the word "SPECIAL"
#define REMOTELINK "RemoteLink" // constant for the word "RemoteLink"
#define SERVER_LITERAL " SERVER" //constant for the word "SERVER"
#define FCU "-FCU AUTO DTS SWITCH" //constant for FCU string
#define TO " TO" //constant for the word TO
#define COMMA ","
#define COMMA_CHAR ','
#define KEYWORD_SEPERATOR_TOKEN "|"
//used to tell getFormatedLined() whether it should format using emailTemplate(0) or emailLocationTemplate(1)
#define EVENT_LINE 0
#define LOCATION_LINE 1
#define SUMMARY_LINE 2

//used to tell addIncidentToLogs which character is currently being processed.
#define TRAIN_STRING_LENGTH 20
#define TRAIN_CLASS 0
#define TRAIN_RUN 1
#define TRAIN_ORIGIN 4
#define TRAIN_DESTINATION 5
#define TRAIN_DISPATCH_TIME 6
#define TRAIN_SCHEDULE_MODE 12
#define LONG_BERTH_LENGTH 13

#define LINE_LENGTH 5

#define SHORTCODE_LENGTH 3

#define ADTSS "ADTSS" //an auto DTS switch 
#define FEPTO "FEPTO" //an out of service repeated error timeout incident

#define SERVER_NAME_SWITCH_FLAG "S" //character used in processing string to check
//if the incident's data (the thing that the incident happened to) should be
//checked against the server name LUT
#define CRITICAL_INCIDENT_FLAG "C" //character used in processing string to check
//if the incident should be checked to see if it a critical incident
#define REVENUE_HOUR_TIME_CHECK_FLAG "R" //character used in processing string to check if
//revenue hours should be checked for
#define EXTENED_TABLE_FLAG "T" //character used in processing string to check if 
//the email message used have the extened table
#define MISSION_CRITICAL_FLAG "M" //character used in processing string to always output
//CRITICAL INCIDENT, in the email for the incidentType
#define GET_PREVIOUS_SERVER_FLAG "N" //character used in processing string to tell the
//program to look at the server name in data and figure out the previous server
#define CHECK_TCS_VHLC_SERVER_FLAG "V" //character used in processing string to tell the
//program to look at the server location to figure out whether the incident happened
//as a TCS or VHLC issue
#define GET_CAR_NUMBER_FLAG "F" //character used in processing string to tell the program
//to look at the data variable and check it against the CCPair lookup table
#define ONBOARD_INCIDENT_FLAG "O" //character used in processing string to tell the
//program that the incident occurs onboard a train
#define SERVER_RELATED_INCIDENT "Q" //character used in processing string to tell the 
//program that the incident is server-based

#define PASSED " PASSED"
#define TRAIN " TRAIN"
#define LINE_NOT_FOUND 501 // A line was expected to be in this file but was
  // not
#define LINE_FOUND 500 // A line that was expected to be in this file was
#define NOT_SEARCHED_FOR 502 // No line was being searched for

#define RECORDS "records" // name of the file which contains the names of the 
  // last files that were read in
#define RECORDS2 "records2"
#define NUM_OF_FOLDERS 6 // number of folder, currently six: TCS-A, TCS-B, 
  // TCS-2A, TCS-2B
  // atc/TCS-A, atc/TCS-B
#define TEMP_FILE "_temp.log" // name of the temp file that is used to make a 
  // copy of an existing WBSS file to avoid interfering with other programs
  // operation

#define FOLDER_NAME_LENGTH 8 // length of the char array used to hold the string
  // TCS-A, TCS-B, etc while creating file extensions
#define DAYS_OF_WEEK 7

/*
** Structures
** -----------------------------------------------------
*/ 
  
// a struct to hold info for time periods during which incident times should
  // not be added to the database because they are irrelevant
  
// startTime is the start of non-revenue hours
// endTime is the end of non-revenue hours
// timesExist is flag for whether this object actually has start and end times
struct FilterTime {
	struct tm startTime;
	struct tm endTime;
	BOOL timesExist;
};

// Record is a struct to hold info about the last file and line within the file 
// that has been read in and parsed

// fileName is the name of the last file that was read in
// lastReadLine is the last line within the file, fileName, that was read in
struct Record {
  char* fileName;
  char* lastReadLine;
};

// a keyword is a word or phrase in an incidentType that is constant throughout
// all incidentType messages recived from the log file. eg : TRAIN , "Long Docked".
// these words or phrases are used to get needed info from the message by giving
// points of reference throughout the message.
// word - the keyword
// next - a pointer to the next keyword
struct Keyword
{
    char* word;
    struct Keyword* next;
};

// A container for keyword linked lists.
struct KeywordList
{
    struct Keyword* head;
    struct Keyword* tail;
    int count;
};

// an IncidentType struct is used to be able to dynamically read in 
// incident types from ./Other/alerts.txt and then parse for them 
// throughout the data logs.
// KeywordList is a pointer to a struct that contains the linked list of keywords
// structOfMessage is a string that shows how the line in the data log is formated
// object is the acutal thing the incident is about
// data is the string that says what the incident is
// typeOfIncident is a string that identifies the type of incident 
// emailTemplate is a string that formates what the email message will look like and say
// thresholdList is a point to a struct that contains the list of thresholds for the IncidentType
// next is a pointer to the next IncidentType in the list
struct IncidentType
{
    struct KeywordList* keywordList;
    char* typeOfIncident;
    char* object;
    char* emailTemplate;
    char* emailTemplateLocation;
    char* summaryTemplate;
    char* processingFlags;
    struct ThresholdList* thresholdList;
    struct IncidentType* next;
};

// An incident is a struct for holding information from a matching incident
// that has been found in the log files

// timeElement is the time the incident occured at
// location is the location, usually a station or interlocking where the
  // incident occured at
// data is the track, signal or server name - The ID of the object
// other is a potential field for certain incidents which need more fields to use
// extra is a backup field in three fields are not enough
// incidentType is a pointer to a struct that defines what type of incident it is
// typeOfIncident is the type, CDF, CTDF, TF, etc
// next is a pointer to the next Incident struct in the linked list.
struct Incident {
	struct TimeElement* timeElement;
	char* location;
	char* data;
	char* other;
        char* extra;
	char* subwayLine;
        char* typeOfIncident;
	struct IncidentType* incidentType;
	struct Incident* next;
};

// A container for IncidentType linked lists.
struct IncidentTypeList
{
    struct IncidentType* head;
    struct IncidentType* tail;
    int count;
};

// container for Incident structs

// head is the first element
// tail is the last element
// count is the number of elements in the linked list
struct IncidentList {
	struct Incident* head;
	struct Incident* tail;
	int count;
};

struct HostnameLUT {
    char* uip;
    char* yusHostname;
    char* bdsHostname;
    char* cyusHostname;
};

struct CCPair
{
	int cc;
	int carNum;
	struct CCPair *next;
};
/*
** Function Prototypes
** -----------------------------------------------------
*/

// Initiate field for Incident List
void createIncidentList(struct IncidentList* il);

// Standard linked-list Queue style insert at the tail of the list

void insertIntoIncidentList(struct IncidentList* il, struct Incident* in);

// use for debuggin purpose and to present output to the user in a friendly way
void printIncidentList(struct IncidentList* il, char* typeOfIncident);

// remove an incident from the list and return a pointer to it. 
// Incident will be removed from the head
void removeFromIncidentList(struct IncidentList* il, struct Incident* in);

// remove an incident from the head of the list and destroy it
void removeAndDestroyIncident(struct IncidentList* il);

//returns the number of incidents in the Incident List
int getCountOfIncidentList(struct IncidentList* il);
// While the list is not empty, remove and destroy the head element. Lastly
// destroy the IncidentList object. 
void destroyIncidentList(struct IncidentList* il);

// lines read in from the logfiles are prefaced by 8 linux characters that are
// not relevant to the info in the line. This method is design ONLY for reading
// in lines from the log files as it automatically removes those 8 characters.
int readInLineAndErase(FILE* fl, char** line, size_t size);

// find matching error message and its macro shortcode
BOOL containsErrorMessage(char* str, char* msg,struct IncidentTypeList* incidentTypeList,struct IncidentType** incidentType);

// Reads in a CSS log file and adds relevant incidents to the IncidentList, il,
// that is passed in. Depending on the value of preLastReadLine, the function
// may skip some entries that have already been read in and processed.
int readInLogFile(char* filePath, char* lastReadLine, struct IncidentList* il, 
  char* newLastReadLine, struct FilterTime* filterTimes[7], 
  struct IncidentList* disabledList, struct DisabledIncidentList* disabledIncidentList,
  struct StationPairList* spl, char* subwayLine,
  struct IncidentTypeList* incidentTypeList);

// function to read in ALL necessary log files. 
// If code has ran previously and is in the middle of an hour then likely only 
// 1 file will be read from
// If it is the start of a new hour then 2 files, last hour's and this hour's
// will both be handle.
// Lastly, if the tool is running for the first time, the last 24 hours worth 
// of files will be handled.
int readInFiles(struct IncidentList* il,struct IncidentTypeList* incidentTypeList);

// Read in file containing incidents that have been disabled due to frequency 
// or non-safety related explanations. Incidents are stored in a linked-list
// or Incident structs and are saved in the file "./Other/Disabled_Incidents"
void readInDisabledIncidents(struct IncidentList* il);

// read in filter times for every day of the week
// these times will be non-revenue hours during which faults and failures
// are ignored because they are do to work and not passneger trains failing
BOOL readInFilterTimes(struct FilterTime* ft[7]);
// Certain tracks sections or switches are considered to be in work yards from the point
// of view WBSS. However, practically, they are on the mainline and revenue trains
// use them daily.

// 'reassignTrackCircuitLocations' will change the location and data field of certain
// tracks and switches to fit better with practical circumstances. It will use a .txt file
// which contains a list of track sections and switches in pairs to rename the location and
// data fields.
BOOL reassignTrackCircuitLocations(struct Incident* in, struct StationPairList* spl);

// check to see if an incident occured during revenue hours or working hours 
// if it was during working hours, then it is ignored.
//	in	-- The incident being checked.
//	ft	-- An array with the beginning and end of revenue hours for each day.
//	return	-- FALSE if outside of revenue Hours, TRUE if within revenue hours (most incidents).
BOOL within_revenue_hours(struct Incident* in, struct FilterTime* ft[7]);

// Read in ThresholdList and DatabaseList associated with this type of incident
// and merge the new incidents that have been read in from the log files into
// the databaseList. Check Threshold, disabled status and if this incident has 
// been emailed about before and if these conditions are all met, prepare an
// email to send.
struct DatabaseList*  processInfo(char* typeOfIncident, struct IncidentList* incidentList, struct EmailInfoList* emailInfoList, struct SummaryEmailList* sel, struct IncidentType* incidentType, struct CCPair* ccPairLLHead);

//Sets the Incident Type List to it's default state
void createIncidentTypeList(struct IncidentTypeList* incidentTypeList);

// reads in alert.txt data file and saves each alert/incident type in the linked list,
// along with the treshold values for each incident type
int readInIncidentTypes(struct IncidentTypeList* incidentTypeList);
//adds an incidentType to the end of an incidentTypeList
void addToIncidentTypeList(struct IncidentTypeList* incidentTypeList, struct IncidentType* incidentType);
//prints all info in an incidentType struct for debugging
void printIncidentType(struct IncidentType* incidentType);
//free all memory allocated for an IncidentTypeList and its memebers, and sets all
//pointers to NULL
void deleteIncidentTypeList(struct IncidentTypeList* incidentTypeList);
//Sets all keywordList variables to their inital state
void createKeywordList(struct KeywordList* keywordList);
//adds a keyword to the end of a keywordList
void addToKeywordList(struct KeywordList* keywordList, struct Keyword* keyword);
//Fress all memory allocated for a keywordList and its members,
//and sets all pointers to NULL
void deleteKeywordList(struct KeywordList* keywordList);
//Frees all memory allocated for a thresholdList and it's members,
//and sets all pointers to NULL
extern void deleteThresholdList(struct ThresholdList* thresholdList);
//splits up the keywords from Incident_Types.txt into a keywordList
int parseKeywords(struct IncidentType* incidentType,char* keywords);
//splits up the threshold string from Incident_Types.txt into a thresholdList
int parseThresholds(struct IncidentType* incidentType, const char* thresholds);
//checks for all special keywords in the keywordList, and if it finds one, it saves it
//in it's specified variable
void parseIncident(struct IncidentType* incidentType,struct Incident* incident,char* line);

// find the number of occurences of a single character in a string
// mainly used for commas and semi-colons when parsing
int getCharCount(char* str, char ch);
//used to check if a line read from a log file contains all the keywords of a incidentType
BOOL containsKeywords(struct IncidentType* incidentType,const char* msg);
//used to check if the substring exists in the string
BOOL contains(const char* string,const char* substring);
//gets the string contained between the previous keyword and next keyword and stores in it in the
//provied char*
BOOL getSpecialKeyword(const char* line,const char* prev,const char* next,char* container);
//puts the start index of the substring in startIndex and the end index of the substring in endIndex
BOOL getIndexOfStr(const char* string,const char* substring,int* startIndex,int* endIndex);
//uses the subway line and server name to get a meaningful server name
void switchServerNames(struct Incident* incident,char* subwayLine);

BOOL readInRecords(struct Record* recordsList[NUM_OF_FOLDERS]);

int checkEnabledDisabled2(struct DisabledIncidentList* dil, struct Incident* in);

int incidentDuringDurationWindow(struct Incident* in, struct Duration* dur);
#endif
