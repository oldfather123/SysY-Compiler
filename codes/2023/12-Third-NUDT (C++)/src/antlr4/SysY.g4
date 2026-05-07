// AliceRemake
// 2023/3/14
grammar SysY;

options {
	language = Cpp;
}

// SysY Lex
INT: 'int';
FLOAT: 'float';
VOID: 'void';
CONST: 'const';
FOR: 'for';
DO: 'do';
WHILE: 'while';
IF: 'if';
ELSE: 'else';
CONTINUE: 'continue';
BREAK: 'break';
RETURN: 'return';

LPAREN: '(';
RPAREN: ')';
LBRACKET: '[';
RBRACKET: ']';
LBRACES: '{';
RBRACES: '}';

COMMA: ',';
SEMICOLON: ';';

ASSIGN: '=';
ADD: '+';
SUB: '-';
MUL: '*';
DIV: '/';
MOD: '%';

LT: '<';
GT: '>';
LE: '<=';
GE: '>=';
EQ: '==';
NE: '!=';

AND: '&&';
OR: '||';
NOT: '!';

INTLITERAL: DECLITERAL | HEXLITERAL | OCTLITERAL;
fragment DECLITERAL: [1-9] [0-9]*;
fragment HEXLITERAL: '0' [xX] [0-9a-fA-F]+;
fragment OCTLITERAL: '0' [0-7]*;

FLOATLITERAL: DECFLOATLITERAL | HEXFLOATLITERAL;
fragment DECFLOATLITERAL
  : [0-9]+ '.' [0-9]* DECEXPONENT?
	| '.' [0-9]+ DECEXPONENT?
	| [0-9]+ DECEXPONENT
  ;
fragment DECEXPONENT: [eE] [+-]? [0-9]+;
fragment HEXFLOATLITERAL
  : '0' [xX] [0-9a-fA-F]+ '.' [0-9a-fA-F]* BINEXPONENT?
  | '0' [xX] '.' [0-9a-fA-F]+ BINEXPONENT?
	| '0' [xX] [0-9a-fA-F]+ BINEXPONENT
  ;
fragment BINEXPONENT: [pP] [+-]? [0-9]+;


IDENTIFIER: LETTER (LETTER | DIGIT)*;
fragment LETTER: [_a-zA-Z];
fragment DIGIT: [0-9];

STRING: '"' (ESC | .)*? '"';
fragment ESC: '\\"' | '\\\\';

SPACE: [ \t\r\n] -> skip;

LINECOMMENT: '//' .*? '\r'? ('\n' | EOF) -> skip;
BLOCKCOMMENT: '/*' .*? '*/' -> skip;

// SysY Parser
// comp.cpp
comp
	: (decl | funcDef)* EOF
	;

// var.cpp
decl
	: CONST? bType varDef (COMMA varDef)* SEMICOLON
	;

varDef
	: lValue
	| lValue ASSIGN init
	;

init
	: exp																		# scalarInit
	| LBRACES (init (COMMA init)*)? RBRACES	# arrayInit
	;

// func.cpp
funcDef
	: returnType IDENTIFIER LPAREN funcFParams? RPAREN blockStmt
	;

funcFParams
	: funcFParam (COMMA funcFParam)*
	;

funcFParam
	: bType IDENTIFIER (LBRACKET RBRACKET (LBRACKET exp RBRACKET)*)?
	;

// stmt.cpp
stmt
	: assignStmt
	| expStmt
	| ifStmt
	| whileStmt
	| breakStmt
	| continueStmt
	| returnStmt
	| blockStmt
	| emptyStmt
	;

assignStmt
	: lValue ASSIGN exp SEMICOLON
	;

expStmt
	: exp SEMICOLON
	;

ifStmt
	: IF LPAREN exp RPAREN stmt (ELSE stmt)?
	;

whileStmt
	: WHILE LPAREN exp RPAREN stmt
	;

breakStmt
	: BREAK SEMICOLON
	;

continueStmt
	: CONTINUE SEMICOLON
	;

returnStmt
	: RETURN exp? SEMICOLON
	;

blockStmt
	: LBRACES (blockItem)* RBRACES
	;

blockItem
	: decl
	| stmt
	;

emptyStmt
	: SEMICOLON
	;

// exp.cpp
exp:
	LPAREN exp RPAREN								# parenExp
	| lValue												# lValueExp
	| number												# numberExp
	| string												# stringExp
	| call													# callExp
	| (ADD | SUB | NOT) exp					# unaryExp
	| exp (MUL | DIV | MOD) exp			# mulExp
	| exp (ADD | SUB) exp						# addExp
	| exp (LT | GT | LE | GE) exp		# relExp
	| exp (EQ | NE) exp							# eqExp
	| exp AND exp										# andExp
	| exp OR exp										# orExp
	;

// call.cpp
call
	: IDENTIFIER LPAREN funcRParams? RPAREN
	;

funcRParams
	: exp (COMMA exp)*
	;

// lvalue.cpp
lValue
	: IDENTIFIER (LBRACKET exp RBRACKET)*
	;

// literal.cpp
number
	: INTLITERAL
	| FLOATLITERAL
	;

string
	: STRING
	;

// type.cpp
returnType
	: VOID
	| INT
	| FLOAT
	;

bType
	: INT
	| FLOAT
	;
