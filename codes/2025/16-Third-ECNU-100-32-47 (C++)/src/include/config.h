/**
 * @file config.h
 * 该头文件定义了一系列aaac可能会用到的常量信息
 */
#ifndef AAAC_MACRO_H
#define AAAC_MACRO_H

#pragma once

#include "common.h"

namespace aaac {
namespace config {
// For Basic Block and CFG
constexpr size_t RESERVE_BASIC_BLOCK_PREDS=4;
constexpr size_t RESERVE_BASIC_BLOCK_SUCCS=2;
constexpr size_t RESERVE_CFG_BASIC_BLOCKS=16;

// For BBTree
constexpr size_t RESERVE_BBTREE_NODES=16;

constexpr size_t MAX_TOKEN_LEN = 256;
constexpr size_t MAX_ID_LEN = 64;
constexpr size_t MAX_LINE_LEN = 256;

constexpr size_t MAX_VAR_LEN = 32;
constexpr size_t MAX_TYPE_LEN = 32;

constexpr size_t MAX_PARAMS = 8;
constexpr size_t MAX_LOCALS = 1500;
constexpr size_t MAX_FIELDS = 64;
constexpr size_t MAX_FUNCS = 512;
constexpr size_t MAX_TYPES = 64;

constexpr size_t MAX_IR_INSTR = 50000;
constexpr size_t MAX_BB_PRED = 128;
constexpr size_t MAX_BB_DOM_SUCC = 64;
constexpr size_t MAX_BB_RDOM_SUCC = 256;
constexpr size_t MAX_GLOBAL_IR = 256;

constexpr size_t MAX_LABEL = 4096;
constexpr size_t MAX_SOURCE = 327680;
constexpr size_t MAX_CODE = 262144;
constexpr size_t MAX_DATA = 262144;

constexpr size_t MAX_SYMTAB = 65536;
constexpr size_t MAX_STRTAB = 65536;

constexpr size_t MAX_HEADER = 1024;
constexpr size_t MAX_SECTION = 1024;
constexpr size_t MAX_ALIASES = 1024;
constexpr size_t MAX_CONSTANTS = 1024;
constexpr size_t MAX_CASES = 128;
constexpr size_t MAX_NESTING = 128;
constexpr size_t MAX_OPERAND_STACK = 32;
constexpr size_t MAX_ANALYSIS_STACK = 1000;

constexpr uint32_t ELF_START = 0x10000;
constexpr size_t PTR_SIZE = 4;
constexpr int REG_CNT = 8;
} // namespace config
} // namespace aaac

#endif // AAAC_MACRO_H
