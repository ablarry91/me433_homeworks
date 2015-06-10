
#include <stddef.h>                     // Defines NULL
#include <stdbool.h>                    // Defines true
#include <stdlib.h>                     // Defines EXIT_FAILURE
#include "system/common/sys_module.h"   // SYS function prototypes
#include <xc.h> // processor SFR definitions
#include <sys/attribs.h> // __ISR macro
#include "i2c_display.h"  //library for using the i2c display
#include "accel.h" //library for using the accelerometer

// *****************************************************************************
// *****************************************************************************
// Section: Main Entry Point
// *****************************************************************************
// *****************************************************************************

// --- Constants --- //

#define CLK_FREQ_HZ 40000000
#define TMR2_PERIOD ((CLK_FREQ_HZ) / 1000)  // 1 kHz




int main ( void )
{
    // startup
    __builtin_disable_interrupts();
    // set the CP0 CONFIG register to indicate that
    // kseg0 is cacheable (0x3) or uncacheable (0x2)
    // see Chapter 2 "CPU for Devices with M4K Core"
    // of the PIC32 reference manual

    __builtin_mtc0(_CP0_CONFIG, _CP0_CONFIG_SELECT, 0xa4210583);
    // no cache on this chip!
    // 0 data RAM access wait states
    BMXCONbits.BMXWSDRM = 0x0;
    // enable multi vector interrupts
    INTCONbits.MVEC = 0x1;
    // disable JTAG to be able to use TDI, TDO, TCK, TMS as digital
    DDPCONbits.JTAGEN = 0;

    // set up USER pin as input on pin B13
    ANSELBbits.ANSB13 = 0;  // disable analog
    TRISBbits.TRISB13 = 1;  // select B13 as input

    // set up LED1 pin as a digital output on pin B7
    RPB7Rbits.RPB7R = 0b0001;
    TRISBbits.TRISB7 = 0;  // select pin B7 output
    LATBbits.LATB7 = 1;    // turn on LED1

    // set up LED2 as OC1 using Timer2 at 1kHz
    RPB15Rbits.RPB15R = 0b0101; // RB15 is OC1
    T2CONbits.TCKPS = 0;   // Timer2 prescaler N=1  PLL 40M Hz = 25 ns, OC1 1kHz = 1ms
    // ( PR2 + 1 ) * prescaler = 1ms / 25ns = 40000   PR2 + 1 = 40000 / 1 = 40000
    PR2 = 39999;
    TMR2 = 0; // initial TMR2 count is 0
    OC1CONbits.OCM = 0b110; // PWM mode without fault pin; other OC1CON bits are defaults
    OC1RS = 20000; // duty cycle = OC1RS/(PR2+1) = 50%
    OC1R = 20000; // initialize before turning OC1 on; afterward it is read-only
    T2CONbits.ON = 1; // turn on Timer2
    OC1CONbits.ON = 1; // turn on OC1

    // set up A0 as AN0
    ANSELAbits.ANSA0 = 1;
    AD1CON3bits.ADCS = 3;
    AD1CHSbits.CH0SA = 0;
    AD1CON1bits.ADON = 1;
    

    __builtin_enable_interrupts();

    // Initialize for our blinking LED.
    _CP0_SET_COUNT(0);
    LATBCLR = 0x80;

    // Start the PWM timer.
    T2CONSET = 0x8000;
    OC1CONSET = 0x8000;

    // Set up the i2c display
    display_init();
    display_clear();
    char str[30];
    sprintf(str, "Hello world 1337!");
    write_string(str, 28, 32);
    
    
    /* Initialize all MPLAB Harmony modules, including application(s). */
    SYS_Initialize ( NULL );
    
//    acc_setup();
    
    while (1)
    {
        /* Maintain state machines of all polled MPLAB Harmony modules. */
        SYS_Tasks ( );
        
            
        

//        short accels[3]; // accelerations for the 3 axes
//
//        acc_read_register(OUT_X_L_A, (unsigned char *) accels, 6);
//        // need to read all 6 bytes in one transaction to get an update.
//
//        display_clear();
//        sprintf(str,"aX = %1.3f",(float)accels[0]/32768*2);
//        write_string(str,0,0);
//        // sprintf(str,"aZ = %1.3f",(float)accels[2]/32768*2);
//        // write_string(str,0,16);
//    
        
        
        
        
        
    }

    /* Execution should not come here during normal operation */

    return ( EXIT_FAILURE );
}


/*******************************************************************************
 End of File
*/


