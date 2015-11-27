/**
*   \file visualize.cpp
*
*   \author Andrew Scott -- ascott@hmc.edu
*
*   \brief Script that takes a file name as a command line argument and
*       then calls all the helper code we have to play the song in the
*       wave file specified by the file name, generate the visualization
*       frames for the song being played, and send the visualization frames
*       over to an FPGA via the serial interface.
*
*/

#include "wav_reader.hpp"
#include "frame.hpp"
#include "piHelpers.h"
#include <iostream>

#define RESET_PIN 20

using namespace std;
int main (int argc, char** argv) 
{
    if (argc != 2) {
        cout << "Incorrect parameters. Format is ./visualize filename.wav" << endl;
        return 1;
    }

    pioInit();
    pTimerInit();
    spiInit(244000, 0);

    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, 1);
    digitalWrite(RESET_PIN, 0);

    string filename = argv[1];
    size_t frame_rate = 16;
    float cutoff = 0.01;
//    scrolling_fft_generator gen = scrolling_fft_generator(frame_rate, cutoff);
    trivial_frame_generator gen(pixel(0, 0, 255));
    gen.play_song(filename);

    return 0;
}
