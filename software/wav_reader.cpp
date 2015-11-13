/**
 * \file wav_reader.cpp
 * 
 * \author Andrew Scott -- ascott@hmc.edu
 * 
 * \brief Simple class to read a WAVE file and provide a function to
 *      get std::vector's of samples that occur in a given time frame
 *      of the file
 *
 *  \remarks WAVE format specification can be found at 
 *  http://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
 *  and here 
 *  http://www-mmsp.ece.mcgill.ca/documents/audioformats/wave/Docs/riffmci.pdf
 * 
 */

#include "wav_reader.hpp"
#include <iostream>
#include <fstream>

wav_reader::wav_reader(std::string filename)
{
    char* file_data;
    unsigned file_size;

    std::ifstream file (filename, std::ios::binary | std::ios::in);
    if (file.is_open()) {
        // read in all of the file data
        file_size = read_header_chunk(file);
        file_data = new char [file_size];
        file.read(file_data, file_size);
        file.close();

        // get formatting info from the file then the data samples
        read_format_chunk(file_data);
        read_data_chunk(file_data);

        // convert data samples to floats
        samples = new float[data_chunk.ck_size];
        for (unsigned i = 0; i < data_chunk.ck_size; i++) {
            samples[i] = float((unsigned char)(file_data[i + 36]));
        }
        //printf("%x\n", (unsigned char)(0x00 | file_data[36]));
        delete[] file_data;
    }
    else {
        std::cerr << "Unable to open file" << std::endl;
        exit(5);
    }
}

wav_reader::~wav_reader()
{
    delete[] samples;
}

std::vector<float> wav_reader::get_range(std::chrono::microseconds start, 
            std::chrono::microseconds duration)
{
    std::vector<float> samples_in_range;
    float samples_per_micros = float(fmt_chunk.dw_samples_per_sec) / 1000000;
    unsigned start_index = unsigned(samples_per_micros * start.count());
    unsigned range_length = unsigned(samples_per_micros * duration.count());
    for (unsigned i = 0; i < range_length; i++) {
        samples_in_range.push_back(samples[start_index + i]);
    }
    return samples_in_range;
}

unsigned wav_reader::read_header_chunk(std::ifstream& file)
{
    char* header_chunk;
    unsigned size;

    header_chunk = new char [8];
    file.read(header_chunk, 8);
    for (unsigned i = 0; i < 4; i++) {
        riff_header.ck_id[i] = header_chunk[i];
    }
    // check for RIFF header
    // if (strcmp(riff_header.ck_id, "RIFF") != 0) {
    //     std::cout << riff_header.ck_id << std::endl;
    //     std::cerr << "Invalid file, not RIFF type" << std::endl;
    //     exit(1);
    // }
    // get file size
    size = *(unsigned *) (header_chunk + sizeof(char) * 4);
    delete[] header_chunk;
    return size;
}

void wav_reader::read_format_chunk(char* file_data)
{
    for (unsigned i = 0; i < 4; i++) {
        fmt_chunk.wav_id[i] = file_data[i];
    }
    // check for WAVE header
    // if (strcmp(fmt_chunk.wav_id, "WAVE")) {
    //     std::cerr << "File missing WAVE identifier" << std::endl;
    //     exit(2);
    // }
    for (unsigned i = 4; i < 8; i++) {
        fmt_chunk.ck_id[i - 4] = file_data[i];
    }
    // check for fmt header
    // if (strcmp(fmt_chunk.ck_id, "fmt ")) {
    //     std::cerr << "File missing fmt chunk" << std::endl;
    //     exit(3);
    // }
    // get format data from the file
    fmt_chunk.ck_size = *(unsigned *) (file_data + sizeof(char) * 8);
    fmt_chunk.w_format_tag = (0x00 | file_data[13]) << 8 | file_data[12];
    fmt_chunk.w_channels = *(unsigned short*) (file_data + sizeof(char) * 14);
    fmt_chunk.dw_samples_per_sec = *(unsigned *) (file_data + sizeof(char) * 16);
    fmt_chunk.dw_avg_bytes_per_sec = *(unsigned *) (file_data + sizeof(char) * 20);
    fmt_chunk.w_block_align = *(unsigned short *) (file_data + sizeof(char) * 24);
    fmt_chunk.w_bits_per_sample = *(unsigned short *) (file_data + sizeof(char) * 26);
    
    /*
    Uncomment to print format information
    cout << "Format chunk size: " << fmt_chunk.ck_size << std::endl;
    printf("Format code: %04x\n", fmt_chunk.w_format_tag);
    printf("Number of channels: %d\n", fmt_chunk.w_channels);
    printf("Samples per second: %d\n", fmt_chunk.dw_samples_per_sec);
    printf("Bytes per second: %d\n", fmt_chunk.dw_avg_bytes_per_sec);
    printf("Data block size: %d\n", fmt_chunk.w_block_align);
    printf("Bits per sample: %d\n", fmt_chunk.w_bits_per_sample);
    */
}

void wav_reader::read_data_chunk(char* file_data)
{
    for (unsigned i = 28; i < 32; i++) {
        data_chunk.ck_id[i - 28] = file_data[i];
    }
    // check for data header
    // if (strcmp(data_chunk.ck_id, "data")) {
    //     std::cerr << "File missing data chunk" << std::endl;
    //     exit(4);
    // }
    data_chunk.ck_size = *(unsigned *) (file_data + sizeof(char) * 32);
    // printf("Data size: %d\n", data_chunk.ck_size);
    // printf("%d %d\n", file_size - 36, data_chunk.ck_size);
}

