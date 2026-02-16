#include <stdio.h>
#include "pico/stdlib.h"
#include <string.h>
#include <stdlib.h>
#include "hardware/pwm.h"

//[ADDED CODE]//
#define pwmPin 15               // GPIO 15 or pico 20
volatile int blink_percent = 0; // stores LED Blink Percentage input
#define rpmPin 11               // GPIO 11 or pin 15: use - INPUT TACHReading
volatile int rpmRead   = 0;     // reads the rpm value on TACHometer
volatile int pulse = 0;         // sets Fan's pulse count at zero

uint pwmSlice;                  // pwm slice that controls fan speed
uint16_t pwmWrap;
#define FAN_PWM_FREQ 25000      // desired pwm frequency to control the fan speed
int pwmDC = 0;                  // the current duty cycle
uint rpmSlice;                  // pwm slice that counts fan rotations
#define ROT_CNT_PERIOD  500     // how often to check the blade rotation count [ms]
int rotCnt = 0;                 //used to measure fan speed
int oldrotCnt = 0;

////////////////

void RegisterCMD(char* cmd, void *function);
void HandleCmd(void);

// CharsReadyCallback is called when there are characters waiting in the serial buffer
// we just set a flag, then handle the characters in the main loop
void CharsReadyCallback(void *flag){
    *(bool *)flag = true;
}

// SetFanPWMDutyCycle sets the duty cycle of the fan pwm between 0% and 100%
// note that because the open drain transistor inverts the pwm signal
// if we set the DC to 25% the fan sees 75%
void SetFanPWMDutyCycle(int DC){
    if(DC >= 0 && DC <=100){
        int level = pwmWrap - DC * pwmWrap / 100;
        pwm_set_chan_level(pwmSlice, PWM_CHAN_B, level);
    }
}


// repeating_timer_callback gets called periodically to check how many times the
// fan blade has rotated
bool repeating_timer_callback(__unused struct repeating_timer *t) {
    oldrotCnt = rotCnt;                     // save the previous count
    rotCnt = pwm_get_counter(rpmSlice);     // get the cuurent count
    int rotations = rotCnt - oldrotCnt;     // the number of rotations since the previous call
    if(rotations < 0)  // did the 16 bit counter roll over?
        rotations += 0x1000;
    rpmRead = (rotations / 2.0) / (ROT_CNT_PERIOD / 1000.0) * 60;    // 2 ticks per rev, convert ms to s, convert s to min
    return true;
}


/////////////////////////[ Serial Monitor Void Command Set Up ]////////////////////////////

// void_CmdSpeed:    speed
void CmdSpeed(int argc, char *argv[]){
    for(int i = 0; i<argc; i++)
        printf("%d\t%s\n", i, argv[i]);
}

// void_CmdCycle:    cycle
void CmdCycle(int argc, char *argv[]){

    if(argc == 2){
        int DC = atoi(argv[1]);
        if(DC >= 0 && DC <= 100){
            pwmDC= DC;
            SetFanPWMDutyCycle(pwmDC);
         }
    }
 
    // echo the current duty cycle whether we changed it or not
    printf("GPIO 15 duty cycle set to %d\n", pwmDC);
}

// void_CmdTACHread: TACHread
void CmdTACHread(int argc, char *argv[]){
     printf("Fan RPM: %d\n", rpmRead);
}

//////////////////////////////////////[ int main() ]///////////////////////////////////////
int main() {
//   char cmd[MAX_CMD_SIZE];    // a place to store the serial over USB command
    volatile bool CharsReadyFlag = false;
    stdio_init_all();
    
    
//---- pwm slice setup for fan control  ----//
    gpio_set_function(pwmPin, GPIO_FUNC_PWM);   // assuming this pin is channel B?
    pwmSlice = pwm_gpio_to_slice_num(pwmPin);
    // calculate the wrap value that we need
    pwmWrap = SYS_CLK_HZ / FAN_PWM_FREQ - 1;
    pwm_set_wrap(pwmSlice, pwmWrap);
    pwm_set_chan_level(pwmSlice, PWM_CHAN_B, pwmWrap);    // 100% duty cycle means the fan sees 0%
    pwm_set_enabled(pwmSlice, true);                // start the pwm slice

//---- pwm slice setup for fan speed measurement  ----//
    gpio_set_function(rpmPin, GPIO_FUNC_PWM);
    rpmSlice = pwm_gpio_to_slice_num(rpmPin);
    pwm_config c = pwm_get_default_config();        // a struct to hold the slice configuration
    pwm_config_set_clkdiv_mode(&c, PWM_DIV_B_RISING); // instead of using a clock we want to count up on the rising edge of ch B
    pwm_init(rpmSlice, &c, true);                   // start the rpm slice running
    
    // request a periodic timer to check fan blade rotations
    struct repeating_timer timer;
    add_repeating_timer_ms(ROT_CNT_PERIOD, repeating_timer_callback, NULL, &timer);

    // register callback to collect characters in the serial buffer
    stdio_set_chars_available_callback(CharsReadyCallback, (void *)&CharsReadyFlag);

//----[SERIAL MONITOR COMMANDS]-----//
    RegisterCMD("speed", CmdSpeed); // "speed"
    RegisterCMD("cycle", CmdCycle); // "cycle"
    RegisterCMD("rpm",CmdTACHread); //  "rpm"


    //----------[WHILE LOOP]------------//
    while(true){
         // CharsReadyCallback sets CharsReadyFlag = true
        if(CharsReadyFlag){
            HandleCmd();
            CharsReadyFlag = false; // reset the flag
        }
    }
}
