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
#include "util.hpp"

#include <algorithm>
#include <cassert>
#include <complex>
#include <cstdlib>
#include <iterator>
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

// gamma correct
static uint8_t gc(uint8_t x)
{
        return 255*pow(double(x)/255, 2.5);
}

void frame::write() const
{
        // For now the format for the spi communication will involve sending
        // row by row, starting with the first row. For each row, we send each
        // column, starting with column 0 up to 31.
        //
        // The FPGA expects the color channels of each pixel to be 4 bits,
        // but this means pixels don't line up on byte boundaries. The Pi's
        // SPI hardware sends bytes in MSB order, but this means that if we
        // a pixel crosses a byte boundary it will not be contiguous on
        // the SPI bus. For this reason we send pixels in LSB order.
        // This requires some nastyness.

        size_t x, y;
        uint8_t r0, g0, b0, r1, g1, b1;
        uint8_t byte1, byte2, byte3;
        for (y = 0; y < HEIGHT; ++y) {
            for (x = 0; x < WIDTH; x += 2) {
                r0 = gc(at(x, y).red()) / 16;
                g0 = gc(at(x, y).green()) / 16;
                b0 = gc(at(x, y).blue()) / 16;
                r1 = gc(at(x + 1, y).red()) / 16;
                g1 = gc(at(x + 1, y).green()) / 16;
                b1 = gc(at(x + 1, y).blue()) / 16;
                byte1 = bit_reverse(uint8_t(g0 << 4 | r0));
                byte2 = bit_reverse(uint8_t(r1 << 4 | b0));
                byte3 = bit_reverse(uint8_t(b1 << 4 | g1));
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

microseconds frame_generator::get_frame_interval() const
{
        return microseconds(1000*1000/get_frame_rate());
}

void frame_generator::play_song(const string& fname)
{
        wav_reader song(fname);
        clock_t::time_point start, next_start;
        size_t frame_count = 0;
        microseconds offset, interval = get_frame_interval();
        pid_t pid;
        frame f;

        // make the first frame before we start playing the song because
        // it's comutationally intensive
        if (!make_next_frame(song, microseconds(0), f))
                throw runtime_error("failed to generate first frame");
        
        pid = fork();
        if (pid < 0)
                throw runtime_error("fork failed");
        else if (pid == 0) {
                // configure the pi to play audio through the audio jack
                system("amixer cset numid=3 1");
                system(("aplay " + fname).c_str());
                exit(0);
        } else {
                start = clock_t::now();
                for (;;) {
                        f.write();
                        next_start = start + ++frame_count*interval;
                        offset = duration_cast<microseconds>(next_start - start);
                        if (!make_next_frame(song, offset, f))
                                break;
                        this_thread::sleep_until(next_start);
                }
        }

        waitpid(pid, NULL, WEXITED);
}

scrolling_fft_generator::scrolling_fft_generator(unsigned frame_rate, float cutoff)
        : frame_rate_(frame_rate), cutoff_(cutoff)
{}

// get the next power of 2 above v, unless v is a power of 2, in which case
// return v. If v is zero, we return 0 because even though that's mathematically
// wrong it's convenient.
//
// taken from here:
// http://graphics.stanford.edu/~seander/bithacks.html#RoundUpPowerOf2Float
static size_t next_power_of2_or_zero(size_t v)
{
        size_t t;
        float f;

        if (v > 1) {
                f = (float)v;
                t = 1U << ((*(unsigned int *)&f >> 23) - 0x7f);
                return t << (t < v);
        } else
                return v;
}

bool scrolling_fft_generator::make_next_frame(const wav_reader& song,
                                              std::chrono::microseconds start,
                                              frame& frame)
{
        size_t x, y;
        vector<complex<float>> spec;
        array<pixel, frame::HEIGHT> new_col;

        if (max_ == -0.0/1.0)
                max_ = song.max_sample();

        if (!make_spectrum(song, start, spec))
                return false;

        new_col = pick_pixels(spec);

        for (y = 0; y < frame::HEIGHT; ++y) {
                for (x = frame::WIDTH; x-- > 1;)
                        frame.at(x, y) = frame.at(x-1, y);
                frame.at(0, y) = new_col.at(y);
        }
        return true;
}

bool
scrolling_fft_generator::make_spectrum(const wav_reader& song,
                                       std::chrono::microseconds start,
                                       vector<complex<float>>& spec)
{
        vector<float> sample = song.get_range(start, start
                                              + get_frame_interval());
        size_t size = next_power_of2_or_zero(sample.size());
       
        // copy the real sample to a complex array and 0-pad it to
        // a power of 2 size
        copy(sample.begin(), sample.end(), back_inserter(spec));
        fill_n(back_inserter(spec), size - spec.size(), 0);

        return fft(spec) == 0;
}

static pixel rainbow(float x)
{
        float f = 2*M_PI*x;
        float phase = 2*M_PI/3;
        
        return pixel(127*(1 + cos(f)),
                     127*(1 + cos(f - phase)),
                     127*(1 + cos(f - 2*phase)));
}

array<pixel, frame::HEIGHT>
scrolling_fft_generator::pick_pixels(const vector<complex<float>>& spec)
{
        size_t bin_size = log2(spec.size());
        float average;
        float bin;
        array<pixel, frame::HEIGHT> col;
        auto iter = spec.begin();

        for_each(col.begin(), col.end(), [&](pixel& pix) {
                average = 0;
                for_each(iter, iter + bin_size, [&](const complex<float>& f) {
                        average += abs(f);
                });
                iter += bin_size;
                bin = average/(bin_size*max_);
                bin = bin > cutoff_ ? bin : 0;
                pix = rainbow(bin);
                bin_size *= 2;
        });

        return col;
}

unsigned scrolling_fft_generator::get_frame_rate() const
{
        return frame_rate_;
}

lambda_generator::lambda_generator(unsigned frame_rate, func_t f)
        : f_(f), frame_rate_(frame_rate)
{}

bool lambda_generator::make_next_frame(const wav_reader& song,
                                       std::chrono::microseconds start,
                                       frame& frame)
{
        return f_(song, start, frame);
}

unsigned lambda_generator::get_frame_rate() const
{
        return frame_rate_;
}
