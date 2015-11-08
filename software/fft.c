/**
 * \file fft.c
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

#include "fft.h"

#define __USE_XOPEN

#include <math.h>
#include <string.h>
#include <stdbool.h>

/* actual work done here */
static void do_fft(double complex *data, size_t n, bool inv, size_t stride)
{
        double complex w_n = cpow(M_E, (inv ? 1 : -1)*I*2*M_PI/n);
        double complex *even = data;
        double complex *odd = data + stride;
        double complex mult = 1;
        size_t i;

        if (n == 1)
                return;
        
        do_fft(even, n/2, inv, stride*2);
        do_fft(odd, n/2, inv, stride*2);

        for (i = 0; i < n*stride; i += stride, mult *= w_n)
                data[i] = even[i*2] + mult*odd[i*2];
}

/*
 * helper b/c fft and ifft do basically the same thing. Input validation
 * and call the actual worker function
 */
static int fft_helper(double complex *in, double complex *out, size_t n,
                      bool inverse)
{
        size_t i;
        
        if (!in || !out || (n & (n-1)) != 0)
                return EINVAL;

        /*
         * implementations are in place, so prepare out if it does not
         * already alias in
         */
        if (in != out)
                memcpy(out, in, n);

        do_fft(out, n, inverse, 1);

        /*
         * we follow the convention of Cha & Molinder and divide by n during
         * the forward transform (instead of the inverse).
         */
        if (!inverse)
                for (i = 0; i < n; ++i)
                        out[i] /= n;

        return 0;
}

/**
 * \brief Discrete Fast Fourier Transform. O(n log n) time complexity.
 * Implemented using the Cooley-Tukey algorithm.
 * 
 * \param in     Array of complex data to transform.
 * \param out    Where to put the output of the transformation. This may
 *               point to the same data as `in` if the transformation is
 *               to be done in place.
 * \param n      Number of entries in both arrays. Note that this must be
 *               a power of 2.
 *
 * \return 0 on success, EINVAL if the size is not a power of 2 or if
 * in or out are null.
 */
int fft(double complex *in, double complex *out, size_t n)
{
        return fft_helper(in, out, n, false);
}

/**
 * \brief Inverse Discrete Fast Fourier Transform. O(n log n) time complexity.
 * Implemented using Cooley-Tuckey
 *
 * \param in     Array of complex data to transform.
 * \param out    Where to put the result of the transformation. May alias in
 * \param n      Size of in and out arrays. Must be a power of 2.
 *
 * \return 0 on success, EINVAL if the size is not a power of 2 or if
 * in or out are null.
 */
int ifft(double complex *in, double complex *out, size_t n)
{
        return fft_helper(in, out, n, true);
}
