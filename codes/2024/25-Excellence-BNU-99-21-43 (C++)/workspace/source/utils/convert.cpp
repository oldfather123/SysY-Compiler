#include "convert.h"

unsigned floatToUnsigned(float value)
{
    unsigned *b = (unsigned *)&value;
    return (*b);
}

float unsignedToFloat(unsigned value)
{
    float *b = (float *)&value;
    return (*b);
}