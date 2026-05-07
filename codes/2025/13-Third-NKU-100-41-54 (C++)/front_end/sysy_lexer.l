%option noyywrap
%{
#include <cmath>
#include <vector>
#include <cstring>
#include "../include/symbol.h"
#include "sysy_parser.tab.hh"

extern int line;
extern SymbolTable identTab;
void yyerror(char* s, ...);

/* 符号匹配原则：
 *		由于UnaryOpAdd 和 UnaryOpSub的存在，在此只匹配常量的绝对值(返回)
 *		值得注意的是：-2^31（即 -2147483648）的补码为：10000000 00000000 00000000 00000000
 *		补码取反加1可以得到一个数的相反数的补码。
 *		故我们保存的
 *		Token.intVal = ~(10000000 00000000 00000000 00000000) + 1 
 *						= 10000000 00000000 00000000 00000000
 *						= -2^31（即 -2147483648）
 *		这其实是不会影响存储的，我们真实存储的是一个 UnaryOpSub 和 Token.intVal
 *		故最终的结果应该是 Ans = - (Token.intVal) = Token.intVal = -2^31
 */

/* 前缀 0 (prefix-zero) 匹配原则：
 * 	    profix-zero allowed:
 * 	    (1) 八进制整数 		0123 | 00000123						
 * 	    (2) 十六进制整数	0x000123 | 0x123
 * 	    (3) 指数部分   		...p001 | ...e001 | ...p1 | ...e1 
 * 	    (4) 十进制浮点数 	000123.45e3
 * 	    (5) 十六进制浮点数  0x0003.3p3
 * 	    profix-zero not allowed:
 * 	    (1) 十进制整数 [0]123	
 * 	    (2) 十六进制标识符 [0]0x
 * 	    除此之外, 浮点数指数中 p 必须存在, e 可选。
*/
%}

COMMENT_BEGIN     "/*"
COMMENT_END       "*/"
COMMENT_LINE      \n
COMMENT_ELEMENT   .
%x COMMENT


DECIMAL_INTEGER    		 ([1-9][0-9]*)|0
HEXADECIMAL_INTEGER 	 0[xX][0-9a-fA-F]+
OCTAL_INTEGER    		 0[0-7]+
DECIMAL_FLOAT     		 (([0-9]*)\.([0-9]+)?|\.[0-9]+)([eE][-+]?[0-9]+)?
SCIENTIFIC_NOTATION   	 [0-9]+[eE][-+]?[0-9]+
HEXADECIMAL_FLOAT    	 0[xX]([0-9a-fA-F]+\.?[0-9a-fA-F]*|\.[0-9a-fA-F]+)([pP][-+]?[0-9]+)?
IDENTIFIER       		 [_a-zA-Z][_a-zA-Z0-9]*


%%

{COMMENT_BEGIN} 		{BEGIN COMMENT; }
<COMMENT>{COMMENT_LINE} {++line;}
<COMMENT>{COMMENT_ELEMENT} {}
<COMMENT>{COMMENT_END}  {BEGIN INITIAL; }

"//".* {}

"*/" { yylval.error_msg = "Unexpected end of comment"; return ERROR; }

"<="     { return LEQ;        }
">="     { return GEQ;        }
"=="     { return EQ;         }
"!="     { return NE;         }
"&&"     { return AND;        }
"||"     { return OR;         }
"const"  { return CONST;      }
"if"     { return IF;         }
"else"   { return ELSE;       }
"while"  { return WHILE;      }
"void"   { return NONE_TYPE;  }
"int"    { return INT;        }
"float"  { return FLOAT;      }
"return" { return RETURN;     }
"break"  { return BREAK;      }
"continue" { return CONTINUE; }


\n {++line;}

[ \t\f\r\v] {}

[\{\}\;\(\)\,\[\]] { return yytext[0];}

[\.\+\-\*\/\=\<\!\%\>] { return yytext[0];}


{IDENTIFIER} {    
	yylval.symbol_token = getSymbol(std::string(yytext));
    return IDENT;
}

{DECIMAL_INTEGER} {
    char *endptr;
    yylval.int_token = strtol(yytext, &endptr, 10);
    return INT_CONST;
}

{DECIMAL_FLOAT} { 
    yylval.float_token = strtod(yytext, NULL);  
    return FLOAT_CONST;
}

{SCIENTIFIC_NOTATION} { 
    yylval.float_token = strtod(yytext, NULL); 
    return FLOAT_CONST;
}

{HEXADECIMAL_INTEGER} { 
    yylval.int_token = strtol(yytext, NULL, 16); 
    return INT_CONST;
}

{HEXADECIMAL_FLOAT} {
    yylval.float_token = strtod(yytext, NULL);
    return FLOAT_CONST;
}

{OCTAL_INTEGER} { 
    yylval.int_token = strtol(yytext, NULL, 8);
    return INT_CONST;
}

. {   
	/* unknown tokens, return ERROR */
    yylval.error_msg =yytext;
    return ERROR;
}
%%


