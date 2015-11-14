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
#include <iostream>


using namespace std;
int main (int argc, char** argv) 
{
    if (argc != 2) {
        cout << "Incorrect parameters. Format is ./visualize filename.wav" << endl;
        return 1;
    }
    string filename = argv[1];
    wav_reader wav_file = wav_reader(filename);
    size_t frame_rate = 16;
    frame_generator* gen;
    gen = new scrolling_fft_generator(frame_rate);
    frame_controller controller = frame_controller(gen);


    delete gen;

    return 0;
}