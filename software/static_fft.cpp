/**
*   \file static_fft.cpp
*
*   \author Eric Mueller -- emueller@hmc.edu
*
*   \brief another visualizer. Just displays the fft.
*/

#include "system_constants.hpp"
#include "frame.hpp"
#include "piHelpers.h"

#include <algorithm>
#include <iostream>

using namespace std;

int main(int argc, char** argv) 
{
        static_fft_generator gen;
        
    if (argc != 2) {
        cout << "usage: ./static_fft filename.wav"
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
