#include <string.h>
#include <stdio.h>
#include "pico/stdlib.h"

#define MAX_CMD_SIZE 50
#define MAX_ARGC 10
#define SEPARATORS " "
#define MAX_NUM_CMDS 32


int NumCmds = 0;  // this will get incremented as cmds are registered

struct CMDList_t{
    void (* function)(int argc, char *argv[]);
    char *CmdStr;
};

struct CMDList_t CMDList[MAX_NUM_CMDS]; //a list of commands with pointers to thier handler functions

//RegisterCMD registers a text command along with the function to be called
void RegisterCMD(char* cmd, void *function){
    NumCmds++;
    if(NumCmds < MAX_NUM_CMDS){
        CMDList[NumCmds].function = function;
        CMDList[NumCmds].CmdStr = cmd;
    }
}

void RegisterDefaultCMD(void *function){
    NumCmds++;
}

void HandleCmd(void){
    char cmd[MAX_CMD_SIZE];  // a place to store the serial over USB command
 
    // collect the characters into cmd string
    int i = 0;
    int c;
    while ((c = getchar_timeout_us(0)) > 0){
        cmd[i] = (char) c;
        i++;
    }
    cmd[i] = '\0'; // terminate the string
    
    // separate the string into separate tokens
    // and place them into the argv[] array
    char *rest = cmd; // a temp variable used by strtok_r()
    int argc = 0;       //the number of arguments including command itself
    char *argv[MAX_ARGC]; //an array of char arrays to store the arguments
    
    argv[argc] = strtok_r(rest, SEPARATORS, &rest); // get the first token (the cammand itself)
    while(argv[argc] != NULL){  // now get the rest of the arguments
        argc++;
        argv[argc] = strtok_r(rest, SEPARATORS, &rest);
    }

    // find the command (argv[0])in the command list and call the appropriate function
    for(i = 0; i < MAX_NUM_CMDS; i++){
        if(strcmp(argv[0], CMDList[i].CmdStr)==0){
            CMDList[i].function(argc, argv);
        }
    }
}
