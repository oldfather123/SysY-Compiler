#include <map>
#include <string>
#include <vector>
#include<cmath>
#include"Parser.h"
#include"PreHandleExtension.h"

//�ʷ���������
static std::string IdentifierStr=""; // ��tok_identifierʱд
static int IntNumVal=0;// ��tok__int_numberʱд
static float FloatNumVal=0;             // ��tok__float_numberʱд
static int numsAfterPoint=0;            //С������λ��

std::string temp_IdentifierStr;

enum NumType {
	INT_N,
	FLOAT_N
};

//SysY���ԵĹؼ���
std::vector < std::string > keywords = {
	"int",
	"float",
	"void",
	"const",
	"return",
	"if",
	"else",
	"for",
	"while",
	"do",
	"break",
	"continue"
};

//�ؼ���
enum Token {
	tok_eof = -1,

	// type
	tok_int = -2,
	tok_float = -3,
	tok_void = -4,
	tok_const = -5,

	//commands
	tok_return = -6,
	tok_if = -7,
	tok_else = -8,
	tok_for = -9,
	tok_while = -10,
	tok_do = -11,
	tok_break = -12,
	tok_continue = -13,

	// primary
	tok_identifier = -14,
	tok_int_number = -15,
	tok_float_number=-16
};

//SysY���Եķ��ţ�����ӳ��&&-257,||-258,==-259,!=-260,<=-261,>=-262
std::vector<int> operators = {
	'=',
	'(',
	')',
	'[',
	']',
	'{',
	'}',
	',',
	';',
	'-',
	'!',
	'+',
	'*',
	'/',
	'%',
	'<',
	'>',
};

//�ж�һ���ַ��Ƿ���������
bool isInVec(int ThisChar, std::vector<int>vec)
{
	for (int i = 0;i < vec.size();i++)
	{
		if (vec[i] == ThisChar)
		{
			return true;
		}
	}

	return false;
}

//�������
void error(std::string str)
{
	printf("%s\n", str.c_str());
}

//��ȡ����ע�ͽ�β
int findEndComment(FILE* lsource)
{
	int lastChar = ' ';
	do {
		lastChar = fgetc(lsource);
	} while (lastChar != '*');
	lastChar = fgetc(lsource);
	if (lastChar == '/') {
		return 1;
	}
	else if (lastChar == EOF) {
		error("error comment def");
		return -1;
	}
	else
	{
		return findEndComment(lsource);
	}
}

//��ȡ��һ��token
static int gettok(FILE* lsource)
{
	static int LastChar = ' ';

	//�����ո�
	while (isspace(LastChar))
		LastChar = fgetc(lsource);

	//����ĸ����ĸ���»��ߣ�ʶ��Ϊ��ʶ��,��ʶ��: [a-zA-Z,_][a-zA-Z0-9]*
	if (isalpha(LastChar) || LastChar == '_')
	{
		std::string temp_str = IdentifierStr;//�ݴ棬��������ǹؼ���֮��Ҫ��������
		IdentifierStr = LastChar;
		while (isalnum((LastChar = fgetc(lsource))) || LastChar == '_')
			IdentifierStr += LastChar;

		//�ж��Ƿ�Ϊ�Ѷ���ؼ��֣����عؼ��ֶ�Ӧ�ı�ʶֵ
		for (int i = 0;i < keywords.size();i++)
		{
			if (IdentifierStr == keywords[i]) {
				IdentifierStr = temp_str;
				return (-2 - i);
			}
		}

		//������ǹؼ���
		return tok_identifier;
	}


	//�ж��Ƿ�Ϊ���֣����͡�����
	//*********��֪��Ҫ��Ҫ�Ѱˡ�ʮ��ʮ�����Ʒֿ�����ʱ������int����*************
	if (isdigit(LastChar) || LastChar == '.')
	{
		std::string NumStr;
		std::string ScientificStr;
		bool isScientific = false;

		NumType nt = INT_N;

		double P_base = 2.0;
		double E_base = 10.0;

		while (isdigit(LastChar) || isalpha(LastChar) || LastChar == '.' || LastChar == '+' || LastChar == '-') {
			NumStr += LastChar;
			if (LastChar == '.') {
				nt = FLOAT_N;
			}
			if (LastChar == 'e' || LastChar == 'E' || LastChar == 'p' || LastChar == 'P') {
				isScientific = true;
				while (isdigit(LastChar) || isalpha(LastChar) || LastChar == '.' || LastChar == '+' || LastChar == '-') {
					ScientificStr += LastChar;
					LastChar = fgetc(lsource);
				}
				break;
			}	
			LastChar = fgetc(lsource);
		}

		if (!isScientific) {
			if (nt == FLOAT_N) {
				FloatNumVal = (float)std::stof(NumStr);
				return tok_float_number;
			}
			else {
				char* pend;
				IntNumVal = (int)strtol(NumStr.c_str(), &pend, 0);
				return tok_int_number;
			}
		}
		else {
			std::string expStr = ScientificStr.substr(1, ScientificStr.length() - 1);
			char* pend;
			int exp = (int)strtol(expStr.c_str(), &pend, 0);
			
			//已经是浮点数了，就直接算指数，相乘，就好了
			if (nt == FLOAT_N) {
				FloatNumVal = (float)std::stof(NumStr);
				if (ScientificStr[0] == 'e' || ScientificStr[0] == 'E') {
					double result = std::pow(E_base, exp);
					FloatNumVal = (float)(result * FloatNumVal);
				}
				else {
					double result = std::pow(P_base, exp);
					FloatNumVal = (float)(result * FloatNumVal);
				}
				return tok_float_number;
			}
			//当前还是整数，如果指数带负号，还要变成浮点数
			else {
				char* pend;
				IntNumVal = (int)strtol(NumStr.c_str(), &pend, 0);
				//如果指数带负号
				if (ScientificStr[1] == '-') {
					if (ScientificStr[0] == 'e' || ScientificStr[0] == 'E') {
						double result = std::pow(E_base, exp);
						FloatNumVal = (float)(result * IntNumVal);
					}
					else {
						double result = std::pow(P_base, exp);
						FloatNumVal = (float)(result * IntNumVal);				
					}
					return tok_float_number;
				}
				else {
					if (ScientificStr[0] == 'e' || ScientificStr[0] == 'E') {
						double result = std::pow(E_base, exp);
						IntNumVal = (float)(result * IntNumVal);
					}
					else {
						double result = std::pow(P_base, exp);
						IntNumVal = (float)(result * IntNumVal);
					}
					return tok_int_number;
				}
			}
		}
	}

	//��ʼ����˫�ַ�����
	//����ע��
	if (LastChar == '/') {
		int ThisChar = LastChar;//�ݴ�һ��
		LastChar = fgetc(lsource);
		if (LastChar == '/') {
			do
				LastChar = fgetc(lsource);
			while (LastChar != EOF && LastChar != '\n' && LastChar != '\r');

			if (LastChar != EOF)
				return gettok(lsource);
		}
		else if (LastChar == '*') {
			int ret = findEndComment(lsource);
			if (ret != -1) {
				LastChar = fgetc(lsource);
				return gettok(lsource);
			}
			return tok_eof;
		}
		else
		{
			return ThisChar;
		}
	}

	if (LastChar == '&') {
		int ThisChar = LastChar;//�ݴ�һ��
		LastChar = fgetc(lsource);
		if (LastChar == '&')
		{
			LastChar = fgetc(lsource);
			return 257;
		}
		else
		{
			return ThisChar;
		}
	}

	if (LastChar == '|') {
		int ThisChar = LastChar;//�ݴ�һ��
		LastChar = fgetc(lsource);
		if (LastChar == '|')
		{
			LastChar = fgetc(lsource);
			return 258;
		}
		else
		{
			return ThisChar;
		}
	}

	if (LastChar == '=') {
		int ThisChar = LastChar;//�ݴ�һ��
		LastChar = fgetc(lsource);
		if (LastChar == '=')
		{
			LastChar = fgetc(lsource);
			return 259;
		}
		else
		{
			return ThisChar;
		}
	}

	if (LastChar == '!') {
		int ThisChar = LastChar;//�ݴ�һ��
		LastChar = fgetc(lsource);
		if (LastChar == '=')
		{
			LastChar = fgetc(lsource);
			return 260;
		}
		else
		{
			return ThisChar;
		}
	}

	if (LastChar == '<') {
		int ThisChar = LastChar;//�ݴ�һ��
		LastChar = fgetc(lsource);
		if (LastChar == '=')
		{
			LastChar = fgetc(lsource);
			return 261;
		}
		else
		{
			return ThisChar;
		}
	}

	if (LastChar == '>') {
		int ThisChar = LastChar;//�ݴ�һ��
		LastChar = fgetc(lsource);
		if (LastChar == '=')
		{
			LastChar = fgetc(lsource);
			return 262;
		}
		else
		{
			return ThisChar;
		}
	}

	if (LastChar == '"') {
		LastChar = fgetc(lsource);
		IdentifierStr = "";
		while (LastChar != '"') {
			IdentifierStr += LastChar;
			LastChar = fgetc(lsource);
		}
		
		LastChar = fgetc(lsource);
		return tok_identifier;
	}

	//�ж��ǲ�������������
	int ThisChar = LastChar;
	if (isInVec(ThisChar, operators))
	{
		LastChar = fgetc(lsource);
		return ThisChar;
	}

	// �ж�EOF
	if (LastChar == EOF)
		return -1;

	// �����������
	error("error unknown operator");
	return -2;
}

/////////////////////////////////////////////////�ʷ������������������﷨����/////////////////////////////////////////


//�﷨��������
static FILE* psource;//�ļ�
ASTContext* pctx;//AST������
static int CurTok;//��ǰ���Ľ��
SymbolTable* psymbolTable;//���ű�
static int getNextToken() { return CurTok = gettok(psource); }//��ȡ��һ��token

static int isInWhile = 0;

//���뵥ԪCU�Ľṹ
std::vector<std::shared_ptr<ExprAST>> TopExpr;

std::vector<std::string> GlobalStrVector;


//�洢��Ԫ����������ȼ�
static std::map<int, int> BinopPrecedence;
//���ö�Ԫ��������ȼ�
void setBinopPrecedence() {
    BinopPrecedence[258] = 10;// '||'
    BinopPrecedence[257] = 20;// '&&'
    BinopPrecedence[259] = 40;// '=='
    BinopPrecedence[260] = 40;// '!='
    BinopPrecedence[60] = 60;// '<'
    BinopPrecedence[62] = 60;// '>'
    BinopPrecedence[261] = 60;// '<='
    BinopPrecedence[262] = 60;// '>='
    BinopPrecedence[43] = 85;// '+'
    BinopPrecedence[45] = 85;// '-'
    BinopPrecedence[42] = 100;// '*'
    BinopPrecedence[47] = 100;// '/'
    BinopPrecedence[37] = 100;// '%'
}

//��ȡ��Ԫ��������ȼ�
static int GetTokPrecedence() {
    int TokPrec = BinopPrecedence[CurTok];
    if (TokPrec <= 0)
        return -1;
    return TokPrec;
}

//����������
static std::shared_ptr<ExprAST> ParseFloatExpr() {
	ValType* valtype = new ValType();
	FloatType* floattype = new FloatType();
	valtype->setType(floattype);
	valtype->setConst();
	auto Result = std::make_shared<FloatLiteralAST>(valtype, FloatNumVal);
	getNextToken(); // �����������
	return Result;
}

//��������
static std::shared_ptr<ExprAST> ParseIntExpr() {
	ValType* valtype = new ValType();
	IntType* inttype = new IntType();
	valtype->setType(inttype);
	valtype->setConst();
	auto Result = std::make_shared<IntegerLiteralAST>(valtype,IntNumVal);
	getNextToken(); // �����������
	return Result;
}

//����С����
static std::shared_ptr<ExprAST> ParseParenExpr() {
	getNextToken(); // ����'('
	auto V = ParseExpression();
	if (!V)
		return nullptr;

	if (CurTok != ')') {
		error("expected ')'");
		return nullptr;
	}
	getNextToken(); // ����')'
	return V;
}

//������ʶ��
static std::shared_ptr<ExprAST> ParseIdentifierExpr() {
	std::string IdName = IdentifierStr;

	getNextToken(); // ���������ʶ��

	if (CurTok == '(') {
		//putf��û��У���������ȷ�Ե�***************
		if (IdName == "putf") {
			getNextToken(); // ����'('
			GlobalStrVector.push_back(IdentifierStr);

			std::vector<std::shared_ptr<ExprAST>> Args;
			Args.push_back(std::make_shared<StringAST>(GlobalStrVector.size() - 1, IdentifierStr));
			getNextToken();

			while (CurTok != ')') {
				if (CurTok == ',') {
					getNextToken();
				}
				else {
					auto Arg = ParseExpression();
					if (!Arg)
						return nullptr;
					Args.push_back(Arg);
				}
			}
			Symbol* symbol = psymbolTable->LookupFunc(IdName);
			if (!symbol) {
				error("can't find func");
				return nullptr;
			}
			// Eat the ')'.
			getNextToken();
			return std::make_shared<CallExprAST>(IdName, symbol, Args);
		}
		else {
			// ��������
			getNextToken(); // ����'('
			std::vector<std::shared_ptr<ExprAST>> Args;
			if (CurTok != ')') {
				while (true) {
					if (auto Arg = ParseExpression())
						Args.push_back(Arg);
					else
						return nullptr;

					if (CurTok == ')')
						break;

					if (CurTok != ',') {
						error("Expected ')' or ',' in argument list");
						return nullptr;
					}

					getNextToken();
				}
			}
			Symbol* symbol = psymbolTable->LookupFunc(IdName);
			if (!symbol) {
				error("can't find func");
				return nullptr;
			}
			// Eat the ')'.
			getNextToken();
			//�������õ�ʱ����һ��ָ����õĽڵ������
			return std::make_shared<CallExprAST>(IdName, symbol, Args);
		}
	}
	//��������
	else if(CurTok=='[') {
		//���ж��ǲ�����������
		Symbol* symbol = psymbolTable->Lookup(IdName);
		if (!symbol || symbol->symbolType == SymbolType::FUNCTION) {
			error("can't find Val");
			return nullptr;
		}
		if (symbol->valtype->getBaseType()->getTypeID() != Type::TypeID::Array) {
			error("error array ref use int/float\n");
			return nullptr;
		}
		//��������
		std::shared_ptr<ExprAST> arrayResult = std::make_shared<DeclRefAST>(IdName, symbol);
		while (CurTok == '[') {
			getNextToken();//����[
			auto E = ParseExpression();//a[E]
			if (!E) {
				error("error parse array");
				return nullptr;
			}
			if (CurTok != ']') {
				error("Expected ']' after '['");
				return nullptr;
			}
			arrayResult = std::make_shared<ArraySubscripAST>(E, arrayResult);
			getNextToken();//����]
		}
		
		return arrayResult;
	}
	else {
		Symbol* symbol = psymbolTable->Lookup(IdName);
		if (!symbol || symbol->symbolType==SymbolType::FUNCTION) {
			error("can't find Val");
			return nullptr;
		}
		return std::make_shared<DeclRefAST>(IdName,symbol);
	}
}

//������������ʽ
static std::shared_ptr<ExprAST> ParsePrimary() {
    switch (CurTok) {
	case '!':
	case '+':
	case '-':
		return ParseUnary();
    case tok_identifier:
        return ParseIdentifierExpr();
    case tok_int_number:
        return ParseIntExpr();
	case tok_float_number:
		return ParseFloatExpr();
    case '(':
        return ParseParenExpr();
	default:
		error("unknown token when expecting an expression");
		return nullptr;
    }
}

//����һԪ�����
static std::shared_ptr<UnaryExprAST> ParseUnary() {
	char op = CurTok;//������һ����һ����'+','-','!'֮һ
	getNextToken();
	auto E = ParsePrimary();
	if (!E) {
		error("error parse expression");
		return nullptr;
	}
	ValType* valtype = new ValType();
	return std::make_shared<UnaryExprAST>(valtype,op,E);
}

//������Ԫ�����
static std::shared_ptr<ExprAST> ParseBinOpRHS(int ExprPrec,std::shared_ptr<ExprAST> LHS) {
	// If this is a binop, find its precedence.
	while (true) {
		int TokPrec = GetTokPrecedence();

		// �ұ�û��������˻��ұ����ȼ����ߣ�˵��ֻ����������
		if (TokPrec < ExprPrec)
			return LHS;

		// ���������Ԫ�����
		int BinOp = CurTok;
		getNextToken(); // ������

		std::shared_ptr<ExprAST>RHS;
		RHS = ParsePrimary();

		// �����ұߵĻ�������ʽ
		if (!RHS)
			return nullptr;

		// a+b*c��*���ȼ����ߣ����������������b����b*c�����������Ȼ��b*c���嵱���������
		int NextPrec = GetTokPrecedence();
		if (TokPrec < NextPrec) {
			RHS = ParseBinOpRHS(TokPrec + 1, RHS);
			if (!RHS)
				return nullptr;
		}

		ValType* valtype = new ValType();
		//���Ϻϲ���ֱ������ʽ����
		LHS =
			std::make_shared<BinaryExprAST>(valtype,BinOp, LHS, RHS);
	}
}

//���ֻ���������ʽ������
static std::shared_ptr<ExprAST> ParseExpressionForExpr(std::shared_ptr<ExprAST> LHS)
{
	if(LHS) {
		return ParseBinOpRHS(0, LHS);
	}
	if (CurTok == '+' || CurTok == '-' || CurTok == '!') {
		auto LHS = ParsePrimary();
		if (!LHS)
			return nullptr;
		return ParseBinOpRHS(0, LHS);
	}
	
	return nullptr;
}

//��������ʽ��a,+a,+a+b*c
static std::shared_ptr<ExprAST> ParseExpression()
{
	auto LHS = ParsePrimary();
	if (!LHS)
		return nullptr;
	return ParseBinOpRHS(0, LHS);
	return nullptr;
}

//������ֵ��䣬��ֻ��һ��Expr��ֵ�󽫱����������
static std::shared_ptr<ExprAST> ParseValStmt() {

	ValType* deftype = new ValType();
	std::shared_ptr<ExprAST> LHS;
	//����Ǳ�ʶ�����ж���һ���ǲ��ǵȺ�
	if (CurTok == tok_identifier) {
		LHS = ParsePrimary();
		if (!LHS) {
			return nullptr;
		}
		if (CurTok != '=') {//˵�����Ǹ�ֵ��䣬ֻ���Ǽ򵥵�Expr����ʽ
			auto E = ParseExpressionForExpr(LHS);
			if (!E) {
				error("Parse Expr stmt error");
				return nullptr;
			}
			if (CurTok != ';') {
				error("needs ; after Expr stmt");
				return nullptr;
			}
			getNextToken();
			return E;
		}
		//˵���Ǹ�ֵ����ʽ
		getNextToken();
		ValType* bitype = new ValType();
		auto RHS = ParseExpression();
		if (!RHS) {
			error("error parse expression");
			return nullptr;
		}
		getNextToken();
		return std::make_shared<BinaryExprAST>(bitype, '=', LHS, RHS);
	}
	//��������֣���ô��һ�������õ�Expr
	else
	{
		LHS = ParsePrimary();
		if (!LHS) {
			return nullptr;
		}
		auto E = ParseExpressionForExpr(LHS);
		if (!E) {
			error("Parse Expr stmt error");
			return nullptr;
		}
		if (CurTok != ';') {
			error("needs ; after Expr stmt");
			return nullptr;
		}
		getNextToken();
		return E;
	}
}

//����whileѭ��
static std::shared_ptr<ExprAST> ParseWhileStmt() {
	//����while
	if (getNextToken() != '(') {
		error("error while need '('");
		return nullptr;
	}
	getNextToken();//����(
	//��������
	auto E = ParseExpression();
	if (!E) {
		error("error if condition");
		return nullptr;
	}
	if (CurTok != ')') {
		error("error need )");
		return nullptr;
	}

	//*********************手动为while的条件加上!运算******************
	//ValType* valtype_fei = new ValType();
	//std::shared_ptr<ExprAST> condition = std::make_shared<UnaryExprAST>(valtype_fei, '!', E);

	getNextToken();//����)
	
	//���while��ֻ��һ����䣬��ôҲ��������compound����
	if (CurTok != '{') {
		auto VE = ParseStmt();
		if (!VE) {
			error("error parse Expr in while");
			return nullptr;
		}	
		std::vector<std::shared_ptr<ExprAST>> Stmts;
		if (VE->getClassName() != "CompoundStmt") {
			Stmts.push_back(VE);
		}
		return std::make_shared<WhileExprAST>(E, std::make_shared<CompoundStmtAST>(psymbolTable->CurrentScope,Stmts));
	}
	else if (CurTok == '{') {
		getNextToken();//����{
		auto CStmt = ParseCompoundStmt();
		if (!CStmt)
			return nullptr;
		if (CurTok != '}') {
			error("need } after {");
			return nullptr;
		}
		getNextToken();
		return std::make_shared<WhileExprAST>(E, CStmt);
	}
	
	else {
		error("need stmt after while");
		return nullptr;
	}
}

//����if���
static std::shared_ptr<ExprAST> ParseIfStmt() {
	psymbolTable->EnterScope();//ע��if��else�Ķ������Ƿֿ���
	//����if
	if (getNextToken() != '(') {
		error("error if need '('");
		return nullptr;
	}


	getNextToken();//����(
	//��������
	auto Condition = ParseExpression();
	if (!Condition) {
		error("error if condition");
		return nullptr;
	}
	if (CurTok != ')') {
		error("error need )");
		return nullptr;
	}
	getNextToken();//����)

	std::shared_ptr<ExprAST> CStmt;
	//�ж���û�д����ţ��Ƿ�ֻ��һ����ֵ���
	if (CurTok !='{') {
		CStmt = ParseStmt();
		if (!CStmt) {
			error("error parse Expr in if");
			return nullptr;
		}
		std::vector<std::shared_ptr<ExprAST>> Stmts;
		if (CStmt->getClassName() != "CompoundStmt") {
			Stmts.push_back(CStmt);
		}
		CStmt = std::make_shared<CompoundStmtAST>(psymbolTable->CurrentScope,Stmts);
	}
	else if (CurTok == '{') {
		getNextToken();//����{
		CStmt = ParseCompoundStmt();
		if (!CStmt) {
			error("error parseCompoundStmt");
			return nullptr;
		}
		if (CurTok != '}') {
			error("need } after {");
			return nullptr;
		}
		getNextToken();
	}
	else {
		error("error parse if stmt");
		return nullptr;
	}

	psymbolTable->ExitScope();//ע��if��else�Ķ������Ƿֿ��ģ����������µĶ��������

	//���û��else
	if (CurTok != tok_else) {
		return std::make_shared<IfElseExprAST>(Condition, CStmt, nullptr);
	}
	psymbolTable->EnterScope();//ע��if��else�Ķ������Ƿֿ��ģ�����else������
	getNextToken();//����else
	std::shared_ptr<ExprAST> ElseStmt;


	//包含了可以识别else if的接口ParseStmt
	if (CurTok != '{') {
		ElseStmt = ParseStmt();
		if (!ElseStmt) {
			error("error parse Expr in else");
			return nullptr;
		}
		std::vector<std::shared_ptr<ExprAST>> Stmts;
		if (ElseStmt->getClassName() != "CompoundStmt") {
			Stmts.push_back(ElseStmt);
		}
		ElseStmt = std::make_shared<CompoundStmtAST>(psymbolTable->CurrentScope,Stmts);
	}
	else if (CurTok == '{') {
		getNextToken();//����{
		ElseStmt = ParseCompoundStmt();
		if (!CStmt) {
			error("error parseCompoundStmt");
			return nullptr;
		}
		if (CurTok != '}') {
			error("need } after {");
			return nullptr;
		}
		getNextToken();//����}
	}
	else {
		error("error parse if stmt");
		return nullptr;
	}
	psymbolTable->ExitScope();
	return std::make_shared<IfElseExprAST>(Condition, CStmt, ElseStmt);
}

//�����������
static std::shared_ptr<ExprAST> ParseReturnStmt() {
	ValType* valtype = new ValType();
	if (getNextToken() == ';') {//����return�����жϺ�����û�и��ű���ʽ��û�о���void
		valtype->setType(new VoidType);
		getNextToken();
		return std::make_shared<ReturnExprAST>(valtype, nullptr);
	}
	auto E = ParseExpression();
	if (!E) {
		error("error return stmt");
		return nullptr;
	}
	if (CurTok != ';') {
		error("error need ;");
		return nullptr;
	}
	getNextToken();
	return std::make_shared<ReturnExprAST>(valtype,E);
}

//�����������ڱ�������
static std::shared_ptr<ExprAST> ParseValDecl() {

	std::vector<std::shared_ptr<ExprAST>> DeclStmt;

	ValType* valtype = new ValType();
	if (CurTok == tok_const) {//����ǳ���������valtype����Ϊtrue
		valtype->setConst();
		getNextToken();
	}
	if (CurTok == tok_int) {
		IntType* inttype = new IntType();
		valtype->setType(inttype);
	}
	else if (CurTok == tok_float) {
		FloatType* floattype = new FloatType();
		valtype->setType(floattype);
	}
	else {
		error("need int/float after const");
		return nullptr;
	}

	getNextToken();
	if (CurTok != tok_identifier) {
		error("need iden in valdecl");
		return nullptr;
	}
	std::string name;//������¼�Ƿ��б�����û�м��������б�
	while (CurTok != ';') {
		if (CurTok == tok_identifier) {
			name = IdentifierStr;
			getNextToken();//������ʶ��
			if (CurTok == '[') {//*************************�������һ��get
				auto E = ParseArrayDef(name, valtype);
				if (!E) {
					error("error parse array in func");
					return nullptr;
				}
				name = " ";
				DeclStmt.push_back(E);
			}
			if ((CurTok != '=') && (CurTok != ',') && (CurTok != ';')) {
				error("Parse valdecl error");
				return nullptr;
			}
		}
		else if (CurTok == '=')
		{
			getNextToken();//�����Ⱥ�
			auto E = ParseExpression();
			if (!E) {
				error("Parse Expression error");
				return nullptr;
			}
			if (psymbolTable->IsInCurrentScope(name)) {
				error("multidef val");
				return nullptr;
			}
			auto VD = std::make_shared<ValDeclAST>(valtype, name, E);
			Symbol* symbol = new Symbol(valtype, psymbolTable->getCurrentScope());
			psymbolTable->AddSymbol(name,symbol);//������ű�
			VD->symbol = symbol;
			DeclStmt.push_back(VD);
			name = " ";//������������Ѿ�����������
		}
		else if (CurTok == ',') {
			getNextToken();//��������
			if (CurTok != tok_identifier) {
				error("Parse Expression error");
				return nullptr;
			}
			if (name != " ") {
				if (psymbolTable->IsInCurrentScope(name)) {
					error("multidef val");
					return nullptr;
				}
				auto VD = std::make_shared<ValDeclAST>(valtype, name, nullptr);
				Symbol* symbol = new Symbol(valtype, psymbolTable->getCurrentScope());
				psymbolTable->AddSymbol(name,symbol);
				VD->symbol = symbol;
				DeclStmt.push_back(VD);
				name = " ";
			}
		}
	}
	if (name != " ") {
		if (psymbolTable->IsInCurrentScope(name)) {
			error("multidef val");
			return nullptr;
		}
		auto VD = std::make_shared<ValDeclAST>(valtype, name, nullptr);
		Symbol* symbol = new Symbol(valtype, psymbolTable->getCurrentScope());
		psymbolTable->AddSymbol(name,symbol);//������ű�
		VD->symbol = symbol;
		DeclStmt.push_back(VD);
		name = " ";
	}
	getNextToken();
	return std::make_shared<DeclStmtAST>(DeclStmt);
}

//����һ�������
static std::shared_ptr<ExprAST> ParseStmt()
{
	switch (CurTok)
	{
	case ';': {
		std::vector<std::shared_ptr<ExprAST>> Stmts;
		return std::make_shared<CompoundStmtAST>(psymbolTable->CurrentScope,Stmts);
		break;
	}
	case '{': {
		getNextToken();
		psymbolTable->EnterScope();
		auto C = ParseCompoundStmt();
		if (!C) {
			error("error parse vardecl");
			return nullptr;
		}
		if (CurTok != '}')
			return nullptr;
		getNextToken();
		psymbolTable->ExitScope();
		return C;
	}
	case tok_const:
	case tok_int:
	case tok_float: {
		auto S = ParseValDecl();
		if (!S) {
			error("error parse vardecl");
			return nullptr;
		}
		return S;
		break;
	}
	case tok_if: {
		auto S = ParseIfStmt();
		if (!S) {
			error("error parse if stmt");
			return nullptr;
		}
		return S;
		break;
	}
	case tok_while: {
		isInWhile++;
		psymbolTable->EnterScope();//�����µĶ�����Χ
		auto S = ParseWhileStmt();
		if (!S) {
			error("error parse while stmt");
			return nullptr;
		}
		isInWhile--;
		psymbolTable->ExitScope();
		return S;
		break;
	}
	case tok_return: {
		auto S = ParseReturnStmt();
		if (!S) {
			error("error parse vardecl");
			return nullptr;
		}
		return S;
		break;
	}
	case tok_break: {
		if (isInWhile > 0) {
			if (getNextToken() != ';') {
				error("need ; after break");
				return nullptr;
			}
			getNextToken();
			return std::make_shared<BreakAST>();
		}
		else {
			error("break need in circle");
			return nullptr;
		}
		break;
	}
	case tok_continue: {
		if (isInWhile > 0) {
			if (getNextToken() != ';') {
				error("need ; after continue");
				return nullptr;
			}
			getNextToken();
			return std::make_shared<ContinueAST>();
		}
		else {
			error("continue need in circle");
			return nullptr;
		}
		break;
	}
	case tok_int_number:
	case tok_float_number:
	case tok_identifier: 
	case '+':
	case '-':
	case '!': {
		auto S = ParseValStmt();
		if (!S) {
			error("error parse vardecl");
			return nullptr;
		}
		return S;
		break;
	}
	default:
		return nullptr;
	}
}



//��������������䣬ParseCompoundStmt��ͣ��}����Ҫ�����и�����ͣ�����Ľ�β������һ��
static std::shared_ptr<ExprAST> ParseCompoundStmt()
{
	std::vector<std::shared_ptr<ExprAST>> Stmts;
	while (CurTok == ';')
		getNextToken();
	while (CurTok != '}') //����{������}�˳�
	{
		auto E = ParseStmt();
		if (!E) {
			error("error parse stmt");
			return nullptr;
		}
		Stmts.push_back(E);
		while (CurTok == ';')
			getNextToken();
	}

	return std::make_shared<CompoundStmtAST>(psymbolTable->CurrentScope,Stmts);
}

//���������ʼ��
static std::shared_ptr<ExprAST> ParseArrayInitial(ValType* valtype)
{
	if (CurTok != '{') {
		error("error array init need {");
		return nullptr;
	}
	getNextToken();
	std::vector<std::shared_ptr<ExprAST>> List;//InitListExpr�б�����ֵ�б�
	//���ʹ��һ�Դ�����{}��ʼ��
	if (CurTok == '}') {
		getNextToken();//����'}'
		return std::make_shared<InitListAST>(valtype, List);
	}

	while (CurTok != '}') {
		if (CurTok == '{') {
			//valtype�½�һ��
			ArrayType* at = (ArrayType*)(valtype->getBaseType());
			//����Ƿ�������{ }
			if ((at->getExprs()).size() > 1) {
				ValType* valtype2 = new ValType();
				if (valtype->isConstQualified())
					valtype2->setConst();
				std::vector<std::shared_ptr<ExprAST>> List2;//�½���List
				for (int i = 1;i < at->exprs.size();i++) {
					List2.push_back(at->exprs[i]);
				}
				valtype2->setType(new ArrayType(at->getElementType(), List2));
				auto E = ParseArrayInitial(valtype2);
				if (!E) {
					error("error init array");
					return nullptr;
				}
				List.push_back(E);
			}
			else {
				error("too many init levels in array");
				return nullptr;
			}
		}
		else {
			auto E = ParseExpression();
			List.push_back(E);
		}
		if (CurTok == ',') {
			getNextToken();
		}
		else if (CurTok != '}') {
			error("error init array");
			return nullptr;
		}
	}
	getNextToken();//����'}'
	//�½�type����ֹ��ͻ
	ValType* initValtype = new ValType();
	if (valtype->isConstQualified())
		initValtype->setConst();
	ArrayType* at = (ArrayType*)(valtype->getBaseType());
	initValtype->setType(new ArrayType(at->getElementType(), at->exprs));
	return std::make_shared<InitListAST>(initValtype,List);
}

//������������ͣ���int[2][3]�����Curtok����]����һ��token
static ArrayType* ParseArrayType(ValType* valtype)
{
	//exprs�洢���ȱ���ʽ
	std::vector<std::shared_ptr<ExprAST>> exprs;
	while (CurTok == '[') {
		getNextToken();//����[
		auto E = ParseExpression();
		if (!E) {
			error("error parse array");
			return nullptr;
		}
		if (CurTok != ']') {
			error("Expected ']' after '['");
			return nullptr;
		}
		getNextToken();//����]
		exprs.push_back(E);
	}
	return new ArrayType(valtype, exprs);
}

//�������鶨��
static std::shared_ptr<ExprAST> ParseArrayDef(std::string iname, ValType* valtype) {
	if (psymbolTable->IsInCurrentScope(iname)) {
		error("multidef val");
		return nullptr;
	}
	//������������
	ArrayType* arrayType = ParseArrayType(valtype);
	ValType* type = new ValType();//��ֹ����ԭ����valtype
	if (valtype->isConstQualified())
		type->setConst();
	type->setType(arrayType);
	//���������ʼ��
	if (CurTok == '=') {
		getNextToken();//����'='
		auto E = ParseArrayInitial(type);
		if (!E) {
			error("error parse array init");
			return nullptr;
		}
		auto VD = std::make_shared<ValDeclAST>(type, iname, E);
		Symbol* symbol = new Symbol(type, psymbolTable->getCurrentScope());//��������Ҳ���Լ���Symbol����������ָ��ͬһ��
		psymbolTable->AddSymbol(iname,symbol);//������ű�
		VD->symbol = symbol;
		return VD;
	}
	else if (CurTok != ',' && CurTok != ';') {
		error("error parse array");
		return nullptr;
	}
	auto VD = std::make_shared<ValDeclAST>(type, iname, nullptr);
	Symbol* symbol = new Symbol(type, psymbolTable->getCurrentScope());
	psymbolTable->AddSymbol(iname,symbol);//������ű�
	VD->symbol = symbol;
	return VD;
}

//�������㶨�����ʽ���ͺ������ڵı���ʽ����Ҫʶ�����ʶ������һ����֪���Ǳ���ʽ����ʶ�����̵�
static int HandleExpression(std::string name, ValType* valtype)
{
	std::string iname = name;//������¼�Ƿ��б�����û�м��������б�
	if (CurTok == ';') {
		//������������ű������Ҽ���AST����֮ǰ����Ƿ���ͬ��
		if (psymbolTable->IsInCurrentScope(name)) {
			error("multidef val");
			return -1;
		}
		auto VD = std::make_shared<ValDeclAST>(valtype, iname, nullptr);
		Symbol* symbol = new Symbol(valtype, psymbolTable->getCurrentScope());
		psymbolTable->AddSymbol(iname,symbol);
		VD->symbol = symbol;
		TopExpr.push_back(VD);
		iname = " ";//������������Ѿ�����������
		getNextToken();
		return 0;
	}
	//��ǰ�Ѿ���name����һ��token��
	while (CurTok != ';') {
		if (CurTok == '[')//ֻ�е�һ�ε�ʱ����ܽ�����һ��if
		{
			auto E = ParseArrayDef(iname, valtype);
			if (!E) {
				error("error parse array");
				return -1;
			}
			iname = " ";
			TopExpr.push_back(E);
		}
		else if (CurTok == tok_identifier) {
			iname = IdentifierStr;
			if (psymbolTable->IsInCurrentScope(iname)) {
				error("multidef val");
				return -1;
			}
			getNextToken();//������ʶ��
			//�п����������������Ǿ�ֱ�������������������********************************
			if (CurTok=='[') {
				auto E = ParseArrayDef(iname, valtype);
				if (!E) {
					error("error parse array");
					return -1;
				}
				iname = " ";
				TopExpr.push_back(E);
			}
			if ((CurTok != '=') && (CurTok != ',') && (CurTok != ';')) {
				error("Parse Expression error");
				return -1;
			}
		}
		else if (CurTok == '=') 
		{
			getNextToken();//�����Ⱥ�
			auto E = ParseExpression();
			if (!E) {
				error("Parse Expression error");
				return -1;
			}
			if (psymbolTable->IsInCurrentScope(iname)) {
				error("multidef val");
				return -1;
			}
			auto VD = std::make_shared<ValDeclAST>(valtype, iname, E);
			Symbol* symbol = new Symbol(valtype, psymbolTable->getCurrentScope());
			psymbolTable->AddSymbol(iname,symbol);//������ű�
			VD->symbol = symbol;
			TopExpr.push_back(VD);
			iname = " ";//������������Ѿ�����������
		}
		else if(CurTok==',') {
			getNextToken();//��������
			if (CurTok != tok_identifier) {
				error("Parse Expression error");
				return -1;
			}
			if (iname != " ") {
				if (psymbolTable->IsInCurrentScope(iname)) {
					error("multidef val");
					return -1;
				}
				auto VD = std::make_shared<ValDeclAST>(valtype, iname, nullptr);
				Symbol* symbol = new Symbol(valtype, psymbolTable->getCurrentScope());
				psymbolTable->AddSymbol(iname,symbol);
				VD->symbol = symbol;
				TopExpr.push_back(VD);
				iname = " ";
			}
		}
		else {
			error("error parse top valdecl");
			return -1;
		}
	}
	if (iname != " ") {
		if (psymbolTable->IsInCurrentScope(iname)) {
			error("multidef val");
			return -1;
		}
		//������ű�
		auto VD = std::make_shared<ValDeclAST>(valtype, iname, nullptr);
		Symbol* symbol = new Symbol(valtype, psymbolTable->getCurrentScope());
		psymbolTable->AddSymbol(iname,symbol);
		VD->symbol = symbol;
		TopExpr.push_back(VD);
		iname = " ";
	}
	getNextToken();//�����ֺ�
	return 0;
}

//�������������е�ָ��
static ArrayType* ParsePointType(ValType* valtype)
{
	std::vector<std::shared_ptr<ExprAST>>exprs;
	if (getNextToken() != ']') {//��һά����ʡȥ
		error("error func param array");
		return nullptr;
	}
	ValType* inttype = new ValType();
	inttype->setConst();
	inttype->setType(new IntType());
	exprs.push_back(std::make_shared<IntegerLiteralAST>(inttype, 0));

	getNextToken();//����]
	while (CurTok == '[') {
		getNextToken();
		auto E = ParseExpression();//ֻ��ʡ�Ե�һά��Ԫ�أ����Ժ���һ��Ҫ�б���ʽ
		if (!E) {
			error("error second or higher dim in func param array");
			return nullptr;
		}
		exprs.push_back(E);
		if (CurTok != ']') {
			error("need ] in func param array");
			return nullptr;
		}
		getNextToken();//����]
	}
	if (CurTok != ')' && CurTok != ',') {
		return nullptr;
	}
	if (CurTok == ',')
		getNextToken();
	return new ArrayType(valtype, exprs);
}

//�������㺯������
static int HandleFunctionDef(ValType* valtype,std::string name)
{
	//����Ƿ���ͬ������
	if (psymbolTable->IsInCurrentScope(name)) {
		error("multidef func");
		return -1;
	}

	psymbolTable->EnterScope();//�����µĶ�����Χ
	psymbolTable->addFuncScope(name, psymbolTable->getCurrentScope());

	FuncType* functype=new FuncType();
	functype->setReturnType(valtype);//���÷���ֵ����

	getNextToken();//����'('
	std::vector<ValType*> ParamTypes;//��Ų�������
	std::vector<std::shared_ptr<ExprAST>> Params;//��Ų�������

	std::vector<Symbol*> ParamSymbols;//��ź��������ڷ��ű��д��ڵ�λ��

	while (CurTok != ')') {
		IntType* inttype = new IntType();
		FloatType* floattype = new FloatType();
		ValType* type = new ValType();//type��������¼ÿһ�����������͵�
		if (CurTok == tok_int) {
			type->setType(std::move(inttype));
			delete floattype;
		}
		else if (CurTok == tok_float) {
			type->setType(std::move(floattype));
			delete inttype;
		}
		else
		{
			return -1;
		}
		if (getNextToken() != tok_identifier) {
			error("required identifier");
			return -1;
		}
		//��麯�������Ƿ���ͬ��
		if (psymbolTable->IsInCurrentScope(IdentifierStr)) {
			error("multidef func param");
			return -1;
		}
		if (getNextToken() == ',') {
			ParamTypes.push_back(type);//���������ͼ������ͱ�
			auto E = std::make_shared<ParamDeclAST>(type, IdentifierStr);
			Symbol* symbol = new Symbol(type, psymbolTable->getCurrentScope());
			ParamSymbols.push_back(symbol);
			E->symbol = symbol;
			Params.push_back(E);
			psymbolTable->AddSymbol(IdentifierStr, symbol);//����������������ű�
			getNextToken();//��������
		}
		else if (CurTok == '[') {
			std::string arrayname = IdentifierStr;
			auto E = ParsePointType(type);
			if (!E) {
				error("parse func param array");
				return -1;
			}
			ValType* pointType = new ValType();//����ˣ����ǻ��и�������
			pointType->setType(E);//����type�����ԣ����ǻ�����int��float������point
			ParamTypes.push_back(pointType);

			auto PD = std::make_shared<ParamDeclAST>(pointType, IdentifierStr);
			Symbol* symbol = new Symbol(pointType, psymbolTable->getCurrentScope());
			ParamSymbols.push_back(symbol);
			PD->symbol = symbol;
			Params.push_back(PD);
			psymbolTable->AddSymbol(arrayname, symbol);//����������������ű�
		}
		else if(CurTok==')') {
			ParamTypes.push_back(type);//���������ͼ������ͱ�
			auto E = std::make_shared<ParamDeclAST>(type, IdentifierStr);
			Symbol* symbol = new Symbol(type, psymbolTable->getCurrentScope());
			ParamSymbols.push_back(symbol);
			E->symbol = symbol;
			Params.push_back(E);
			psymbolTable->AddSymbol(IdentifierStr, symbol);//����������������ű�
			break;//�����������������ѭ��
		}
		else {
			error("parse func param");
			return -1;
		}
	}
	functype->setParamTypes(ParamTypes);
	if (getNextToken() != '{') {
		error("needs { after prototype");
		return -1;
	}

	//����������ȫ�ַ��ű�
	Symbol* symbol = new Symbol(functype, psymbolTable->getGlobalScope());
	symbol->ParamSymbols = ParamSymbols;
	psymbolTable->AddSymbol(name, symbol);

	getNextToken();//����{
	auto E = ParseCompoundStmt();
	if (CurTok != '}') {
		error("need } after function");
		return -1;
	}

	auto FC = std::make_shared<FunctionAST>(functype, name, Params, E);
	FC->symbol = symbol;
	TopExpr.push_back(FC);

	psymbolTable->ExitScope();//�˳�����һ����Χ(ȫ�ַ�Χ)
	//������������ű�
	psymbolTable->AddSymbol(name, symbol);

	getNextToken();//����'}'
	return 0;
}

//�������㶨�壺���������䣨����������
static int HandleDefinition()
{
	bool isConst = false;
	bool isFunc = false;
	//�½�һ���������������
	ValType* valtype = new ValType();
	if (CurTok == tok_const) {//����ǳ���������valtype����Ϊtrue
		isConst = true;//const����һ��Ҫ���������Ǻ���
		valtype->setConst();
		getNextToken();
	}

	if (CurTok == tok_int) {
		IntType* inttype = new IntType();
		valtype->setType(inttype);
	}
	else if (CurTok == tok_float) {
		FloatType* floattype = new FloatType();
		valtype->setType(floattype);
	}
	else if (CurTok == tok_void) {
		if (isConst) {
			error("void can't after const");
			return -1;
		}
		isFunc = true;
		VoidType* voidtype = new VoidType();
		valtype->setType(voidtype);
	}
	else {
		error("error need qualifier");
		return -1;
	}
	
	getNextToken();
    if (CurTok != tok_identifier)//��һ��tokenһ���Ǳ�ʶ��
        return -1;
    
    std::string IdName = IdentifierStr;//�ݴ��ʶ��������
	getNextToken();
	//ʶ��Ϊ��������
	if (CurTok == '(')
	{
		int ret = HandleFunctionDef(valtype,IdName);
		if (ret==-1) {
			error("error function def");
			return -1;
		}
		return 0;
	} 
	//ʶ��Ϊ�䣨����������
	int ret = HandleExpression(IdName,valtype);
	if (ret==-1){
		error("error top definition");
		return -1;
	}
	return 0;
}

//��ѭ������㣬һ��һ��token��ȡ�����ظ����
std::shared_ptr<ExprAST> MainLoop(FILE* lsource,ASTContext* ctx, SymbolTable* symbolTable, std::vector<std::string>& StrVector)
{
	preHandleExtension(symbolTable);
    setBinopPrecedence();
	psource = lsource;
	pctx = ctx;
	psymbolTable = symbolTable;

    getNextToken();

    while (true)
    {
        switch (CurTok)
        {//�������㶨�壺����������ʶ������
        case tok_eof:
			StrVector = GlobalStrVector;
            return std::make_shared<CompUnitAST>(TopExpr);//ʶ�����
        case ';': //����������ֺ�
            getNextToken();
            break;
        case tok_int:
        case tok_float:
        case tok_const:
        case tok_void:
			if (HandleDefinition() == -1) 
				return nullptr;
            break;
        default:
            error("Paser TopLevel error");
            return nullptr;
        }
    }
}