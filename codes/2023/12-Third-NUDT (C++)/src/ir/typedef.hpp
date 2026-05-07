#ifndef __TYPEDEF_HPP__
#define __TYPEDEF_HPP__

#include <bits/stdc++.h>

using std::string_view;

namespace IR {

using BType = enum BType : size_t {
  VOID,
  CHAR,
  I1,
  I32,
  FLOAT,
  LABEL,
  POINTER,
  ARRAY,
  FUNCTION,
  DYNAMIC,   // only for dynamic args in function decl
  UNDEFINE,  // undifine
};

using IType = enum IType : size_t {
  GLOBAL,
  RET,
  JMP,
  BR,
  FNEG,
  ADD,
  FADD,
  SUB,
  FSUB,
  MUL,
  FMUL,
  SDIV,
  FDIV,
  SREM,
  SHL,
  LSHR,
  ASHR,
  AND,
  OR,
  XOR,
  ALLOCA,
  LOAD,
  STORE,
  GETELEMENTPTR,
  TRUNC,
  ZEXT,
  SEXT,
  FPTRUNC,
  FPEXT,
  FPTOSI,
  SITOFP,
  PTRTOINT,
  INTTOPTR,
  IEQ,
  INE,
  ISGT,
  ISGE,
  ISLT,
  ISLE,
  FOEQ,
  FONE,
  FOGT,
  FOGE,
  FOLT,
  FOLE,
  PHI,
  CALL,
};

constexpr string_view ITypeName[] = {
    "global",   "ret",      "br",       "br",       "fneg",     "add",      "fadd",     "sub",
    "fsub",     "mul",      "fmul",     "sdiv",     "fdiv",     "srem",     "shl",      "lshr",
    "ashr",     "and",      "or",       "xor",      "alloca",   "load",     "store",    "getelementptr",
    "trunc",    "zext",     "sext",     "fptrunc",  "fpext",    "fptosi",   "sitofp",   "ptrtoint",
    "inttoptr", "icmp eq",  "icmp ne",  "icmp sgt", "icmp sge", "icmp slt", "icmp sle", "fcmp oeq",
    "fcmp one", "fcmp ogt", "fcmp oge", "fcmp olt", "fcmp ole", "phi",      "call",
};

}  // namespace IR

#endif
