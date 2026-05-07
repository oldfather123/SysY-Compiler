#include "AsmUtility.h"
#include <stdexcept>

namespace Backend
{
    bool isPowerOf2(int x)
    {
        return x > 0 && (x & (x - 1)) == 0;
    }

    int log2Floor(int x)
    {
        if (x < 0)
            throw std::invalid_argument("x must be positive");
        
        int res = -1;

        while (x > 0)
        {
            x >>= 1;
            res++;
        }

        return res;
    }

} // namespace Backend
