/**
 * \file frame.cpp
 *
 * \author Eric Mueller -- emueller@hmc.edu
 *
 * \brief Frame management implementation.
 */

#include "fft.hpp"
#include "frame.hpp"
#include "piHelpers.h"

#include <algorithm>
#include <complex>
#include <cstdlib>
#include <limits>
#include <stdexcept>
#include <thread>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

using namespace std;
using namespace chrono;

pixel::pixel(uint8_t red, uint8_t green, uint8_t blue)
        : rgb_(red, green, blue)
{}

uint8_t pixel::red() const
{
        return get<0>(rgb_);
}

uint8_t pixel::green() const
{
        return get<1>(rgb_);
}

uint8_t pixel::blue() const
{
        return get<2>(rgb_);
}

uint8_t& pixel::red()
{
        return get<0>(rgb_);
}

uint8_t& pixel::green()
{
        return get<1>(rgb_);
}

uint8_t& pixel::blue()
{
        return get<2>(rgb_);
}

pixel& frame::at(size_t x, size_t y)
{
        return array::at(x*WIDTH + y);
}

const pixel& frame::at(size_t x, size_t y) const
{
        return array::at(x*WIDTH + y);
}

frame_controller::frame_controller(frame_generator* gen)
        : gen_(gen), frame_()
{
    pioInit();
    spiInit(244000, 0);
}

void frame_controller::set_generator(frame_generator* gen)
{
        gen_ = gen;
}

microseconds frame_controller::get_frame_interval() const
{
        return microseconds(1000*1000/gen_->get_frame_rate());
}

void frame_controller::play_song(const string& fname)
{
        wav_reader song(fname);
        clock_t::time_point start, next_start;
        size_t current_frame = 0;
        microseconds offset, interval = get_frame_interval();
        pid_t pid;

        // make the first frame before we start playing the song because
        // it's comutationally intensive
        if (!make_next_frame(song, microseconds(0)))
                throw runtime_error("failed to generate first frame");
        
        pid = fork();
        if (pid < 0)
                throw runtime_error("fork failed");
        else if (pid == 0) {
                system(("aplay " + fname).c_str());
                exit(0);
        } else {
                start = clock_t::now();
                for (;;) {
                        write_frame();
                        next_start = start + ++current_frame*interval;
                        offset = duration_cast<microseconds>(next_start - start);
                        if (!make_next_frame(song, offset))
                                break;
                        this_thread::sleep_until(next_start);
                }
        }

        waitpid(pid, NULL, WEXITED);
}

bool frame_controller::make_next_frame(const wav_reader& song,
                                       microseconds start)
{
        vector<float> sample = song.get_range(start, get_frame_interval());
        return gen_->make_next_frame(frame_, sample);
}

void frame_controller::write_frame() const
{
        // SPI stuff. still need to decide on the on-the-wire frame
        // format. need to be careful b/c the LED matrix only has 4 bit color
        // channel depth, but our pixels use 8 bit color channels

        // For now the format for the spi communication will involve sending
        // row by row, starting with the first row. For each row, we send each
        // column, starting with column 0 up to 31. For each individual pixel
        // at (row, column), we do 3 sends, each 8 bits long representing in 
        // order, red, green and then blue for that pixel. This will require
        // a minimum of 32 * 32 * 3 * 8 = 24,568 clock cycles to send a new
        // frame.

        // We may want to consider adding a LOAD signal that is high for the 
        // duration of the transfer or right before the transfer so that the
        // FPGA knows it should be loading data (this will require more 
        // thought).
        size_t x, y;
        for (y = 0; y < frame::HEIGHT; ++y) {
            for (x = 0; x < frame::WIDTH; ++x) {
                spiSendReceive(frame_.at(x, y).red());
                spiSendReceive(frame_.at(x, y).green());
                spiSendReceive(frame_.at(x, y).blue());
            }
        }
}

scrolling_fft_generator::scrolling_fft_generator(unsigned frame_rate)
        : frame_rate_(frame_rate), last_frame_()
{
        // zero the frame
        last_frame_.fill(pixel());
}

bool scrolling_fft_generator::make_next_frame(frame& frame,
                                              vector<float>& sample)
{
        size_t x, y;
        unsigned order = 8*sizeof(size_t) - (__builtin_clz(frame.size()) + 1);
        vector<complex<float>> c_sample(1 << order);
        array<pixel, frame::HEIGHT> new_col;

        // copy to complex array and take the fft
        copy(sample.begin(), sample.end(), c_sample.begin());
        if (!fft(c_sample))
                return false;

        new_col = bin_sample(c_sample);

        for (y = 0; y < frame::HEIGHT; ++y) {
                for (x = frame::WIDTH; x-- > 1;)
                        frame.at(x, y) = frame.at(x-1, y);

                frame.at(0, y) = new_col.at(y);
        }
        return true;
}

array<pixel, frame::HEIGHT>
scrolling_fft_generator::bin_sample(vector<complex<float>>& sample)
{
        array<pixel, frame::HEIGHT> binned;
        complex<float> average;
        vector<complex<float>>::iterator it;
        float max, r;
        unsigned bin_size;
        
        // normalize
        max = real(sample.at(0));
        for_each(sample.begin(), sample.end(), [&](complex<float>& i) {
                        if (real(i) > max)
                                max = real(i);
                });
        for_each(sample.begin(), sample.end(), [&](complex<float>& i) {
                        i /= max;
                });

        // bin the sample. each element in the binned sample gets the
        // average of the next bin_size elements from the sample
        it = sample.begin();
        bin_size = sample.size()/(2*binned.size());
        for_each(binned.begin(), binned.end(), [&](pixel& i) {
                        average = 0;
                        for_each(it, it + bin_size, [&](complex<float>& i) {
                                        average += i;
                                });
                        r = real(average/complex<float>(bin_size));

                        // larger values get 
                        i.green() = numeric_limits<uint8_t>::max() * r;
                        i.blue() = numeric_limits<uint8_t>::max() - i.green();

                        it += bin_size;
                });

        return binned;
}

unsigned scrolling_fft_generator::get_frame_rate() const
{
        return frame_rate_;
}
