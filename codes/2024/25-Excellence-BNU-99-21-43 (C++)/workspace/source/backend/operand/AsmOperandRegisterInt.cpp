#include "AsmOperandRegisterInt.h"
using namespace Backend;

std::unordered_map<int, AsmOperandRegisterInt *> AsmOperandRegisterInt::cache;

AsmOperandRegisterInt *AsmOperandRegisterInt::S0 = AsmOperandRegisterInt::get(8);
AsmOperandRegisterInt *AsmOperandRegisterInt::S1 = AsmOperandRegisterInt::get(9);
AsmOperandRegisterInt *AsmOperandRegisterInt::ZERO = AsmOperandRegisterInt::get(0);
AsmOperandRegisterInt *AsmOperandRegisterInt::RA = AsmOperandRegisterInt::get(1);
AsmOperandRegisterInt *AsmOperandRegisterInt::SP = AsmOperandRegisterInt::get(2);
AsmOperandRegisterInt *AsmOperandRegisterInt::T0 = AsmOperandRegisterInt::get(5);
