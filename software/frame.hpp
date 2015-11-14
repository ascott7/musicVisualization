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

        static constexpr unsigned WIDTH = 32;
        static constexpr unsigned HEIGHT = 32;
};

// abstract base class for all frame generating things. music visualizers
// should inherit from this class and implement all the virtual methods
class frame_generator {
public:
        virtual ~frame_generator() = default;
        // generate the next frame to display based on a set of samples
        // for the next time slice.
        virtual bool
        make_next_frame(frame& frame, std::vector<float>& sample) = 0;
        virtual unsigned get_frame_rate() const = 0;
};

// frame controller. uses a song and a frame generator and spits out
// frames on an interval. Construct one and call run() to start writing
// frames to the FPGA
class frame_controller {
public:
        frame_controller(frame_generator* gen = nullptr);

        // change the internal frame generator
        void set_generator(frame_generator* gen);

        // play and visualize a song.
        void play_song(const std::string& fname);
private:
        // generate the next frame using gen_. Returns true on success, false
        // if there are no more frames to generate, i.e. if the song is over
        bool make_next_frame(const wav_reader& song,
                             std::chrono::microseconds start);

        // write the next frame out to the FPGA via SPI
        void write_frame() const;
        std::chrono::microseconds get_frame_interval() const;

        using clock_t = std::chrono::high_resolution_clock;

        frame_generator* gen_;
        frame frame_;
};

// basic fft frame generator. not yet implemented
class scrolling_fft_generator : public frame_generator {
public:
        scrolling_fft_generator(unsigned frame_rate);
        ~scrolling_fft_generator() = default;
        bool make_next_frame(frame& frame, std::vector<float>& sample);
        unsigned get_frame_rate() const;
private:
        static std::array<pixel, frame::HEIGHT>
        bin_sample(std::vector<std::complex<float>>& sample);
        
        const unsigned frame_rate_;
        frame last_frame_;
};
