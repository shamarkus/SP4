#include "Threshold.h"

/*------------------------------------------------------
**
** File: Threshold.c
** Author: Matt Weston & Richard Tam
** Created: August 31, 2015
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - Changed #include files
**                      - 
*/

// initialize filed for threshold list
//	tl	- The Threshold List to be created
//	return	- Void
void createThresholdList(struct ThresholdList* tl) {
	tl->head = NULL;
	tl->tail = tl->head;
	tl->count = 0;
}

// Standard linked-list queue-style 
// inserts threshold struct at the tail of the list
//	tl	- The Threshold List in which the new threshold will be inserted
//	th	- The new threshold
//	return	- Void
void insertIntoThresholdList(struct ThresholdList* tl, struct Threshold* th) {
	th->next = NULL; // just to be sure.
	if(tl->head == NULL) {
		tl->head = th;
		tl->tail = th;
		tl->count = 1;
	}
	else {
		tl->tail->next = th;
		tl->tail = th;
		tl->count++;
	}
}

// used for debugging purposes and to present 
// output to the user in a friendly way
//	tl	- The Threshold List to be printed
//	return	- Void
void printThresholdList(struct ThresholdList* tl) {
	struct Threshold* tmp = tl->head;
	if(tl->count > 0) {
		while(tmp != NULL) {
			printf("%d incidents in %d minutes\n", tmp->numOfIncidents, tmp->numOfMinutes);
			tmp = tmp->next;
		}
	}
	else {
		printf("ThresholdList is empty: count = %d\n", tl->count);
	}
}

// remove a threshold struct from the list and return a pointer to it. 
// threshold struct will be removed from the head
//	tl	- The Threshold List which will have it's head removed
//	th	- ///////////////////////////
//	return	- Void
void removeFromThresholdList(struct ThresholdList* tl, struct Threshold* th) {
	th = tl->head;
	if(tl->count > 1) {
		tl->head = tl->head->next;
		tl->count--;
		th->next = NULL; // ensure complein sever from the list
	}
	else if(tl->count == 1) {
		tl->head = NULL;
		tl->tail = tl->head;
		tl->count = 0;
		th->next = NULL; // ensure complein sever from the list
	}
	else { // count is 0
		tl->count=0; // just a precaution
	}
}

// remove a threshold struct from the head of the list and destroy it
//	tl	- The Threshold List which will have it's head remove and memory freed
//	return	- Void
void removeAndDestroyThreshold(struct ThresholdList* tl) {
	struct Threshold* th = tl->head;
	if(tl->count > 1) {
		tl->head = tl->head->next;
		tl->count--;
		th->next = NULL; // ensure complete sever from the list
	}
	else if(tl->count == 1) {
		tl->head = NULL;
		tl->tail = tl->head;
		tl->count = 0;
		th->next = NULL; // ensure complete sever from the list
	}
	else { // count is 0
		tl->count=0; // just a precaution
	}
	free(th);
}
//Returns the number of thresholds in the Threshold List
//	tl	- The Threshold List who's count will be returned
//	return	- The number of threshold in the Threshold List
int getCountOfThresholdList(struct ThresholdList* tl) {
	return tl->count;
}

// While the count of the threshold list is greater than zero call the
// 'removeAndDestroyThreshold'. Once the list is empty, free the memory of the
// list structure 
//	tl	- The Threshold List to be destroyed
//	return	- Void
void destroyThresholdList(struct ThresholdList* tl) {
	while(tl->count > 0) {
		removeAndDestroyThreshold(tl);
	}
	free(tl);
}

// This method will return the largest 'number of minutes' value 
// in the threshold list. This is important because records more recent than
// the expiring time must be kept to be able to check threshold conditions
// properly.
//	tl	- The Threshold List to be searched to find the largest num of minutes
//	return	- The largset number of minutes found
int getExpiringTime(struct ThresholdList* tl) {
  if(tl->count > 0) {
    int max=0;
    struct Threshold* th = tl->head;
    while(th != NULL)
    {
      if(th->numOfMinutes > max) 
      {
        max = th->numOfMinutes; 
      }
      th=th->next;
    }
    return max;
  }
  else {
    return DEFAULT_THRESHOLD_TIME;
  }
}
//goes through the threshold list and free all thresholds and then the threshold list
//	thresholdList	- The Theshold List to be deleted
//	return		- Void
void deleteThresholdList(struct ThresholdList* thresholdList)
{
    struct Threshold* thresholdListTraveller = thresholdList->head;
    while(thresholdListTraveller != NULL)
    {
        struct Threshold* threshold = thresholdListTraveller;
        thresholdListTraveller = thresholdListTraveller->next;
        free(threshold);
    }
    free(thresholdList);
    thresholdList = NULL;
}
