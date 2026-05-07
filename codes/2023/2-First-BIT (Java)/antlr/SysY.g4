grammar SysY;

compilationUnit: (declaration | functionDefinition)*;

declaration: constantDeclaration | variableDeclaration;

constantDeclaration:
	CONST typeSpecifier constantDefinition (COMMA constantDefinition)* SEMI;

typeSpecifier: type=(VOID | INT | FLOAT);

constantDefinition:
	Identifier (LBRACKET expression RBRACKET)* ASSIGN initializer;

initializer:
	expression
	| LBRACE (initializer (COMMA initializer)*)? RBRACE;

variableDeclaration:
	typeSpecifier variableDefinition (COMMA variableDefinition)* SEMI;

variableDefinition:
	Identifier (LBRACKET expression RBRACKET)*
	| Identifier (LBRACKET expression RBRACKET)* ASSIGN initializer;

functionDefinition:
	typeSpecifier Identifier LPAREN parameterList? RPAREN compoundStatement;

parameterList:
	parameterDeclaration (COMMA parameterDeclaration)*;

parameterDeclaration:
	typeSpecifier Identifier (LBRACKET RBRACKET (LBRACKET expression RBRACKET)*)?;

compoundStatement: LBRACE blockItem* RBRACE;

blockItem: declaration | statement;

statement:
	lValue ASSIGN expression SEMI # assignmentStatement
	| expression? SEMI # expressionStatement
	| compoundStatement # childCompoundStatement
	| IF LPAREN expression RPAREN statement (ELSE statement)? # ifStatement
	| WHILE LPAREN expression RPAREN statement # whileStatement
	| BREAK SEMI # breakStatement
	| CONTINUE SEMI # continueStatement
	| RETURN expression? SEMI # returnStatement
    ;

expression: logicalOrExpression;

logicalOrExpression:
	logicalAndExpression # childLogicalAndExpression
	| logicalOrExpression LOR logicalAndExpression # binaryLogicalOrExpression
	;

logicalAndExpression:
	equalityExpression # childEqualityExpression
	| logicalAndExpression LAND equalityExpression # binaryLogicalAndExpression
	;

equalityExpression:
	relationalExpression # childRelationalExoression
	| equalityExpression op=(EQ | NE) relationalExpression # binaryEqualityExpression
	;

relationalExpression:
	additiveExpression # childAdditiveExpression
	| relationalExpression op=(LT | GT | LE | GE) additiveExpression # binaryRelationalExpression
	;

additiveExpression:
	multiplicativeExpression # childMultiplicativeExpression
	| additiveExpression op=(ADD | SUB) multiplicativeExpression # binaryAdditiveExpression
	;

multiplicativeExpression:
	unaryExpression # childUnaryExpression
	| multiplicativeExpression op=(MUL | DIV | MOD) unaryExpression # binaryMultiplicativeExpression
	;

unaryExpression:
	primaryExpression # childPrimaryExpression
	| Identifier LPAREN argumentExpressionList? RPAREN # functionCallExpression
	| unaryOperator unaryExpression # unaryOperatorExpression
	;

primaryExpression:
	LPAREN expression RPAREN # parenthesizedExpression
	| lValue # childLValue
	| number # childNumber
	;

lValue: Identifier (LBRACKET expression RBRACKET)*;

number:
	IntegerConstant # integerConstant
	| FloatingConstant # floatingConstant
	;

argumentExpressionList: expression (COMMA expression)*;

unaryOperator: op=(ADD | SUB | LNOT);

BREAK: 'break';
CONST: 'const';
CONTINUE: 'continue';
ELSE: 'else';
FLOAT: 'float';
IF: 'if';
INT: 'int';
RETURN: 'return';
VOID: 'void';
WHILE: 'while';

ASSIGN: '=';

ADD: '+';
SUB: '-';
MUL: '*';
DIV: '/';
MOD: '%';

EQ: '==';
NE: '!=';
LT: '<';
LE: '<=';
GT: '>';
GE: '>=';

LNOT: '!';
LAND: '&&';
LOR: '||';

LPAREN: '(';
RPAREN: ')';
LBRACKET: '[';
RBRACKET: ']';
LBRACE: '{';
RBRACE: '}';

COMMA: ',';
SEMI: ';';

Identifier: Nondigit (Nondigit | Digit)*;

fragment Nondigit: [a-zA-Z_];

fragment Digit: [0-9];

IntegerConstant:
	DecimalConstant
	| OctalConstant
	| HexadecimalConstant;

fragment DecimalConstant: NonzeroDigit Digit*;

fragment NonzeroDigit: [1-9];

fragment OctalConstant: '0' OctalDigit*;

fragment OctalDigit: [0-7];

fragment HexadecimalConstant:
	HexadecimalPrefix HexadecimalDigit+;

fragment HexadecimalPrefix: '0' [xX];

fragment HexadecimalDigit: [0-9a-fA-F];

FloatingConstant:
	DecimalFloatingConstant
	| HexadecimalFloatingConstant;

fragment DecimalFloatingConstant:
	FractionalConstant ExponentPart?
	| Digit+ ExponentPart;

fragment FractionalConstant: Digit* '.' Digit+ | Digit+ '.';

fragment ExponentPart: [eE] Sign? Digit+;

fragment Sign: [+-];

fragment HexadecimalFloatingConstant:
	HexadecimalPrefix (HexadecimalFractionalConstant | HexadecimalDigit+) BinaryExponentPart;

fragment HexadecimalFractionalConstant:
	HexadecimalDigit* '.' HexadecimalDigit+
	| HexadecimalDigit+ '.';

fragment BinaryExponentPart: [pP] Sign? Digit+;

Whitespace: [ \t]+ -> skip;

Newline: ('\r' '\n'? | '\n') -> skip;

BlockComment: '/*' .*? '*/' -> skip;

LineComment: '//' ~[\r\n]* -> skip;
