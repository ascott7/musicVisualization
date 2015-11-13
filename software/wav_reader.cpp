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


int main (int argc, char** argv) {
    if (argc != 2) {
        cout << "Incorrect parameters. Format is ./wav_reader filename.wav" << endl;
        return 1;
    }
    string filename = argv[1];
    char * headerChunk;
    char * formatChunkHeader;
    char * formatChunk;
    char * wavDataHeader;
    char * wavData;
    unsigned size, formatChunkSize, wavDataSize;

    ifstream file (filename, ios::binary | ios::in);
    if (file.is_open()) {
        // read the header chunk
        headerChunk = new char [12];
        file.read(headerChunk, 12);

        // TODO: check that these first 4 characters are RIFF
        cout << "CHUNK ID: ";
        for (unsigned i = 0; i < 4; i++) {
            cout << headerChunk[i];
        }
        cout << endl;
        size = *(unsigned *) (headerChunk + sizeof(char) * 4);
        cout << "CHUNK SIZE: " << size << endl;
        cout << "WAVE ID: ";
        for (unsigned i = 8; i < 12; i++) {
            cout << headerChunk[i];
        }
        cout << endl;

        // read the header of the fmt chunk
        formatChunkHeader = new char [8];
        file.read(formatChunkHeader, 8);
        for (unsigned i = 0; i < 4; i++) {
            cout << formatChunkHeader[i];
        }
        cout << endl;
        formatChunkSize = *(unsigned *) (formatChunkHeader + sizeof(char) * 4);
        cout << "Format chunk size: " << formatChunkSize << endl;

        // read the body of the fmt chunk
        formatChunk = new char [formatChunkSize];
        file.read(formatChunk, formatChunkSize);
        printf("Format code: %02x%02x\n", formatChunk[1], formatChunk[0]);
        printf("Number of channels: %d\n", *(unsigned short*) (formatChunk + sizeof(char) * 2));
        printf("Samples per second: %d\n", *(unsigned *) (formatChunk + sizeof(char) * 4));
        printf("Bytes per second: %d\n", *(unsigned *) (formatChunk + sizeof(char) * 8));
        printf("Data block size: %d\n", *(unsigned short *) (formatChunk + sizeof(char) * 12));
        printf("Bits per sample: %d\n", *(unsigned short *) (formatChunk + sizeof(char) * 14));

        // read the header of the data chunk
        wavDataHeader = new char [8];
        file.read(wavDataHeader, 8);
        for (unsigned i = 0; i < 4; i++) {
            cout << wavDataHeader[i];
        }
        cout << endl;
        wavDataSize = *(unsigned *) (wavDataHeader + sizeof(char) * 4);
        printf("Data size: %d\n", wavDataSize);

        // read the data itself
        wavData = new char [wavDataSize];
        file.read(wavData, wavDataSize);
        file.close();

        for (unsigned i = 0; i < 10; i++) {
            printf("Sample %d: %x\n", i, wavData[i]);
        }

        delete[] headerChunk;
        delete[] formatChunkHeader;
        delete[] formatChunk;
        delete[] wavDataHeader;
        delete[] wavData;
    }
    else {
        cout << "Unable to open file";
    }
    return 0;
}

