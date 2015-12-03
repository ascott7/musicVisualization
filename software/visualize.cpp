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

#include "system_constants.hpp"
#include "wav_reader.hpp"
#include "frame.hpp"
#include "piHelpers.h"

#define _USE_MATH_DEFINES

#include <algorithm>
#include <cmath>
#include <iostream>

using namespace std;

static pixel rainbow32(size_t x)
{
        return pixel(
                127*(1 + cos(2*M_PI*x/32)),
                127*(1 + cos(2*M_PI*x/32 - 2*M_PI/3)),
                127*(1 + cos(2*M_PI*x/32 - 4*M_PI/3)));
}

int main (int argc, char** argv) 
{
    if (argc != 3 && argc != 2) {
        cout << "usage: ./visualize filename.wav [type (fft||test)]"
             << endl;
        return 1;
    }

    string filename = argv[1];
    string type = "fft";

    if (argc == 3)
            type = argv[2];

    pioInit();
    pTimerInit();
    spiInit(7812000, 0);

    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, 1);
    digitalWrite(RESET_PIN, 0);

    size_t frame_rate = 50;
    float cutoff = 0.3;
    float spec_frac = 0.02;
    scrolling_fft_generator fft_gen(frame_rate, cutoff, spec_frac);

    // at some point we should move cool shit to another file, but for
    // now I'm just dumping it here. This makes a rainbow.
    lambda_generator rainbow(10, [&](const wav_reader& song,
                                 chrono::microseconds start,
                                 frame& frame) -> bool
    {
            (void)song;
            (void)start;
            for (size_t row = 0; row < 32; ++row)
                    for (size_t col = 0; col < 32; ++col)
                            frame.at(col, row) = rainbow32(col);
            return true;
    });

    lambda_generator test_gen(10, [&](const wav_reader& song,
                                      chrono::microseconds start,
                                      frame& frame) -> bool
    {
            (void)song;
            (void)start;
            for (size_t col = 0; col < 32; ++col)
                    frame.at(col, 1) = pixel(255, 0, 0);
            return true;
    });

    if (type == "fft")
            fft_gen.play_song(filename);
    else if (type == "test")
            test_gen.play_song(filename);
    else if (type == "rainbow")
            rainbow.play_song(filename);

    return 0;
}
