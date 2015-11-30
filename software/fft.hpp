/**
 * \file fft.hpp
 * 
 * \author Eric Mueller -- emueller@hmc.edu
 * 
 * \brief Implementation of the Fast Fourier Transform using the Cooley-Tukey
 * algorithm. 
 *
 * \detail Here are some resources about this algorithm.
 * 
 *     The Wiki article:
 *         https://en.wikipedia.org/wiki/Cooley-Tukey_FFT_algorithm
 *     Original paper:
 *         http://www.ams.org/journals/mcom/1965-19-090/S0025-5718-1965-0178586-1/S0025-5718-1965-0178586-1.pdf
 *     Some good slides:
 *         http://sip.cua.edu/res/docs/courses/ee515/chapter08/ch8-2.pdf
 *
 * The interesting functions in this file are fft and ifft.
 */

#pragma once

#define _USE_MATH_DEFINES

#include <cerrno>
#include <cmath>
#include <complex>
#include <limits> 
#include <vector>

#include "util.hpp"

namespace detail {

// sort a vector based on the bitwise reverse representation of its indices.
template <typename float_t>
void bit_reverse_sort(std::vector<std::complex<float_t>>& data)
{
        size_t size = data.size();
        unsigned order = __builtin_ffs(size) - 1;
        size_t i, rev;

        for (i = 1; i < size; ++i) {
                rev = bit_reverse(i);
                rev >>= 8*sizeof rev - order;
                if (rev >= i)
                        std::swap(data.at(i), data.at(rev));
        }
}

template <bool forward, typename float_t>
int fft_impl(std::vector<std::complex<float_t>>& data)
{
        const size_t n = data.size();
        const std::complex<float_t> w_n = std::exp(
                std::complex<float_t>(2*M_PI*(forward ? -1 : 1)/n)*
                std::complex<float_t>(0, 1));
        std::complex<float_t> even, odd, w_curr, w_step;
        size_t grp_size, grp, i, start;

        static_assert(std::numeric_limits<float_t>::is_iec559,
                      "float_t must be a floating point type");

        // size must be a sane power of 2
        if ((n & (n-1)) != 0 || n < 2)
                return EINVAL;

        bit_reverse_sort(data);

        // There are a lot of local variables to keep track of here, so
        // we'll explain some of the important ones.
        // 
        // grp_size (group size): is the width of the current set of
        //     overlapping butterflies. Initially we just have butterflies of
        //     adjacent values, so it starts at 2. The next layer of
        //     butterflies overlaps in sets of 4, so we double on each round.
        //
        // even and odd: the two elements we are about to perform the
        //     butterfly operation on. These are not necessarily of even and
        //     odd index, but they are from the even and odd sub-ffts of
        //     the current fft
        // 
        // w_curr: the complex root of unity we multiply even and odd by to 
        //     perform the current butterfly operation
        //
        // w_step: factor we multiply w_curr by to get the next w_curr.
        //     dependent on the size of the current group.
        for (grp_size = 2; grp_size <= n; grp_size *= 2) {
                w_step = std::pow(w_n, n/grp_size);
                for (grp = 0; grp < n/grp_size; ++grp) {
                        start = grp*grp_size;
                        w_curr = 1;
                        for (i = start; i < start + grp_size/2; ++i) {
                                even = data.at(i);
                                odd = data.at(i+grp_size/2);
                                data.at(i) = even + odd*w_curr;
                                data.at(i+grp_size/2) = even - odd*w_curr;
                                w_curr *= w_step;
                        }
                }
        }

        // we follow the convention of Cha & Molinder and divide by n during
        // the forward transform (instead of the inverse).
        if (forward)
                for (i = 0; i < n; ++i)
                        data.at(i) /= n;

        return 0;
}

}; // namespace detail 

/// \brief Discrete Fast Fourier Transform. O(n log n) time complexity.
/// Implemented using the Cooley-Tukey algorithm.
///
/// \param data   Array of complex data to transform. Size must be power of
///               2. Transformation is performed in place.
///
/// \return 0 on success, EINVAL if the size is not a power of 2.
template <typename float_t>
int fft(std::vector<std::complex<float_t>>& data)
{
        return detail::fft_impl<true>(data);
}

/// \brief Inverse Discrete Fast Fourier Transform. O(n log n) time
/// complexity. Implemented using Cooley-Tuckey
///
/// \param data   Array of complex data to transform. Size must be power of
///               2. Transformation is performed in place.
///
/// \return 0 on success, EINVAL if the size is not a power of 2.
template <typename float_t>
int ifft(std::vector<std::complex<float_t>>& data)
{
        return detail::fft_impl<false>(data);
}
