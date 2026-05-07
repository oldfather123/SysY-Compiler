/**
 * @file IR.h
 * @brief 定义编译系统的中间表示，参考 LLVM IR，面向 RISC-V 汇编
 */
#pragma once
#include "error.h"
#include "basictype.h"
#include "use.h"
#include "value.h"
#include "module.h"
#include "IRBuilder.h"
#include "constantfolder.h"
#include "arraytype.h"
#include "errortype.h"
#include "floattype.h"
#include "int32type.h"
#include "pointertype.h"
#include "functiontype.h"
#include "voidtype.h"
#include "labeltype.h"
#include "globalvalue.h"
#include "argument.h"
#include "constant.h"
#include "user.h"
#include "basicblock.h"
#include "instruction.h"
#include "binaryinstr.h"
#include "unaryinstr.h"
#include "memoryinstr.h"
#include "otherinstr.h"
#include "terminatorinstr.h"

#ifdef LOCAL
#include "debug.h"
#else
#define debug(...) \
    do             \
    {              \
    } while (0)
#define pause(...) \
    do             \
    {              \
    } while (0)
#endif
