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

void pass_node(char *text){
     yylval.node = new_syntax_tree_node(text);
}
static void yyunput ( int c, char *buf_ptr  );
void unput_str(char *str){
     int len = strlen(str);
     while(len > 0){
          len --;
          unput(str[len]);
     }
}
/*****************声明和选项设置  end*****************/

%}

nondigit       [a-zA-Z_]
digit          [0-9]
nonzerodigit   [1-9]
octaldigit     [0-7]
hexdigit       [0-9a-fA-F]

sign           [+-]
exponent       ([eE]{sign}?{digit}+)
fraction       ({digit}*\.{digit}+|{digit}+\.)

bin_exponent   ([pP]{sign}?{digit}+)
hex_fraction   ({hexdigit}*\.{hexdigit}+|{hexdigit}+\.)

%x comment
%%

__LINE__       { char s[10]; sprintf(s, "%d", lines); unput_str(s); }
stoptime\(\)   { unput_str("_sysy_stoptime(__LINE__)"); }
starttime\(\)  { unput_str("_sysy_starttime(__LINE__)"); }

\+ 	{pos_start = pos_end; pos_end ++; pass_node(yytext); return ADD;}
-    {pos_start = pos_end; pos_end ++; pass_node(yytext); return SUB;}
\*   {pos_start = pos_end; pos_end ++; pass_node(yytext); return MUL;}
\/   {pos_start = pos_end; pos_end ++; pass_node(yytext); return DIV;}
\%   {pos_start = pos_end; pos_end ++; pass_node(yytext); return MOD;} 
&&   {pos_start = pos_end; pos_end += 2; pass_node(yytext); return AND;}
\|\| {pos_start = pos_end; pos_end += 2; pass_node(yytext); return OR;}
!    {pos_start = pos_end; pos_end ++; pass_node(yytext); return NOT;}
\(   {pos_start = pos_end; pos_end ++; pass_node(yytext); return LP;}
\)   {pos_start = pos_end; pos_end ++; pass_node(yytext); return RP;}
\<=  {pos_start = pos_end; pos_end += 2; pass_node(yytext); return LE;}
\>=  {pos_start = pos_end; pos_end += 2; pass_node(yytext); return GE;}
\<   {pos_start = pos_end; pos_end ++; pass_node(yytext); return LT;}
\>   {pos_start = pos_end; pos_end ++; pass_node(yytext); return GT;}
==   {pos_start = pos_end; pos_end += 2; pass_node(yytext); return EQ;}
!=   {pos_start = pos_end; pos_end += 2; pass_node(yytext); return NE;}
;    {pos_start = pos_end; pos_end ++; pass_node(yytext); return SEMI;}
=    {pos_start = pos_end; pos_end ++; pass_node(yytext); return ASSIGN;}
,    {pos_start = pos_end; pos_end ++; pass_node(yytext); return COMMA;}
\{   {pos_start = pos_end; pos_end ++; pass_node(yytext); return LBRACE;}
\}   {pos_start = pos_end; pos_end ++; pass_node(yytext); return RBRACE;}
\[   {pos_start = pos_end; pos_end ++; pass_node(yytext); return LBRACK;}
\]   {pos_start = pos_end; pos_end ++; pass_node(yytext); return RBRACK;}

else      {pos_start = pos_end; pos_end += 4; pass_node(yytext); return ELSE;}
if        {pos_start = pos_end; pos_end += 2; pass_node(yytext); return IF;}
int       {pos_start = pos_end; pos_end += 3; pass_node(yytext); return INT;}
return    {pos_start = pos_end; pos_end += 6; pass_node(yytext); return RETURN;}
void      {pos_start = pos_end; pos_end += 4; pass_node(yytext); return VOID;}
while     {pos_start = pos_end; pos_end += 5; pass_node(yytext); return WHILE;}
float     {pos_start = pos_end; pos_end += 5; pass_node(yytext); return FLOAT;}
const     {pos_start = pos_end; pos_end += 5; pass_node(yytext); return CONST;}
break     {pos_start = pos_end; pos_end += 5; pass_node(yytext); return BREAK;}
continue  {pos_start = pos_end; pos_end += 8; pass_node(yytext); return CONTINUE;}

{nonzerodigit}{digit}*   {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTCONST;}
0{octaldigit}*           {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTCONST;}
0[xX]{hexdigit}+         {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return INTCONST;}

{fraction}{exponent}?    {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATCONST;}
{digit}+{exponent}       {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATCONST;}
0[xX]{hex_fraction}{bin_exponent}? {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATCONST;}
0[xX]{hexdigit}+{bin_exponent}    {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return FLOATCONST;}

{nondigit}({nondigit}|{digit})* {pos_start = pos_end; pos_end += strlen(yytext); pass_node(yytext); return IDENT;}

" "|\t              { pos_start = pos_end; pos_end++; }
\n|\r|\n\r          { pos_start = pos_end = 1; lines++; }
\/\/.*(\n|\r|\n\r)? {pos_start = pos_end = 1; lines ++; }

 /*using start condition to deal with the comments*/
\/\*                     { BEGIN(comment); }
<comment>[^*\n\r]*       { }
<comment>\*+[^*/\n\r]*   { }
<comment>\n|\r|\n\r      lines ++;
<comment>\*\/            BEGIN(INITIAL);

. { pos_start = pos_end; pos_end++; return ERROR; }

%%
