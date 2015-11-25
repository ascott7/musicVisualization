#ifndef PIHELPERS_H_INCLUDED
#define PIHELPERS_H_INCLUDED 1
#include <sys/mman.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/////////////////////////////////////////////////////////////////////
// Constants
/////////////////////////////////////////////////////////////////////

#define LEDPIN 21

// GPIO FSEL Types
#define INPUT  0
#define OUTPUT 1
#define ALT0   4
#define ALT1   5
#define ALT2   6
#define ALT3   7
#define ALT4   3
#define ALT5   2

#define GPFSEL   ((volatile unsigned int *) (gpio + 0))
#define GPSET    ((volatile unsigned int *) (gpio + 7))
#define GPCLR    ((volatile unsigned int *) (gpio + 10))
#define GPLEV    ((volatile unsigned int *) (gpio + 13))
#define INPUT  0
#define OUTPUT 1

// Physical addresses
#define BCM2836_PERI_BASE       0x3F000000
#define GPIO_BASE               (BCM2836_PERI_BASE + 0x200000)
#define BLOCK_SIZE (4*1024)
#define SYS_TIMER_BASE          (BCM2836_PERI_BASE + 0x3000)
#define SPIO_BASE               (BCM2836_PERI_BASE + 0x204000)

/////////////////////////////////////////////////////////////////////
// Rasperry Pi Helper Functions
/////////////////////////////////////////////////////////////////////

void pioInit();

void pTimerInit();

/*
Function to set the mode of a pin. Function is the new GPFSEL value for the
specified pin.
*/
void pinMode(int pin, int function);

/*
Function to write val to a pin. Val can be either 0 or 1.
*/
void digitalWrite(int pin, int val);


int digitalRead(int pin);


void sleepMicros(int micros);


void sleepMillis(int millis);

void spiInit(int freq, int settings);

char spiSendReceive(char send);

    
double getVoltage();
#endif // PIHELPERS_H_INCLUDED

