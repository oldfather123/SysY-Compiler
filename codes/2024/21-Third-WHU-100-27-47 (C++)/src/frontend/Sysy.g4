grammar Sysy;

//编译单元
compUnit: (decl | funcDef)*;

//声明:常量|变量声明
decl: constDecl | varDecl;

//常量声明
constDecl: 'const' bType constDef (',' constDef)* ';';

//基本类型
bType: 'int' | 'float';

//常数定义
constDef: Ident ('[' constExp ']')* '=' constInitVal;

//常量初值
constInitVal:
	constExp										# singleConstInitVal
	| '{' (constInitVal (',' constInitVal)*)? '}'	# multiConstInitVal;

//变量声明
varDecl: bType varDef (',' varDef)* ';';

//变量定义
varDef:
	Ident ('[' constExp ']')*				# NoinitVarDef
	| Ident ('[' constExp ']')* '=' initVal	# InitVarDef;

//变量初值
initVal:
	exp									# singleInitVal
	| '{' (initVal (',' initVal)*)? '}'	# multiInitVal;

//函数定义
funcDef: funcType Ident '(' (funcFParams)? ')' block;

//函数类型
funcType: 'void' | 'int' | 'float';

//函数形参表
funcFParams: funcFParam (',' funcFParam)*;

//函数形参 //修改
funcFParam: bType Ident ('[' epsilon ']' ('[' exp ']')*)?;

//语句块
block: '{' (blockItem)* '}';

//语句
blockItem: decl | stmt;

//语句
stmt:
	lVal '=' exp ';'						# assignStmt
	| (exp)? ';'							# expStmt
	| block									# blockStmt
	| 'if' '(' exp ')' stmt					# ifStmt
	| 'if' '(' exp ')' stmt ('else' stmt)	# ifElseStmt
	| 'while' '(' exp ')' stmt				# whileStmt
	| 'break' ';'							# breakStmt
	| 'continue' ';'						# continueStmt
	| 'return' (exp)? ';'					# returnStmt;

//表达式 注：SysY 表达式是 int/float 型表达式 
exp: cond;

//条件表达式
cond: lOrExp;

//左值表达式
lVal: Ident ('[' exp ']')*;

//基本表达式
primaryExp:
	'(' exp ')'	# parensPrimaryExp
	| lVal		# lValPrimaryExp
	| number	# numberPrimaryExp;

//数值
number: IntConst | FloatConst;

//一元表达式
unaryExp:
	primaryExp						# primaryUnaryExp
	| Ident '(' (funcRParams)? ')'	# funcCallUnaryExp
	| unaryOp unaryExp				# unaryOpUnaryExp;

//单目运算符
unaryOp: '+' | '-' | '!';

//函数实参列表
funcRParams: exp (',' exp)*;

//乘除模表达式
mulExp:
	unaryExp							# unaryMulExp
	| mulExp ('*' | '/' | '%') unaryExp	# mulMulExp;

//加减表达式
addExp:
	mulExp						# mulAddExp
	| addExp ('+' | '-') mulExp	# addAddExp;

//关系表达式
relExp:
	addExp										# addRelExp
	| relExp ('<' | '>' | '<=' | '>=') addExp	# relRelExp;

//相等性表达式
eqExp: relExp # relEqExp | eqExp ('==' | '!=') relExp # eqEqExp;

//逻辑与表达式
lAndExp: eqExp # eqLAndExp | lAndExp '&&' eqExp # lAndLAndExp;

//逻辑或表达式
lOrExp: lAndExp # lAndLOrExp | lOrExp '||' lAndExp # lOrLOrExp;

//常量表达式
constExp: addExp;

//标识符
Ident: [_a-zA-Z][_a-zA-Z0-9]*;

//整型常量
IntConst: [0-9]+ | ('0x' | '0X') [0-9a-fA-F]+;

//浮点型常量
FloatConst:
	[0-9]+ '.' [0-9]*
	| '.' [0-9]+ ([eE][+-]? [0-9]+)?
	| [0-9]+ ('.' [0-9]*)? [eE][+-]? [0-9]+
	| ('0x' | '0X') [0-9a-fA-F]+ '.' [0-9a-fA-F]*
	| ('0x' | '0X') '.' [0-9a-fA-F]+ ([pP][+-]? [0-9a-fA-F]+)?
	| ('0x' | '0X') [0-9a-fA-F]+ ('.' [0-9a-fA-F]*)? [pP][+-]? [0-9a-fA-F]+;

// 空串
epsilon:;

//空白字符处理
Whitespace: [ \f\t\r\n]+ -> skip;

//注释
LineComment: '//' ~[\r\n]* -> skip;
BlockComment: '/*' .*? '*/' -> skip;
