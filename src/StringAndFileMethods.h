#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>

#define BOOL int
#define TRUE 1
#define FALSE 0

#define OFFSET 0
#define ERROR -1
#define NO_ERROR 1
#define STRING_LENGTH 512 //length for a standard string so there won't be any issues with
//strings being too short
#define LONG_STRING_LENGTH 1024 //for strings in which 512 may be too short
#define LONG_SUBJECT_LENGTH 32768 //for strings in which 512 may be too short

#define READ_IN_STRING 400 // a valid string was read in
#define NO_STRING_READ 401 // a string was not read in, the length is 0
#define END_OF_FILE 402 // The end of the file has been reach.
#define STRANGE_END_OF_FILE 403

#define EMAIL_SUBJECT_CHAR_LIMIT 255
#define EMAIL_SUBJECT_SUFFIX_CHAR_LIMIT 30
#define EMAIL_SUBJECT_SUFFIX "... - Automated CSS Alarm Tool"

#define CDF "CDF"
#define CRITICAL_DETECTION_FAILURE "CRITICAL DETECTION FAILURE"
#define CTDF "CTDF"
#define CRITICAL_TRAIN_DETECTION_FAILURE "CRITICAL TRAIN DETECTION FAILURE"
#define TF "TF"
#define TRACK_FAILURE "TRACK FAILURE"
#define PTSLS "PTSLS"
#define PRIMARY_TCS_SERVER_LINK_TO_SERVER "PRIMARY TCS SERVER LINK TO SERVER"
#define ALL "ALL"
#define THRESHOLDS "THRESHOLDS"
#define SERVER_TYPE_TCS "TCS SERVER"
#define SERVER_TYPE_PNL "PNL SERVER"
#define SERVER_TYPE_ORS "ORS SERVER"
#define SERVER_TYPE_COM "COM SERVER"
#define SERVER_TYPE_WBSS "WBSS SERVER"
#define SERVER_SWITCHOVER "PRIMARY -- SWITCHOVER COMPLETED"
#define PNL_SWITCHOVER_NAME "PNL SWITCHOVER"
#define TCS_SWITCHOVER_NAME "TCS SWITCHOVER"
#define ORS_SWITCHOVER_NAME "ORS SWITCHOVER"
#define COM_SWITCHOVER_NAME "COM SWITCHOVER"
#define WBSS_SWITCHOVER_NAME "WBSS SWITCHOVER"
#define SERVER_SWITCHOVER_NAME "SERVER SWITCHOVER"
#define FCU_AUTO_SWITCH "FCU AUTO DTS SWITCH"
#define FCUCF_STRING1 "FEP"
#define FCUCF_STRING2 "OUT OF SERVICE REPEATED ERROR TIME OUT"
#define RSO_STRING1 "PASSED SIGNAL"
#define RSO_STRING2 "AT STOP"
#define PNLSO "PNLSO"
#define TCSSO "TCSSO"
#define ORSSO "ORSSO"
#define COMSO "COMSO"
#define WBSSO "WBSSO"
#define SERVSO "SERVSO"
#define FCUAS "FCUAS"
#define FCUAS_NAME "FCU AUTO SWITCH"
#define VHLCAS_NAME "VHLC AUTO SWITCH"
#define FCUCF "FCUCF"
#define FCUCF_NAME "FCU COMMUNICATION FAULT"
#define VHLCCF_NAME "VHLC COMMUNICATION FAULT"
#define RSO "RSO"
#define RSO_NAME "RED SIGNAL OVERRUN"
#define RLSC "RLSC"
#define RLSC_NAME "REMOTE LINK STATUS CHANGE"
#define RLSC_STRING1 "RemoteLink"
#define RLSC_STRING2 " - STATUS CHANGED"

#define INCIDENT_LOGS_FILE "CSS_Alarms" // name of the file that holds the last tracked incidents
#define INCIDENT_TYPES_FILE "Incident_Types" // name of the file that holds the incidentTypes
#define OBJECTS_FILE "Objects" // name of the file that holds the incidentTypes
#define LOG_FILES "Log_Files"
#define DATABASE_FILES "Database_Files" // name of the folder that holds the
  // four database files
#define OTHER "Other" // name of the folder used to hold files that do no have
  // a folder to themselves. A catch-all
#define INCIDENT_LOGS "Incidents" //name of the folder that holds the incidents that have last been tracked by the ACAT tool
#define DISABLED_INCIDENTS "Disabled_Incidents" // name of the file that
  // contains the list of disabled incidents
#define DISABLED_INCIDENTS_FILE_PATH "DisabledIncidentsFilePath" // name of the file
  // that contains the filePath for temporarily disabled incidents.
#define CONVENTIONAL_TO_WBSS_LIST "Conventional_to_WBSS_list"
#define TRACK_CIRCUIT_LOCATION_REASSIGNMENT_LIST "Track_Circuit_Location_Reassignment_list"
#define LOG_FOLDER_PATHS "LogFolderPaths" // name of the file that contains the
  // list of folders paths to the folders TCS-A, TCS-B, etc
#define CRITICAL_INCIDENTS_FILE "Critical_Incidents"

#define SEMI_COLON ";" // a semi colon

#define DOT_LOG ".log" // an extension
#define DOT_TXT ".txt" // an extension
#define DOT_HTML ".html" // an extension
#define DOT_CSV ".csv" // an extension
/*
** Function Prototypes
** -----------------------------------------------------
*/

// Due to Linux vs Windows differences in filepaths a method that could easily
// change the direction of slahes was neccesary. Additionally, this ensures
// consistency in file and folder names
// This function concatenates a folder name and a filename with the "local"
// directory "./"
void constructLocalFilepath(char* filePath, char* folderName, char* fileName, char* extention);

// remove the first num chars from a string
// useful for trimming 8 leading charatcers from CSS logfile entries.
void removeFirstChars(char *s, int num);

// find the position of a substring within another using the power of pointers
// and indexes
int getPositionOfSubstring(char* str, char* substr);

// copy chars from one string into another until a certain word within the
// main string
void getCharsUpTo(char* str, char* output, char* afterWord);

// Reads in a single line, up to a '/r', '/n' or 'EOF' from a file and 
// copies it into 'line'
// returns a integer indicating what cause the method to finish and will
// dynamically resize the char array if string is too long
// filters out comments
int readInLine(FILE* fl, char** line, size_t size);
