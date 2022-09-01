#include "StringAndFileMethods.h"
#include "Incidents.h"

/*------------------------------------------------------
**
** File: StringAndFileMethods.c
** Author: Matt Weston & Richard Tam
** Created: August 31, 2015
**
** Copyright �2015 Toronto Transit Commission
** 
** Revision History
**
** 18 Feb 2016: Rev 2.0 - MWeston
**                      - Changed #include files                                                                                                                   
**                      - modified getCharsUpTo method
**                      - modified readInLine method
 * 25 11 2016: Rev 2.2      - MTedesco
 *                          - Performance improvements made to removeFirstChars
*/

// remove the first num chars from a string
// useful for trimming 8 leading characters from CSS logfile entries.
//	s	- The string to remove the first num characters from
//	num	- The number of characters to remove
//	return	- Void
void removeFirstChars(char *s, int num) {
    int len=strlen(s);
    if(num > len || num == 0 || s == NULL) {
        return;
    }
    memmove(s, s+num, len - num + 1);
}

// find the position of a substring within another using the power of pointers
// and indexes
//	str	- The string in which the substring will be searched for
//	substr	- The substring to be searched for
//	return	- an int representing the offset of the at which the substring starts
int getPositionOfSubstring(char* str, char* substr) {
	char* ptr  = strstr( str, substr );
	return ptr - str;
}

// copy chars from one string into another until a certain word within the
// main string
//	str		- The string that will be copied up to a point
//	output		- The string in which str will be copied
//	afterWord	- The substring at which the coping will stop
//	return		- Void
void getCharsUpTo(char* str, char* output, char* afterWord) {
  int endIndex = getPositionOfSubstring(str, afterWord);
  
  if( endIndex < 0 ) {
    strcpy(output, str);
  }
  else {
    strncpy(output, str, endIndex);
    output[endIndex] = '\0';
  }
}

// Do to Linux vs Windows differences in filepaths a method that could easily
// change the direction of slahes was neccesary. Additionally, this ensures
// consistency is file and folder names
// This function concatenates a folder name and a filename with the "local"
// directory "./"
//	filePath	- The string in which the file path will be written into
//	folderName	- The name of the folder in which the file is located
//	fileName	- The name of the file
//	extention	- The extention of the file
//	retuen		- Void
void constructLocalFilepath(char* filePath, char* folderName, char* fileName, 
  char* extention) {
  //clear and copy "./"
  int i = 0;
  while(*(filePath + i))
  {
	*(filePath + i) = '\0';
	i++;
  }
  filePath[0] = '.';
  filePath[1] = '/';
  //add a folderName if there is one
  if(folderName != NULL) {
    strcat(filePath, folderName);
    strcat(filePath, "/");
  }
  //add the file name and extension
  strcat(filePath, fileName);
  strcat(filePath, extention);
}

// Reads in a single line, up to a '/r', '/n' or 'EOF' from a file and 
// copies it into 'line'
// returns a integer indicating what cause the method to finish and will
// dynamically resize the char array if string is too long
// filters out comments
//	fl	- The file to be read from
//	line	- A pointer to the string in which the read line will be saved
//	size	- The amount of allocated space in line.
//	return	- An int indiciating the status of the read line
int readInLine(FILE* fl, char** line, size_t size) {
  strcpy(*line, "");
  int counter = 0;   
  int character = getc(fl);

  // while the character is not a return character, not and end of line
  // character and not an end_of_file character, keep reading in chars one-by-
  // one and adding them to the retuned line
  while(character!='\r' && character!='\n' && character!=EOF) {
    // single slashes may not be a comment, next chat must be checked
    // as well.
    if(character == '/') {
      character = getc(fl);
      // if 2 slashes occur in a row, the rest of line is assumed to be a
      // comment
      if( character == '/') {
        // read in the rest of the line, the comment portion, to prepare
        // cursor location for next line.
        while(character!='\r' && character!='\n' && character!=EOF) { 
          character = getc(fl); // do nothing wuth it
        }
        // (*line)[counter] is undefined or empty at this point
        int tmpCounter = counter-1;
        
        // there could be spaces between the comment and the end of the line
        // that are only for aesthetics. Remove them.
        while(tmpCounter > 0 && (*line)[tmpCounter]==' ') {
          (*line)[tmpCounter] = '\0';
          tmpCounter--;
        }
      }
      // If the character was not a slash then insert the first slash that was 
      // already read in and continue
      else {
        (*line)[counter] = '/'; // first slahes
        counter++; // increment the counter
				
        // resize if array bounds have been exceded
        if(counter == size-1) {
          *line = (char*)realloc(*line, size*2);
          size*=2;
        }

        // if the second character read in wasn't a /r, /n or EOF than copy
        // that character into the char array
        if( !(character=='\r' || character=='\n' || character==EOF) ) {
          (*line)[counter] = (char)character;
          counter++;
          // resize if neccessary
          if(counter == size-1) {
            *line = (char*)realloc(*line, size*2);
            size*=2;
          }
          // read in character for next time while loop runs
          character = getc(fl);
        }
      }
    }
    // a slash was not read in so no checking for comments in neccesary,
    // copy character and realloc array length if necessary
    else {
      (*line)[counter] = (char)character;
      counter++;
      if(counter == size-1) {
        *line = (char*)realloc(*line, size*2);
        size*=2;
      }
      // read in character for next time while loop runs
      character = getc(fl);
    }
  }

  if(character == EOF) {
    if(counter > 0) {
      printf("Strange EOF in readInLine, Counter = %d\n", counter);
      (*line)[counter] = '\0';
      return STRANGE_END_OF_FILE; 
    }
    else {
      return END_OF_FILE;
    }
  } else if(character == '\r') {
    // get rid of new line char, 
    // increases compatability between Linux and Windows
    character=getc(fl);
  }

  if(counter > 0) {
    // ensure line is treated as string
    (*line)[counter] = '\0';
    return READ_IN_STRING;
  }
  else {
    return NO_STRING_READ;
  }
}

