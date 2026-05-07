// 确定一个SysY2022的语法LL(*) 大写字母开头定义词法，小写字母开头定义语法
grammar SysY2022;

compUnit: (decl | funcDef)*;

decl: constDecl | varDecl;

constDecl: 'const' bType constDef (',' constDef)* ';';

bType: 'int' | 'float';

// Identifier常量名称，constExp表示有可能是数组
constDef: Identifier ('[' constExp ']')* '=' constInitVal;

// ?表示0或1个，即初始化列表可选
constInitVal:
	constExp										# itemConstInitVal
	| '{' (constInitVal (',' constInitVal)*)? '}'	# listConstInitVal;

varDecl: bType varDef (',' varDef)* ';';

varDef:
	Identifier ('[' constExp ']')*					# varDefWithOutInitVal
	| Identifier ('[' constExp ']')* '=' initVal	# varDefWithInitVal;

initVal:
	exp									# itemInitVal
	| '{' (initVal (',' initVal)*)? '}'	# listInitVal;

funcDef: funcType Identifier '(' (funcFParams)? ')' block;

// 没有int*吗
funcType: 'void' | 'int' | 'float';

// 函数定义的参数表
funcFParams: funcFParam (',' funcFParam)*;


// bug:第一维可以有数   []值传递，因为是传首个指针，填不填数都是一样的
funcFParam: bType Identifier ('[' ']' ('[' constExp ']')*)?;

block: '{' (blockItem)* '}';

blockItem: decl | stmt;

// return;考虑void型
stmt:
	lVal '=' exp ';'						# assignStmt
	| (exp)? ';'							# expStmt
	| block									# blockStmt
	| 'if' '(' cond ')' stmt				# ifStmt
	| 'if' '(' cond ')' stmt 'else' stmt	# ifElseStmt
	| 'while' '(' cond ')' stmt				# whileStmt
	| 'break' ';'							# breakStmt
	| 'continue' ';'						# continueStmt
	| 'return' (exp)? ';'					# returnStmt;

exp: addExp;

cond: lOrExp;

// 左值，可以被赋值的表达式(locator value有明确存储地址)
lVal: Identifier ('[' exp ']')*;

primaryExp:
	'(' exp ')'	# expPrimaryExp
	| lVal		# lValPrimaryExp
	| number	# numberPrimaryExp;

number: FloatConst | IntConst;

// 一元表达式
unaryExp:
	primaryExp							# primaryUnaryExp
	| Identifier '(' (funcRParams)? ')'	# functionCallUnaryExp
	| unaryOp unaryExp					# unaryUnaryExp;

// +(x+1),-(x+1),!(x>5)在哪
unaryOp: '+' | '-' | '!';

// 函数调用的参数表
funcRParams: funcRParam (',' funcRParam)*;

funcRParam: exp;

mulExp:
	unaryExp							# unaryMulExp
	| mulExp ('*' | '/' | '%') unaryExp	# mulMulExp;

addExp:
	mulExp						# mulAddExp
	| addExp ('+' | '-') mulExp	# addAddExp;

relExp:
	addExp										# addRelExp
	| relExp ('<' | '>' | '<=' | '>=') addExp	# relRelExp;

eqExp: relExp 					 # relEqExp 
	| eqExp ('==' | '!=') relExp # eqEqExp;

// &&优先级高于||
lAndExp: eqExp 			 # eqLAndExp 
	| lAndExp '&&' eqExp # lAndLAndExp;

lOrExp: lAndExp # lAndLOrExp 
	| lOrExp '||' lAndExp # lOrLOrExp;

constExp: addExp;

// to-do：这里可能需要精简 相关测例 95_float.sy
FloatConst:
	[0-9]+ '.' [0-9]*
	| '.' [0-9]+ ([eE][+-]? [0-9]+)?
	| [0-9]+ ('.' [0-9]*)? [eE][+-]? [0-9]+
	| '0x' [0-9a-fA-F]+ '.' [0-9a-fA-F]*
	| '0x' '.' [0-9a-fA-F]+ ([pP][+-]? [0-9a-fA-F]+)?
	| '0x' [0-9a-fA-F]+ ('.' [0-9a-fA-F]*)? [pP][+-]? [0-9a-fA-F]+;

// + 一个或多个
IntConst: [0-9]+ | '0x' [0-9a-fA-F]+;

Identifier: [a-zA-Z_][a-zA-Z_0-9]*;

WS: [\f\r\t\n ]+ -> skip;

// .*? 表示懒惰匹配，会匹配任意字符零次或多次，但尽可能少地匹配字符 ->skip跳过
COMMENT: '/*' .*? '*/' -> skip;

// ~匹配不在字符集中的字符
LINE_COMMENT: '//' ~[\r\n]* -> skip;
