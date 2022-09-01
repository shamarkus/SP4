#ifndef EMAILINFO_H
#define EMAILINFO_H
#include "StringAndFileMethods.h"

#define EMAIL_RECIPIENTS "email_recipients" // name of the file which will
  // contain the list of email recipients
#define EMAIL_INFO "Email_Info" // name of the folder which contains the file
  // "email_recipients" and the .html email files before they are sent
#define ADMIN_GROUP_IDENTIFIER "admin" //identifier for admin group in email_recipients.txt file


/*
** Structures
** -----------------------------------------------------
*/
  
// EmailInfo is a object to hold information on recipients of emails and
// what types of emails they should be emailoed about

// personsEmail is the address of the person
// emailFileName is the name of the .html file that will be emailed to them
// BOOL sendEmail is a flag for sending emails
// BOOL isAdmin is a flag for people in the "admin" group that should receive 24-hour all-clear emails
// typeOfIncidentList is a list of types of incidents they should be emailed 
  // about
// next is a pointer to the element in the linked list 
struct EmailInfo {
  char* personsEmail;
  char* emailFileName;
  char* bodyFileName;
  BOOL sendEmail;
  BOOL isAdmin;
  struct SubjectList* subject_list;
  struct TypeOfIncidentList* typeOfIncidentList;
  struct EmailInfo* next;
};
//SummaryEmail is a struct that holds all the information needed to send summary emails to the right person

//emailFileName is the name of the file name that will hold the email
//location is the location of the incident that the email is about
//data is the ID of the object that the incident happened to
//other and extra are bonus fields that are used to store additional info about the incident
//typeOfIncident is the short name of the incdient the email is about, ie TF, or TLD
//tl is the time list of all the times the incident happened
//next is a pointer to the next summary email (for linked list use)
struct SummaryEmail {
    char* emailFileName;
    char* location;
    char* data;
    char* other;
    char* extra;
    char* typeOfIncident;    
    struct TimeList* tl;
    struct SummaryEmail* next;
};

// container for EmailInfo structs

// head is the first element
// tail is the last element
// count is the number of elements in the linked list
struct EmailInfoList {
	struct EmailInfo* head;
	struct EmailInfo* tail;
	int count;
};
// container for SummaryEmail structs

// head is the first element
// tail is the last element
// count is the number of elements
struct SummaryEmailList {
    struct SummaryEmail* head;
    struct SummaryEmail* tail;
    int count;
};

//container for SubjectIncident structs (incidents to be added to subject line)
// head is the first element
// tail is the last element
// count is the number of elements
struct SubjectList
{
  struct SubjectIncident* head;
  struct SubjectIncident* tail;
  int count;
};

struct SubjectIncident
{
  char* type_of_incident;
  char* incident_name;
  struct LocationList* location_list;
  struct SubjectIncident* next;
};

//container for Location structs
struct LocationList
{
  struct Location* head;
  struct Location* tail;
  int count;
};

struct Location
{
  char* location;
  struct Location* next;
};

/*
** Function Prototypes
** -----------------------------------------------------
*/

// Initiate linked list of EmailInfo objects
void createEmailInfoList(struct EmailInfoList* el);

// Standard linked-list queue-style
// Inserts a EmailInfo Object at the tail of the list
void insertIntoEmailInfoList(struct EmailInfoList* el, struct EmailInfo* ei);

// Used for debugging purposes
// Prints out the email info of the recipients during execution
void printEmailInfoList(struct EmailInfoList* el);

// Remove an email from the EmailInfoList and returns a pointer
// elements are removed from the head of the list
void removeFromEmailInfoList(struct EmailInfoList* el, struct EmailInfo* ei);

// removes an an EmailInfo element from the EmailInfoList and destroys it
void removeAndDestroyEmailInfo(struct EmailInfoList* el);

int getCountOfEmailInfoList(struct EmailInfoList* el);

// While the list is not empty, removeAndDestroyEmailInfoList is called
// repeated. Finally the EmailInfoList struct is freed
void destroyEmailInfoList(struct EmailInfoList* el);

// Initiate linked list of SubjectList objects
//  sl - SubjectList to be created
//	return	- Void
void 
create_subject_list(struct SubjectList* sl);

// Standard linked-list queue-style
// Inserts a SubjectInfo struct at the tail of the list
//	sl	- The SubjectList in which the new SubjectIncident will be inserted
//	si	- The new SubjectIncident
//	return	- Void
void 
insert_into_subject_list(struct SubjectList* sl, struct SubjectIncident* si);

// Initiate linked list of LocationList objects
//  list - LocationList to be created
//	return	- Void
void 
create_location_list(struct LocationList* list);

// Standard linked-list queue-style
// Inserts a SubjectInfo struct at the tail of the list
//	sl	- The SubjectList in which the new SubjectIncident will be inserted
//	si	- The new SubjectIncident
//	return	- Void
void 
insert_into_location_list(struct LocationList* list, struct Location* loc);

// removes a SubjectInfo element from the SubjectList and destroys it
//	sl	- The SubjectList who's head will be removed and freed
//	return	- Void
void 
remove_and_destroy_subject_incident(struct SubjectList* sl);

void 
remove_and_destroy_location(struct LocationList* list);


//Returns the number of SubjectIncidents in the SubjectList
//	sl	- The SubjectList whose count will be returned
//	return	- The count of Email Infos in the EmailInfoList
int 
get_count_of_subject_list(struct SubjectList* sl);

int 
get_count_of_location_list(struct LocationList* list);

// While the list is not empty, remove_and_destroy_subject_incident is called
// repeatedly. Finally the SubjectList struct is freed
//	sl	- The SubjectList to be deleted.
//	return	- Void
void
destroy_subject_list(struct SubjectList* sl);

void
destroy_location_list(struct LocationList* list);


// Readin the list of emailRecipients from the file 
// "./Email_Info/email_recipients.txt" and stored them in a linked-list of
// EmailInfo structs

// the email_recipients.txt file is similarly structured to the 4 database files
// Individual fields are separated by semicolons (;) while the list of types of
// incidents the person is supposed to be emailed about are separated by 
// commas (,) ex. "Matthew.Weston@ttc.ca;CDF,TF,CTDF"
void readInEmailInfoFile(struct EmailInfoList* el);

// Check if a person should receive an email give their emailInfoList field.
BOOL shouldReceiveEmail(struct EmailInfo* ei, char* typeOfIncident);
//Sets the Summary Email List to its inital state
void createSummaryEmailList(struct SummaryEmailList* sel);
//Removes and frees the memory of the head of the Email Summary List
void removeAndDestroySummaryEmail(struct SummaryEmailList* sel);
//Removes and frees every summary email in the Summary Email List
void destroySummaryEmailList(struct SummaryEmailList* sel);
//Returns the number of summary emails in a Summary Email List
int getCountOfSummaryEmailList(struct SummaryEmailList* sel);

void insertIntoSummaryEmailList(struct SummaryEmailList* sel, struct SummaryEmail* se);
#endif
