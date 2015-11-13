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

namespace detail {

size_t bit_reverse_word(size_t word, unsigned order)
{
        // generated automatically using make_bit_rev_lut.py
        static const unsigned char bit_rev_lut[256] =
                {0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0, 0x10, 0x90, 
                 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0, 0x08, 0x88, 0x48, 0xc8, 
                 0x28, 0xa8, 0x68, 0xe8, 0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 
                 0x78, 0xf8, 0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4, 
                 0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4, 0x0c, 0x8c, 
                 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec, 0x1c, 0x9c, 0x5c, 0xdc, 
                 0x3c, 0xbc, 0x7c, 0xfc, 0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 
                 0x62, 0xe2, 0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2, 
                 0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea, 0x1a, 0x9a, 
                 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa, 0x06, 0x86, 0x46, 0xc6, 
                 0x26, 0xa6, 0x66, 0xe6, 0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 
                 0x76, 0xf6, 0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee, 
                 0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe, 0x01, 0x81, 
                 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1, 0x11, 0x91, 0x51, 0xd1, 
                 0x31, 0xb1, 0x71, 0xf1, 0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 
                 0x69, 0xe9, 0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9, 
                 0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5, 0x15, 0x95, 
                 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5, 0x0d, 0x8d, 0x4d, 0xcd, 
                 0x2d, 0xad, 0x6d, 0xed, 0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 
                 0x7d, 0xfd, 0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3, 
                 0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3, 0x0b, 0x8b, 
                 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb, 0x1b, 0x9b, 0x5b, 0xdb, 
                 0x3b, 0xbb, 0x7b, 0xfb, 0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 
                 0x67, 0xe7, 0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7, 
                 0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef, 0x1f, 0x9f, 
                 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff};

        size_t reversed = 0;
        size_t mask = 0xff;
        size_t index, shift;
        unsigned i;

        // index into the LUT with the ith byte of word, then OR it with
        // reversed as sizeof word - ith byte
        for (i = 0; i < 8*sizeof word; i+=8) {
                index = (word & mask << i) >> i;
                shift = 8*sizeof word - (i+8);
                reversed |= size_t(bit_rev_lut[index]) << shift;
        }
        reversed >>= 8*sizeof word - order;

        return reversed;
}

// sort a vector based on the bitwise reverse representation of its indices.
template <typename float_t>
void bit_reverse_sort(std::vector<std::complex<float_t>>& data)
{
        size_t size = data.size();
        unsigned order = __builtin_ffs(size) - 1;
        size_t i, rev;

        for (i = 1; i < size; ++i) {
                rev = bit_reverse_word(i, order);
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
