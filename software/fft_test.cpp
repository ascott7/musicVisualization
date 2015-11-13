/**
 * \file fft_test.cpp
 * 
 * \author Eric Mueller -- emueller@hmc.edu
 *
 * \brief Some small tests for the fft and ifft functions in fft.hpp
 */

#include "fft.hpp"

#include <cassert>
#include <iomanip>
#include <iostream>

using namespace std;

int main(void)
{
        vector<complex<double>> data, copy;
        size_t i;
        int ret;

        for (i = 0; i < 1 << 20; ++i)
                data.push_back(cos(complex<double>(i)));

        copy = data;
        ret = fft(copy);
        assert(ret == 0);

        ret = ifft(copy);
        assert(ret == 0);

        for (i = 0; i < data.size(); ++i)
                assert(abs(copy[i] - data[i]) < 1e-8);

        cout << "test passed" << endl;
}
