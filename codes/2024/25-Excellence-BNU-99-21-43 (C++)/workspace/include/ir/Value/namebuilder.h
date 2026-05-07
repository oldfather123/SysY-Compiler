#pragma once

#include "IR.h"

namespace IR
{

    struct NameBuilder
    {
        int tot = 0;
        NameBuilder() {}
        int getNewName()
        {
            return tot++;
        }
    };
}