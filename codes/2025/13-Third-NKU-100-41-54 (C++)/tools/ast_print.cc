#include "../include/ast.h"
#include "../include/type.h"

extern std::string type_status[6];

/*****************  Program ***********************/
void __Program::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "Program\n";
    if (comp_list != nullptr) {
        for (auto comp : (*comp_list)) {
            comp->printAST(s, pad + 2);
        }
    }
}
/*****************  Decl& Def ***********************/
void CompUnit_Decl::printAST(std::ostream &s, int pad) { decl->printAST(s, pad); }

/*
DeclStmt
    Id name:a scope:0 type:int
*/
void VarDecl::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "VarDecls   "
      << "Type: " << type_decl->getString() << "\n";
    if (var_def_list != nullptr) {
        for (auto var : (*var_def_list)) {
            var->printAST(s, pad + 2);
        }
    }
}

void ConstDecl::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "ConstDecls   "
      << "Type: " << type_decl->getString() << "\n";
    if (var_def_list != nullptr) {
        for (auto var : (*var_def_list)) {
            var->printAST(s, pad + 2);
        }
    }
}

void VarDef_no_init::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "VarDef   name:" << name->getName()<< "   "
      << "scope:" << scope << "\n";
    if (dims != nullptr) {
        s << std::string(pad + 2, ' ') << " Dimensions:\n";
        for (auto dim : (*dims)) {
            dim->printAST(s, pad + 4);
        }
    }
    s << "\n";
}

void VarDef::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "VarDef   name:" << name->getName() << "   "
      << "scope:" << scope << "\n";
    if (dims != nullptr) {
        s << std::string(pad + 2, ' ') << "Dimensions:\n";
        for (auto dim : (*dims)) {
            dim->printAST(s, pad + 4);
        }
    }
    s << std::string(pad + 2, ' ') << "init:\n";
    init->printAST(s, pad + 4);
}

void ConstDef::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "ConstDef   name:" <<name->getName()<< "   "
      << "scope:" << scope << "\n";
    if (dims != nullptr) {
        for (auto dim : (*dims)) {
            s << std::string(pad + 2, ' ') << "Dimensions:\n";
            dim->printAST(s, pad + 4);
        }
    }
    s << std::string(pad + 2, ' ') << "init:\n";
    init->printAST(s, pad + 4);
}
/*****************  InitVal ***********************/
void ConstInitValList::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "ConstInitValList\n";
    if (initval != nullptr) {
        for (auto init : (*initval)) {
            init->printAST(s, pad + 2);
        }
    }
}

void VarInitValList::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "VarInitValList\n";
    if (initval != nullptr) {
        for (auto init : (*initval)) {
            init->printAST(s, pad + 2);
        }
    }
}
void ConstInitVal::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "ConstInitVal   " << attribute.GetAttributeInfo() << "\n";
    if (exp != nullptr) {
        exp->printAST(s, pad + 2);
    } else {
        s << std::string(pad + 2, ' ') << "Empty Exp\n";
    }
}



void VarInitVal::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "VarInitVal\n";
    if (exp != nullptr) {
        exp->printAST(s, pad + 2);
    } else {
        s << std::string(pad + 2, ' ') << "Empty Exp\n";
    }
}

/************************* Func ***********************/

// FunctionDefine function name:main, type:int()
//   FuncParams
//   BlockStmt
void CompUnit_FuncDef::printAST(std::ostream &s, int pad) { func_def->printAST(s, pad); }

// FunctionDefine function name:main, type:int()
void __FuncDef::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "FuncDef   Name:" << name->getName()
      << "   ReturnType: " << return_type->getString() << "\n";
    if (formals != nullptr) {
        for (auto param : *formals) {
            param->printAST(s, pad + 2);
        }
    }
    if (block != nullptr) {
        block->printAST(s, pad + 2);
    }
}

void __FuncFParam::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "FuncFParam   name:" << name->getName() << "   Type:" << type_decl->getString()
      << "   "
      << "scope:" << scope << "\n";
    if (dims != nullptr) {
        s << std::string(pad + 2, ' ') << "Dimensions:\n";
        for (auto dim : (*dims)) {
            if (dim == nullptr) {
                s << std::string(pad + 4, ' ') << "Null dim\n";
            } else {
                dim->printAST(s, pad + 4);
            }
        }
    }
    s << "\n";
}

void __Block::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "Block   "
      << "Size:" << item_list->size() << "\n";
    if (item_list != nullptr) {
        for (auto stmt : (*item_list)) {
            stmt->printAST(s, pad + 2);
        }
    }
}

void BlockItem_Decl::printAST(std::ostream &s, int pad) { decl->printAST(s, pad); }

void BlockItem_Stmt::printAST(std::ostream &s, int pad) { stmt->printAST(s, pad); }

/******************** Stmt ********************/

void AssignStmt::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "AssignStmt:\n";

    lval->printAST(s, pad + 2);
    exp->printAST(s, pad + 2);
}

void ExprStmt::printAST(std::ostream &s, int pad) {
    if(exp!=nullptr){
        s << std::string(pad, ' ') << "ExpressionStmt:   " << attribute.GetAttributeInfo() << "\n";
        exp->printAST(s, pad + 2);
    }else{
        s << std::string(pad, ' ') << "ExpressionStmt:  empty \n";
    }
}

void BlockStmt::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "BlockStmt:\n";
    b->printAST(s, pad + 2);
}

/********************** Expr **********************/
void Exp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "Expression   " << attribute.GetAttributeInfo() << "\n";
    addExp->printAST(s, pad + 2);
}
void ConstExp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "ConstExp   " << attribute.GetAttributeInfo() << "\n";
    addExp->printAST(s, pad + 2);
}

void AddExp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "AddExp (" <<op.GetOpTypeString()<<"):  "<< attribute.GetAttributeInfo() << "\n";
    addExp->printAST(s, pad + 2);
    mulExp->printAST(s, pad + 2);
}

void MulExp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "MulExp ("<< op.GetOpTypeString()<<"):  "<< attribute.GetAttributeInfo() << "\n";
    mulExp->printAST(s, pad + 2);
    unaryExp->printAST(s, pad + 2);
}

void RelExp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "RelExp ("<< op.GetOpTypeString()<<"):  "<< attribute.GetAttributeInfo() << "\n";
    relExp->printAST(s, pad + 2);
    addExp->printAST(s, pad + 2);
}

void EqExp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "EqExp ("<< op.GetOpTypeString()<<"):  "<< attribute.GetAttributeInfo() << "\n";
    eqExp->printAST(s, pad + 2);
    relExp->printAST(s, pad + 2);
}

void LAndExp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "LAndExp: (&&)   " << attribute.GetAttributeInfo() << "\n";
    lAndExp->printAST(s, pad + 2);
    eqExp->printAST(s, pad + 2);
}

void LOrExp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "LOrExp: (||)   " << attribute.GetAttributeInfo() << "\n";
    lOrExp->printAST(s, pad + 2);
    lAndExp->printAST(s, pad + 2);
}

/*********************** Lval ***********************/


void Lval::printAST(std::ostream &s, int pad) {
   // s<<"isp="<<attribute.type->isPointer;
    s << std::string(pad, ' ') << "Lval   " << attribute.GetAttributeInfo() << "   name:" << name->getName() << "   "
       << "scope:" << scope << "\n";
    if (dims != nullptr) {
        s << std::string(pad + 2, ' ') << "dims:\n";
        for (auto dim : (*dims)) {
            dim->printAST(s, pad + 4);
        }
    }
}

void FuncRParams::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "FuncRParams:\n";
    if (rParams != nullptr) {
        for (auto param : (*rParams)) {
            param->printAST(s, pad + 2);
        }
    }
}

void FuncCall::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "FuncCall   name:" << name->getName()<< "   " << attribute.GetAttributeInfo()
      << "\n";
    if (funcRParams != nullptr) {
        funcRParams->printAST(s, pad + 2);
    } else {
        s << std::string(pad + 2, ' ') << "Empty params\n";
    }
}

/******************** UnaryExp ******************* */
void UnaryExp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "UnaryExp ("<<unaryOp.GetOpTypeString()<<") :  "<< attribute.GetAttributeInfo() << "\n";
    unaryExp->printAST(s, pad + 2);
}

void IntConst::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "Intconst   val:" << val << "   " << attribute.GetAttributeInfo() << "\n";
}

void FloatConst::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "Floatconst   val:" << val << "   " << attribute.GetAttributeInfo() << "\n";
}

// void StringConst::printAST(std::ostream &s, int pad) {
//     s << std::string(pad, ' ') << "StringConst   type:string   val:" << str->get_string() << "\n";
// }


void PrimaryExp::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "PrimaryExp_branch   " << attribute.GetAttributeInfo() << "\n";
    exp->printAST(s, pad + 2);
}

/**************************** Stmt *********************/

void IfStmt::printAST(std::ostream &s, int pad) {
    
    if(elseStmt != nullptr) {
        s << std::string(pad, ' ') << "IfElseStmt:\n";
        s << std::string(pad+2, ' ') << "Cond   type:bool\n";
        Cond->printAST(s, pad + 4);
        s << std::string(pad+2, ' ') << "if_stmt:\n";
        ifStmt->printAST(s, pad + 4);
        s << std::string(pad+2, ' ') << "else_Stmt:\n";
        elseStmt->printAST(s, pad + 4);
    }else{
        s << std::string(pad, ' ') << "IfStmt:\n";
        s << std::string(pad+2, ' ') << "Cond   type:bool\n";
        Cond->printAST(s, pad + 4);
        s << std::string(pad+2, ' ') << "if_stmt:\n";
        ifStmt->printAST(s, pad + 4);
    }
}

void WhileStmt::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "WhileStmt:\n";
    s << std::string(pad+2, ' ') << "Cond   type:bool\n";
    Cond->printAST(s, pad + 4);
    s << std::string(pad+2, ' ') << "Body:\n";
    loopBody->printAST(s, pad + 4);
}

void ContinueStmt::printAST(std::ostream &s, int pad) { s << std::string(pad, ' ') << "ContinueStmt\n"; }

void BreakStmt::printAST(std::ostream &s, int pad) { s << std::string(pad, ' ') << "BreakStmt\n"; }

void RetStmt::printAST(std::ostream &s, int pad) {
    s << std::string(pad, ' ') << "ReturnStmt:\n";
    s << std::string(pad+2, ' ') << "ReturnType:    " << attribute.type->getString() << "\n";
    if(retExp != nullptr) {
        s << std::string(pad+2, ' ') << "ReturnValue:\n";
        retExp->printAST(s, pad + 4);
    }
}


//void null_stmt::printAST(std::ostream &s, int pad) { s << std::string(pad, ' ') << "NullStmt\n"; }