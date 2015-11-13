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

#define HEADER_CHUNK_SIZE 8

struct RIFFHeader {
    char ckID[4];
    unsigned ckSize;
} riffHeader;

struct fmtChunk {
    char wavID[4];
    char ckID[4];
    unsigned cksize;
    unsigned short wFormatTag;            // Format category
    unsigned short wChannels;             // Number of channels
    unsigned dwSamplesPerSec;      // Sampling rate
    unsigned dwAvgBytesPerSec;     // For buffer estimation
    unsigned short wBlockAlign;           // Data block size
    unsigned short wBitsPerSample;
} fmtChunk;

struct dataChunk {
    char ckID[4];
    unsigned cksize;
} dataChunk;


unsigned readHeaderChunk(ifstream& file)
{
    char* headerChunk;
    unsigned size;

    headerChunk = new char [HEADER_CHUNK_SIZE];
    file.read(headerChunk, HEADER_CHUNK_SIZE);
    for (unsigned i = 0; i < 4; i++) {
        riffHeader.ckID[i] = headerChunk[i];
    }
    // check for RIFF header
    if (strcmp(riffHeader.ckID, "RIFF") != 0) {
        cerr << "Invalid file, not RIFF type" << endl;
        exit(1);
    }
    // get file size
    size = *(unsigned *) (headerChunk + sizeof(char) * 4);
    delete[] headerChunk;
    return size;
}

void readFormatChunk(char* fileData)
{
    //fmtChunk fmt;
    for (unsigned i = 0; i < 4; i++) {
        fmtChunk.wavID[i] = fileData[i];
    }
    // check for WAVE header
    if (strcmp(fmtChunk.wavID, "WAVE")) {
        cerr << "File missing WAVE identifier" << endl;
        exit(2);
    }
    for (unsigned i = 4; i < 8; i++) {
        fmtChunk.ckID[i - 4] = fileData[i];
    }
    // check for fmt header
    if (strcmp(fmtChunk.ckID, "fmt ")) {
        cerr << "File missing fmt chunk" << endl;
        exit(3);
    }
    // get format data from the file
    fmtChunk.cksize = *(unsigned *) (fileData + sizeof(char) * 8);
    fmtChunk.wFormatTag = (0x00 | fileData[13]) << 8 | fileData[12];
    fmtChunk.wChannels = *(unsigned short*) (fileData + sizeof(char) * 14);
    fmtChunk.dwSamplesPerSec = *(unsigned *) (fileData + sizeof(char) * 16);
    fmtChunk.dwAvgBytesPerSec = *(unsigned *) (fileData + sizeof(char) * 20);
    fmtChunk.wBlockAlign = *(unsigned short *) (fileData + sizeof(char) * 24);
    fmtChunk.wBitsPerSample = *(unsigned short *) (fileData + sizeof(char) * 26);
    
    /*
    Uncomment to print format information
    cout << "Format chunk size: " << fmtChunk.cksize << endl;
    printf("Format code: %04x\n", fmtChunk.wFormatTag);
    printf("Number of channels: %d\n", fmtChunk.wChannels);
    printf("Samples per second: %d\n", fmtChunk.dwSamplesPerSec);
    printf("Bytes per second: %d\n", fmtChunk.dwAvgBytesPerSec);
    printf("Data block size: %d\n", fmtChunk.wBlockAlign);
    printf("Bits per sample: %d\n", fmtChunk.wBitsPerSample);
    */
}

void readDataChunk(char* fileData)
{
    for (unsigned i = 28; i < 32; i++) {
        dataChunk.ckID[i - 28] = fileData[i];
    }
    // check for data header
    if (strcmp(dataChunk.ckID, "data")) {
        cerr << "File missing data chunk" << endl;
        exit(4);
    }
    dataChunk.cksize = *(unsigned *) (fileData + sizeof(char) * 32);
    printf("Data size: %d\n", dataChunk.cksize);
    //printf("%d %d\n", fileSize - 36, dataChunk.cksize);
}


float* readData(string filename) 
{
    char* fileData;
    float* wavData;
    unsigned fileSize;

    ifstream file (filename, ios::binary | ios::in);
    if (file.is_open()) {
        fileSize = readHeaderChunk(file);
        fileData = new char [fileSize];
        file.read(fileData, fileSize);
        file.close();

        readFormatChunk(fileData);
        readDataChunk(fileData);

        wavData = new float[dataChunk.cksize];
        for (unsigned i = 0; i < dataChunk.cksize; i++) {
            wavData[i] = float(unsigned(fileData[i + 36]));
        }
        delete[] fileData;
        return wavData;
    }
    else {
        cout << "Unable to open file";
        exit(5);
    }
}

int main (int argc, char** argv) 
{
    if (argc != 2) {
        cout << "Incorrect parameters. Format is ./wav_reader filename.wav" << endl;
        return 1;
    }
    string filename = argv[1];
    float* data;
    data = readData(filename);
    return 0;
}

