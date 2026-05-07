//
// Created by yanayn on 7/10/24.
//

#include "MySysYParserVisitor.h"

antlrcpp::Any MySysYParserVisitor::visitProgram(
    SysYParserParser::ProgramContext* ctx) {
  Integer_Operator_OpTag_Table = map<string, OpTag>();
  Logic_Operator_OpTag_Table = map<string, OpTag>();
  Integer_Operator_OpTag_Table["+"] = OpTag::ADD;
  Integer_Operator_OpTag_Table["-"] = OpTag::SUB;
  Integer_Operator_OpTag_Table["*"] = OpTag::MUL;
  Integer_Operator_OpTag_Table["/"] = OpTag::SDIV;
  Integer_Operator_OpTag_Table["%"] = OpTag::SREM;
  Integer_Operator_OpTag_Table[">"] = OpTag::SGT;
  Integer_Operator_OpTag_Table["<"] = OpTag::SLT;
  Integer_Operator_OpTag_Table[">="] = OpTag::SGE;
  Integer_Operator_OpTag_Table["<="] = OpTag::SLE;
  Integer_Operator_OpTag_Table["!="] = OpTag::NE;
  Integer_Operator_OpTag_Table["=="] = OpTag::EQ;
  Logic_Operator_OpTag_Table["&&"] = OpTag::AND;
  Logic_Operator_OpTag_Table["||"] = OpTag::OR;
  F_Operator_OpTag_Table = map<string, OpTag>();
  F_Operator_OpTag_Table["+"] = OpTag::FADD;
  F_Operator_OpTag_Table["-"] = OpTag::FSUB;
  F_Operator_OpTag_Table["*"] = OpTag::FMUL;
  F_Operator_OpTag_Table["/"] = OpTag::FDIV;
  F_Operator_OpTag_Table["%"] = OpTag::FREM;
  F_Operator_OpTag_Table[">"] = OpTag::OGT;
  F_Operator_OpTag_Table["<"] = OpTag::OLT;
  F_Operator_OpTag_Table[">="] = OpTag::OGE;
  F_Operator_OpTag_Table["<="] = OpTag::OLE;
  F_Operator_OpTag_Table["!="] = OpTag::ONE;
  F_Operator_OpTag_Table["=="] = OpTag::OEQ;
  loopLayer = stack<WhileBlockInfo*>();
  current = new VariableTable(nullptr);
  init_extern_function();
  visit(ctx->compUnit());
  functionRetType = nullptr;
  return nullptr;
}
void MySysYParserVisitor::buildCondition() {
  Int1Type* intType = dynamic_cast<Int1Type*>(currentRet->getType());
  if (intType == nullptr) {
    if (currentRet->isInteger()) {
      currentRet = module.addIcmpInst(OpTag::NE, currentRet, zero, "icmp.ne");
    } else {
      Value* fzero = FloatConstant::getConstFloat(0);
      currentRet =
          module.addFcmpInst(OpTag::ONE, currentRet, fzero, "fcmp.one");
    }
  }
  module.addBranchInst(currentRet, trueBasicBlock, falseBasicBlock);
}

antlrcpp::Any MySysYParserVisitor::visitInitLVal(
    SysYParserParser::InitLValContext* ctx) {
  int dimesion = ctx->constExp().size();
  Type* t = currDefType;
  for (int i = dimesion - 1; i >= 0; i--) {
    visit(ctx->constExp(i));
    t = new ArrayType(stoi(currentRet->toString()), t);
  }
  currDefType = t;
  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitConstDefSingle(
    SysYParserParser::ConstDefSingleContext* ctx) {
  string varName = ctx->Identifier()->getText();
  visit(ctx->constInitVal());
  currentRet = tranformType(currentRet, currDefType);
  current->put(varName, currentRet);

  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitConstDefArray(
    SysYParserParser::ConstDefArrayContext* ctx) {
  visit(ctx->initLVal());
  // currentRetType=Array.type
  currAggregate = new AggregateValue((ArrayType*)currDefType);
  visit(ctx->constInitVal());
  String varName = ctx->initLVal()->Identifier()->getText();
  buildVariable(varName, currDefType);
  setArrayInitVal(current->get(varName), currAggregate->toArrayConstant());
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitInitValArray(
    SysYParserParser::InitValArrayContext* ctx) {
  for (auto* elem : ctx->initVal()) {
    if (auto* initVarSingle =
            dynamic_cast<SysYParserParser::InitValSingleContext*>(elem)) {
      visit(initVarSingle->exp());
      currAggregate->pushValue(currentRet);
    } else {
      currAggregate->inBrace();
      visit(elem);
    }
  }
  currAggregate->outBrace();
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitConstInitValArray(
    SysYParserParser::ConstInitValArrayContext* ctx) {
  for (auto* elem : ctx->constInitVal()) {
    if (auto* initVarSingle =
            dynamic_cast<SysYParserParser::ConstInitValSingleContext*>(elem)) {
      visit(initVarSingle->constExp());
      currAggregate->pushValue(currentRet);
    } else {
      currAggregate->inBrace();
      visit(elem);
    }
  }
  currAggregate->outBrace();
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitVarDefSingle(
    SysYParserParser::VarDefSingleContext* ctx) {
  string varName = ctx->Identifier()->getText();
  buildVariable(varName, currDefType);
  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitVarDefArray(
    SysYParserParser::VarDefArrayContext* ctx) {
  visit(ctx->initLVal());
  buildVariable(ctx->initLVal()->Identifier()->getText(), currDefType);
  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitVarDefSingleInitVal(
    SysYParserParser::VarDefSingleInitValContext* ctx) {
  string varName = ctx->Identifier()->getText();
  visit(ctx->initVal());
  currentRet = tranformType(currentRet, currDefType);
  buildVariable(varName, currentRet);

  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitVarDefArrayInitVal(
    SysYParserParser::VarDefArrayInitValContext* ctx) {
  visit(ctx->initLVal());
  // currentRetType=Array.type
  currAggregate = new AggregateValue((ArrayType*)currDefType);
  visit(ctx->initVal());
  String varName = ctx->initLVal()->Identifier()->getText();
  buildVariable(varName, currDefType);
  setArrayInitVal(current->get(varName), currAggregate->toArrayConstant());
  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitConstDecl(
    SysYParserParser::ConstDeclContext* ctx) {
  if ((ctx->bType()->getText().compare("int")) == 0) {
    currDefType = new Int32Type();
  } else {
    currDefType = new FloatType();
  }
  Type* bType = currDefType;
  for (SysYParserParser::ConstDefContext* c : ctx->constDef()) {
    currDefType = bType;
    visit(c);
  }
  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitVarDecl(
    SysYParserParser::VarDeclContext* ctx) {
  if ((ctx->bType()->getText().compare("int")) == 0) {
    currDefType = new Int32Type();
  } else {
    currDefType = new FloatType();
  }
  Type* bType = currDefType;
  for (SysYParserParser::VarDefContext* c : ctx->varDef()) {
    currDefType = bType;
    visit(c);
  }
  return nullptr;
}
// TODO: SysYParserParser::visitConstInitValContext()等同类的模块
antlrcpp::Any MySysYParserVisitor::visitFuncDef(
    SysYParserParser::FuncDefContext* ctx) {
  if (ctx->funcType()->getText().compare("int") == 0) {
    functionRetType = new Int32Type();
  } else if (ctx->funcType()->getText().compare("float") == 0) {
    functionRetType = new FloatType();
  } else {
    functionRetType = new VoidType();
  }
  current = new VariableTable(current);
  vector<Argument*> args = vector<Argument*>();
  if (ctx->funcFParams() != nullptr) {
    // 在函数参数定义范围内
    for (SysYParserParser::FuncFParamContext* co :
         ctx->funcFParams()->funcFParam()) {
      SysYParserParser::FuncFParamSingleContext* st =
          dynamic_cast<SysYParserParser::FuncFParamSingleContext*>(co);
      if (st == nullptr) {
        SysYParserParser::FuncFParamArrayContext* ac =
            dynamic_cast<SysYParserParser::FuncFParamArrayContext*>(co);
        Type* bt;
        if (ac->bType()->getText().compare("int") == 0) {
          bt = Type::getInt32Type();
        } else {
          bt = Type::getFloatType();
        }
        int dimens = ac->exp().size();
        for (int i = dimens - 1; i >= 0; i--) {
          visit(ac->exp(i));
          bt = new ArrayType(stoi(currentRet->toString()), bt);
        }
        Type* prt = new PointerType(bt);
        Argument* argu = new Argument(ac->Identifier()->getText(), prt);
        args.push_back(argu);
        current->put(ac->Identifier()->getText(), argu);
      } else {
        string nm = st->Identifier()->getText();
        Argument* argu;
        if (st->bType()->getText().compare("int") == 0) {
          argu = new Argument(nm, Type::getInt32Type());
        } else {
          argu = new Argument(nm, Type::getFloatType());
        }
        args.push_back(argu);
      }
    }
  }
  Type* rt;
  if (ctx->funcType()->getText() == "int") {
    rt = Type::getInt32Type();
  } else if (ctx->funcType()->getText() == "float") {
    rt = Type::getFloatType();
  } else {
    rt = Type::getVoidType();
  }
  FuncType* funcType = Type::getFuncType(rt, args);
  String funcName = ctx->Identifier()->getText();
  Function* mFunction = module.addFunction(funcType, funcName);
  current->parent->put(funcName, mFunction);
  currentIn = mFunction;
  BasicBlock* basicBlock = module.addBasicBlock(mFunction, "entry");
  module.setCurrBasicBlock(basicBlock);

  // set argument alloca
  for (Argument* arg : args) {
    if (arg->getType()->getTypeTag() != TT_POINTER) {
      buildVariable(arg->getName(), arg);
    }
  }
  for (SysYParserParser::BlockItemContext* bc : ctx->block()->blockItem()) {
    visit(bc);
  }
  if (functionRetType->getTypeTag() == TT_VOID) {
    module.addReturnInst();
  } else {
    module.addReturnInst(functionRetType->getZeroInit());
  }
  current = current->parent;
  // 退出函数参数定义范围内
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitBlock(
    SysYParserParser::BlockContext* ctx) {
  VariableTable* scope = new VariableTable(current);
  current = scope;
  for (SysYParserParser::BlockItemContext* c : ctx->blockItem()) {
    visit(c);
  }
  current = current->parent;
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitStmtCond(
    SysYParserParser::StmtCondContext* ctx) {
  BasicBlock* trueBlock = module.addBasicBlock(currentIn, "if.true");
  BasicBlock* falseBlock = module.addBasicBlock(currentIn, "if.false");
  BasicBlock* exitBlock;
  if (ctx->stmt().size() == 2) {
    exitBlock = module.addBasicBlock(currentIn, "if.end");
  } else {
    exitBlock = falseBlock;
  }
  trueBasicBlock = trueBlock;
  falseBasicBlock = falseBlock;
  visit(ctx->cond());
  module.setCurrBasicBlock(trueBlock);
  visit(ctx->stmt(0));
  module.addJumpInst(exitBlock);
  if (ctx->stmt(1) != nullptr) {
    module.setCurrBasicBlock(falseBlock);
    visit(ctx->stmt(1));
    module.addJumpInst(exitBlock);
  }
  module.setCurrBasicBlock(exitBlock);
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitStmtWhile(
    SysYParserParser::StmtWhileContext* ctx) {
  BasicBlock* condBlock = module.addBasicBlock(currentIn, "while.begin");
  BasicBlock* exitBlock = module.addBasicBlock(currentIn, "while.end");
  BasicBlock* bodyBlock = module.addBasicBlock(currentIn, "while.loop");
  module.addJumpInst(condBlock);
  module.setCurrBasicBlock(condBlock);
  trueBasicBlock = bodyBlock;
  falseBasicBlock = exitBlock;
  visit(ctx->cond());
  loopLayer.push(new whileBlockInfo(condBlock, exitBlock));
  module.setCurrBasicBlock(bodyBlock);
  visit(ctx->stmt());
  module.addJumpInst(condBlock);
  module.setCurrBasicBlock(exitBlock);
  loopLayer.pop();
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitStmtBreak(
    SysYParserParser::StmtBreakContext* ctx) {
  module.addJumpInst(loopLayer.top()->exitBlock);
  BasicBlock* break_block =
      module.addBasicBlock(currentIn, "after_while_break");
  module.setCurrBasicBlock(break_block);
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitStmtContinue(
    SysYParserParser::StmtContinueContext* ctx) {
  module.addJumpInst(loopLayer.top()->condBlock);
  BasicBlock* continue_block =
      module.addBasicBlock(currentIn, "after_while_continue");
  module.setCurrBasicBlock(continue_block);
  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitStmtReturn(
    SysYParserParser::StmtReturnContext* ctx) {
  if (ctx->exp() != nullptr) {
    visit(ctx->exp());
    currentRet = tranformType(currentRet, functionRetType);
    module.addReturnInst(currentRet);
  } else {
    module.addReturnInst();
  }
  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitLOrExp(
    SysYParserParser::LOrExpContext* ctx) {
  BasicBlock* falseBlock = falseBasicBlock;
  for (int i = 0; i < ctx->lAndExp().size(); i++) {
    if (i == ctx->lAndExp().size() - 1) {
      falseBasicBlock = falseBlock;
      visit(ctx->lAndExp(i));
    } else {
      BasicBlock* nextBlock = module.addBasicBlock(currentIn, "or.cond");
      falseBasicBlock = nextBlock;
      visit(ctx->lAndExp(i));
      module.setCurrBasicBlock(nextBlock);
    }
  }
  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitLAndExp(
    SysYParserParser::LAndExpContext* ctx) {
  BasicBlock* trueBlock = trueBasicBlock;
  for (int i = 0; i < ctx->eqExp().size(); i++) {
    if (i == ctx->eqExp().size() - 1) {
      trueBasicBlock = trueBlock;
      visit(ctx->eqExp(i));
      buildCondition();
    } else {
      BasicBlock* nextBlock = module.addBasicBlock(currentIn, "and.cond");
      trueBasicBlock = nextBlock;
      visit(ctx->eqExp(i));
      buildCondition();
      module.setCurrBasicBlock(nextBlock);
    }
  }
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitUnaryExpFuncR(
    SysYParserParser::UnaryExpFuncRContext* ctx) {
  vector<Value*> args = vector<Value*>();
  Function* callee = (Function*)current->get(ctx->Identifier()->getText());
  FuncType* funcType = (FuncType*)callee->getType();
  int arg_size = funcType->getArgSize();
  args.reserve(arg_size);

  if (callee->isExtern()) {
    module.pushExternFunction(callee);
  }
  if (callee->getName() == "_sysy_starttime" ||
      callee->getName() == "_sysy_stoptime") {
    args.push_back(IntegerConstant::getConstInt(ctx->getStart()->getLine()));
  } else {
    for (int i = 0; i < arg_size; i++) {
      visit(ctx->funcRParams()->funcRParam()[i]);
      currentRet =
          tranformType(currentRet, funcType->getArgument(i)->getType());
      args.push_back(currentRet);
    }
  }
  currentRet =
      module.addCallInst(callee, args, "call" + ctx->Identifier()->getText());
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitAddExp(
    SysYParserParser::AddExpContext* ctx) {
  visit(ctx->left);
  Value* l_Val = currentRet;
  Value* r_Val;
  // if (l_Val && l_Val->isPointer()) {
  //   LoadInst* li = module.addLoadInst(l_Val, "lval");
  //   l_Val = li;
  // }
  for (int i = 0; i < ctx->op.size(); ++i) {
    visit(ctx->right[i]);
    r_Val = currentRet;
    if (r_Val->isPointer()) {
      r_Val = module.addLoadInst(r_Val, "rval");
    }

    // lhs and rhs both are constant
    if (dynamic_cast<Constant*>(l_Val) && dynamic_cast<Constant*>(r_Val)) {
      if (l_Val->isa(VT_FLOATCONST) || r_Val->isa(VT_FLOATCONST)) {
        // result is float

        float lhs, rhs;
        if (l_Val->isa(VT_INTCONST)) {
          lhs = ((IntegerConstant*)l_Val)->getValue();
        } else {
          lhs = ((FloatConstant*)l_Val)->getValue();
        }
        if (r_Val->isa(VT_INTCONST)) {
          rhs = ((IntegerConstant*)r_Val)->getValue();
        } else {
          rhs = ((FloatConstant*)r_Val)->getValue();
        }
        float ret_float = 0;
        if (ctx->op[i]->getText() == "+") {
          ret_float = lhs + rhs;
        } else {
          ret_float = lhs - rhs;
        }
        l_Val = FloatConstant::getConstFloat(ret_float);
        continue;
      } else {
        // result is integer
        int lhs = ((IntegerConstant*)l_Val)->getValue();
        int rhs = ((IntegerConstant*)r_Val)->getValue();
        int ret_int;
        if (ctx->op[i]->getText() == "+") {
          ret_int = lhs + rhs;
        } else {
          ret_int = lhs - rhs;
        }
        l_Val = IntegerConstant::getConstInt(ret_int);
        continue;
      }
    }
    OpTag t;
    if ((l_Val->getType()->getTypeTag() == TypeTag::TT_FLOAT) ||
        (r_Val->getType()->getTypeTag() == TypeTag::TT_FLOAT)) {
      t = F_Operator_OpTag_Table[ctx->op[i]->getText()];
      if (l_Val->getType()->getTypeTag() == TypeTag::TT_INT32) {
        l_Val = module.addSitofpInst(l_Val, "lval");
      }
      if (r_Val->getType()->getTypeTag() == TypeTag::TT_INT32) {
        r_Val = module.addSitofpInst(r_Val, "lval");
      }
      l_Val = module.addBinaryOpInst(t, l_Val, r_Val, "addres");
    } else {
      t = Integer_Operator_OpTag_Table[ctx->op[i]->getText()];
      l_Val = module.addBinaryOpInst(t, l_Val, r_Val, "addres");
    }
  }
  currentRet = l_Val;
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitMulExp(
    SysYParserParser::MulExpContext* ctx) {
  visit(ctx->left);
  Value* l_Val = currentRet;
  Value* r_Val;
  // if (l_Val && l_Val->isPointer()) {
  //   LoadInst* li = module.addLoadInst(l_Val, "lval");
  //   l_Val = li;
  // }
  for (int i = 0; i < ctx->op.size(); ++i) {
    visit(ctx->right[i]);
    r_Val = currentRet;
    if (r_Val->isPointer()) {
      r_Val = module.addLoadInst(r_Val, "rval");
    }
    // lhs and rhs both are constant
    if (dynamic_cast<Constant*>(l_Val) && dynamic_cast<Constant*>(r_Val)) {
      if (l_Val->isa(VT_FLOATCONST) || r_Val->isa(VT_FLOATCONST)) {
        // result is float

        float lhs, rhs;
        if (l_Val->isa(VT_INTCONST)) {
          lhs = ((IntegerConstant*)l_Val)->getValue();
        } else {
          lhs = ((FloatConstant*)l_Val)->getValue();
        }
        if (r_Val->isa(VT_INTCONST)) {
          rhs = ((IntegerConstant*)r_Val)->getValue();
        } else {
          rhs = ((FloatConstant*)r_Val)->getValue();
        }
        float ret_float = 0;
        if (ctx->op[i]->getText() == "*") {
          ret_float = lhs * rhs;
        } else if (ctx->op[i]->getText() == "/") {
          ret_float = lhs / rhs;
        } else {
          ret_float = std::fmod(lhs, rhs);
        }
        l_Val = FloatConstant::getConstFloat(ret_float);
        continue;
      } else {
        // result is integer
        int lhs = ((IntegerConstant*)l_Val)->getValue();
        int rhs = ((IntegerConstant*)r_Val)->getValue();
        int ret_int;
        if (ctx->op[i]->getText() == "*") {
          ret_int = lhs * rhs;
        } else if (ctx->op[i]->getText() == "/") {
          ret_int = lhs / rhs;
        } else {
          ret_int = lhs % rhs;
        }
        l_Val = IntegerConstant::getConstInt(ret_int);
        continue;
      }
    }
    OpTag t;
    if ((l_Val->getType()->getTypeTag() == TypeTag::TT_FLOAT) ||
        (r_Val->getType()->getTypeTag() == TypeTag::TT_FLOAT)) {
      t = F_Operator_OpTag_Table[ctx->op[i]->getText()];
      if (l_Val->getType()->getTypeTag() == TypeTag::TT_INT32) {
        l_Val = module.addSitofpInst(l_Val, "lval");
      }
      if (r_Val->getType()->getTypeTag() == TypeTag::TT_INT32) {
        r_Val = module.addSitofpInst(r_Val, "lval");
      }
      l_Val = module.addBinaryOpInst(t, l_Val, r_Val, "addres");
    } else {
      t = Integer_Operator_OpTag_Table[ctx->op[i]->getText()];
      l_Val = module.addBinaryOpInst(t, l_Val, r_Val, "addres");
    }
  }
  currentRet = l_Val;
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitRelExp(
    SysYParserParser::RelExpContext* ctx) {
  visit(ctx->left);
  Value* l_Val = currentRet;
  Value* r_Val;
  // if (l_Val && l_Val->isPointer()) {
  //   LoadInst* li = module.addLoadInst(l_Val, "lval");
  //   l_Val = li;
  // }
  if (l_Val->getType()->getTypeTag() == TT_INT1 && ctx->op.size() > 0) {
    if (l_Val->isa(VT_BOOLCONST)) {
      l_Val = IntegerConstant::getConstInt(((BoolConstant*)l_Val)->getValue());
    } else {
      l_Val = module.addZextInst(l_Val, Type::getInt32Type(), "zext");
    }
  }
  for (int i = 0; i < ctx->op.size(); ++i) {
    visit(ctx->right[i]);
    r_Val = currentRet;
    if (r_Val->isPointer()) {
      r_Val = module.addLoadInst(r_Val, "rval");
    }
    if (r_Val->getType()->getTypeTag() == TT_INT1) {
      if (r_Val->isa(VT_BOOLCONST)) {
        r_Val =
            IntegerConstant::getConstInt(((BoolConstant*)r_Val)->getValue());
      } else {
        r_Val = module.addZextInst(r_Val, Type::getInt32Type(), "zext");
      }
    }
    // lhs and rhs both are constant
    if (dynamic_cast<Constant*>(l_Val) && dynamic_cast<Constant*>(r_Val)) {
      if (l_Val->isa(VT_FLOATCONST) || r_Val->isa(VT_FLOATCONST)) {
        // result is float

        float lhs, rhs;
        if (l_Val->isa(VT_INTCONST)) {
          lhs = ((IntegerConstant*)l_Val)->getValue();
        } else {
          lhs = ((FloatConstant*)l_Val)->getValue();
        }
        if (r_Val->isa(VT_INTCONST)) {
          rhs = ((IntegerConstant*)r_Val)->getValue();
        } else {
          rhs = ((FloatConstant*)r_Val)->getValue();
        }
        int ret_float = 0;
        if (ctx->op[i]->getText() == "<") {
          ret_float = (lhs < rhs) ? 1 : 0;
        } else if (ctx->op[i]->getText() == ">") {
          ret_float = (lhs > rhs) ? 1 : 0;
        } else if (ctx->op[i]->getText() == "<=") {
          ret_float = (lhs <= rhs) ? 1 : 0;
        } else if (ctx->op[i]->getText() == ">=") {
          ret_float = (lhs >= rhs) ? 1 : 0;
        }
        l_Val = BoolConstant::getConstBool(ret_float);
        continue;
      } else {
        // result is integer
        int lhs = ((IntegerConstant*)l_Val)->getValue();
        int rhs = ((IntegerConstant*)r_Val)->getValue();
        int ret_int;
        if (ctx->op[i]->getText() == "<") {
          ret_int = lhs < rhs;
        } else if (ctx->op[i]->getText() == ">") {
          ret_int = lhs > rhs;
        } else if (ctx->op[i]->getText() == "<=") {
          ret_int = lhs <= rhs;
        } else if (ctx->op[i]->getText() == ">=") {
          ret_int = lhs >= rhs;
        }
        l_Val = BoolConstant::getConstBool(ret_int);
        continue;
      }
    }
    OpTag t;
    if ((l_Val->getType()->getTypeTag() == TypeTag::TT_FLOAT) ||
        (r_Val->getType()->getTypeTag() == TypeTag::TT_FLOAT)) {
      t = F_Operator_OpTag_Table[ctx->op[i]->getText()];
      if (l_Val->getType()->getTypeTag() == TypeTag::TT_INT32) {
        l_Val = module.addSitofpInst(l_Val, "lval");
      } else if (l_Val->getType()->getTypeTag() == TypeTag::TT_INT1) {
        l_Val = module.addZextInst(l_Val, Type::getInt32Type(), "lVal");
        l_Val = module.addSitofpInst(l_Val, "lval");
      }
      if (r_Val->getType()->getTypeTag() == TypeTag::TT_INT32) {
        r_Val = module.addSitofpInst(r_Val, "rval");
      } else if (r_Val->getType()->getTypeTag() == TypeTag::TT_INT1) {
        r_Val = module.addZextInst(r_Val, Type::getInt32Type(), "rVal");
        r_Val = module.addSitofpInst(r_Val, "rval");
      }
      l_Val = module.addFcmpInst(t, l_Val, r_Val, "addres");
    } else {
      t = Integer_Operator_OpTag_Table[ctx->op[i]->getText()];
      if (i > 0) {
        l_Val = module.addZextInst(l_Val, Type::getInt32Type(), "lval");
      }
      l_Val = module.addIcmpInst(t, l_Val, r_Val, "addres");
    }
  }
  currentRet = l_Val;
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitEqExp(
    SysYParserParser::EqExpContext* ctx) {
  visit(ctx->left);
  Value* l_Val = currentRet;
  Value* r_Val;
  // if (l_Val && l_Val->isPointer()) {
  //   LoadInst* li = module.addLoadInst(l_Val, "lval");
  //   l_Val = li;
  // }
  if (l_Val->getType()->getTypeTag() == TT_INT1 && ctx->op.size() > 0) {
    if (l_Val->isa(VT_BOOLCONST)) {
      l_Val = IntegerConstant::getConstInt(((BoolConstant*)l_Val)->getValue());
    } else {
      l_Val = module.addZextInst(l_Val, Type::getInt32Type(), "zext");
    }
  }
  for (int i = 0; i < ctx->op.size(); ++i) {
    visit(ctx->right[i]);
    r_Val = currentRet;
    if (r_Val->isPointer()) {
      r_Val = module.addLoadInst(r_Val, "rval");
    }
    if (r_Val->getType()->getTypeTag() == TT_INT1) {
      if (r_Val->isa(VT_BOOLCONST)) {
        r_Val =
            IntegerConstant::getConstInt(((BoolConstant*)r_Val)->getValue());
      } else {
        r_Val = module.addZextInst(r_Val, Type::getInt32Type(), "zext");
      }
    }
    // lhs and rhs both are constant
    if (dynamic_cast<Constant*>(l_Val) && dynamic_cast<Constant*>(r_Val)) {
      if (l_Val->isa(VT_FLOATCONST) || r_Val->isa(VT_FLOATCONST)) {
        // result is float

        float lhs, rhs;
        if (l_Val->isa(VT_INTCONST)) {
          lhs = ((IntegerConstant*)l_Val)->getValue();
        } else {
          lhs = ((FloatConstant*)l_Val)->getValue();
        }
        if (r_Val->isa(VT_INTCONST)) {
          rhs = ((IntegerConstant*)r_Val)->getValue();
        } else {
          rhs = ((FloatConstant*)r_Val)->getValue();
        }
        int ret = 0;
        if (ctx->op[i]->getText() == "==") {
          ret = (lhs == rhs) ? 1 : 0;
        } else if (ctx->op[i]->getText() == "!=") {
          ret = (lhs != rhs) ? 1 : 0;
        }
        l_Val = BoolConstant::getConstBool(ret);
        continue;
      } else {
        // result is integer
        int lhs = ((IntegerConstant*)l_Val)->getValue();
        int rhs = ((IntegerConstant*)r_Val)->getValue();
        int ret_int;
        if (ctx->op[i]->getText() == "==") {
          ret_int = lhs == rhs;
        } else if (ctx->op[i]->getText() == "!=") {
          ret_int = lhs != rhs;
        }
        l_Val = BoolConstant::getConstBool(ret_int);
        continue;
      }
    }
    OpTag t;
    if ((l_Val->getType()->getTypeTag() == TypeTag::TT_FLOAT) ||
        (r_Val->getType()->getTypeTag() == TypeTag::TT_FLOAT)) {
      t = F_Operator_OpTag_Table[ctx->op[i]->getText()];
      if (l_Val->getType()->getTypeTag() == TypeTag::TT_INT32) {
        l_Val = module.addSitofpInst(l_Val, "lval");
      } else if (l_Val->getType()->getTypeTag() == TypeTag::TT_INT1) {
        l_Val = module.addZextInst(l_Val, Type::getInt32Type(), "lVal");
        l_Val = module.addSitofpInst(l_Val, "lval");
      }
      if (r_Val->getType()->getTypeTag() == TypeTag::TT_INT32) {
        r_Val = module.addSitofpInst(r_Val, "rval");
      } else if (r_Val->getType()->getTypeTag() == TypeTag::TT_INT1) {
        r_Val = module.addZextInst(r_Val, Type::getInt32Type(), "rVal");
        r_Val = module.addSitofpInst(r_Val, "rval");
      }
      l_Val = module.addFcmpInst(t, l_Val, r_Val, "eqres");
    } else {
      t = Integer_Operator_OpTag_Table[ctx->op[i]->getText()];
      if (i > 0) {
        l_Val = module.addZextInst(l_Val, Type::getInt32Type(), "lval");
      }
      l_Val = module.addIcmpInst(t, l_Val, r_Val, "eqres");
    }
  }
  currentRet = l_Val;
  return nullptr;
}
antlrcpp::Any MySysYParserVisitor::visitLValSingle(
    SysYParserParser::LValSingleContext* ctx) {
  Value* get = current->get(ctx->getText());

  if (get->isPointer()) {
    PointerType* type = (PointerType*)get->getType();
    if (get->isa(VT_ARG)) {
      currentRet = get;
    } else if (type->getElemType()->getTypeTag() == TT_ARRAY) {
      currentRet = module.addGetElemPtrInst(get, zero, zero, "getptr");
    } else if (get->isa(VT_ALLOCA) || get->isa(VT_GLOBALVAR)) {
      currentRet = module.addLoadInst(get, "get");
    } else {
      assert(0);
    }
  } else {
    currentRet = get;
  }
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitLValArray(
    SysYParserParser::LValArrayContext* ctx) {
  Value* ptr = current->get(ctx->Identifier()->getText());
  if (ptr->getValueTag() == ValueTag::VT_ARG) {
    visit(ctx->exp(0));
    ptr = module.addGetElemPtrInst(ptr, currentRet, "ret");
    for (int i = 1; i < ctx->exp().size(); ++i) {
      visit(ctx->exp(i));
      ptr = module.addGetElemPtrInst(ptr, zero, currentRet, "ret");
    }
  } else {
    for (int i = 0; i < ctx->exp().size(); ++i) {
      visit(ctx->exp(i));
      ptr = module.addGetElemPtrInst(ptr, zero, currentRet, "ret");
    }
  }
  if (((PointerType*)ptr->getType())->getElemType()->getTypeTag() !=
      TypeTag::TT_ARRAY) {
    Value* realValue = module.addLoadInst(ptr, "int");
    currentRet = realValue;
  } else {
    currentRet = module.addGetElemPtrInst(ptr, zero, zero, "getptr");
  }
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitStmtAssign(
    SysYParserParser::StmtAssignContext* ctx) {
  auto* lVal = dynamic_cast<SysYParserParser::LValSingleContext*>(ctx->lVal());
  if (lVal != nullptr) {
    Value* addr = current->get(lVal->Identifier()->getText());
    visit(ctx->exp());
    currentRet = tranformType(currentRet,
                              ((PointerType*)addr->getType())->getElemType());
    module.addStoreInst(currentRet, addr);
  } else {
    auto* lVal = dynamic_cast<SysYParserParser::LValArrayContext*>(ctx->lVal());
    Value* ptr = current->get(lVal->Identifier()->getText());
    if (ptr->getValueTag() == ValueTag::VT_ARG) {
      visit(lVal->exp(0));
      ptr = module.addGetElemPtrInst(ptr, currentRet, "ret");
      for (int i = 1; i < lVal->exp().size(); ++i) {
        visit(lVal->exp(i));
        ptr = module.addGetElemPtrInst(ptr, zero, currentRet, "ret");
      }
    } else {
      for (int i = 0; i < lVal->exp().size(); ++i) {
        visit(lVal->exp(i));
        ptr = module.addGetElemPtrInst(ptr, zero, currentRet, "ret");
      }
    }
    visit(ctx->exp());
    currentRet =
        tranformType(currentRet, ((PointerType*)ptr->getType())->getElemType());

    module.addStoreInst(currentRet, ptr);
  }
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitUnaryExpUnary(
    SysYParserParser::UnaryExpUnaryContext* ctx) {
  visit(ctx->unaryExp());
  if (ctx->unaryOp()->getText() == "-") {
    if (dynamic_cast<Constant*>(currentRet)) {
      // is a constant
      if (currentRet->isa(VT_INTCONST)) {
        int int_val = ((IntegerConstant*)currentRet)->getValue();
        currentRet = IntegerConstant::getConstInt(-int_val);
      } else {
        float float_val = ((FloatConstant*)currentRet)->getValue();
        currentRet = FloatConstant::getConstFloat(-float_val);
      }
    } else {
      if (currentRet->getType()->getTypeTag() == TT_INT1) {
        currentRet =
            module.addZextInst(currentRet, Type::getInt32Type(), "zext.ret");
        currentRet = module.addBinaryOpInst(SUB, zero, currentRet, "minus");
      } else if (currentRet->getType()->getTypeTag() == TT_INT32) {
        currentRet = module.addBinaryOpInst(SUB, zero, currentRet, "minus");
      } else {
        FloatConstant* fzero = FloatConstant::getConstFloat(0);
        currentRet = module.addBinaryOpInst(FSUB, fzero, currentRet, "minus");
      }
    }
  } else if (ctx->unaryOp()->getText() == "!") {
    if (dynamic_cast<Constant*>(currentRet)) {
      if (currentRet->isa(VT_INTCONST)) {
        int int_val = ((IntegerConstant*)currentRet)->getValue();
        currentRet = IntegerConstant::getConstInt(!int_val);
      } else {
        float float_val = ((FloatConstant*)currentRet)->getValue();
        currentRet = FloatConstant::getConstFloat(!float_val);
      }
    } else {
      if (currentRet->getType()->getTypeTag() == TT_INT1) {
        currentRet =
            module.addZextInst(currentRet, Type::getInt32Type(), "zext.ret");
        currentRet = module.addIcmpInst(EQ, zero, currentRet, "eq.ret");
      } else if (currentRet->getType()->getTypeTag() == TT_INT32) {
        currentRet = module.addIcmpInst(EQ, zero, currentRet, "eq.ret");
      } else {
        currentRet = module.addFcmpInst(OEQ, zero, currentRet, "oeq.ret");
      }
    }
  }
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitIntOctConst(
    SysYParserParser::IntOctConstContext* ctx) {
  int val = stoi(ctx->getText(), nullptr, 0);
  Value* num = IntegerConstant::getConstInt(val);
  currentRet = num;
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitIntDecConst(
    SysYParserParser::IntDecConstContext* ctx) {
  int val = stoi(ctx->getText(), nullptr, 0);
  Value* num = IntegerConstant::getConstInt(val);
  currentRet = num;
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitIntHexConst(
    SysYParserParser::IntHexConstContext* ctx) {
  int val = stoi(ctx->getText(), nullptr, 0);
  Value* num = IntegerConstant::getConstInt(val);
  currentRet = num;
  return nullptr;
}

antlrcpp::Any MySysYParserVisitor::visitFloatConst(
    SysYParserParser::FloatConstContext* ctx) {
  float val = stof(ctx->getText());
  Value* num = FloatConstant::getConstFloat(val);
  currentRet = num;
  return nullptr;
}

void MySysYParserVisitor::buildVariable(std::string name, Value* val) {
  if (current->parent == nullptr) {
    GlobalVariable* gv =
        module.addGlobalVariable(val->getType(), (Constant*)val, name);
    current->put(name, gv);
    currentRet = gv;
  } else {
    AllocaInst* inst = module.addAllocaInst(val->getType(), name + ".addr");
    module.addStoreInst(val, inst);
    current->put(name, inst);
    currentRet = inst;
  }
}

void MySysYParserVisitor::buildVariable(std::string name, Type* type) {
  if (current->parent == nullptr) {
    Value* md = module.addGlobalVariable(type, name);
    current->put(name, md);
  } else {
    AllocaInst* inst = module.addAllocaInst(type, name + ".addr");
    current->put(name, inst);
  }
}

void MySysYParserVisitor::setModule(ANTPIE::Module m) { module = m; }

MySysYParserVisitor::MySysYParserVisitor(VariableTable* current)
    : current(current) {}

void MySysYParserVisitor::setArrayInitVal(Value* arr,
                                          ArrayConstant* arrayValue) {
  if (arr->isa(VT_GLOBALVAR)) {
    // Global variable
    GlobalVariable* garr = dynamic_cast<GlobalVariable*>(arr);
    garr->setInitValue(arrayValue);
    return;
  }
  // local array
  module.pushExternFunction(memsetFunc);
  Value* addr = arr;
  std::function<void(ArrayConstant*)> arrayInitDfs =
      [&](ArrayConstant* arrayConst) {
        ArrayType* arrayType = (ArrayType*)arrayConst->getType();
        int len = arrayType->getLen();
        int elemWidth = arrayType->getWidth() / len;
        auto* elemMap = arrayConst->getElementMap();
        int preLoc = -1;
        if (arrayType->getElemType()->getTypeTag() != TT_ARRAY) {
          for (auto [loc, elem] : *elemMap) {
            if (loc - preLoc > 1) {
              // memset (preLoc + 1, 0, loc - preLoc - 1)
              Value* beginAddr = module.addGetElemPtrInst(
                  addr, zero, IntegerConstant::getConstInt(preLoc + 1),
                  "begin.addr");
              vector<Value*> params{
                  beginAddr, zero,
                  IntegerConstant::getConstInt((loc - preLoc - 1) << 2)};
              module.addCallInst(memsetFunc, params, "");
            }
            auto finalAddr = module.addGetElemPtrInst(
                addr, zero, new IntegerConstant(loc), "arr.faddr");
            elem = tranformType(
                elem, ((PointerType*)finalAddr->getType())->getElemType());
            module.addStoreInst(elem, finalAddr);
            preLoc = loc;
          }
          if (len - preLoc > 1) {
            // memset (preLoc + 1, 0, loc - preLoc - 1)
            Value* beginAddr = module.addGetElemPtrInst(
                addr, zero, IntegerConstant::getConstInt(preLoc + 1),
                "begin.addr");
            vector<Value*> params{
                beginAddr, zero,
                IntegerConstant::getConstInt((len - preLoc - 1) << 2)};
            module.addCallInst(memsetFunc, params, "");
          }

        } else {
          Value* oldAddr = addr;
          for (auto [loc, elem] : *elemMap) {
            if (loc - preLoc > 1) {
              // memset (preLoc + 1, 0, (loc - preLoc - 1) * width)
              Value* beginAddr = module.addGetElemPtrInst(
                  oldAddr, zero, IntegerConstant::getConstInt(preLoc + 1),
                  "begin.addr");
              vector<Value*> params{
                  beginAddr, zero,
                  IntegerConstant::getConstInt((loc - preLoc - 1) * elemWidth)};
              module.addCallInst(memsetFunc, params, "");
            }
            addr = module.addGetElemPtrInst(
                oldAddr, zero, new IntegerConstant(loc), "arr.addr");
            arrayInitDfs((ArrayConstant*)elem);
            preLoc = loc;
          }
          if (len - preLoc > 1) {
            // memset (preLoc + 1, 0, loc - preLoc - 1)
            Value* beginAddr = module.addGetElemPtrInst(
                oldAddr, zero, IntegerConstant::getConstInt(preLoc + 1),
                "begin.addr");
            vector<Value*> params{
                beginAddr, zero,
                IntegerConstant::getConstInt((len - preLoc - 1) * elemWidth)};
            module.addCallInst(memsetFunc, params, "");
          }
          addr = oldAddr;
        }
      };
  arrayInitDfs(arrayValue);
}

void MySysYParserVisitor::init_extern_function() {
  Type* i32Type = Type::getInt32Type();
  Type* floatType = Type::getFloatType();
  Type* voidType = Type::getVoidType();
  Type* intPtrType = Type::getPointerType(i32Type);
  Type* floatPtrType = Type::getPointerType(floatType);
  FuncType* funcType;
  Function* function;

  vector<Function*> externFunctions;

  // int memset()
  vector<Argument*> memsetArgs = {new Argument("p", intPtrType),
                                  new Argument("val", i32Type),
                                  new Argument("cnt", i32Type)};
  funcType = Type::getFuncType(voidType, memsetArgs);
  function = new Function(funcType, true, "memset");
  externFunctions.push_back(function);
  memsetFunc = function;

  // int getint()
  funcType = Type::getFuncType(i32Type);
  function = new Function(funcType, true, "getint");
  externFunctions.push_back(function);

  // int getch()
  funcType = Type::getFuncType(i32Type);
  function = new Function(funcType, true, "getch");
  externFunctions.push_back(function);

  // float getfloat()
  funcType = Type::getFuncType(floatType);
  function = new Function(funcType, true, "getfloat");
  externFunctions.push_back(function);

  // int getarray(int[])
  vector<Argument*> getarrayArgs = {new Argument("arr", intPtrType)};
  funcType = Type::getFuncType(i32Type, getarrayArgs);
  function = new Function(funcType, true, "getarray");
  externFunctions.push_back(function);

  // int getfarray(int[])
  vector<Argument*> getfarrayArgs = {new Argument("arr", floatPtrType)};
  funcType = Type::getFuncType(i32Type, getfarrayArgs);
  function = new Function(funcType, true, "getfarray");
  externFunctions.push_back(function);

  // void putint(int)
  vector<Argument*> putintArgs = {new Argument("i", i32Type)};
  funcType = Type::getFuncType(voidType, putintArgs);
  function = new Function(funcType, true, "putint");
  externFunctions.push_back(function);

  // void putch(int)
  vector<Argument*> putchArgs = {new Argument("c", i32Type)};
  funcType = Type::getFuncType(voidType, putchArgs);
  function = new Function(funcType, true, "putch");
  externFunctions.push_back(function);

  // void putfloat(float)
  vector<Argument*> putfloatArgs = {new Argument("f", floatType)};
  funcType = Type::getFuncType(voidType, putfloatArgs);
  function = new Function(funcType, true, "putfloat");
  externFunctions.push_back(function);

  // void putarray(int,int[])
  vector<Argument*> putarrayArgs = {new Argument("n", i32Type),
                                    new Argument("arr", intPtrType)};
  funcType = Type::getFuncType(voidType, putarrayArgs);
  function = new Function(funcType, true, "putarray");
  externFunctions.push_back(function);

  // void putfarray(int,float[])
  vector<Argument*> putfarrayArgs = {new Argument("n", i32Type),
                                     new Argument("arr", floatPtrType)};
  funcType = Type::getFuncType(voidType, putfarrayArgs);
  function = new Function(funcType, true, "putfarray");
  externFunctions.push_back(function);

  // void putf(<format>, int, …)putf ??

  // void starttime() -> void _sysy_starttime(int line)
  vector<Argument*> startArgs = {new Argument("l", i32Type)};
  funcType = Type::getFuncType(voidType, startArgs);
  function = new Function(funcType, true, "_sysy_starttime");
  current->put("starttime", function);

  // void stoptime() -> void _sysy_stoptime(int line)
  vector<Argument*> stopArgs = {new Argument("l", i32Type)};
  funcType = Type::getFuncType(voidType, stopArgs);
  function = new Function(funcType, true, "_sysy_stoptime");
  current->put("stoptime", function);

  // int antpie_multimod(int x, int y, int m)
  vector<Argument*> mmArgs = {new Argument("x", i32Type),
                              new Argument("y", i32Type),
                              new Argument("m", i32Type)};
  funcType = Type::getFuncType(i32Type, mmArgs);
  function = new Function(funcType, true, "antpie_multimod");
  externFunctions.push_back(function);
  
  for (Function* function : externFunctions) {
    current->put(function->getName(), function);
    module.addLibFunction(function);
  }
}

Value* MySysYParserVisitor::tranformType(Value* src, Type* dest) {
  if (src->getType()->getTypeTag() == dest->getTypeTag()) return src;
  if (src->isInteger()) {
    if (src->isa(VT_INTCONST)) {
      int val_int = ((IntegerConstant*)src)->getValue();
      return FloatConstant::getConstFloat((float)val_int);
    }
    return module.addSitofpInst(src, "s2f");
  } else {
    if (src->isa(VT_FLOATCONST)) {
      int val_float = ((FloatConstant*)src)->getValue();
      return IntegerConstant::getConstInt((int)val_float);
    }
    return module.addFptosiInst(src, "f2s");
  }
}
