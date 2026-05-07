grammar SysY;

/*===-------------------------------------------===*/
/* Lexer rules                                     */
/*===-------------------------------------------===*/


// keywords
CONST: 'const';
INT: 'int';
FLOAT: 'float';
VOID: 'void';
IF: 'if';
ELSE: 'else';
WHILE: 'while';
BREAK: 'break';
CONTINUE: 'continue';
RETURN: 'return';

// operators
ADD: '+';
SUB: '-';
MUL: '*';
DIV: '/';
MOD: '%';

// relational operators
EQ: '==';
NE: '!=';
LT: '<';
LE: '<=';
GT: '>';
GE: '>=';

// logical operators
AND: '&&';
OR: '||';
NOT: '!';

// assignment operators
ASSIGN: '=';

// punctuations
COMMA: ',';
SEMICOLON: ';';
LPAREN: '(';
RPAREN: ')';
LBRACE: '{';
RBRACE: '}';
LBRACK: '[';
RBRACK: ']';

// fragments
fragment DecDigit: [0-9];
fragment OctDigit: [0-7];
fragment HexDigit: [0-9a-fA-F];
fragment OctPrefix: '0';
fragment HexPrefix: '0' [xX];
fragment NonZeroDecDigit: [1-9];
fragment Sign: '+' | '-';
fragment DecFractional: DecDigit* '.' DecDigit+ | DecDigit+ '.';
fragment Exponent: [eE] Sign? DecDigit+;
fragment DecFloat: DecFractional Exponent? | DecDigit+ Exponent;
fragment HexFractional: HexDigit* '.' HexDigit+ | HexDigit+ '.';
fragment BinExponent: [pP] Sign? DecDigit+;
fragment HexFloat:
	HexPrefix HexFractional BinExponent
	| HexPrefix HexDigit+ BinExponent;

fragment ESC: '\\"' | '\\\\';


// identifier
fragment ALPHA: [a-zA-Z];
fragment ALPHANUM: [a-zA-Z0-9];
fragment NONDIGIT: [a-zA-Z_];
Ident: NONDIGIT (ALPHANUM | '_')*;

// IntConst -> ILITERAL
// FloatConst -> FLITERAL
// literals
ILITERAL:
	NonZeroDecDigit DecDigit*
	| OctPrefix OctDigit*
	| HexPrefix HexDigit+;

// fliteral
FLITERAL: DecFloat | HexFloat;

// string
STRING: '"' (ESC | .)*? '"';

// white space and comments
WS: [\t\r\n ] -> skip;
LINECOMMENT: '//' .*? '\r'? '\n' -> skip;
BLOCKCOMMENT: '/*' .*? '*/' -> skip;

/*===-------------------------------------------===*/
/* Syntax rules                                    */
/*===-------------------------------------------===*/

// CompUnit: (CompUnit)? (decl |funcDef);

compUnit: (globalDecl |funcDef)+;

globalDecl: constDecl                           # globalConstDecl
        | varDecl                               # globalVarDecl;

decl: constDecl | varDecl;

constDecl: CONST bType constDef (COMMA constDef)* SEMICOLON;

bType: INT | FLOAT;

constDef: Ident (LBRACK constExp RBRACK)* ASSIGN constInitVal;

constInitVal: constExp												# constScalarInitValue
			| LBRACE (constInitVal (COMMA constInitVal)*)? RBRACE   # constArrayInitValue;

varDecl: bType varDef (COMMA varDef)* SEMICOLON;

varDef: Ident (LBRACK constExp RBRACK)*
		| Ident (LBRACK constExp RBRACK)* ASSIGN initVal;

initVal: exp											# scalarInitValue
		| LBRACE (initVal (COMMA initVal)*)? RBRACE		# arrayInitValue;

funcType: VOID | INT | FLOAT;

funcDef: funcType Ident LPAREN funcFParams? RPAREN blockStmt;

funcFParams: funcFParam (COMMA funcFParam)*;

// 函数形参感觉定义有问题
// 应该是funcFParam: bType Ident ((LBRACK RBRACK)* (LBRACK exp RBRACK)*)?;
funcFParam: bType Ident (LBRACK RBRACK (LBRACK exp RBRACK)*)?;

blockStmt: LBRACE blockItem* RBRACE;

blockItem: decl | stmt;

stmt: lValue ASSIGN exp SEMICOLON					#assignStmt
		| exp? SEMICOLON							#expStmt
		| blockStmt									#blkStmt
		| IF LPAREN cond RPAREN stmt (ELSE stmt)?	#ifStmt
		| WHILE LPAREN cond RPAREN stmt				#whileStmt
		| BREAK SEMICOLON							#breakStmt
		| CONTINUE SEMICOLON						#continueStmt
		| RETURN exp? SEMICOLON						#returnStmt;

exp: addExp;
cond: lOrExp;
lValue: Ident (LBRACK exp RBRACK)*;

// 为了方便测试 primaryExp 可以是一个string
primaryExp: LPAREN exp RPAREN
			| lValue
			| number
			| string;

number: ILITERAL | FLITERAL;
call: Ident LPAREN (funcRParams)? RPAREN;
unaryExp: primaryExp								
			| call
			| unaryOp unaryExp;

unaryOp: ADD|SUB|NOT;
funcRParams: exp (COMMA exp)*;
string: STRING;
mulExp: unaryExp ((MUL|DIV|MOD) unaryExp)*;
addExp: mulExp ((ADD|SUB) mulExp)*;
relExp: addExp ((LT|GT|LE|GE) addExp)*;
eqExp: relExp ((EQ|NE) relExp)*;
lAndExp: eqExp (AND eqExp)*;
lOrExp: lAndExp (OR lAndExp)*;
constExp: addExp;
// mulExp: unaryExp ((MUL|DIV|MOD) unaryExp)*;
// addExp: mulExp | addExp (ADD|SUB) mulExp;
// relExp: addExp | relExp (LT|GT|LE|GE) addExp;
// eqExp: relExp | eqExp (EQ|NE) relExp;
// lAndExp: eqExp | lAndExp AND eqExp;
// lOrExp: lAndExp | lOrExp OR lAndExp;
// constExp: addExp;