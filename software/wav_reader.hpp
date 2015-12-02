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
            std::chrono::microseconds duration) const;

        float max_sample() const;
    private:
        struct riff_header {
            char ck_id[5];                      // should be "RIFF"
            unsigned ck_size;                   // size of the whole file
            char wav_id[5];                     // should be "WAVE"
        } riff_header;

        struct fmt_chunk {
            unsigned ck_size;                   // size of the rest of the file
            unsigned short w_format_tag;        // Format category
            unsigned short w_channels;          // Number of channels
            unsigned dw_samples_per_sec;        // Sampling rate
            unsigned dw_avg_bytes_per_sec;      // For buffer estimation
            unsigned short w_block_align;       // Data block size
            unsigned short w_bits_per_sample;   // Number of bits per sample
        } fmt_chunk;

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
        size_t read_header_chunk(std::ifstream& file);

        void read_general_chunk(char* file_data, size_t& file_offset);

        std::vector<int16_t> samples_;  ///> the data samples themselves
        size_t num_samples_;            ///> the number of samples
        float max_sample_;              ///> the largest sample
};

#endif // WAVREADER_HPP_INCLUDED
