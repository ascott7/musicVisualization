/**
 * \file frame.cpp
 *
 * \authors Eric Mueller -- emueller@hmc.edu, Andrew Scott -- ascott@hmc.edu
 *
 * \brief Frame management implementation.
 */

#include "fft.hpp"
#include "frame.hpp"
#include "piHelpers.h"

#include <algorithm>
#include <cassert>
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
{}

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
                        if (!make_next_frame(song, offset)) {
                                printf("break it down\n");
                                break;
                        }
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
        // a minimum of 32 * 32 * 3 * 4 = 12,288 clock cycles to send a new
        // frame.

        size_t x, y;
        uint8_t r0, g0, b0, r1, g1, b1;
        uint8_t byte1, byte2, byte3;
        for (y = 0; y < frame::HEIGHT; ++y) {
            for (x = 0; x < frame::WIDTH; x += 2) {
                r0 = frame_.at(x, y).red() / 16;
                g0 = frame_.at(x, y).green() / 16;
                b0 = frame_.at(x, y).blue() / 16;
                r1 = frame_.at(x + 1, y).red() / 16;
                g1 = frame_.at(x + 1, y).green() / 16;
                b1 = frame_.at(x + 1, y).blue() / 16;
                byte1 = reverse(uint8_t(g0 << 4 | r0));
                byte2 = reverse(uint8_t(r1 << 4 | b0));
                byte3 = reverse(uint8_t(b1 << 4 | g1));
                // print statements to check that the above conversions
                // are working properly
                // printf("red0 %02x green0 %02X\n", r0, g0);
                // printf("combined %02X\n", byte1);
                // printf("blue0 %02x red1 %02X\n", b0, r1);
                // printf("combined %02X\n", byte2);
                // printf("green1 %02x blue1 %02X\n", g1, b1);
                // printf("combined %02X\n", byte3);
                spiSendReceive(byte1);
                spiSendReceive(byte2);
                spiSendReceive(byte3);
            }
        }
}


//http://stackoverflow.com/questions/2602823/in-c-c-whats-the-simplest-way-to
// -reverse-the-order-of-bits-in-a-byte
uint8_t frame_controller::reverse(uint8_t byte) const
{
   byte = (byte & 0xF0) >> 4 | (byte & 0x0F) << 4;
   byte = (byte & 0xCC) >> 2 | (byte & 0x33) << 2;
   byte = (byte & 0xAA) >> 1 | (byte & 0x55) << 1;
   return byte;
}

scrolling_fft_generator::scrolling_fft_generator(unsigned frame_rate, float cutoff)
        : frame_rate_(frame_rate), last_frame_(), cutoff_(cutoff)
{
        // zero the frame
        last_frame_.fill(pixel());
}

bool scrolling_fft_generator::make_next_frame(frame& frame,
                                              vector<float>& sample)
{
        size_t x, y;
        unsigned order = 8*sizeof(size_t) - (__builtin_clzl(frame.size()) + 1);
        vector<complex<float>> c_sample(1 << order);
        array<pixel, frame::HEIGHT> new_col;

        // copy to complex array and take the fft
        copy(sample.begin(), sample.end(), c_sample.begin());
        if (fft(c_sample))
                return false;

        new_col = create_next_column(c_sample);

        for (y = 0; y < frame::HEIGHT; ++y) {
                for (x = frame::WIDTH; x-- > 1;)
                        frame.at(x, y) = frame.at(x-1, y);

                frame.at(0, y) = new_col.at(y);
        }
        return true;
}

array<pixel, frame::HEIGHT>
scrolling_fft_generator::create_next_column(
        vector<complex<float>>& spectrum)
{
        array<pixel, frame::HEIGHT> binned;
        complex<float> max, sum;
        vector<complex<float>>::iterator it;
        unsigned bin_size;

        // find the max element in the spectrum
        max = *max_element(spectrum.begin(), spectrum.end(),
                          [&](complex<float>& lhs, complex<float>& rhs) {
                return abs(lhs) < abs(rhs);
        });

        // normalize by that element
        for_each(spectrum.begin(), spectrum.end(), [&](complex<float>& i) {
                i /= max;
        });

        // bin the spectrum. each element in the binned spectrum gets the
        // average of the next bin_size elements from the spectrum. We
        // multiply binned.size() by 2 because we only want half of
        // the spectrum (it's symetric)
        it = spectrum.begin();
        bin_size = spectrum.size()/(2*binned.size());
        for_each(binned.begin(), binned.end(), [&](pixel& pix) {
                sum = 0;
                for_each(it, it + bin_size, [&](complex<float>& i) {
                        sum += i;
                });

                pix = normal_to_pixel(abs(sum/complex<float>(bin_size)));
                it += bin_size;
        });

        return binned;
}

pixel scrolling_fft_generator::normal_to_pixel(float norm)
{
        uint8_t max = numeric_limits<uint8_t>::max();
        assert(norm <= 1.0);

        // for now we just use a green/blue pixel, with high values
        // being all green and low values being all blue
        return norm > cutoff_ ? pixel(0, norm*max, (1.0 - norm)*max)
                : pixel();
}

unsigned scrolling_fft_generator::get_frame_rate() const
{
        return frame_rate_;
}

trivial_frame_generator::trivial_frame_generator(pixel p)
        : p_(p)
{}

bool trivial_frame_generator::make_next_frame(frame& frame,
                                              vector<float>& sample)
{
        (void)sample;

        // make an frame of all the same pixel, just for testing
        fill(frame.begin(), frame.end(), p_);

        p_ = pixel(p_.green(), p_.blue(), p_.red());
        
        return true;
}

unsigned trivial_frame_generator::get_frame_rate() const
{
        // arbitrary
        return 30;
}

