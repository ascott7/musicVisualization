/**
*   \file scrolling_fft.cpp
*
*   \author Andrew Scott -- ascott@hmc.edu and Eric Mueller -- emueller@hmc.edu
*
*   \brief Script that takes a file name as a command line argument and
*       then calls all the helper code we have to play the song in the
*       wave file specified by the file name, generate the visualization
*       frames for the song being played, and send the visualization frames
*       over to an FPGA via the serial interface.
*
*/

#include "system_constants.hpp"
#include "frame.hpp"
#include "piHelpers.h"

#include <algorithm>
#include <iostream>

using namespace std;

int main (int argc, char** argv) 
{
    scrolling_fft_generator gen;
        
    if (argc != 2) {
        cout << "usage: ./scrolling_fft filename.wav"
             << endl;
        return 1;
    }

    // set up the Pi's peripherals, including an 8MHz SPI clock
    pioInit();
    pTimerInit();
    spiInit(7812000, 0);

    // reset the display before trying to display anything
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, 1);
    digitalWrite(RESET_PIN, 0);

    gen.play_song(argv[1]);
    return 0;
}
