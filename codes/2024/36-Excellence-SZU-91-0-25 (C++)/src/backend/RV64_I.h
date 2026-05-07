#pragma once

#include "../utility/System.h"

//Arithmetic
string addw(string reg_des_i,string reg_src1_i,string reg_src2_i);
string addwi(string reg_des_i,string reg_src_i,int immediate);
string add(string reg_des_i,string reg_src1_i,string reg_src2_i);
string addi(string reg_des_i,string reg_src_i,int immediate);
string subw(string reg_des_i,string reg_src1_i,string reg_src2_i);
string mul(string reg_des_i,string reg_src1_i,string reg_src2_i);
string divw(string reg_des_i,string reg_src1_i,string reg_src2_i);
string remw(string reg_des_i,string reg_src1_i,string reg_src2_i);
string negw(string reg_des_i,string reg_src_i);//[Pseudo]


//Logic
string not_(string reg_des_i,string reg_src_i);
string and_(string reg_des_i,string reg_src1_i,string reg_src2_i);
string andi(string reg_des_i,string reg_src_i,int immediate);
string or_(string reg_des_i,string reg_src1_i,string reg_src2_i);
string ori(string reg_des_i,string reg_src_i,int immediate);
string xor_(string reg_des_i,string reg_src1_i,string reg_src2_i);
string xori(string reg_des_i,string reg_src_i,int immediate);


//Shift
string sllw(string reg_des_i,string reg_src1_i,string reg_src2_i);//<<(logic)
string slliw(string reg_des_i,string reg_src_i,string immediate);
string srlw(string reg_des_i,string reg_src1_i,string reg_src2_i);//>>(logic)
string srliw(string reg_des_i,string reg_src_i,string immediate);
string sraw(string reg_des_i,string reg_src1_i,string reg_src2_i);//>>(arithmetic)
string sraiw(string reg_des_i,string reg_src_i,string immediate);


//Compare arithmetic
string slt(string reg_des_i,string reg_src1_i,string reg_src2_i);//<
string slti(string reg_des_i,string reg_src_i,int immediate);
string seqz(string reg_des_i,string reg_src_i);//==0 [Pseudo]
string snez(string reg_des_i,string reg_src_i);//!=0 [Pseudo]
string sltz(string reg_des_i,string reg_src_i);//<0  [Pseudo]
string sgtz(string reg_des_i,string reg_src_i);//>0  [Pseudo]


//Compare branch
string beq(string reg_src1_i,string reg_src2_i,string lable);//== 
string bne(string reg_src1_i,string reg_src2_i,string lable);//!=
string bgt(string reg_src1_i,string reg_src2_i,string lable);//>
string bge(string reg_src1_i,string reg_src2_i,string lable);//>=
string blt(string reg_src1_i,string reg_src2_i,string lable);//<
string ble(string reg_src1_i,string reg_src2_i,string lable);//<=
string beqz(string reg_src_i,string lable);//==0 [Pseudo]
string bnez(string reg_src_i,string lable);//!=0 [Pseudo]
string bgtz(string reg_src_i,string lable);//>0  [Pseudo]
string bgez(string reg_src_i,string lable);//>=0 [Pseudo]
string bltz(string reg_src_i,string lable);//<0  [Pseudo]
string blez(string reg_src_i,string lable);//<=0 [Pseudo]


//Function
string call(string function);//[Pseudo]
string ret();//[Pseudo]
string tail();//[Pseudo]


//Jump
string j(string lable);//[Pseudo]


//Memory
string la(string reg_des_i,string variable);//[Pseudo]
string li(string reg_des_i,string immediate);//[Pseudo]
string lw(string reg_des_i,int offset,string reg_src_i);
string ld(string reg_des_i,int offset,string reg_src_i);
string sw(string reg_src1_i,int offset,string reg_src2_i);
string sd(string reg_src1_i,int offset,string reg_src2_i);


//Move
string mv(string reg_des_i,string reg_src_i);//[Pseudo]
