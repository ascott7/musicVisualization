/**
 * \file reset.cpp
 *
 * \author Eric Mueller -- emueller@hmc.edu
 *
 * \brief Poke the reset pin to test that resetting works.
 */

#include "piHelpers.h"

#define RESET_PIN 20

int main (void)
{
    pioInit();
    pinMode(RESET_PIN, OUTPUT);
    digitalWrite(RESET_PIN, 1);
    digitalWrite(RESET_PIN, 0);
}
