%option noyywrap
%{
#include <cstdlib>
#include <string>
#include <cctype>
#include <cstring> 
#include "../include/symtab.h"
#include "../parser/parser.tab.h"
#include <stdio.h>

extern int line_number;  
extern int col_number;   
extern int cur_col_number;
void yyerror(char* s, ...);
std::string tmp_str;
%}

%x COMMENT
%x STRING

digit        [0-9]
nonzero      [1-9]
octal        [0-7]
hexdigit     [0-9a-fA-F]
exponent     [eE][\+\-]?{digit}+
bin_exponent [pP][\+\-]?{digit}+

float1       {digit}*\.{digit}+{exponent}?
float2       {digit}+\.{exponent}?
float3       {digit}+{exponent}
hex_float    0[xX]({hexdigit}+\.?|{hexdigit}*\.{hexdigit}+){bin_exponent}

%%

"//".* {
    cur_col_number = col_number;
    col_number += strlen(yytext);
}

"/*" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    BEGIN(COMMENT);
}

<COMMENT>{
    "*/" {
        cur_col_number = col_number;
        col_number += strlen(yytext);
        BEGIN(INITIAL);
    }
    \n {
        ++line_number;
        col_number = 1;
    }
    <<EOF>> {
        printf("Line %d, Column %d: EOF in comment\n", line_number, cur_col_number);
        return ERROR;
    }
    . {
        cur_col_number = col_number;
        col_number += strlen(yytext);
    }
}

"+"      { cur_col_number = col_number; col_number += strlen(yytext); return PLUS; }
"-"      { cur_col_number = col_number; col_number += strlen(yytext); return MINUS; }
"*"      { cur_col_number = col_number; col_number += strlen(yytext); return MUL; }
"/"      { cur_col_number = col_number; col_number += strlen(yytext); return DIV; }
"="      { cur_col_number = col_number; col_number += strlen(yytext); return ASSIGN; }
"!"      { cur_col_number = col_number; col_number += strlen(yytext); return NOT; }
"%"      { cur_col_number = col_number; col_number += strlen(yytext); return MOD; }
"<"      { cur_col_number = col_number; col_number += strlen(yytext); return LT; }
">"      { cur_col_number = col_number; col_number += strlen(yytext); return GT; }
"("      { cur_col_number = col_number; col_number += strlen(yytext); return LPAREN; }
")"      { cur_col_number = col_number; col_number += strlen(yytext); return RPAREN; }
","      { cur_col_number = col_number; col_number += strlen(yytext); return COMMA; }
";"      { cur_col_number = col_number; col_number += strlen(yytext); return SEMI; }
"["      { cur_col_number = col_number; col_number += strlen(yytext); return LBRACKET; }
"]"      { cur_col_number = col_number; col_number += strlen(yytext); return RBRACKET; }
"{"      { cur_col_number = col_number; col_number += strlen(yytext); return LBRACE; }
"}"      { cur_col_number = col_number; col_number += strlen(yytext); return RBRACE; }

"<="     { cur_col_number = col_number; col_number += strlen(yytext); return LEQ; }
">="     { cur_col_number = col_number; col_number += strlen(yytext); return GEQ; }
"=="     { cur_col_number = col_number; col_number += strlen(yytext); return EQ; }
"!="     { cur_col_number = col_number; col_number += strlen(yytext); return NE; }
"&&"     { cur_col_number = col_number; col_number += strlen(yytext); return AND; }
"||"     { cur_col_number = col_number; col_number += strlen(yytext); return OR; }

"const"     { cur_col_number = col_number; col_number += strlen(yytext); return CONST; }
"if"        { cur_col_number = col_number; col_number += strlen(yytext); return IF; }
"else"      { cur_col_number = col_number; col_number += strlen(yytext); return ELSE; }
"while"     { cur_col_number = col_number; col_number += strlen(yytext); return WHILE; }
"void"      { cur_col_number = col_number; col_number += strlen(yytext); return VOID; }
"int"       { cur_col_number = col_number; col_number += strlen(yytext); return INT; }
"float"     { cur_col_number = col_number; col_number += strlen(yytext); return FLOAT; }
"return"    { cur_col_number = col_number; col_number += strlen(yytext); return RETURN; }
"break"     { cur_col_number = col_number; col_number += strlen(yytext); return BREAK; }
"continue"  { cur_col_number = col_number; col_number += strlen(yytext); return CONTINUE; }

\n          { ++line_number; col_number = 1; }
[ \t\f\r\v] { col_number += strlen(yytext); }

[_a-zA-Z][_a-zA-Z0-9]* {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.str_val = strdup(yytext); 
    return IDENT;
}

{float1}|{float2}|{float3} {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.float_val = strtod(yytext, NULL);
    return FLOAT_CONST;
}

{hex_float} {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.float_val = strtod(yytext, NULL); 
    return FLOAT_CONST;
}

"0"[xX]{hexdigit}+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.int_val = strtol(yytext, NULL, 16);
    return INT_CONST;
}

"0"[0-7]+ {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.int_val = strtol(yytext, NULL, 8);
    return INT_CONST;
}

{nonzero}{digit}* {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.int_val = strtol(yytext, NULL, 10);
    return INT_CONST;
}

"0" {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    yylval.int_val = 0;
    return INT_CONST;
}

\" {
    cur_col_number = col_number;  // 添加位置记录
    col_number += strlen(yytext);
    tmp_str.clear();
    BEGIN(STRING);
}

<STRING>{
    \" {
        col_number += strlen(yytext);
        yylval.str_val = strdup(tmp_str.c_str());
        BEGIN(INITIAL);
        return STR_CONST;
    }
    \\n     { tmp_str += '\n'; col_number += 2; }
    \\t     { tmp_str += '\t'; col_number += 2; }
    \\\"    { tmp_str += '\"'; col_number += 2; }
    \\\\    { tmp_str += '\\'; col_number += 2; }
    \\r     { tmp_str += '\r'; col_number += 2; }
    \\a     { tmp_str += '\a'; col_number += 2; }
    \\b     { tmp_str += '\b'; col_number += 2; }
    \\f     { tmp_str += '\f'; col_number += 2; }
    \\v     { tmp_str += '\v'; col_number += 2; }

    \\[^nt\"\\rabfv] {
        col_number += 2;
        BEGIN(INITIAL);
        return ERROR;
    }

    \n {
        printf("Line %d, Column %d: Unterminated string\n", line_number, cur_col_number);
        ++line_number;
        col_number = 1;
        BEGIN(INITIAL);
        return ERROR;
    }

    <<EOF>> {
        printf("Line %d, Column %d: Unterminated string at EOF\n", line_number, cur_col_number);
        BEGIN(INITIAL);
        return ERROR;
    }

    . {
        tmp_str.push_back(yytext[0]);
        col_number += 1;
    }
}

. {
    cur_col_number = col_number;
    col_number += strlen(yytext);
    printf("Line %d, Column %d: Unexpected character: %c\n", line_number, cur_col_number, yytext[0]);
    return ERROR;
}
%%
