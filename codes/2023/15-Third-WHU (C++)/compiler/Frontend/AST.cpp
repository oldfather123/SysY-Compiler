#include"AST.h"
#include"SymbolTable.h"

//节点打印*******************目前是简陋版本
void ValDeclAST::print() {
	ValType* type = this->getType();
	std::string str = "ValDecl  ";
	str += this->getName();
	str += "  ";
	str += type->getTypeStr();
	str += "\n";
	printf(str.c_str());
}

void CompUnitAST::print() {
	printf("CompUnit\n");
}

void BinaryExprAST::print() {
	ValType* type = this->getType();
	std::string str = "BinaryExpr  ";
	if (this->getOp() > 255) {
		str += std::to_string(this->getOp());
	}
	else {
		str += (char)(this->getOp());
	}
	str += "  ";
	str += type->getTypeStr();
	str += "\n";
	printf(str.c_str());
}

void UnaryExprAST::print() {
	ValType* type = this->getType();
	std::string str = "UnaryExpr  ";
	str += (char)this->getOp();
	str += "  ";
	str += type->getTypeStr();
	str += "\n";
	printf(str.c_str());
}

void IntegerLiteralAST::print() {
	ValType* type = this->getType();
	std::string str = "IntegerLiteral  ";
	str += std::to_string(this->getVal());
	str += "  ";
	str += type->getTypeStr();
	str += "\n";
	printf(str.c_str());
}
void FloatLiteralAST::print() {
	ValType* type = this->getType();
	std::string str = "FloatLiteral  ";
	str += std::to_string(this->getVal());
	str += "  ";
	str += type->getTypeStr();
	str += "\n";
	printf(str.c_str());

	/*printf(str.c_str());*/
	//char fmt[10];
	//sprintf_s(fmt, "%%.%df", ((FloatType*)(type->getBaseType()))->getNumAfterPoint()); // 构造动态格式化字符串,************这里的问题是有一个sprintf_s
	//printf(fmt, this->getVal()); // 输出浮点数
	//printf("\n");

}

void FunctionAST::print() {
	FuncType* type = this->getFunctype();
	std::string str = "Function  ";
	str += this->getName();
	str += "  ";
	str += type->getTypeStr();
	str += "\n";
	printf(str.c_str());
}

void ParamDeclAST::print() {
	ValType* type = this->getValType();
	std::string str = "ParamDecl  ";
	str += this->getName();
	str += "  ";
	str += type->getTypeStr();
	str += "\n";
	printf(str.c_str());
}

void DeclRefAST::print() {
	ValType* type = this->UseDecl->valtype;
	
	std::string str = "DeclRef  ";
	str += this->getName();
	str += "  ";
	str += type->getTypeStr();
	str += "  ";
	if (type->getBaseType()->getTypeID() == Type::TypeID::Int)
		str += std::to_string(this->UseDecl->constValue.iVal);
	else if (type->getBaseType()->getTypeID() == Type::TypeID::Float)
		str += std::to_string(this->UseDecl->constValue.fVal);
	str += " ";
	str += "\n";
	printf(str.c_str());
}

void DeclStmtAST::print() {
	printf("DeclStmt\n");
}

void CompoundStmtAST::print() {
	printf("CompoundStmt\n");
}

void ReturnExprAST::print() {
	ValType* type = this->getType();
	std::string str = "ReturnStmt  ";
	str += "  ";
	str += type->getTypeStr();
	str += "\n";
	printf(str.c_str());
}

void IfElseExprAST::print() {
	printf("IfElseStmt\n");
}

void WhileExprAST::print() {
	printf("WhileStmt\n");
}

void BreakAST::print() {
	printf("BreakStmt\n");
}

void ContinueAST::print() {
	printf("ContinueStmt\n");
}

void CallExprAST::print() {
	FuncType* type = this->UseFunc->functype;
	std::string str = "CallStmt  ";
	str += this->getName();
	str += "  ";
	str += type->getTypeStr();
	str += "\n";
	printf(str.c_str());
}

void ImplCastExprAST::print()
{
	ValType* totype = this->getToType();
	ValType* fromtype = this->getFromType();
	std::string str = "ImplicitCast  ";
	str += fromtype->getTypeStr();
	str += " To ";
	str += totype->getTypeStr();
	str += "\n";
	printf(str.c_str());
}

void InitListAST::print()
{
	std::string str = "InitList";
	str += " ";
	str += Type->getTypeStr();
	str += " ";
	str += "\n";
	printf(str.c_str());
}

void ImplicitValueInit::print()
{
	ValType* type = this->type;
	std::string str = "ImplicitValueInit";
	str += " ";
	str += type->getTypeStr();
	str += " ";
	str += "\n";
	printf(str.c_str());
}

void ArraySubscripAST::print()
{
	ValType* type = this->Type;
	std::string str = "ArraySub";
	str += " ";
	str += type->getTypeStr();
	str += " ";
	str += "\n";
	printf(str.c_str());
}

void StringAST::print()
{
	std::string str = "String";
	str += " \"";
	str += this->str;
	str += "\"";
	str += "\n";
	printf(str.c_str());
}

//打印树的结构*****************
void PrintDiv(int level) {
	for (int i = 0;i < level;i++) {
		printf("  ");
	}
	printf("|-");
}


//遍历所有节点的主函数***************
void ASTContext::visit(ExprAST* Expr,int level) {
	std::string str = Expr->getClassName();
	if (str == "CompUnit") {
		visitCompUnit(Expr,level);
	}
	else if (str == "ValDecl") {
		visitValDecl(Expr,level);
	}
	else if (str == "BinaryExpr") {
		visitBinaryExpr(Expr, level);
	}
	else if (str == "UnaryExpr") {
		visitUnaryExpr(Expr, level);
	}
	else if (str == "IntegerLiteral") {
		visitIntegerExpr(Expr, level);
	}
	else if (str == "FloatLiteral") {
		visitFloatExpr(Expr, level);
	}
	else if (str == "Function") {
		visitFunctionExpr(Expr, level);
	}
	else if (str == "ParamDecl") {
		visitParamDeclExpr(Expr, level);
	}
	else if (str == "DeclRef") {
		visitDeclRefExpr(Expr, level);
	}
	else if (str == "DeclStmt") {
		visitDeclStmtExpr(Expr, level);
	}
	else if (str == "CompoundStmt") {
		visitCompoundStmtExpr(Expr, level);
	}
	else if (str == "ReturnExpr") {
		visitReturnStmtExpr(Expr, level);
	}
	else if (str == "IfElseExpr") {
		visitIfElseExpr(Expr, level);
	}
	else if (str == "WhileExpr") {
		visitWhileExpr(Expr, level);
	}
	else if (str == "Break") {
		visitBreak(Expr, level);
	}
	else if (str == "Continue") {
		visitContinue(Expr, level);
	}
	else if (str == "CallExpr") {
		visitCall(Expr, level);
	}
	else if (str == "ImplCastExpr") {
		visitImplicCast(Expr, level);
	}
	else if (str == "InitList") {
		visitInitList(Expr,level);
	}
	else if (str == "ImplicitValueInit") {
		visitImplicitValueInit(Expr, level);
	}
	else if (str == "ArraySubscrip") {
		visitArraySubscrip(Expr, level);
	}
	else if (str == "StringAST") {
		visitStringAST(Expr, level);
	}
}

//对各个节点分别具体处理的函数***************
void ASTContext::visitCompUnit(ExprAST* Expr, int level) {
	CompUnitAST* cu = dynamic_cast<CompUnitAST*>(Expr);//动态类型转换
	cu->print();
	if (cu != nullptr) {
		std::vector<ExprAST*> TopExpr = cu->getTopExpr();
		for (std::vector<ExprAST*>::iterator it = TopExpr.begin(); it != TopExpr.end(); ++it) {
			visit(( * it), level + 1);
		}
	}
}

void ASTContext::visitValDecl(ExprAST* Expr, int level) {
	PrintDiv(level);
	ValDeclAST* vd = dynamic_cast<ValDeclAST*>(Expr);
	vd->print();
	if (vd->getCastOrVal()) {
		visit(vd->getCastOrVal(),level+1);
	}
}

void ASTContext::visitBinaryExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	BinaryExprAST* be = dynamic_cast<BinaryExprAST*>(Expr);
	be->print();
	visit(be->getLHS(),level+1);
	visit(be->getRHS(), level + 1);
}

void ASTContext::visitUnaryExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	UnaryExprAST* ue = dynamic_cast<UnaryExprAST*>(Expr);
	ue->print();
	visit(ue->getRHS(), level + 1);
}

void ASTContext::visitIntegerExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	IntegerLiteralAST* i = dynamic_cast<IntegerLiteralAST*>(Expr);
	i->print();
}

void ASTContext::visitFloatExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	FloatLiteralAST* f = dynamic_cast<FloatLiteralAST*>(Expr);
	f->print();
}

void ASTContext::visitFunctionExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	FunctionAST* f = dynamic_cast<FunctionAST*>(Expr);
	std::vector<ExprAST*> Params = f->getParams();
	auto S = f->getCompoundStmt();
	f->print();
	if (Params.size() != 0) {
		for (int i = 0;i < Params.size();i++) {
			visit(Params[i], level + 1);
		}
	}
	if (!S) {
		return;
	}
	visit(S, level + 1);
}

void ASTContext::visitParamDeclExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	ParamDeclAST* p = dynamic_cast<ParamDeclAST*>(Expr);
	p->print();
}

void ASTContext::visitDeclRefExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	DeclRefAST* d = dynamic_cast<DeclRefAST*>(Expr);
	d->print();
}

void ASTContext::visitDeclStmtExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	DeclStmtAST* d = dynamic_cast<DeclStmtAST*>(Expr);
	d->print();
	auto E=d->getValDecls();
	if (E.size() != 0) {
		for (int i = 0;i < E.size();i++) {
			visit(E[i], level + 1);
		}
	}
}

void ASTContext::visitCompoundStmtExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	CompoundStmtAST* c = dynamic_cast<CompoundStmtAST*>(Expr);
	c->print();
	auto E = c->getStmts();
	if (E.size() != 0) {
		for (int i = 0;i < E.size();i++) {
			visit(E[i], level + 1);
		}
	}
}

void ASTContext::visitReturnStmtExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	ReturnExprAST* r = dynamic_cast<ReturnExprAST*>(Expr);
	r->print();
	auto E = r->getStmt();
	if(r->getStmt())
		visit(E, level + 1);
}

void ASTContext::visitIfElseExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	IfElseExprAST* i = dynamic_cast<IfElseExprAST*>(Expr);
	i->print();
	if (i->getCondition() != nullptr) {
		visit(i->getCondition(), level + 1);
	}
	if (i->getStmts() != nullptr) {
		visit(i->getStmts(), level+1);
	}
	if (i->getElseStmt() != nullptr) {
		visit(i->getElseStmt(), level + 1);
	}
}

void ASTContext::visitWhileExpr(ExprAST* Expr, int level) {
	PrintDiv(level);
	WhileExprAST* w = dynamic_cast<WhileExprAST*>(Expr);
	w->print();
	if (w->getCondition() != nullptr) {
		visit(w->getCondition(), level + 1);
	}
	if (w->getStmt() != nullptr) {
		visit(w->getStmt(), level + 1);
	}
}

void ASTContext::visitBreak(ExprAST* Expr, int level) {
	PrintDiv(level);
	BreakAST* b = dynamic_cast<BreakAST*>(Expr);
	b->print();
}

void ASTContext::visitContinue(ExprAST* Expr, int level) {
	PrintDiv(level);
	ContinueAST* c = dynamic_cast<ContinueAST*>(Expr);
	c->print();
}

void ASTContext::visitCall(ExprAST* Expr, int level) {
	PrintDiv(level);
	CallExprAST* c = dynamic_cast<CallExprAST*>(Expr);
	c->print();
	auto E = c->getArgs();
	if (E.size() != 0) {
		for (int i = 0;i < E.size();i++) {
			visit(E[i], level + 1);
		}
	}
}

void ASTContext::visitImplicCast(ExprAST* Expr, int level)
{
	PrintDiv(level);
	ImplCastExprAST* i = dynamic_cast<ImplCastExprAST*>(Expr);
	i->print();
	visit(i->getVal(), level + 1);
}

void ASTContext::visitInitList(ExprAST* Expr, int level)
{
	PrintDiv(level);
	InitListAST* i = dynamic_cast<InitListAST*>(Expr);
	i->print();
	if (i->List.size() != 0) {
		for (int j = 0;j < (i->List.size());j++) {
			visit(i->List[j].get(), level + 1);
		}
	}
}

void ASTContext::visitImplicitValueInit(ExprAST* Expr, int level) {
	PrintDiv(level);
	ImplicitValueInit* i = dynamic_cast<ImplicitValueInit*>(Expr);
	i->print();
}

void ASTContext::visitArraySubscrip(ExprAST* Expr, int level) {
	PrintDiv(level);
	ArraySubscripAST* i = dynamic_cast<ArraySubscripAST*>(Expr);
	i->print();
	visit(i->Addr.get(), level + 1);
	visit(i->ArraySub.get(),level+1);
}

void ASTContext::visitStringAST(ExprAST* Expr, int level) {
	PrintDiv(level);
	StringAST* s = dynamic_cast<StringAST*>(Expr);
	s->print();
}





