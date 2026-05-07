%{
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdbool.h>

#include "../include/lexer.h"
#include "../include/vector.h"
#include "../include/util.h"

char *CurString;

vector_template(tokenIdent *, idents);


tokenIdent *tokenMap;
size_t immInt;
float immFloat;

uint32_t    curId;
Token       curTok;

int yywrap(){
	return 1;
}


%}
/* TODO 科学计数法和复数complex number,没有处理注释 */

%option yylineno
%x NOTE
%x NOTES

%x STRING


/* 定义正则表达式 */
INTEGER     [1-9][0-9]*
HEX_float  ((0x)|(0X))[0-9a-zA-Z]*"."[0-9a-zA-Z]+(([+-]){number}+)?

/* 16进制 */
HEX_NUMBER  ((0x)|(0X))[0-9a-fA-F]+

number [0-9]
enumber ({number}\.|\.|\.|{number})+[eE](([+-])?{number}+)?
/* 注意[0-9]* ,.1这样的形式省略了0，例如-.1 */
FLOAT      [0-9]*\.[0-9]*
/* 2进制 */  
BIN_NUMBER  (0b)|(0B)[0-1]+



IDENTEFIER  [a-zA-Z_][a-zA-Z0-9_]*

/* 字符串 */
STRING_MARK   \"

%%




const {return TOK_CONST;}
int {return TOK_INT;}
float {return TOK_FLOAT;}
void {return TOK_VOID;}
if {return TOK_IF;}
return {return TOK_RETURN;}
break {return TOK_BREAK;}
continue {return TOK_CONTINUE;}
else {return TOK_ELSE;}
while {return TOK_WHILE;}

0[0-9]+ {
    immInt= strtol(yytext,NULL,8);
    printf("\n8进制%s\n",yytext);
    printf("\nimm %d",immInt);
    return TOK_IMM_INT;
}
0 {
    immInt = 0;
    return TOK_IMM_INT;
}
{HEX_float} {
    printf("HEX_float is %s\n",yytext);
    sscanf(yytext, "%a", &immFloat);
    // immFloat = atof(yytext);
    return TOK_IMM_FLOAT;
}
{HEX_NUMBER} {
    //十六进制数字要在十进制数字和标识符面前,否则0x会被识别为0和x
    immInt= strtol(yytext,NULL,16);
    printf("\n%s\n",yytext);
    printf("\nimm %d",immInt);
    return TOK_IMM_INT;
}

{INTEGER} {
    immInt = strtol(yytext,NULL,10);
    return TOK_IMM_INT;
}
{enumber} {
    printf("enumber is %s\n",yytext);
    immFloat = atof(yytext);
    return TOK_IMM_FLOAT;
}
{FLOAT} {
    printf("FLOAT is %s\n",yytext);
    immFloat = atof(yytext);
    return TOK_IMM_FLOAT;
}

{IDENTEFIER} {
    tokenIdent *ptr = defineToken(yytext,yyleng);
    curId = ptr->tok;
    return TOK_IDENT;
}


{BIN_NUMBER} {
    immInt = strtol(yytext,NULL,2);
    return TOK_IMM_INT;
}





\" {
    BEGIN(STRING);
    
}
<STRING>\" {
    BEGIN(0);
    yytext[yyleng -1] ='\0';
    CurString = strdup(yytext);
    return TOK_STRING;
} 
<STRING>. {
    yymore();
}

== { return TOK_Equal;/* != */}
!\= {return TOK_NotEq;}
\<\= { return TOK_LesEq;}
\>\= { return TOK_GreEq;}
\< { return TOK_Less;}
\> { return TOK_Greater;}
\= { return TOK_Assign;}
\! {return TOK_NOT;}
\|\| {return TOK_OR;}
\&\& {return TOK_AND;}

\+ {return TOK_Add;}
\- { return TOK_Sub;}
\* { return TOK_Mul;}
\% { return TOK_Mod;}
\/ { return TOK_Div;}


\( { return TOK_ParL;}
\) { return TOK_ParR;}
\[ { return TOK_SqurL;}
\] { return TOK_SqurR;}
\{ { return TOK_CurlyL;}
\} { return TOK_CurlyR;}
\. { return TOK_Dot;}
\; { return TOK_SemiColon;}
\, { return TOK_Comma;}

\n {
    if(yylineno < 150){
        fprintf(stderr,"l:%d",yylineno);
    }
}

" "|\t {

}

<<EOF>> {return TOK_EOF;}

\/\* {BEGIN(NOTES);}
<NOTES>\*\/ {BEGIN(0);} 
<NOTES>. 

\/\/ {BEGIN(NOTE);}
<NOTE>\n {BEGIN(0);} 
<NOTE>. 

%%


size_t identCounter;
tokenIdent *defineToken(char *str,size_t len){
    tokenIdent *t = NULL;

    HASH_FIND_STR(tokenMap, str, t);
    if (!t) {
        t = (tokenIdent *)sy_malloc(sizeof(tokenIdent));
        t->str = strdup(str);
        HASH_ADD_STR(tokenMap, str, t);

        t->tok = identCounter;
        identCounter++;
        vector_push_back(&idents, t);
        // t->len = len;
        return t;
    }
    return t;
}
// extern FILE *yyout;
Token next(){
    // yyout = NULL;
    curTok = yylex();
    return curTok;
}
bool  match(Token t) {
    if (t != curTok) {
        return false;
    }
    next();
    return true;
};
void consume(Token t, const char *msg) {
    if (t != curTok) {
        sy_error(msg);
    }
    next();
}

tokenIdent *getToken(uint32_t tok) {
    return vector_get(&idents,tok);
}