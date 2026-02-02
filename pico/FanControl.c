
 
#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>
//#include "HandleSerialCommands.h"

void RegisterCMD(char* cmd, void *function);
void HandleCmd(void);

// CharsReadyCallback is called when there are characters waiting in the serial buffer
// we just set a flag, then handle the characters in the main loop
void CharsReadyCallback(void * flag){
    *(bool *) flag = true;
}

void CmdStop(int argc, char *argv[]){
    printf("CmdStop!!\n");
}

void CmdSpeed(int argc, char *argv[]){
    for(int i = 0; i<argc; i++)
        printf("%d\t%s\n", i, argv[i]);
}

int main() {
 //   char cmd[MAX_CMD_SIZE];  // a place to store the serial over USB command
    volatile bool CharsReadyFlag = false;

    stdio_init_all();
        
    gpio_init(PICO_DEFAULT_LED_PIN);
    gpio_set_dir(PICO_DEFAULT_LED_PIN, GPIO_OUT);

     //register callback
    stdio_set_chars_available_callback( CharsReadyCallback , (void *) &CharsReadyFlag);

    RegisterCMD("Stop", CmdStop);
    RegisterCMD("speed", CmdSpeed);
    
    while (true) {
        
        // CharsReadyCallback sets CharsReadyFlag = true
        if(CharsReadyFlag){            
 
            HandleCmd();
            CharsReadyFlag = false; // reset the flag
            
        } 
    }
}

