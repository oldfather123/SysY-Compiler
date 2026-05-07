#pragma once
#ifndef RV_Register_H
#define RV_Register_H

#include <string>

//寄存器种类
enum RegUsage { caller, callee, other };

const int icaller = 15;
//const int icaller = 8;
const int icallee = 11;//不包括s0
const int fcaller = 20;
const int fcallee = 12;

//RISC-V32个整形寄存器
const int x0 = 0;			//Hardwired zero
const int x1 = 1;			//Return address
const int x2 = 2;			//Stack pointer
const int x3 = 3;			//Global pointer
const int x4 = 4;			//Thread pointer
const int x5 = 5;			//Temporary
const int x6 = 6;			//Temporary
const int x7 = 7;			//Temporary
const int x8 = 8;			//Saved register
const int x9 = 9;			//Saved register, frame pointer
const int x10 = 10;			//Function argument, return value
const int x11 = 11;			//Function argument, return value
const int x12 = 12;			//Function argument
const int x13 = 13;			//Function argument
const int x14 = 14;			//Function argument
const int x15 = 15;			//Function argument
const int x16 = 16;			//Function argument
const int x17 = 17;			//Function argument
const int x18 = 18;			//Saved register
const int x19 = 19;			//Saved register
const int x20 = 20;			//Saved register
const int x21 = 21;			//Saved register
const int x22 = 22;			//Saved register
const int x23 = 23;			//Saved register
const int x24 = 24;			//Saved register
const int x25 = 25;			//Saved register
const int x26 = 26;			//Saved register
const int x27 = 27;			//Saved register
const int x28 = 28;			//Temporary
const int x29 = 29;			//Temporary
const int x30 = 30;			//Temporary
const int x31 = 31;			//Temporary

//RISC-V整形寄存器别名
const int zero = x0;		//Hardwired zero
const int ra = x1;			//Return address
const int sp = x2;			//Stack pointer
const int gp = x3;			//Global pointer
const int tp = x4;			//Thread pointer
const int fp = x8;			//frame pointer
const int s0 = x8;			//Saved register
const int s1 = x9;			//Saved register
const int s2 = x18;			//Saved register
const int s3 = x19;			//Saved register
const int s4 = x20;			//Saved register
const int s5 = x21;			//Saved register
const int s6 = x22;			//Saved register
const int s7 = x23;			//Saved register
const int s8 = x24;			//Saved register
const int s9 = x25;			//Saved register
const int s10 = x26;		//Saved register
const int s11 = x27;		//Saved register
const int t0 = x5;			//Temporary
const int t1 = x6;			//Temporary
const int t2 = x7;			//Temporary
const int t3 = x28;			//Temporary
const int t4 = x29;			//Temporary
const int t5 = x30;			//Temporary
const int t6 = x31;			//Temporary
const int a0 = x10;			//Function argument, return value
const int a1 = x11;			//Function argument, return value
const int a2 = x12;			//Function argument
const int a3 = x13;			//Function argument
const int a4 = x14;			//Function argument
const int a5 = x15;			//Function argument
const int a6 = x16;			//Function argument
const int a7 = x17;			//Function argument
const int x32 = 32;			//program count
const int pc = x32;			//program count

const int RegQuan_I = 33;	//Quantity of registers

//寄存器名称
const std::string REG_I[RegQuan_I] = {
	"zero", "ra", "sp", "gp", "tp",										//x0-x4
	"t0", "t1", "t2", "s0", "s1",										//x5-x9
	"a0", "a1", "a2", "a3", "a4", "a5", "a6", "a7",						//x10-x17
	"s2", "s3", "s4", "s5", "s6", "s7", "s8", "s9", "s10", "s11",		//x18-x27
	"t3", "t4", "t5", "t6", "pc"										//x28-x31，pc
};


//标志I类型寄存器保存者种类数组
const RegUsage REG_USAGE_I[RegQuan_I] = {
	other, caller, callee, other,
	other, caller, caller, caller,
	callee, callee, caller, caller,
	caller, caller, caller, caller,
	caller, caller, callee, callee,
	callee, callee, callee, callee,
	callee, callee, callee, callee,
	caller, caller, caller, caller
};


//RISC_V32个浮点寄存器
const int f0 = 0;			//FP Temporary
const int f1 = 1;			//FP Temporary
const int f2 = 2;			//FP Temporary
const int f3 = 3;			//FP Temporary
const int f4 = 4;			//FP Temporary
const int f5 = 5;			//FP Temporary
const int f6 = 6;			//FP Temporary
const int f7 = 7;			//FP Temporary
const int f8 = 8;			//FP Saved register
const int f9 = 9;			//FP Saved register
const int f10 = 10;			//FP Function argument, return value
const int f11 = 11;			//FP Function argument, return value
const int f12 = 12;			//FP Function argument
const int f13 = 13;			//FP Function argument
const int f14 = 14;			//FP Function argument
const int f15 = 15;			//FP Function argument
const int f16 = 16;			//FP Function argument
const int f17 = 17;			//FP Function argument
const int f18 = 18;			//FP Saved register
const int f19 = 19;			//FP Saved register
const int f20 = 20;			//FP Saved register
const int f21 = 21;			//FP Saved register
const int f22 = 22;			//FP Saved register
const int f23 = 23;			//FP Saved register
const int f24 = 24;			//FP Saved register
const int f25 = 25;			//FP Saved register
const int f26 = 26;			//FP Saved register
const int f27 = 27;			//FP Saved register
const int f28 = 28;			//FP Temporary
const int f29 = 29;			//FP Temporary	
const int f30 = 30;			//FP Temporary
const int f31 = 31;			//FP Temporary

//RISC_V32个浮点寄存器别名
const int ft0 = f0;			//FP Temporary
const int ft1 = f1;			//FP Temporary
const int ft2 = f2;			//FP Temporary
const int ft3 = f3;			//FP Temporary
const int ft4 = f4;			//FP Temporary
const int ft5 = f5;			//FP Temporary
const int ft6 = f6;			//FP Temporary
const int ft7 = f7;			//FP Temporary
const int ft8 = f28;		//FP Temporary
const int ft9 = f29;		//FP Temporary
const int ft10 = f30;		//FP Temporary
const int ft11 = f31;		//FP Temporary 
const int fs0 = f8;			//FP Saved register
const int fs1 = f9;			//FP Saved register
const int fa0 = f10;		//FP Function argument, return value
const int fa1 = f11;		//FP Function argument, return value
const int fa2 = f12;		//FP Function argument
const int fa3 = f13;		//FP Function argument
const int fa4 = f14;		//FP Function argument
const int fa5 = f15;		//FP Function argument
const int fa6 = f16;		//FP Function argument
const int fa7 = f17;		//FP Function argument
const int fs2 = f18;		//FP Saved register
const int fs3 = f19;		//FP Saved register
const int fs4 = f20;		//FP Saved register
const int fs5 = f21;		//FP Saved register
const int fs6 = f22;		//FP Saved register
const int fs7 = f23;		//FP Saved register
const int fs8 = f24;		//FP Saved register
const int fs9 = f25;		//FP Saved register
const int fs10 = f26;		//FP Saved register
const int fs11 = f27;		//FP Saved register

const int RegQuan_F = 32;	//Quantity of registers

const std::string REG_F[RegQuan_F] = {
	"ft0", "ft1", "ft2", "ft3", "ft4", "ft5", "ft6", "ft7", 
	"fs0", "fs1", 
	"fa0", "fa1", "fa2", "fa3", "fa4", "fa5", "fa6", "fa7",
	"fs2", "fs3", "fs4", "fs5", "fs6", "fs7", "fs8", "fs9", "fs10", "fs11",
	"ft8", "ft9", "ft10", "ft11"
};

const RegUsage REG_USAGE_F[RegQuan_F] = {
	caller, caller, caller, caller, caller, caller, caller, caller,
	callee, callee,
	caller, caller, caller, caller, caller, caller, caller, caller, 
	callee, callee, callee, callee, callee, callee, callee, callee, callee, callee,
	caller, caller, caller, caller
};
	
//const int reg_I = 0;
//const int reg_F = 1;
//int allocateRegister(int regtype);


#endif