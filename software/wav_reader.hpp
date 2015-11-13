/**
*   \file wav_reader.hpp
*
*   \author Andrew Scott -- ascott@hmc.edu
*
*   \brief Header file containing declarations for the wav_reader class
*
*/

#ifndef WAVREADER_HPP_INCLUDED
#define WAVREADER_HPP_INCLUDED 1

#include <chrono>
#include <string>
#include <vector>

class wav_reader {
    public:
        wav_reader(std::string filename);
        wav_reader() = delete;
        ~wav_reader();

        /**
        *   \brief Returns a vector containing all of the samples that fall
        *       into the time range spcified by start and duration.
        *
        *   \param start The start of the time range in microseconds
        *
        *   \param duration The length of the time range in microseconds
        *
        */
        std::vector<float> get_range(std::chrono::microseconds start, 
            std::chrono::microseconds duration);
    
    private:
        struct fmt_chunk {
            char wav_id[4];                     // should be "WAVE"
            char ck_id[4];                      // should be "fmt "
            unsigned ck_size;                   // size of the rest of the file
            unsigned short w_format_tag;        // Format category
            unsigned short w_channels;          // Number of channels
            unsigned dw_samples_per_sec;        // Sampling rate
            unsigned dw_avg_bytes_per_sec;      // For buffer estimation
            unsigned short w_block_align;       // Data block size
            unsigned short w_bits_per_sample;   // Number of bits per sample
        } fmt_chunk;

        struct data_chunk {
            char ck_id[4];                      // should be "data"
            unsigned ck_size;                   // size of the sample data
        } data_chunk;

        struct riff_header {
            char ck_id[4];                      // should be "RIFF"
            unsigned ck_size;                   // size of the whole file
        } riff_header;

        /**
        *   \brief Reads the first 8 bytes of the file, checking that the
        *       file begins with "RIFF" and then getting the file size from the
        *       following 4 bytes
        *
        *   \param file A reference to a filestream representing the currently
        *       open file
        *
        *   \returns The size of the file
        *
        */
        unsigned read_header_chunk(std::ifstream& file);

        /**
        *   \brief Reads the first 28 bytes past the first 8 bytes of the file
        *       (aka the 8th to 36th bytes), grabbing the formatting data
        *       located there and placing it in the fmt_chunk struct.
        *
        *   \param file_data A character array holding the file data
        *
        */
        void read_format_chunk(char* file_data);

        /**
        *   \brief Reads the actual samples from the file and places
        *       them in the samples array
        *   \param file_data A character array holding the file data
        *
        */
        void read_data_chunk(char* file_data);

        float* samples;     ///> the data samples themselves
};

#endif // WAVREADER_HPP_INCLUDED