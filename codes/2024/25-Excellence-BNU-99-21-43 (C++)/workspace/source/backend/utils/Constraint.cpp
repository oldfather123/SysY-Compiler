#include "Constraint.h"

using namespace Backend;

bool Backend::ImmediateWithin12Bits(long long value)
{
    return value >= -2048 && value <= 2047;
}