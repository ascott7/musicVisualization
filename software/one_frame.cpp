#include "system_constants.hpp"
#include "frame.hpp"
#include "piHelpers.h"

#include <algorithm>
#include <thread>

using namespace std;

static pixel rainbow32(size_t x)
{
        return pixel(
                127*(1 + cos(2*M_PI*x/32)),
                127*(1 + cos(2*M_PI*x/32 - 2*M_PI/3)),
                127*(1 + cos(2*M_PI*x/32 - 4*M_PI/3)));
}

int main(void)
{
        frame f;
        size_t row, col;

        pioInit();
        pTimerInit();
        spiInit(244000, 0);

        pinMode(RESET_PIN, OUTPUT);
        digitalWrite(RESET_PIN, 1);
        digitalWrite(RESET_PIN, 0);

        for (size_t row = 0; row < 32; ++row)
                for (size_t col = 0; col < 32; ++col)
                        f.at(col, row) = rainbow32(row);

        f.write();

        return 0;
}
