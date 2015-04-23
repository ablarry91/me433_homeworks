// Author: Austin Lawrence

#include <xc.h> // processor SFR definitions
#include <sys/attribs.h> // __ISR macro
#include "i2c_display.h"  //library for using the i2c display
#include "accel.h" //library for using the accelerometer

//These are the available DEVCFG bits for the PIC32MX250F128B are listed in the documentation that comes with XC32, in microchip/xc32/v1.33/docs/config_docs/32mx250f128b.html

// DEVCFG0
#pragma config DEBUG = OFF // no debugging
#pragma config JTAGEN = OFF // no jtag
#pragma config ICESEL = ICS_PGx1 // use PGED1 and PGEC1
#pragma config PWP = OFF // no write protect
#pragma config BWP = OFF // not boot write protect
#pragma config CP = OFF // no code protect

// DEVCFG1
#pragma config FNOSC = PRIPLL // use primary oscillator with pll
#pragma config FSOSCEN = OFF // turn off secondary oscillator
#pragma config IESO = OFF // no switching clocks
#pragma config POSCMOD = HS // high speed crystal mode
#pragma config OSCIOFNC = OFF // free up secondary osc pins by turning sosc off
#pragma config FPBDIV = DIV_1 // divide CPU freq by 1 for peripheral bus clock
#pragma config FCKSM = CSDCMD // do not enable clock switch
#pragma config WDTPS = PS1048576 // slowest wdt
#pragma config WINDIS = OFF // no wdt window
#pragma config FWDTEN = OFF // wdt off by default
#pragma config FWDTWINSZ = WINSZ_25 // wdt window at 25%

// DEVCFG2 - get the CPU clock to 40MHz
#pragma config FPLLIDIV = DIV_2 // divide input clock to be in range 4-5MHz
#pragma config FPLLMUL = MUL_20 // multiply clock after FPLLIDIV
#pragma config FPLLODIV = DIV_2 // divide clock after FPLLMUL to get 40MHz
#pragma config UPLLIDIV = DIV_2 // divide 8MHz input clock, then multiply by 12 to get 48MHz for USB
#pragma config UPLLEN = ON // USB clock on

// DEVCFG3
#pragma config USERID = 0 // some 16bit userid, doesn't matter what
#pragma config PMDL1WAY = ON // allow only one reconfiguration
#pragma config IOL1WAY = ON // allow only one reconfiguration
#pragma config FUSBIDIO = ON // USB pins controlled by USB module
#pragma config FVBUSONIO = ON // controlled by USB module


// --- Constants --- //

#define CLK_FREQ_HZ 40000000
#define TMR2_PERIOD ((CLK_FREQ_HZ) / 1000)  // 1 kHz

// function prototypes
int readADC(void);

int main() {
    int val;

    short accels[3]; // accelerations for the 3 axes
    short mags[3]; // magnetometer readings for the 3 axes
    short temp;

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

    // Set up the accelerometer through SPI
    acc_setup();

    while(1)
    {
        // --- Invert LED1 every 0.5 s. --- //

        if( (_CP0_GET_COUNT() >= (CLK_FREQ_HZ / 4)) || !(PORTB & 0x2000) )
        {
            LATBINV = 0x80;
            _CP0_SET_COUNT(0);

            // Try making a reading from the accelerometer
            // read the accelerometer from all three axes
            // the accelerometer and the pic32 are both little endian by default (the lowest address has the LSB)
            // the accelerations are 16-bit twos compliment numbers, the same as a short
            acc_read_register(OUT_X_L_A, (unsigned char *) accels, 6);
            // need to read all 6 bytes in one transaction to get an update.
            acc_read_register(OUT_X_L_M, (unsigned char *) mags, 6);
            // read the temperature data. Its a right justified 12 bit two's compliment number
            acc_read_register(TEMP_OUT_L, (unsigned char *) &temp, 2);

            display_clear();
            sprintf(str,"accel = %d",accels[0]);
            write_string(str,28,24);
        }

        // Set PWM duty cycle % to the pot voltage output %
        OC1RS = readADC() * (TMR2_PERIOD - 1) / 1024;
    }
}

int readADC(void) {
    int elapsed = 0;
    int finishtime = 0;
    int sampletime = 20;
    int a = 0;

    AD1CON1bits.SAMP = 1;
    elapsed = _CP0_GET_COUNT();
    finishtime = elapsed + sampletime;
    while (_CP0_GET_COUNT() < finishtime) {
    }
    AD1CON1bits.SAMP = 0;
    while (!AD1CON1bits.DONE) {
    }
    a = ADC1BUF0;
    return a;
}
