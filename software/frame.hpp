/**
 * \file frame.hpp
 * 
 * \author Eric Mueller -- emueller@hmc.edu
 *
 * \brief Frame management objects and a frame_generator interface
 */

#pragma once

#include "wav_reader.hpp"

#include <array>
#include <chrono>
#include <complex>
#include <cstdint>
#include <functional>
#include <string>
#include <tuple>

// wrapper class for RGB 3-tuples with 8-bit color channels. No alpha
// because the underlying display doesn't support it.
class pixel {
public:
        pixel(uint8_t red = 0, uint8_t green = 0, uint8_t blue = 0);

        // get the color values
        uint8_t red() const;
        uint8_t green() const;
        uint8_t blue() const;

        // set the color values
        uint8_t& red();
        uint8_t& green();
        uint8_t& blue();

private:
        std::tuple<uint8_t, uint8_t, uint8_t> rgb_;
};

// frame object. stores an array of pixels. basically just a wrapper for
// std::array<pixel>
class frame : public std::array<pixel, 32*32> {
public:
        // convenience functions to access the 1-dimensional array using
        // (x,y) coordinates
        pixel& at(size_t x, size_t y);
        const pixel& at(size_t x, size_t y) const;

        // write the contents of the frame over SPI to the FPGA
        void write() const;

        static constexpr unsigned WIDTH = 32;
        static constexpr unsigned HEIGHT = 32;
};

// abstract base class for all frame generating things. music visualizers
// should inherit from this class and implement all the virtual methods
// Also provides a song playing method for all frame generators to use
class frame_generator {
public:
        virtual ~frame_generator() = default;

        // play and visualize a song.
        void play_song(const std::string& fname);

protected:
        // generate the next frame to display based on a set of samples
        // for the next time slice.
        virtual bool
        make_next_frame(const wav_reader& song,
                        std::chrono::microseconds start,
                        frame& f) = 0;
        virtual unsigned get_frame_rate() const = 0;

        std::chrono::microseconds get_frame_interval() const;

private:
        using clock_t = std::chrono::high_resolution_clock;
};

// basic fft frame generator. not yet implemented
class scrolling_fft_generator : public frame_generator {
public:
        scrolling_fft_generator(unsigned frame_rate);
        ~scrolling_fft_generator() = default;

protected:
        bool make_next_frame(const wav_reader& song,
                             std::chrono::microseconds start,
                             frame& frame);

        unsigned get_frame_rate() const;

private:
        // find what fraction of the spectrum has interesting data
        void calc_parameters(const wav_reader& song);
        
        // create the spectrum of the next time sample
        bool make_spectrum(const wav_reader& song,
                           std::chrono::microseconds start,
                           std::vector<std::complex<float>>& spec);

        // use spectrum to choose the next column of pixels to display
        std::array<pixel, frame::HEIGHT>
        pick_pixels(const std::vector<std::complex<float>>& spec);

        // given a float in the range 0 <= x < 1, compute the pixel value
        // that is x percent of the way through a rainbow
        static pixel rainbow(float x);

        // In pick_pixels we want to bin the spectrum into bins of
        // logrithmic size where each bin size is b_i = alpha*b_{i-1}.
        // This function computes alpha given b_0, the size of the first
        // bin and n, the number of samples of in the spectrum
        static float compute_alpha(size_t b_0, size_t n);

        const unsigned frame_rate_;
        float cutoff_;
        float max_ = -0.0/1.0;
        float spec_frac_;
};

// lambda generator. holds a function that is called in place of
// "make_next_frame". Makes for easy prototyping
class lambda_generator : public frame_generator {
        using func_t = std::function<bool(const wav_reader&,
                                          std::chrono::microseconds,
                                          frame&)>;
        func_t f_;
        const unsigned frame_rate_;

public:
        lambda_generator(unsigned frame_rate, func_t f);
        ~lambda_generator() = default;

protected:
        bool make_next_frame(const wav_reader& song,
                             std::chrono::microseconds start,
                             frame& frame);
        unsigned get_frame_rate() const;
};
