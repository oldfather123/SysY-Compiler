#include "Register.h"
#include "Instruction.h"
#include "Generate.h"

bool isLegalImm(InstrunicsTypes type, int imm) {
	switch (type) {
	case IType:
	case SType:
		//I、S类型立即数为12位
		if (imm <= 2047 && imm >= -2048) return true;
		break;
	case UType:
		//U类型立即数为20位
		if (imm <= 524287 && imm >= -524288) return true;
		break;
	case JType:
		//J类型立即数为21位,且末位为0
		if (imm <= 1048575 && imm >= -1048576 && imm % 2 == 0) return true;
		break;
	default:
		break;
	}
	return false;
}

int f32Toi32(float f) {
	return *((int*)&f);
}
