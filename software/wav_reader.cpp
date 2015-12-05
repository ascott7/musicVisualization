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

#include <cstring>
#include <iostream>
#include <iterator>
#include <fstream>
#include <algorithm>
using namespace std;

#define CHUNK_SIZE_LENGTH 4
#define CHUNK_ID_LENGTH 4

wav_reader::wav_reader(string filename)
{
    char* file_data;
    uint32_t file_size;
    size_t file_offset = 0;

    ifstream file (filename, ios::binary | ios::in);
    if (file.is_open()) {
        // read in all of the file data
        file_size = read_header_chunk(file);
        file_data = new char [file_size];
        file.read(file_data, file_size);
        file.close();

        // keep reading chunks until we get to the end of the file
        while (file_offset < file_size) {
            read_general_chunk(file_data, file_offset);
        }


        delete[] file_data;
    }
    else {
        cerr << "Unable to open file" << endl;
        exit(5);
    }
}

float wav_reader::max_sample() const
{
        return max_sample_;
}

vector<float> wav_reader::get_range(chrono::microseconds start, 
            chrono::microseconds duration) const
{
    vector<float> samples_in_range;
    const float samples_per_micros = float(fmt_chunk.dw_samples_per_sec) / 1000000;
    const uint32_t start_index = uint32_t(samples_per_micros * start.count());
    const uint32_t range_length = uint32_t(samples_per_micros * duration.count());
    for (uint32_t i = start_index; i < start_index + range_length; i++) {
            if (i >= samples_.size())
                    return samples_in_range;
            samples_in_range.push_back(float(samples_.at(i)));
    }

    return samples_in_range;
}

vector<float> wav_reader::get_all_samples() const
{
        vector<float> samples;
        copy(samples_.begin(), samples_.end(), back_inserter(samples));
        return samples;
}

size_t wav_reader::read_header_chunk(ifstream& file)
{
    char* header_chunk;
    uint32_t size;

    header_chunk = new char [12];
    file.read(header_chunk, 12);
    for (uint32_t i = 0; i < 4; i++) {
        riff_header.ck_id[i] = header_chunk[i];
    }
    riff_header.ck_id[4] = '\0';
    // check for RIFF header
    if (strcmp(riff_header.ck_id, "RIFF") != 0) {
        cout << riff_header.ck_id << endl;
        cerr << "Invalid file, not RIFF type" << endl;
        exit(1);
    }
    // get file size
    size = *(uint32_t *) (header_chunk + sizeof(char) * 4);
    riff_header.ck_size = size;
    for (size_t i = 8; i < 12; i++) {
        riff_header.wav_id[i - 8] = header_chunk[i];
    }
    riff_header.wav_id[4] = '\0';
    // check for WAVE header
    if (strcmp(riff_header.wav_id, "WAVE")) {
        cerr << "File missing WAVE identifier" << endl;
        exit(2);
    }
    delete[] header_chunk;
    return size - 4;        // accounts for the 'WAVE' characters
}

void wav_reader::read_general_chunk(char* file_data, size_t& file_offset)
{
    char id [5];
    // get the id for the current chunk we are looking at
    for (size_t i = file_offset; i < file_offset + CHUNK_ID_LENGTH; i++) {
        id[i - file_offset] = file_data[i];
    }
    id[4] = '\0';
    size_t size = *(uint32_t *) (file_data + sizeof(char) * (file_offset + 4));

    // if we are at the format chunk, get the formatting information
    if (strcmp(id, "fmt ") == 0) {
        fmt_chunk.ck_size = *(size_t *) (file_data + sizeof(char) * 4);
        fmt_chunk.w_format_tag = (0x00 | file_data[9]) << 8 | file_data[8];
        fmt_chunk.w_channels = *(uint16_t*) (file_data + sizeof(char) * 10);
        fmt_chunk.dw_samples_per_sec = *(uint32_t *) (file_data + sizeof(char) * 12);
        fmt_chunk.dw_avg_bytes_per_sec = *(uint32_t *) (file_data + sizeof(char) * 16);
        fmt_chunk.w_block_align = *(uint16_t *) (file_data + sizeof(char) * 20);
        fmt_chunk.w_bits_per_sample = *(uint16_t *) (file_data + sizeof(char) * 22);
    }
    // if we are at the data chunk, get the data samples
    if (strcmp(id, "data") == 0) {
        num_samples_ = size;
        if (fmt_chunk.w_channels == 1) {
            if (fmt_chunk.w_bits_per_sample == 8) {
                    for (size_t i = file_offset + 8; i < file_offset + 8 + num_samples_; i++) {
                            samples_.push_back(uint8_t(file_data[i]));
                    }
            }
            else if (fmt_chunk.w_bits_per_sample == 16) {
                    num_samples_ /= 2;
                    for (size_t i = file_offset + 8; i < file_offset + 8 + num_samples_ * 2; i+=2) {
                            uint8_t byte1 = uint8_t(file_data[i]);
                            uint8_t byte2 = uint8_t(file_data[i + 1]);
                            int16_t sample = int16_t(byte2) << 8 | byte1;
                            samples_.push_back(sample);
                    }
            }
        } else if (fmt_chunk.w_channels == 2) {
            if (fmt_chunk.w_bits_per_sample == 8) {
                    num_samples_ /= 2;
                    for (size_t i = file_offset + 8; i < file_offset + 8 + num_samples_ * 2; i+=2) {
                            uint8_t byte1 = uint8_t(file_data[i]);
                            uint8_t byte2 = uint8_t(file_data[i + 1]);
                            int16_t sample = (byte1 + byte2) / 2;
                            samples_.push_back(sample);
                    }
            }
            else if (fmt_chunk.w_bits_per_sample == 16) {
                    num_samples_ /= 4;
                    for (size_t i = file_offset + 8; i < file_offset + 8 + num_samples_ * 4; i+=4) {
                            uint8_t byte1 = uint8_t(file_data[i]);
                            uint8_t byte2 = uint8_t(file_data[i + 1]);
                            uint8_t byte3 = uint8_t(file_data[i + 2]);
                            uint8_t byte4 = uint8_t(file_data[i + 3]);
                            int16_t sample1 = int16_t(byte2) << 8 | byte1;
                            int16_t sample2 = int16_t(byte4) << 8 | byte3;
                            int16_t sample = (int32_t(sample1) + sample2) / 2;
                            samples_.push_back(sample);
                    }
            }
        }

        max_sample_ = float(*max_element(samples_.begin(), samples_.end()));
    }

    file_offset = file_offset + size + CHUNK_ID_LENGTH + CHUNK_SIZE_LENGTH;
}

