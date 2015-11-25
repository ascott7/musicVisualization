/**
 * \file fft_test.cpp
 * 
 * \author Eric Mueller -- emueller@hmc.edu
 *
 * \brief Some small tests for the fft and ifft functions in fft.hpp
 */

#include "fft.hpp"
#include "wav_reader.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>

using namespace std;

int main(void)
{
        wav_reader reader("space_oddity.wav");
        chrono::microseconds start = chrono::microseconds(100);
        chrono::microseconds length = chrono::microseconds(1000);
        vector<float> samples = reader.get_range(start, length);
        
        vector<complex<double>> data, copy;
        size_t i;
        int ret;

        for (i = 0; i < samples.size(); ++i)
                data.push_back(complex<double>(samples[i]));

        copy = data;
        ret = fft(copy);
        assert(ret == 0);

        ret = ifft(copy);
        assert(ret == 0);

        for (i = 0; i < data.size(); ++i)
                assert(abs(copy[i] - data[i]) < 1e-8);

        cout << "test passed" << endl;
}
