#include "../backend/RV64_F.h"
#include "../backend/RV64_I.h"

//Arithmetic
string fadd_s(string reg_des_f,string reg_src1_f,string reg_src2_f)
{
    return "fadd.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f;
}

string fsub_s(string reg_des_f,string reg_src1_f,string reg_src2_f)
{
    return "fsub.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f;
}

string fmul_s(string reg_des_f,string reg_src1_f,string reg_src2_f)
{
    return "fmul.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f;
}

string fdiv_s(string reg_des_f,string reg_src1_f,string reg_src2_f)
{
    return "fdiv.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f;
}


string fabs_s(string reg_des_f,string reg_src_f)//[Pseudo]
{
    return "fabs.s "+reg_des_f+","+reg_src_f;
}

string fneg_s(string reg_des_f,string reg_src_f)//[Pseudo]
{
    return "fneg.s "+reg_des_f+","+reg_src_f;
}

string fsqrt_s(string reg_des_f,string reg_src_f)
{
    return "fsqrt.s "+reg_des_f+","+reg_src_f;
}


string fmax_s(string reg_des_f,string reg_src1_f,string reg_src2_f)
{
    return "fmax.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f;
}

string fmin_s(string reg_des_f,string reg_src1_f,string reg_src2_f)
{
    return "fmin.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f;
}


string fmadd_s(string reg_des_f,string reg_src1_f,string reg_src2_f,string reg_src3_f) //d=a*b+c
{
    return "fmadd.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f+","+reg_src3_f;
}

string fnmadd_s(string reg_des_f,string reg_src1_f,string reg_src2_f,string reg_src3_f)//d=-(a*b+c)
{
    return "fnmadd.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f+","+reg_src3_f;
}

string fmsub_s(string reg_des_f,string reg_src1_f,string reg_src2_f,string reg_src3_f) //d=a*b-c
{
    return "fmsub.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f+","+reg_src3_f;
}

string fnmsub_s(string reg_des_f,string reg_src1_f,string reg_src2_f,string reg_src3_f)//d=-(a*b-c)
{
    return "fnmsub.s "+reg_des_f+","+reg_src1_f+","+reg_src2_f+","+reg_src3_f;
}



//Compare arithmetic
string feq_s(string reg_des_i,string reg_src1_f,string reg_src2_f)//==
{
    return "feq.s "+reg_des_i+","+reg_src1_f+","+reg_src2_f;
}

string fle_s(string reg_des_i,string reg_src1_f,string reg_src2_f)//<=
{
    return "fle.s "+reg_des_i+","+reg_src1_f+","+reg_src2_f;
}

string flt_s(string reg_des_i,string reg_src1_f,string reg_src2_f)//<
{
    return "flt.s "+reg_des_i+","+reg_src1_f+","+reg_src2_f;
}



//Convert
string fcvt_s_w(string reg_des_f,string reg_src1_i)//i32 => f32
{
    return "fcvt.s.w "+reg_des_f+","+reg_src1_i;
}

string fcvt_w_s(string reg_des_i,string reg_src1_f)//f32 => i32
{
    return "fcvt.w.s "+reg_des_i+","+reg_src1_f;
}



//Memory
string flw(string reg_des_f,int offset,string reg_src_i)
{
    if(abs(offset)<1000)
        return "flw "+reg_des_f+","+to_string(offset)+"("+reg_src_i+")";
        
    string flw_str;
    flw_str+=li("t6",to_string(offset))+"\n\t";
    flw_str+=add("t6","t6",reg_src_i)+"\n\t";
    flw_str+="flw "+reg_des_f+",0(t6)";
    return flw_str;
}

string fsw(string reg_src1_f,int offset,string reg_src2_i)
{
    if(abs(offset)<1000)
        return "fsw "+reg_src1_f+","+to_string(offset)+"("+reg_src2_i+")";
    
    string fsw_str;
    fsw_str+=li("t6",to_string(offset))+"\n\t";
    fsw_str+=add("t6","t6",reg_src2_i)+"\n\t";
    fsw_str+="fsw "+reg_src1_f+",0(t6)";
    return fsw_str;
}



//Move
string fmv_s(string reg_des_f,string reg_src_f)  //move f32 to f32 [Pseudo]
{
    return "fmv.s "+reg_des_f+","+reg_src_f;
}

string fmv_w_x(string reg_des_f,string reg_src_i)//move i32 to f32 [Pseudo]
{
    return "fmv.w.x "+reg_des_f+","+reg_src_i;
}

string fmv_x_w(string reg_des_i,string reg_src_f)//move f32 to i32 [Pseudo]
{
    return "fmv.x.w "+reg_des_i+","+reg_src_f;
}
