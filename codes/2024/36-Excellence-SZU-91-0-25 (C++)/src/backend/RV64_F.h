#pragma once

#include "../utility/System.h"

//Arithmetic
string fadd_s(string reg_des_f,string reg_src1_f,string reg_src2_f);
string fsub_s(string reg_des_f,string reg_src1_f,string reg_src2_f);
string fmul_s(string reg_des_f,string reg_src1_f,string reg_src2_f);
string fdiv_s(string reg_des_f,string reg_src1_f,string reg_src2_f);

string fabs_s(string reg_des_f,string reg_src_f);//[Pseudo]
string fneg_s(string reg_des_f,string reg_src_f);//[Pseudo]
string fsqrt_s(string reg_des_f,string reg_src_f);

string fmax_s(string reg_des_f,string reg_src1_f,string reg_src2_f);
string fmin_s(string reg_des_f,string reg_src1_f,string reg_src2_f);

string fmadd_s(string reg_des_f,string reg_src1_f,string reg_src2_f,string reg_src3_f); //d=a*b+c
string fnmadd_s(string reg_des_f,string reg_src1_f,string reg_src2_f,string reg_src3_f);//d=-(a*b+c)
string fmsub_s(string reg_des_f,string reg_src1_f,string reg_src2_f,string reg_src3_f); //d=a*b-c
string fnmsub_s(string reg_des_f,string reg_src1_f,string reg_src2_f,string reg_src3_f);//d=-(a*b-c)


//Compare arithmetic
string feq_s(string reg_des_i,string reg_src1_f,string reg_src2_f);//==
string fle_s(string reg_des_i,string reg_src1_f,string reg_src2_f);//<=
string flt_s(string reg_des_i,string reg_src1_f,string reg_src2_f);//<


//Convert
string fcvt_s_w(string reg_des_f,string reg_src1_i);//i32 => f32
string fcvt_w_s(string reg_des_i,string reg_src1_f);//f32 => i32


//Memory
string flw(string reg_des_f,int offset,string reg_src_i);
string fsw(string reg_src1_f,int offset,string reg_src2_i);


//Move
string fmv_s(string reg_des_f,string reg_src_f);  //move f32 to f32 [Pseudo]
string fmv_w_x(string reg_des_f,string reg_src_i);//move i32 to f32 
string fmv_x_w(string reg_des_i,string reg_src_f);//move f32 to i32 
