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

#include <algorithm>
#include <iostream>

#define RESET_PIN 20

using namespace std;
int main (int argc, char** argv) 
{
    if (argc != 3 && argc != 2) {
        cout << "usage: ./visualize filename.wav [type (fft||test)]"
             << endl;
        return 1;
    }

    string filename = argv[1];
    string type = "fft";

    if (argc == 3) {
            type = argv[2];
            if (type != "fft" && type != "test") {
                    cout << "type argument must be 'fft' or 'test'" << endl;
                    return 1;
            }
    }

    pioInit();
    pTimerInit();
    spiInit(244000, 0);

    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, 1);
    digitalWrite(RESET_PIN, 0);

    size_t frame_rate = 16;
    float cutoff = 0.001;
    scrolling_fft_generator fft_gen(frame_rate, cutoff);

    lambda_generator test_gen(10, [&](const wav_reader& song,
                                 chrono::microseconds start,
                                 frame& frame) -> bool
    {
            (void)song;
            (void)start;
            for (size_t row = 0; row < 32; ++row) {
                    for (size_t col = 0; col < 32; ++col) {
                            pixel p;
                            if (col < 6)
                                    p = pixel(255,0,0);
                            else if (col < 11)
                                    p = pixel(255,255,0);
                            else if (col < 16)
                                    p = pixel(0,255,0);
                            else if (col < 21)
                                    p = pixel(0,255,255);
                            else if (col < 26)
                                    p = pixel(0,0,255);
                            else
                                    p = pixel(255,0,255);
                            frame.at(col, row) = p;
                    }
            }
            return true;
    });

    if (type == "fft")
            fft_gen.play_song(filename);
    else
            test_gen.play_song(filename);

    return 0;
}
