/**
 * \file fft.h
 * 
 * \author Eric Mueller -- emueller@hmc.edu
 *
 * \brief Declarations for simple fft code. See fft.c for more info. 
 */

#ifndef MUSIC_VIS_FFT_H
#define MUSIC_VIS_FFT_H

#include <complex.h>
#include <unistd.h>
#include <errno.h>

int fft(double complex *in, double complex *out, size_t n);
int ifft(double complex *in, double complex *out, size_t n);

#endif /* MUSIC_VIS_FFT_H */
