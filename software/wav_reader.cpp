/**
 * \file wav_reader.cpp
 * 
 * \author Andrew Scott -- ascott@hmc.edu
 * 
 * \brief Simple program to read a WAVE file and output some basic
 *      information about the file (for now this includes the chunk ID,
 *      chunk size, and wave ID data members at the start of the file).
 *
 *  \remarks WAVE format specification can be found at 
 *  http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
 * 
 */

#include "wav_reader.hpp"

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main () {
    char * memblock;
    unsigned size; 

    ifstream file ("space_oddity.wav", ios::binary | ios::in);
    if (file.is_open()) {
        memblock = new char [12];
        file.read(memblock, 12);
        file.close();

        // print ckID
        cout << "CHUNK ID: ";
        for (unsigned i = 0; i < 4; i++) {
            cout << memblock[i];
        }
        cout << endl;

        // print ckSize
        size = *(unsigned *) (memblock + sizeof(char) * 4);
        cout << "CHUNK SIZE: " << size << endl;

        // print WAVEID
        cout << "WAVE ID: ";
        for (unsigned i = 8; i < 12; i++) {
            cout << memblock[i];
        }
        cout << endl;

        delete[] memblock;
    }
    else {
        cout << "Unable to open file";
    }
    return 0;
}

