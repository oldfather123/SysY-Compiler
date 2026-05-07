%option noyywrap
%{
/*****************声明和选项设置  begin*****************/
#include <stdio.h>
#include <stdlib.h>

#include "syntax_tree.h"
#include "syntax_analyzer.h"

int lines = 1;
int pos_start = 1;
int pos_end = 1;

void pass_node(char * text) {
     yylval.node = new_syntax_tree_node(text);
}

static void yyunput ( int c, char *buf_ptr );

void unput_str(char *str){
     int len = strlen(str);
     while(len > 0){
          len --;
          unput(str[len]);
     }
}

/*****************声明和选项设置  end*****************/
%}

DecConst       [1-9][0-9]*
OctConst       0[0-7]*
HexConst       0[xX][0-9a-fA-F]+
DigitSeq       [0-9]+
ExpPart        [eE][+-]?{DigitSeq}
FracConst      {DigitSeq}?\.{DigitSeq}|{DigitSeq}\.
DecFloatConst  ({FracConst}|{DigitSeq}){ExpPart}?[fFlL]?
HexDigitSeq    [0-9a-fA-F]+
HexFracConst   {HexDigitSeq}?\.{HexDigitSeq}|{HexDigitSeq}\.
BinExpPart     [pP][+-]?{DigitSeq}
HexFloatConst  0[xX]({HexFracConst}|{HexDigitSeq}){BinExpPart}[fFlL]?

%x COMMENT

%%

__LINE__       { char s[10]; sprintf(s, "%d", lines); unput_str(s); }
stoptime\(\)   { unput_str("_sysy_stoptime(__LINE__)"); }
starttime\(\)  { unput_str("_sysy_starttime(__LINE__)"); }

\+ 	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Add; }
\-	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Sub; }
\*	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Mul; }
\/	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Div; }
%         { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Mod; }
\<	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LT; }
\<=	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LTE; }
\>	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return GT; }
\>=	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return GTE; }
==	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Eq; }
!=	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return NEq; }
!         { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Not; }
&&        { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return And; }
\|\|      { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Or; }
=	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Assign; }
;	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Semicolon; }
,	     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Comma; }
\(        { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LParenthese; }
\)        { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return RParenthese; }
\[        { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LBracket; }
\]        { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return RBracket; }
\{        { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return LBrace; }
\}        { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return RBrace; }
const     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Const; }
int       { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Int; }
float     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Float; }
if        { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return If; }
else      { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Else; }
return    { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Return; }
void      { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Void; }
while     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return While; }
break     { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Break; }
continue  { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Continue; }
[ ]       { pos_start = pos_end; pos_end += yyleng; }
\t        { pos_start = pos_end; pos_end += yyleng; }
\/\/.*    { pos_start = pos_end; pos_end += yyleng; }
\/\*      { pos_start = pos_end; pos_end += yyleng; BEGIN COMMENT; }
\r        { pos_start = 1; pos_end = 1; }
\n        { pos_start = 1; pos_end = 1; lines++; }
[_a-zA-Z][_a-zA-Z0-9]*   { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Ident; }
{DecConst}               { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return DecIntConst; }
{OctConst}               { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return OctIntConst; }
{HexConst}               { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return HexIntConst; }
{DecFloatConst}          { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return DecFloatConst; }
{HexFloatConst}          { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return HexFloatConst; }
<COMMENT>\*\/            { pos_start = pos_end; pos_end += yyleng; BEGIN INITIAL; }
<COMMENT>[ ]             { pos_start = pos_end; pos_end += yyleng; }
<COMMENT>\t              { pos_start = pos_end; pos_end += yyleng; }
<COMMENT>\r              { pos_start = 1; pos_end = 1; }
<COMMENT>\n              { pos_start = 1; pos_end = 1; lines++; }
<COMMENT>.               { pos_start = pos_end; pos_end += yyleng; }

.                        { pos_start = pos_end; pos_end += yyleng; pass_node(yytext); return Error; }

%%
