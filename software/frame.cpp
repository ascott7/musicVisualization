
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
#include <fstream>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <thread>
#include <iostream>

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

void frame::move_right()
{
        size_t x, y;
         for (y = 0; y < HEIGHT; ++y) {
                for (x = WIDTH; x-- > 1;)
                        at(x, y) = at(x-1, y);
                at(0, y) = pixel(0, 0, 0);
        }
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
        microseconds offset, interval;
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
                interval = get_frame_interval();
                for (;;) {
                        f.write();
                        next_start = start + ++frame_count*interval;
                        offset = duration_cast<microseconds>(next_start - start);
                        if (!make_next_frame(song, offset, f))
                                break;
                        
                        this_thread::sleep_until(next_start);
                }
                // scroll what was on the screen off the screen (a bit hacky
                // to have this in the frame_generator abstract class but it
                // was hard to find a better way
                for (size_t i = 0; i < 32; ++i) {
                        this_thread::sleep_until(next_start);
                        f.move_right();
                        f.write();
                        next_start = start + ++frame_count*interval;
                }
        }
        waitpid(pid, NULL, 0);
}

scrolling_fft_generator::scrolling_fft_generator()
        : frame_rate_(0), cutoff_(0.0), spec_frac_(0.5)
{}

void scrolling_fft_generator::calc_parameters(const wav_reader& song)
{
        max_ = song.max_sample();
        string line;
        ifstream param_file ("parameters.txt");
        size_t count = 1;
        if (param_file.is_open()) {
                while ( getline (param_file,line) ){
                        if (line.find("order") != string::npos) {
                                continue;
                        }
                        switch (count) {
                        case 1:
                                cutoff_ = stof(line);
                                break;
                        case 2:
                                spec_frac_ = stof(line);
                                break;
                        case 3:
                                frame_rate_ = stoul(line);
                                break;
                        default:
                                break;
                        }
                        count++;
                }
                param_file.close();
        }  
}


bool scrolling_fft_generator::make_next_frame(const wav_reader& song,
                                              std::chrono::microseconds start,
                                              frame& frame)
{
        size_t x, y;
        vector<complex<float>> spec;
        array<pixel, frame::HEIGHT> new_col;
        static bool called = false;

        // if this is our first time being called, calculate/read visualizer
        // parameters
        if (!called) {
                called = true;
                calc_parameters(song);
        }

        // generate the spectrum for the current time slice
        if (!make_spectrum(song, start, spec))
                return false;

        // pick the pixels for the new column
        new_col = pick_pixels(spec);

        // shift the frame over and add the new column on the left edge
        for (y = 0; y < frame::HEIGHT; ++y) {
                for (x = frame::WIDTH - 1; x-- > 1;)
                        frame.at(x, y) = frame.at(x-1, y);
                frame.at(0, y) = new_col.at(y);
        }
        return true;
}

bool
scrolling_fft_generator::make_spectrum(const wav_reader& song,
                                       microseconds start,
                                       vector<complex<float>>& spec)
{
        vector<float> sample = song.get_range(start, get_frame_interval());
        size_t i, n;
        float sigma = 0.45;

        // use a gaussian window.
        // https://en.wikipedia.org/wiki/Window_function#Gaussian_window
        n = sample.size();
        //for (i = 0; i < n; ++i)
        //        sample.at(i) *= pow(M_E, -0.5*pow((i - (n - 1)/2)/
        //                                          (sigma*(n-1)/2), 2));

        // copy the real sample to a complex sample
        copy(sample.begin(), sample.end(), back_inserter(spec));
        return fft(spec) == 0 && spec.size() > frame::HEIGHT;
}

pixel scrolling_fft_generator::rainbow(float x)
{
        float f = 2*M_PI*x;
        float phase = 2*M_PI/3;

        if (x > 1 || x < 0)
                cout << __func__ << ": got x=" << x << ", which is > 1 or < 0"
                     << endl;
        
        return pixel(127*(1 + cos(f - phase)),
                     127*(1 + cos(f)),
                     127*(1 + cos(f - 2*phase)));
}

// we implement this using guess and check because hey, it works, and it's
// pretty quick. Basically we just keep guessing at alpha untill we get
// close enough to n
float
scrolling_fft_generator::compute_alpha(size_t b_0, size_t n)
{
        float alpha = 1.0;
        float step = 0.001;
        float tolerance = 1.0;
        float delta;

        n /= b_0;

        for (;;) {
                delta = n - (pow(alpha, frame::HEIGHT) - 1)/(alpha - 1);
                if (delta > 0 && delta < tolerance)
                        return alpha;
                else if (delta < 0)
                         return alpha - step;
                alpha += step;
        }
}

array<pixel, frame::HEIGHT>
scrolling_fft_generator::pick_pixels(const vector<complex<float>>& spec)
{
        const size_t b_0 = 8;
        float alpha = compute_alpha(b_0, spec.size()*spec_frac_ - frame_rate_);
        auto iter = spec.begin() + frame_rate_;
        array<pixel, frame::HEIGHT> col;
        complex<float> sum;
        size_t i, b;
        float bin;

        for (i = 0; i < col.size(); ++i, iter += b) {
                b = b_0*pow(alpha, i);
                sum = accumulate(iter, iter + b, complex<float>(0));
                bin = log(abs(sum) + 1)/log(b*max_);
                if (bin < cutoff_)
                        col[i] = pixel(0,0,0);
                else {
                        bin = (bin - cutoff_)/(1 - cutoff_);
                        bin = pow(bin, 1.0/3);                
                        col[i] = rainbow(0.8 - bin);
                }
        }

        reverse(col.begin(), col.end());
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
