#include "piHelpers.h"

void pioInit() {
  int  mem_fd;
  void *reg_map;

  // /dev/mem is a psuedo-driver for accessing memory in the Linux filesystem
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
    printf("can't open /dev/mem \n");
    exit(-1);
  }

  reg_map = mmap(
         NULL,                 //Address at which to start local mapping (null means don't-care)
         BLOCK_SIZE,           //Size of mapped memory block
         PROT_READ|PROT_WRITE, // Enable both reading and writing to the mapped memory
         MAP_SHARED,           // This program does not have exclusive access to this memory
         mem_fd,               // Map to /dev/mem
         GPIO_BASE);           // Offset to GPIO peripheral

  if (reg_map == MAP_FAILED) {
    printf("gpio mmap error %d\n", (int)(size_t)reg_map);
    close(mem_fd);
    exit(-1);
  }

  gpio = (volatile unsigned *)reg_map;
}

void pTimerInit() {
  int  mem_fd;
  void *reg_map;

  // /dev/mem is a psuedo-driver for accessing memory in the Linux filesystem
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
    printf("can't open /dev/mem \n");
    exit(-1);
  }

  reg_map = mmap(
     NULL,                 //Address at which to start local mapping (null means don't-care)
     BLOCK_SIZE,           //Size of mapped memory block
     PROT_READ|PROT_WRITE, // Enable both reading and writing to the mapped memory
     MAP_SHARED,           // This program does not have exclusive access to this memory
     mem_fd,               // Map to /dev/mem
     SYS_TIMER_BASE);      // Offset to SYS_TIMER peripheral

  if (reg_map == MAP_FAILED) {
    printf("sys_timer mmap error %d\n", (int)(size_t)reg_map);
    close(mem_fd);
    exit(-1);
  }

  sys_timer = (volatile unsigned *)reg_map;
}

/*
Function to set the mode of a pin. Function is the new GPFSEL value for the
specified pin.
*/
void pinMode(int pin, int function)
{
  unsigned int offset, shift;
  if (pin > 53 || pin < 0) {
    printf("bad pin, got pin %d\n", pin);
    return;
  } else if (function > 7 || function < 0) {
    printf("bad function, got function %d\n", function);
  }
  offset = pin / 10;
  shift = (pin % 10) * 3;

  // AND gpio[offset] with all 1s and function at the proper location
  gpio[offset] &= ~((~function & 7) << shift);
  // OR gpio[offset] with all 0s and function at the proper location
  gpio[offset] |= function << shift;
}

/*
Function to write val to a pin. Val can be either 0 or 1.
*/
void digitalWrite(int pin, int val)
{
  unsigned int set, clr;
  if (pin > 53 || pin < 0) {
    printf("bad pin, got pin %d\n", pin);
    return;
  }

  if (val){
    set = pin < 32 ? 7 : 8;             // select the proper set address
    gpio[set] = 0x1 << (pin % 32);      // write to the set address
  } else {
    clr = pin < 32 ? 10 : 11;           // select the proper clear address
    gpio[clr] = 0x1 << (pin % 32);      // write to the clear address
  }
}

int digitalRead(int pin)
{
  int out;
  if (pin > 53 || pin < 0) {
    printf("bad pin, got pin %d\n", pin);
    return 0;
  }

  // read from the proper address (depends on the pin number)
  if (pin < 32) {
    out = (gpio[13] >> pin) & 1;
  } else {
    out = (gpio[14] >> (pin - 32)) & 1;
  }
  return out;
}

void sleepMicros(int micros)
{
  if (micros == 0) {
    return;
  }
  sys_timer[4] = sys_timer[1] + micros;    // C1 = CLO + micros
  sys_timer[0] = 0x2;//0b0010;                   // clear M1
  while (!!(sys_timer[0] & 0x2) == 0);  // wait for M1 to go high again
}

void sleepMillis(int millis)
{
  sleepMicros(1000 * millis);     // sleep 1000 microseconds for each millisecond
}

void spiInit(int freq, int settings)
{
  int  mem_fd;
  void *reg_map;
  
  // /dev/mem is a psuedo-driver for accessing memory in the Linux filesystem
  if ((mem_fd = open("/dev/mem", O_RDWR|O_SYNC) ) < 0) {
    printf("can't open /dev/mem \n");
    exit(-1);
  }

  reg_map = mmap(
     NULL,                 //Address at which to start local mapping (null means don't-care)
     BLOCK_SIZE,           //Size of mapped memory block
     PROT_READ|PROT_WRITE, // Enable both reading and writing to the mapped memory
     MAP_SHARED,           // This program does not have exclusive access to this memory
     mem_fd,               // Map to /dev/mem
     SPIO_BASE);      // Offset to SYS_TIMER peripheral

  if (reg_map == MAP_FAILED) {
    printf("sys_timer mmap error %d\n", (int)(size_t)reg_map);
    close(mem_fd);
    exit(-1);
  }

  spi0 = (volatile unsigned *)reg_map;


  // set pins 8-11 to be used for spi0
  pinMode(8, ALT0);
  pinMode(9, ALT0);
  pinMode(10, ALT0);
  pinMode(11, ALT0);

  spi0[2] = 250000000 / freq;     // set clock rate
  spi0[0] = settings;             // set the settings
  spi0[0] |= 0x00000080;          // set Transfer Active bit
}

char spiSendReceive(char send)
{
  spi0[1] = send;
  while (!(spi0[0] & 0x00010000)) {
  }
   return spi0[1];
}

    
double getVoltage()
{
  char one = spiSendReceive(0x68);
  char two = spiSendReceive(0x00);
  int result = 0x00000000;
  result = (result | (one & 0x03)) << 8;
  result |= two;
  return (double)(result * 5) / 1024;
}