#include "ConditionAna.h"

bool cutResult = false;

static std::map<ExprAST*, std::vector<std::shared_ptr<ExprAST>>> tempConditionToOriginTrueExit;
static std::map<ExprAST*, std::vector<std::shared_ptr<ExprAST>>> tempConditionToOriginFalseExit;

std::shared_ptr<ExprAST> getNewTrueCondition()
{
	ValType* valtype = new ValType();
	valtype->setType(new IntType());
	valtype->setConst();
	return std::make_shared<IntegerLiteralAST>(valtype, 1);
}


ConditionConst CallExprAST::conditionAna()
{
	return ConditionConst::NONE;
}

ConditionConst DeclRefAST::conditionAna()
{
	if (this->isBranch) {
		if (this->UseDecl->isConst) {
			ValTypeStruct val = this->Evaluate();
			if (val.type == "int") {
				if (val.iVal != 0)
					return ConditionConst::TRUE;
				else
					return ConditionConst::FALSE;
			}
			else if (val.type == "float") {
				if (val.fVal != 0)
					return ConditionConst::TRUE;
				else
					return ConditionConst::FALSE;
			}
		}
		return ConditionConst::NONE;
	}
	return ConditionConst::NONE;
}

ConditionConst ImplCastExprAST::conditionAna()
{
	if (this->isBranch) {
		if (this->ToType->isConstQualified()) {
			ValTypeStruct val = this->Evaluate();
			if (val.type == "int") {
				if (val.iVal != 0)
					return ConditionConst::TRUE;
				else
					return ConditionConst::FALSE;
			}
			else if (val.type == "float") {
				if (val.fVal != 0)
					return ConditionConst::TRUE;
				else
					return ConditionConst::FALSE;
			}
		}
		else {
			this->Val->isBranch = true;
			this->Val->TrueExit = this->TrueExit;
			this->Val->FalseExit = this->FalseExit;

			ConditionConst result = this->Val->conditionAna();
			if (result == ConditionConst::BINARY_GET_LEFT) {
				this->Val = std::dynamic_pointer_cast<BinaryExprAST>(this->Val)->LHS;
			}
			else if (result == ConditionConst::BINARY_GET_RIGHT) {
				this->Val = std::dynamic_pointer_cast<BinaryExprAST>(this->Val)->RHS;
			}
			else if (result == ConditionConst::UNARY_GET_RIGHT) {
				this->Val = std::dynamic_pointer_cast<UnaryExprAST>(this->Val)->RHS;
			}
			else {
				return result;
			}

			/*this->Val->isBranch = true;
			this->Val->TrueExit = this->TrueExit;
			this->Val->FalseExit = this->FalseExit;*/

			return  ConditionConst::IMPLI_GET_RIGHT;
		}
	}
	
	return ConditionConst::NONE;
}

ConditionConst ArraySubscripAST::conditionAna()
{
	if (this->isBranch) {
		//���ж��ǲ��ǳ������ǵĻ�ֱ�Ӽ�����
		if (this->Type->isConstQualified()) {
			ValTypeStruct val = this->Evaluate();
			if (val.type == "int") {
				if (val.iVal != 0)
					return ConditionConst::TRUE;
				else
					return ConditionConst::FALSE;
			}
			else if (val.type == "float") {
				if (val.fVal != 0)
					return ConditionConst::TRUE;
				else
					return ConditionConst::FALSE;
			}
		}
	}
	return ConditionConst::NONE;
}

ConditionConst IntegerLiteralAST::conditionAna()
{
	if (this->isBranch) {
		if (this->Val != 0)
			return ConditionConst::TRUE;
		else
			return ConditionConst::FALSE;
	}
	return ConditionConst::NONE;
}

ConditionConst FloatLiteralAST::conditionAna()
{
	if (this->isBranch) {
		if (this->Val != 0)
			return ConditionConst::TRUE;
		else
			return ConditionConst::FALSE;
	}
	return ConditionConst::NONE;
}

//#define LOGICAL_NOT "33"    //!
ConditionConst UnaryExprAST::conditionAna()
{
	if (this->isBranch) {
		//���ж��ǲ��ǳ������ǵĻ�ֱ�Ӽ�����
		if (this->Type->isConstQualified()) {
			ValTypeStruct val = this->Evaluate();
			if (val.type == "int") {
				if (val.iVal != 0)
					return ConditionConst::TRUE;
				else
					return ConditionConst::FALSE;
			}
			else if (val.type == "float") {
				if (val.fVal != 0)
					return ConditionConst::TRUE;
				else
					return ConditionConst::FALSE;
			}
		}
		//���ǳ�����Ҫ�ж��ǲ��� !
		if (this->Op == '!') {
			auto temp = this->TrueExit;
			this->RHS->isBranch=true;
			this->RHS->TrueExit = this->FalseExit;
			this->RHS->FalseExit = temp;

			ConditionConst result = this->RHS->conditionAna();
			if (result == ConditionConst::BINARY_GET_LEFT) {
				this->RHS = std::dynamic_pointer_cast<BinaryExprAST>(this->RHS)->LHS;
			}
			else if (result == ConditionConst::BINARY_GET_RIGHT) {
				this->RHS = std::dynamic_pointer_cast<BinaryExprAST>(this->RHS)->RHS;
			}
			else if (result == ConditionConst::UNARY_GET_RIGHT) {
				this->RHS = std::dynamic_pointer_cast<UnaryExprAST>(this->RHS)->RHS;
			}
			else if (result == ConditionConst::FALSE) {
				return ConditionConst::TRUE;
			}
			else if (result == ConditionConst::IMPLI_GET_RIGHT) {
				this->RHS = std::dynamic_pointer_cast<ImplCastExprAST>(this->RHS)->Val;
			}
			else if (result == ConditionConst::TRUE) {
				return ConditionConst::FALSE;
			}

			/*if (result != ConditionConst::UNARY_GET_RIGHT) {
				temp = this->TrueExit;
				this->RHS->isBranch = true;
				this->RHS->TrueExit = this->FalseExit;
				this->RHS->FalseExit = temp;
			}*/
				
			return ConditionConst::UNARY_GET_RIGHT;
		}
	}
	return ConditionConst::NONE;
}


//#define LOGICAL_AND "257"  //&&
//#define LOGICAL_OR "258"   // ||
ConditionConst BinaryExprAST::conditionAna()
{
	if (this->Op == 257|| this->Op == 258 ) {
		if (this->Op == 257) {
			this->LHS->isBranch = true;
			this->LHS->TrueExit = this->RHS;
			this->LHS->FalseExit = this->FalseExit;

			this->RHS->isBranch = true;
			this->RHS->TrueExit = this->TrueExit;
			this->RHS->FalseExit = this->FalseExit;
		}
		else {
			this->LHS->isBranch = true;
			this->LHS->TrueExit = this->TrueExit;
			this->LHS->FalseExit = this->RHS;

			this->RHS->isBranch = true;
			this->RHS->TrueExit = this->TrueExit;
			this->RHS->FalseExit = this->FalseExit;
		}
		
		ConditionConst leftResult = this->LHS->conditionAna();
		if (leftResult == ConditionConst::BINARY_GET_LEFT) {
			this->LHS = std::dynamic_pointer_cast<BinaryExprAST>(this->LHS)->LHS;
		}
		else if (leftResult == ConditionConst::BINARY_GET_RIGHT) {
			this->LHS = std::dynamic_pointer_cast<BinaryExprAST>(this->LHS)->RHS;
		}
		else if (leftResult == ConditionConst::UNARY_GET_RIGHT) {
			this->LHS = std::dynamic_pointer_cast<UnaryExprAST>(this->LHS)->RHS;
		}
		else if (leftResult == ConditionConst::IMPLI_GET_RIGHT) {
			this->LHS = std::dynamic_pointer_cast<ImplCastExprAST>(this->LHS)->Val;
		}

		//右边进行了变化，需要把左边的重新赋出口****************
		ConditionConst rightResult = this->RHS->conditionAna();
		if (rightResult == ConditionConst::BINARY_GET_LEFT || rightResult == ConditionConst::BINARY_GET_RIGHT
			|| rightResult == ConditionConst::UNARY_GET_RIGHT || rightResult == ConditionConst::IMPLI_GET_RIGHT) {

			auto temp = this->RHS;

			if (rightResult == ConditionConst::BINARY_GET_LEFT) {
				this->RHS = std::dynamic_pointer_cast<BinaryExprAST>(this->RHS)->LHS;
			}
			else if (rightResult == ConditionConst::BINARY_GET_RIGHT) {
				this->RHS = std::dynamic_pointer_cast<BinaryExprAST>(this->RHS)->RHS;
			}
			else if (rightResult == ConditionConst::UNARY_GET_RIGHT) {
				
			}
			else if (rightResult == ConditionConst::IMPLI_GET_RIGHT) {
				this->RHS = std::dynamic_pointer_cast<ImplCastExprAST>(this->RHS)->Val;
			}

			//重新赋出口
			if (this->LHS->TrueExit == temp) {
				this->LHS->TrueExit = this->RHS;
			}
			else if (this->LHS->FalseExit == temp) {
				this->LHS->FalseExit = this->RHS;
			}
		}
		
		

		////���¸�ֵһ��
		//if (this->Op == 257) {
		//	if (leftchange == 0) {
		//		this->LHS->isBranch = true;
		//		this->LHS->TrueExit = this->RHS;
		//		this->LHS->FalseExit = this->FalseExit;
		//	}
		//	
		//	if (rightFei == 0) {
		//		this->RHS->isBranch = true;
		//		this->RHS->TrueExit = this->TrueExit;
		//		this->RHS->FalseExit = this->FalseExit;
		//	}
		//	else {
		//		this->RHS->isBranch = true;
		//		this->RHS->TrueExit = this->FalseExit;
		//		this->RHS->FalseExit = this->TrueExit;
		//	}
		//	
		//}
		//else {
		//	if (leftFei == 0) {
		//		this->LHS->isBranch = true;
		//		this->LHS->TrueExit = this->TrueExit;
		//		this->LHS->FalseExit = this->RHS;
		//	}
		//	else {
		//		this->LHS->isBranch = true;
		//		this->LHS->TrueExit = this->RHS;
		//		this->LHS->FalseExit = this->TrueExit;
		//	}
		//	
		//	if (leftFei == 0) {
		//		this->RHS->isBranch = true;
		//		this->RHS->TrueExit = this->TrueExit;
		//		this->RHS->FalseExit = this->FalseExit;
		//	}
		//	else {
		//		this->RHS->isBranch = true;
		//		this->RHS->TrueExit = this->FalseExit;
		//		this->RHS->FalseExit = this->TrueExit;
		//	}
		//	
		//}

		//������Ҫ��֦�����
		ConditionConst result;
		if (this->Op == 257) {
			if (leftResult == ConditionConst::FALSE) {
				return ConditionConst::FALSE;
			}
			else if(leftResult == ConditionConst::TRUE) {
				if (rightResult == ConditionConst::TRUE) {
					return  ConditionConst::TRUE;
				}
				else if (rightResult == ConditionConst::FALSE) {
					return  ConditionConst::FALSE;
				}
				else {
					return ConditionConst::BINARY_GET_RIGHT;
				}
			}
			else {
				if (rightResult == ConditionConst::TRUE) {
					return ConditionConst::BINARY_GET_LEFT;
				}
				else {
					return ConditionConst::NONE;
				}
			}
		}
		else if (this->Op == 258) {
			if (leftResult == ConditionConst::FALSE) {
				if (rightResult == ConditionConst::TRUE) {
					return ConditionConst::TRUE;
				}
				else if (rightResult == ConditionConst::FALSE) {
					return ConditionConst::FALSE;
				}
				else {
					return ConditionConst::BINARY_GET_RIGHT;
				}
			}
			else if (leftResult == ConditionConst::TRUE) {
				return ConditionConst::TRUE;
			}
			else {
				if (rightResult == ConditionConst::FALSE) {
					return ConditionConst::BINARY_GET_LEFT;
				}
				else {
					return ConditionConst::NONE;
				}
			}
		}
	}
	else {
		if (isBranch) {
			//���ж��ǲ��ǳ������ǵĻ�ֱ�Ӽ�����
			if (this->Type->isConstQualified()) {
				ValTypeStruct val = this->Evaluate();
				if (val.type == "int") {
					if (val.iVal != 0)
						return ConditionConst::TRUE;
					else
						return ConditionConst::FALSE;
				}
				else if (val.type == "float") {
					if (val.fVal != 0)
						return ConditionConst::TRUE;
					else
						return ConditionConst::FALSE;
				}
			}
		}
		return ConditionConst::NONE;
	}
	return ConditionConst::NONE;
}


ConditionConst WhileExprAST::conditionAna()
{
	this->Condition->isBranch = true;
	this->Condition->TrueExit = this->Stmt;
	this->Condition->FalseExit = this->Next;

	ConditionConst result = this->Condition->conditionAna();
	
	if (result != ConditionConst::FALSE && result != ConditionConst::TRUE) {
		if (result == ConditionConst::BINARY_GET_LEFT) {
			this->Condition = std::dynamic_pointer_cast<BinaryExprAST>(this->Condition)->LHS;
		}
		else if (result == ConditionConst::BINARY_GET_RIGHT) {
			this->Condition = std::dynamic_pointer_cast<BinaryExprAST>(this->Condition)->RHS;
		}
		else if (result == ConditionConst::UNARY_GET_RIGHT) {
			this->Condition = std::dynamic_pointer_cast<UnaryExprAST>(this->Condition)->RHS;
		}
		else if (result == ConditionConst::IMPLI_GET_RIGHT) {
			this->Condition = std::dynamic_pointer_cast<ImplCastExprAST>(this->Condition)->Val;
		}

		//if (result == ConditionConst::UNARY_GET_RIGHT) {
		//	this->Condition->isBranch = true;
		//	this->Condition->TrueExit = this->Next;
		//	this->Condition->FalseExit = this->Stmt;
		//}
		//else {
		//	this->Condition->isBranch = true;
		//	this->Condition->TrueExit = this->Stmt;
		//	this->Condition->FalseExit = this->Next;
		//}
		

		travelTrueCondition(this->Condition);

		tempConditionToOriginTrueExit.clear();
		tempConditionToOriginFalseExit.clear();
		this->Stmt->conditionAna();
		return ConditionConst::NONE;
	}
	else {
		
		this->Condition->isBranch = true;
		this->Condition->TrueExit = this->Stmt;
		this->Condition->FalseExit = this->Next;

		travelTrueCondition(this->Condition);

		tempConditionToOriginTrueExit.clear();
		tempConditionToOriginFalseExit.clear();
		this->Stmt->conditionAna();
		return result;
	}

	tempConditionToOriginTrueExit.clear();
	tempConditionToOriginFalseExit.clear();
	this->Stmt->conditionAna();

	return NONE;
}

ConditionConst IfElseExprAST::conditionAna()
{	
	this->Condition->isBranch = true;
	this->Condition->TrueExit = this->Stmts;
	if (!this->ElseStmt) {
		this->Condition->FalseExit = this->Next;
	}
	else {
		this->Condition->FalseExit = this->ElseStmt;
	}
	ConditionConst result=this->Condition->conditionAna();

	if (this->ElseStmt) {
		this->ElseStmt->conditionAna();
	}
	if (result != ConditionConst::FALSE && result != ConditionConst::TRUE) {
		if (result == ConditionConst::BINARY_GET_LEFT) {
			this->Condition = std::dynamic_pointer_cast<BinaryExprAST>(this->Condition)->LHS;
		}
		else if (result == ConditionConst::BINARY_GET_RIGHT) {
			this->Condition = std::dynamic_pointer_cast<BinaryExprAST>(this->Condition)->RHS;
		}
		else if (result == ConditionConst::UNARY_GET_RIGHT) {
			this->Condition = std::dynamic_pointer_cast<UnaryExprAST>(this->Condition)->RHS;
		}
		else if (result == ConditionConst::IMPLI_GET_RIGHT) {
			this->Condition = std::dynamic_pointer_cast<ImplCastExprAST>(this->Condition)->Val;
		}

		/*if (result == ConditionConst::UNARY_GET_RIGHT) {
			this->Condition->isBranch = true;
			this->Condition->FalseExit = this->Stmts;
			if (!this->ElseStmt) {
				this->Condition->TrueExit = this->Next;
			}
			else {
				this->Condition->TrueExit = this->ElseStmt;
			}
		}
		else {
			this->Condition->isBranch = true;
			this->Condition->TrueExit = this->Stmts;
			if (!this->ElseStmt) {
				this->Condition->FalseExit = this->Next;
			}
			else {
				this->Condition->FalseExit = this->ElseStmt;
			}
		}*/
		

		travelTrueCondition(this->Condition);

		tempConditionToOriginTrueExit.clear();
		tempConditionToOriginFalseExit.clear();
		this->Stmts->conditionAna();

		return ConditionConst::NONE;
	}
	else{

		this->Condition->isBranch = true;
		this->Condition->TrueExit = this->Stmts;
		if (!this->ElseStmt) {
			this->Condition->FalseExit = this->Next;
		}
		else {
			this->Condition->FalseExit = this->ElseStmt;
		}

		tempConditionToOriginTrueExit.clear();
		tempConditionToOriginFalseExit.clear();
		this->Stmts->conditionAna();
		return result;
	}

}

ConditionConst CompoundStmtAST::conditionAna()
{
	for (size_t i = 0;i < this->Stmts.size();i++) {
		if (i != this->Stmts.size() - 1) {
			this->Stmts[i]->Next = this->Stmts[i + 1];
		}
		else {
			this->Stmts[i]->Next = this->Next;
		}
		ConditionConst cancut = this->Stmts[i]->conditionAna();
		if (cancut == ConditionConst::TRUE || cancut == ConditionConst::FALSE) {
			//�����if,����������const����ô���Լ�֦��ȡif/else/��һ�����
			if (this->Stmts[i]->getClassName() == "IfElseExpr") {
				//�������ȡifbody����compoundStmt�滻if
				if (cancut == ConditionConst::TRUE){
					this->Stmts[i] = std::dynamic_pointer_cast<IfElseExprAST>(this->Stmts[i])->Stmts;
				}
				//��������elseȡelse��û�о�ɾ��
				else {
					if (std::dynamic_pointer_cast<IfElseExprAST>(this->Stmts[i])->ElseStmt) {
						this->Stmts[i] = std::dynamic_pointer_cast<IfElseExprAST>(this->Stmts[i])->ElseStmt;
					}
					else {
						this->Stmts.erase(Stmts.begin() + i);
						i--;
					}
				}
			}

			//�����if,����cancutΪ�棬��ô���Լ�֦��ȡif/else/��һ�����
			else if (this->Stmts[i]->getClassName() == "WhileExpr") {
				//��������Ϊ1����dag��ʱ���ٷ���
				if (cancut == ConditionConst::TRUE) {
					std::shared_ptr<WhileExprAST> whilestmt = std::dynamic_pointer_cast<WhileExprAST>(this->Stmts[i]);
					whilestmt->Condition = getNewTrueCondition();
					whilestmt->Condition->isBranch = true;
					whilestmt->Condition->TrueExit = whilestmt->Stmt;
					whilestmt->Condition->FalseExit = this->Next;
				}
				//�����پ�ɾ��
				else {
					this->Stmts.erase(Stmts.begin() + i);
					i--;
				}
			}
		}
	}
	return NONE;
}

ConditionConst FunctionAST::conditionAna()
{
	this->CompoundStmt->conditionAna();
	return NONE;
}


ConditionConst CompUnitAST::conditionAna()
{
	for (size_t i = 0;i < this->TopExpr.size();i++) {
		this->TopExpr[i]->conditionAna();
	}

	return NONE;
}


void ConditionAna(ASTContext* atx) {
	atx->CompileUnit->conditionAna();
}

void travelTrueCondition(std::shared_ptr<ExprAST> expr)
{
	if (expr->isBranch) {
		if (expr->getClassName() == "BinaryExpr") {
			std::shared_ptr<BinaryExprAST> b = std::dynamic_pointer_cast<BinaryExprAST>(expr);
			if (b->Op == 257 || b->Op == 258) {
				std::vector<std::shared_ptr<ExprAST>> toChangeTrue = tempConditionToOriginTrueExit[b.get()];
				std::vector<std::shared_ptr<ExprAST>> toChangeFalse = tempConditionToOriginFalseExit[b.get()];
				for (auto expr : toChangeTrue) {
					expr->TrueExit = b->LHS;
				}
				for (auto expr : toChangeFalse) {
					expr->FalseExit = b->LHS;
				}
				std::vector<std::shared_ptr<ExprAST>> LHStvec = tempConditionToOriginTrueExit[b->LHS.get()];
				std::vector<std::shared_ptr<ExprAST>> LHSfvec = tempConditionToOriginFalseExit[b->LHS.get()];

				LHStvec.insert(LHStvec.end(), toChangeTrue.begin(), toChangeTrue.end());
				LHSfvec.insert(LHSfvec.end(), toChangeFalse.begin(), toChangeFalse.end());
				tempConditionToOriginTrueExit[b->LHS.get()] = LHStvec;
				tempConditionToOriginFalseExit[b->LHS.get()] = LHSfvec;

				travelTrueCondition(b->LHS);
				travelTrueCondition(b->RHS);
			}
			else {
				tempConditionToOriginTrueExit[b->TrueExit.get()].push_back(expr);
				tempConditionToOriginFalseExit[b->FalseExit.get()].push_back(expr);
			}
		}
		else {
			tempConditionToOriginTrueExit[expr->TrueExit.get()].push_back(expr);
			tempConditionToOriginFalseExit[expr->FalseExit.get()].push_back(expr);
		}
	}
}
