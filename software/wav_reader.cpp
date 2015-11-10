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
 *  and here 
 *  http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/Docs/riffmci.pdf
 * 
 */

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

/*typedef unsigned WORD;
typedef unsigned long DWORD;
typedef unsigned char BYTE;
typedef DWORD FOURCC;           // Four-character code
typedef FOURCC CKID;            // Four-character-code chunk identifier
typedef DWORD CKSIZE;           // 32-bit unsigned size value
typedef struct {                // Chunk structure
    CKID ckID;                  // Chunk type identifier
    CKSIZE ckSize;              // Chunk size field (size of ckData)
    BYTE ckData[ckSize];        // Chunk data
} CK;

struct {
    WORD wFormatTag;            // Format category
    WORD wChannels;             // Number of channels
    DWORD dwSamplesPerSec;      // Sampling rate
    DWORD dwAvgBytesPerSec;     // For buffer estimation
    WORD wBlockAlign;           // Data block size
} fmt_ck;
*/


int main () {
    char * memblock;
    char * chunk1;
    unsigned size, chunk1Size; 

    ifstream file ("space_oddity.wav", ios::binary | ios::in);
    if (file.is_open()) {
        memblock = new char [12];
        file.read(memblock, 12);

        chunk1 = new char [48];
        file.read(chunk1, 48);
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

        cout << "READING CHUNK 1" << endl;

        for (unsigned i = 0; i < 4; i++) {
            cout << chunk1[i];
        }
        cout << endl;

        chunk1Size = *(unsigned *) (chunk1 + sizeof(char) * 4);
        cout << "CHUNK 1 SIZE: " << chunk1Size << endl;

        printf("Format code: %02x%02x\n", chunk1[9], chunk1[8]);
        printf("Number of channels: %d\n", *(unsigned short*) (chunk1 + sizeof(char) * 10));
        printf("Samples per second: %d\n", *(unsigned *) (chunk1 + sizeof(char) * 12));
        printf("Bytes per second: %d\n", *(unsigned *) (chunk1 + sizeof(char) * 16));
        printf("Data block size: %d\n", *(unsigned short *) (chunk1 + sizeof(char) * 20));
        printf("Bits per sample: %d\n", *(unsigned short *) (chunk1 + sizeof(char) * 22));
        printf("Sample 1: %01x\n", chunk1[24]);
        printf("Sample 2: %01x\n", chunk1[25]);
        printf("Sample 2: %01x\n", chunk1[26]);
        printf("Sample 2: %01x\n", chunk1[27]);
        delete[] memblock;
        delete[] chunk1;
    }
    else {
        cout << "Unable to open file";
    }
    return 0;
}

