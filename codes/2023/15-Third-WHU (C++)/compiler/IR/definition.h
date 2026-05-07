#pragma once
#ifndef DEF_H
#define DEF_H

//所有opnode可能出现的名字（除了函数调用）
//运算符
#define PLUS "43"    //+
#define SUBTRACT "45"//-
#define MULTIPLY "42"  //*
#define DIVIDE "47"    // "/"
#define GREATER_THAN "62" //>
#define GREATER_EQUAL "262" //>=
#define LESS_THAN "60"    //<  
#define LESS_EQUAL "261"  //<=
#define EQUAL "259"		//== 
#define NOT_EQUAL "260"    //!=
#define LOGICAL_NOT "33"    //!
#define LOGICAL_AND "257"  //&&
#define LOGICAL_OR "258"   // ||
#define REMAINDER "37"     //%

//特殊符号
#define RETURN_NODE "%ret"
#define BRANCH_NODE "%branch"
#define ARRAY_INIT_NODE "%initlist"
#define IMPLICIT_INIT_NODE "%impli_init"
#define ARRAYSUB_NODE "%array_sub"
#define INT_TO_FLOAT "%i32Tf32"
#define FLOAT_TO_INT "%f32Ti32"
#define ARRAY_TO_POINT "%arrayTpoint"

//库函数
#define GET_INT "getint"
#define GET_CH "getch"
#define GET_FLOAT "getfloat"
#define GET_ARRAY "getarray"
#define GET_F_ARRAY "getfarray"
#define PUT_INT "putint"
#define PUT_CHAR "putch"
#define PUT_FLOAT "putfloat"
#define PUT_ARRAY "putarray"
#define PUT_F_ARRAY "putfarray"
#define PUT_F "putf"
#define START_TIME "starttime"
#define STOP_TIME "stoptime"


//store节点的左右
#define sd_variableName 0
#define sd_assignment 1
#endif // !DEF_H

