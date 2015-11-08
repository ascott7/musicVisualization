/**
 * \file fft_test.c
 * 
 * \author Eric Mueller -- emueller@hmc.edu
 *
 * \brief Some small tests for the fft and ifft functions in fft.c
 */

#include "fft.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

static double dabs(double x)
{
        return x < 0 ? -x : x;
}

int main(void)
{
        double complex *in, *out, *inv;
        const size_t n = 1 << 20;
        size_t i;
        int ret;
        double delta, epsilon = 1e-4;

        in = malloc(n * sizeof *in);
        out = malloc(n * sizeof *out);
        inv = malloc(n * sizeof *inv);

        if (!in || !out || !inv) {
                printf("failed to allocated enough memory for tests\n");
                exit(1);
        }

        srand(time(NULL));
        for (i = 0; i < n; ++i)
                in[i] = rand() + I*rand();

        ret = fft(in, out, n);
        if (ret) {
                printf("fft failed with error %s\n", strerror(ret));
                exit(ret);
        }

        ret = ifft(out, inv, n);
        if (ret) {
                printf("ifft failed with error %s\n", strerror(ret));
                exit(ret);
        }

        for (i = 0; i < n; ++i) {
                delta = dabs(cabs(in[i]) - cabs(inv[i]));
                if (delta > epsilon) {
                        printf("error: got delta of %f at index %lu\n",
                               delta, i);
                        exit(2);
                }
        }

        return 0;
}
