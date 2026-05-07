#pragma once

#include "../utility/System.h"

enum StoreType
{
    GLOBAL,
    LOCAL
};

static vector<string> StoreTypeMark
{
    "$",
    "%"
};



enum DataType
{
    VOID,
    I32,I32_PTR,
    F32,F32_PTR
};

static vector<string> DataTypeText
{
    "void",
    "i32","i32_ptr",
    "f32","f32_ptr"
};
