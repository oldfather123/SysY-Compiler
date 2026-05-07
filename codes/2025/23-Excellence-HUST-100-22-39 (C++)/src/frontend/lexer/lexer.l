%option noyywrap

%{
#include <string>
#include "ast.hpp"
#include "parser.hpp"
int yycolumn=1;
#define YY_USER_ACTION \
    yylloc.first_line = yylloc.last_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;
%}

%option yylineno

ID [a-z_A-Z][a-z_A-Z0-9]*
INT ([1-9][0-9]*|0[0-7]*|(0x|0X)[0-9a-fA-F]+)
DEC_EXP ([Ee][-+]?[0-9]+)
DEC_FLOAT_HAVE_POINT_HAVE_RIGHT ([0-9]*\.[0-9]+{DEC_EXP}?)
DEC_FLOAT_HAVE_POINT_NO_RIGHT ([0-9]+\.{DEC_EXP}?)
DEC_FLOAT_NO_POINT ([0-9]+{DEC_EXP})
DEC_FLOAT ({DEC_FLOAT_HAVE_POINT_HAVE_RIGHT}|{DEC_FLOAT_HAVE_POINT_NO_RIGHT}|{DEC_FLOAT_NO_POINT})
HEX_EXP ([Pp][-+]?[0-9]+)
HEX_FLOAT_HAVE_POINT_HAVE_RIGHT ((0[xX])[a-fA-F0-9]*\.[a-fA-F0-9]+{HEX_EXP})
HEX_FLOAT_HAVE_POINT_NO_RIGHT ((0[xX])[a-fA-F0-9]+\.{HEX_EXP})
HEX_FLOAT_NO_POINT ((0[xX])[a-fA-F0-9]+{HEX_EXP})
HEX_FLOAT ({HEX_FLOAT_HAVE_POINT_HAVE_RIGHT}|{HEX_FLOAT_HAVE_POINT_NO_RIGHT}|{HEX_FLOAT_NO_POINT})
FLOAT ({DEC_FLOAT}|{HEX_FLOAT})
SingleLineComment "//".*
MultilineComment "/*"([^\*]|(\*)*[^\*/])*(\*)*"*/"

%%

{INT}               { yylval.int_val = strtol(yytext, nullptr, 0); return INT; }
{FLOAT}             { yylval.float_val = strtof(yytext, nullptr); return FLOAT; }
"int"               { return INTTYPE; }
"float"             { return FLOATTYPE; }
"void"              { return VOID; }
"const"             { return CONST; }
"return"            { return RETURN; }
"if"                { return IF; }
"else"              { return ELSE; }
"while"             { return WHILE; }
"break"             { return BREAK; }
"continue"          { return CONTINUE; }
{ID}                { yylval.token = new string(yytext); return ID; }
">="                { return GTE; }
"<="                { return LTE; }
">"                 { return GT; }
"<"                 { return LT; }
"=="                { return EQ; }
"!="                { return NEQ; }
"("                 { return LP; }
")"                 { return RP; }
"["                 { return LB; }
"]"                 { return RB; }
"{"                 { return LC; }
"}"                 { return RC; }
","                 { return COMMA; }
";"                 { return SEMICOLON; }
"!"                 { return NOT; }
"="                 { return ASSIGN; }
"-"                 { return MINUS; }
"+"                 { return ADD; }
"*"                 { return MUL; }
"/"                 { return DIV; }
"%"                 { return MOD; }
"&&"                { return AND; }
"||"                { return OR; }
[\n]                { yycolumn = 1; }
[ \r\t]             { /* Ignore useless whitespaces */ }
{SingleLineComment} { /* Ignore Comments */ }
{MultilineComment}  { /* Ignore Comments */ }
.                   { printf("Error type A :Mysterious character \"%s\"\n\t at Line %d\n", yytext, yylineno); }

%%