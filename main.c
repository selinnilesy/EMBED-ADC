/*
 * File:   RecitationFour.c
 * Author: Selin
 *
 * Created on May 24, 2021, 6:34 PM
 */
#include "pragmas.h"

#include "ADC.h"
#include "LCD.h"
#include <string.h>
#include <stdio.h>

volatile char timer1_interrupt_counter=0;
volatile char attempts=3;
volatile char temp_attempt;
volatile char word_counter=0; // word (password piece) index
volatile char valid=1;
volatile char time=90;
volatile char state=0; // state0=power-on, 1=password set, 2=password check, 3=success, 4=fail.
volatile char values[10] = {0};
volatile unsigned short convertion = 0;
volatile float celcius= 0.0f;
volatile unsigned short set1 = 0;
volatile unsigned short set2 = 0;
volatile unsigned short set3 = 0;
volatile unsigned short set3_real_val= 0;
volatile unsigned short numeric_value =0; // isnt float in password case.
volatile char wait_heater_conversion =0; // wait until conversion finishes
volatile char heater_conversion_set =0; // to execute the necessary code as soon as value converted
volatile char conv_set_st1=0;   // state 1 potentiometer conversion flag
volatile char conv_set_st2=0;   // state 2 potentiometer conversion flag
volatile char wait_n_conversion =0;  // state 2 potentiometer result flag, as soon as generated, execute necessary code
char wait_display =0;      // if 7-segment-display has started, wait its ending in the main loop, do not execute anything else.
char display_counter =0;
char display_finished =0;

void __interrupt(high_priority) FNC()
{
    if(PIR1bits.ADIF)   // conversion
    {
        ADCON0bits.GODONE = 0;
        PIR1bits.ADIF = 0;
        ADCON0bits.ADON = 0;
        convertion = (unsigned short)((ADRESH << 8)+ADRESL);
        if (wait_heater_conversion){
            heater_conversion_set=1;
            return;
        }
        else{
            if(state==1) {
                conv_set_st1=1;
            }
            else if(state==2){
                conv_set_st2=1;
                if(word_counter==0 && (T0CONbits.TMR0ON==0) && (convertion != set3_real_val)){ 
                        T0CONbits.TMR0ON=1; time--; // detect input change to set 90-sec timer.
                }
            }
        }
        
    }
    else if(PIR1bits.TMR1IF){   // leds on attempt failure
        timer1_interrupt_counter++;
        if(timer1_interrupt_counter==1){
             if(attempts < 3) PORTBbits.RB7 =  PORTBbits.RB6 = PORTBbits.RB5 = PORTBbits.RB4 = PORTBbits.RB3 = PORTBbits.RB2 = PORTBbits.RB1 = 1;
        }
        else if(timer1_interrupt_counter==10){
            if(attempts < 3) PORTBbits.RB7 =  PORTBbits.RB6 = PORTBbits.RB5 = PORTBbits.RB4 = PORTBbits.RB3 = PORTBbits.RB2 = PORTBbits.RB1 = 0; 
             timer1_interrupt_counter = 0;
        }
        TMR1H = 0x00;
        TMR1L = 0x24;
        PIR1bits.TMR1IF = 0;
    }
    else if(INTCONbits.TMR0IF)      // for 90-sec timer, per second.
    {  
        time--;
        TMR0H = 0x67;
        TMR0L = 0x6A;
        if(!time) {
            state = 4;
            T0CONbits.TMR0ON = 0; // finished with the timer. because user failed.
        }
        // TO DO : SEVEN-SEGMENT DISPLAY. UPDATE ITS DISPLAY NOW.
        INTCONbits.TMR0IF = 0;
    }
    else if(INTCONbits.INT0F){  // RB0
        INTCONbits.INT0F = 0;
        if(state==1){
            if(!word_counter && valid) set1 = numeric_value;
            else if(word_counter==1 && valid) set2 = numeric_value;
            else if(word_counter==2 && valid) {set3 = numeric_value; set3_real_val=convertion;} // save this to check against potentiometer change.
            if(valid) word_counter++;
            if(word_counter==3) state=2;
        }
        else if(state==2){
            if(!word_counter) {
                if(T0CONbits.TMR0ON==0){ T0CONbits.TMR0ON=1; time--;}
                if(set1==numeric_value)  word_counter++;
                else if(numeric_value < 100) attempts--;
            }
            else if(word_counter==1) {
                if(set2==numeric_value)  word_counter++;
                else if(numeric_value < 100) attempts--;
            }
            else if(word_counter==2) {
                if(set3==numeric_value) { state=3; word_counter++;}
                else if(numeric_value < 100) attempts--;
            }
            if(!attempts) {
                state=4; // fail.
                T0CONbits.TMR0ON = 0; // finished with the timer. because user ran out of attempts.
            }
        }
    }
    else if(INTCONbits.RBIF){ // RB4 interrupt.
        if(PORTBbits.RB4==0){ // 0 pin means pressed. do not answer to other button presses.
            TMR1H = 0x00;
            TMR1L = 0x24;
            TMR0H = 0x67;
            TMR0L = 0x6A;
            TRISBbits.RB4=0;
            PORTBbits.RB4=0;
            attempts=3;
            state=2;
            time=90;
            valid=1;
            word_counter=0;
            timer1_interrupt_counter=0;
            INTCONbits.RBIF=0;
            INTCONbits.RBIE=0;
        }
    }
}

char seven_segment( char num){
    switch(num){
        case 0 : return 0x3F;
        case 1 : return 0x06;
        case 2 : return 0x5B;
        case 3 : return 0x4F;
        case 4 : return 0x66;
        case 5 : return 0x6D;
        case 6 : return 0x7D;
        case 7 : return 0x07;
        case 8 : return 0x7F;
        case 9 : return 0x6F;
        
    }
}
void display_seven_segment(char num1, char num2){
    if(display_counter==0){
        switch(attempts){
            case 1 : {temp_attempt= 0x08; break;}
            case 2 : {temp_attempt= 0x48; break;}
            case 3 : {temp_attempt= 0x49; break;}
        }
        PORTAbits.RA4=PORTAbits.RA5 = 0;
        PORTD=temp_attempt;
        PORTAbits.RA3=1; 
        if(PORTCbits.RC5==1){  if(TMR1H==0x4C){ display_counter++;}}
        else{
            if(TMR1H==0x4C){ display_counter++;} // wait approx. 15 msec for display. then display next segment by counter++
        }
    }
    else if(display_counter==1){
        PORTAbits.RA3=PORTAbits.RA5 = 0;
        PORTD=seven_segment(num1);
        PORTAbits.RA4=1;
        if(PORTCbits.RC5==1){
            if(TMR1H==0xDC){ display_counter++;}
        }
        else{
            if(TMR1H==0x99){ display_counter++;} // wait approx. 15 msec more 
        }
    }
    else if(display_counter==2){
        PORTAbits.RA3=PORTAbits.RA4 = 0;
        PORTD=seven_segment(num2); 
        PORTAbits.RA5=1;
        if(PORTCbits.RC5==1) { if(TMR1H==0x90){display_counter = 0; wait_display=0; display_finished=1;}}
        else{
            if(TMR1H==0xE6){display_counter = 0; wait_display=0; display_finished=1;} // wait approx. 15 msec more
        }
    }
   
}
void power_on(){
    PLLEN =0 ;
    OSCCONbits.IRCF2 = OSCCONbits.IRCF1 = OSCCONbits.IRCF0 = 1;
    InitLCD();    
    initADC();
   
    TRISB = 0x01; // all leds are output except RB0 interrupt pin.
    PORTBbits.RB7 =  PORTBbits.RB6 = PORTBbits.RB5 = PORTBbits.RB4 = PORTBbits.RB3 = PORTBbits.RB2 = PORTBbits.RB1 = 0;
    TRISA = 0x00; // output for 7-segment display.
    //TRISD = 0x00; // output for 7-segment display. SET BY LCD FUNCTION.
    TRISC = 0x00; // heater on/off bit
    TRISAbits.RA0 = 1; // potentiometer is input
    TRISAbits.RA2 = 1; // thermometer is input
    
    INTCONbits.INT0IE = 1; //Enable INT0 pin interrupt for RB0 button press.
    INTCONbits.INT0IF = 0;  // block false positives !!! 
    INTCONbits.GIE = 1;
    INTCONbits.PEIE = 1; // !!!!! made me waste my 3 hours .
    
    T1CON = 0xB4;
    TMR1H = 0x00;
    TMR1L = 0x24; // 655000 cycles needed. 65536-36 = 65500 cycles per 1 interrupt. Count 10 interrupts in total !
    PIE1bits.TMR1IE =1;   // TIMER3 for 500ms toggle of leds. Interrupt Enable
    PIR1bits.TMR1IF =0; 
    
    PIE1bits.ADIE =1;   // ADC Interrupt Enable
    PIR1bits.ADIF=0; // clear ADC Interrupt to block false positives.
 
    // INTCONbits.PEIE = 1; // RB4 change interrupt. TO DO : USE THIS IN SUCCESS STATE !!!
    INTCONbits.TMR0IE = 1;
    INTCONbits.TMR0IF = 0;
    T0CON = 0x07; // 16 bits mode internal clock. Count for 1 sec. 
    TMR0H = 0x67;
    TMR0L = 0x6A;
    
    LCDGoto(1 , 1 ),
    LCDStr("SuperSecureSafe!");
    __delay_ms(3000);
    state+=1;
}
void password_set_display(){
    LCDGoto(1 , 1 );
    LCDStr("Set Password:   ");
    LCDGoto(1 , 2 );
    LCDStr("__-__-__       "); 
}
void input_password_display(){
    LCDGoto(1 , 1 );
    LCDStr("Input Password: ");
    LCDGoto(1 , 2 );
    LCDStr("__-__-__       "); 
}
void fail_display(){
    LCDGoto(1 , 1 );
    LCDStr("You Failed!    ");
    LCDGoto(1 , 2 );
    LCDStr("               "); 
}
void success_display(){
    LCDGoto(1 , 1 );
    LCDStr("Unlocked; Press ");
    LCDGoto(1 , 2 );
    LCDStr("RB4 to lock!    "); 
}
void print_digits(unsigned short numeric_value){
    if(numeric_value > 99){
        LCDStr("XX");
        valid = 0;
    }
    else if(numeric_value < 10){
        sprintf(values, "%d%d", 0, numeric_value);
        LCDStr(values);
         valid = 1;
    }   
    else{
        sprintf(values, "%d", numeric_value);
        LCDStr(values);
         valid = 1;
    }
}
/* All the functions and calculation choices have their inline comments. To explain in short also here,
 * 
 * In this program timer1 and timer0 are used for time measurement purposes. 
 * timer0 interrupts every 1 sec to count 90 in total.
 * timer0 is set to be 16-bits counter with 256 prescale. that means 39062 cycles in a sec under 10mhz ins. freq.
 * therefore timer0 is initialized with 0x676a to count 39062 cycles upto 65565, max number.
 * timer1 is set to count 500 msec for blinking purposes. but it is also used for measuring the time
 * between waiting durations of 7-segment display.
 * ----------
 * 7-Segment :
 * Display requires 15 ms equal waits between each displayed digit to make it crystal clear, but
 * if there is a heater conversion,  more branching in main loop happens so more time is wasted compared to normal potent. conversion.
 * To compansate that extra wasted time in the main loop, and block the flickring caused by it,
 * i have decided to use 2 times the waiting time of to the normal 7-segment display waiting (15 ms) if heater turned-on.
 * More importanly, there was a "background" flickring in last displayed digit of time, since it is most frequently changing one,
 * to block it I saved the PORTD and closed RA5 for an extremely short time in LCD.h, to eliminate background shadows.
 * Display times are all equal for displaying of each segment and looks up a specific TMRH threshold to determine time.
 * This is done by polling (without busy waiting). That means, in the main loop, if 7-segment is active,
 * no other code is executed until display finishes. 
 * This is because all segments need to be display sequentially within time for a proper human-eye catch.
 * Main loop iterates several times if 7-segment is active, to complete it.
 * --------
 * ADC Module :
 * As I've adjusted interrupt mechanism for ADCON, both potent. and heater conversions 
 * took equal amount of time resulting seemingly 1 in-total conversion in a few consequent iterations of the main loop.
 * ADC result is occured by interrupt. 
 * TAD = 0.8 (microsec), as the minimum is 0.7. it must be as close as possible to the threshold for performance constraints.
 * TAD : 1/40 us * 32 = 8/10 = 0.8 us >= 0.7 us. OK. since programmed no need to employ a timer.
 * acquisiton time : 0.8 us * 4 = 3.2 us >= 2.4 us. OK. since programmed no need to employ a timer.
 * In ADC.h there there are further comments relating to details of ADC module, analog pins, right justified, on/idle, etc
 * ---------
 *  Blinking + 7-Segment Waiting :
 * since timer1 needs to count 500 msec (when attempts < 3  blinking occur in ISR) ,
 * 10 interrupts must occur as we cannot count more cycles using 8 prescale (max available). 
 * 1 interrupt of timer1 = 50 msec, so less than half of it (TIMER1=0x4C36)
 * is used for counting 15 msec for 7-segment display waiting time purposes.
 * 7-segment display waiting are in short conducted by timers.
 * ---------
 * No function calls in ISRs. Variables of ISR are declared as volatile to block fault-prone compiler optimization.
 * ---------
 
 */
void main(void) {
    power_on();
    while(1){
        if(state==1){
            password_set_display();
            while(word_counter!=3)   {   
                if(ADCON0bits.GODONE == 0 && !conv_set_st1){
                    readADCChannel(0); //Read potentiometer output
                } 
                if(conv_set_st1){
                    if(word_counter==0){
                        numeric_value = convertion / 10; 
                        LCDGoto(1, 2);  
                        print_digits(numeric_value);
                    }
                    else if(word_counter==1){
                        numeric_value = (1023-convertion) / 10; // negative increase 
                        LCDGoto(4, 2);  
                        print_digits(numeric_value);
                    }
                    else if(word_counter==2){
                        numeric_value = convertion / 10; 
                        LCDGoto(7, 2);  
                        print_digits(numeric_value);
                    }
                    conv_set_st1=0;
                }
            }
            word_counter=0; // reset the word index for the next state's use.   
        }
        else if(state==2){
            input_password_display();
            while(word_counter!=3)   {
                if( T1CONbits.TMR1ON==0) T1CONbits.TMR1ON = 1; // timer for 7-segment-displays and blinker of portb upon attempt faiulre.
                if(wait_display==0){ // start display initially.
                    wait_display = 1;
                    display_counter =0;
                    display_finished=0;
                    display_seven_segment( time/10, time%10);
                }
                else if(wait_display) display_seven_segment( time/10, time%10); // let it finish. here it finishes. below code only works if finished. only then next display can begin.
                if(!wait_display && display_finished){ // the whole body: conversion, state changes, etc... to be executed when display finishes its job.
                    if(!attempts || !time){
                        state = 4; break;
                    }
                    if(attempts==1){ // attempts==2 is handles in portb led interrupt.
                       if( PORTCbits.RC5==0){
                           PORTCbits.RC5 =1; // turn heater on.
                       } 
                       if(PORTCbits.RC5==1){ // when heater is on,
                           if(!wait_n_conversion && ADCON0bits.GODONE == 0 && !heater_conversion_set){ // if no prev result from heater, new conversion
                                wait_heater_conversion=1;
                                readADCChannel(2); //Read thermometer.
                           }
                           else if(!wait_n_conversion &&wait_heater_conversion && heater_conversion_set){
                                celcius = ((convertion*5.0f)/1023.0f)*100.0f;
                                if(celcius > 40.0f){
                                    state = 4; 
                                    break; // immediately move to state FAIL.
                                }
                                heater_conversion_set=0;
                                wait_heater_conversion=0;
                           }
                        }
                    }
                    if(ADCON0bits.GODONE == 0 && !heater_conversion_set && !conv_set_st2){
                        wait_n_conversion = 1;
                        readADCChannel(0); //Read potentiometer output
                    } 

                    if(wait_n_conversion && conv_set_st2){
                        if(word_counter==0){
                            numeric_value = convertion / 10; 
                            LCDGoto(1, 2);  
                            print_digits(numeric_value);
                             }
                        else if(word_counter==1){
                            numeric_value = (1023-convertion) / 10; // negative increase 
                            LCDGoto(4, 2);  
                            print_digits(numeric_value);
                        }
                        else if(word_counter==2){
                            numeric_value = convertion / 10; 
                            LCDGoto(7, 2);  
                            print_digits(numeric_value);
                        }
                        conv_set_st2=0;
                        wait_n_conversion = 0;
                    }
                }
                               
            }
        }
        else if(state==3){
            if( PORTCbits.RC5==1) PORTCbits.RC5 =0; // turn off the heater
            if( T0CONbits.TMR0ON==1) T0CONbits.TMR0ON = 0;
            if( T1CONbits.TMR1ON==1) T1CONbits.TMR1ON = 0;
            PORTD = 0x00;
            PORTAbits.RA3 = PORTAbits.RA4 = PORTAbits.RA5 = 0;
            PORTBbits.RB7 =  PORTBbits.RB6 = PORTBbits.RB5 = PORTBbits.RB4 = PORTBbits.RB3 = PORTBbits.RB2 = PORTBbits.RB1 = 0;
            success_display();
            TRISBbits.RB4=1;
            if(INTCONbits.RBIE==0) INTCONbits.RBIE=1;
        }
        else if(state==4){
            if( PORTCbits.RC5==1) PORTCbits.RC5 =0; // turn off the heater
            if( T0CONbits.TMR0ON==1) T0CONbits.TMR0ON = 0;
            if( T1CONbits.TMR1ON==1) T1CONbits.TMR1ON = 0;
            if( INTCONbits.TMR0IE==1) INTCONbits.TMR0IE = 0;
            if( PIE1bits.TMR1IE ==1) PIE1bits.TMR1IE = 0;
            PORTD = 0x00;
            TRISAbits.RA2=0;
            PORTAbits.RA2 = PORTAbits.RA3 = PORTAbits.RA4 = PORTAbits.RA5 = 0;
            PORTBbits.RB7 =  PORTBbits.RB6 = PORTBbits.RB5 = PORTBbits.RB4 = PORTBbits.RB3 = PORTBbits.RB2 = PORTBbits.RB1 = 1;
            fail_display();
        }
    }
    
    
    
  
    
    
    return;
}
