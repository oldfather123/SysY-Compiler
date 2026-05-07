//
// Created by 陈硕 on 2023/6/16.
//

#include "MyVisitor.h"
#include "Utils.h"
#include <memory>

using namespace std;

ValueRef* getConstArrayValue(vector<ValueRef*>& idxs, Array_Const* constVal){
    any val = constVal->values;
    for(ValueRef* idx: idxs){
        assert(idx->type == IntConst);
        int i = stoi(idx->get_Ref());
        auto v = any_cast<vector<any>>(val);
        if(i >= v.size())
            return nullptr; // 越界了，说明此处未初始化，返回0
        else
            val = v[i];
    }
    return any_cast<ValueRef*>(val);
}

ValueRef * Parse_Any(std::any input){
    // 真是奇怪，将Pointer* any_cast ValueRef* 都不行
    if(input.type() == typeid(Pointer*)){
        return any_cast<Pointer*>(input);
    }
    else if(input.type() == typeid(Array*)){
        return any_cast<Array*>(input);
    }
    return any_cast<ValueRef*>(input);
}

std::any MyVisitor::visitProgram(antlr4::SysYParser::ProgramContext *ctx) {
    globalScope = new Scope(nullptr, 0);
    curScope = globalScope;

    antlr4::SysYParserBaseVisitor::visitProgram(ctx);
    vector<Function *> func_vector;
    func_vector.reserve(globalUnit->func_table.size());
    for(auto & it : globalUnit->func_table){
        func_vector.push_back(it.second);
    }
    return nullptr;
}


Type* getTypeByStr(const string& type) {
    if (type == "void")
        return new Type(VOIDTYPE);
    if (type == "int")
        return new Type(INT32TYPE);
    if (type == "float")
        return new Type(FLOATTYPE);
    cerr <<"no type"<<endl;
    return nullptr;
}

bool block_ret;
Symbol * retVal;
BasicBlock * retBlock;

std::any MyVisitor::visitFuncDef(antlr4::SysYParser::FuncDefContext *ctx) {
    std::string funcName = ctx->IDENT()->getText();
    //生成返回值类型

    string retTypeCtx = ctx->funcType()->getText();
    Type* retType = getTypeByStr(retTypeCtx);

    //生成参数类型
    std::string args;

    vector<Type*> paramsTypes;
    int paramsCnt = 0;
    if (ctx->funcFParams() != nullptr) {
        paramsCnt = ctx->funcFParams()->funcFParam().size();
        for (int i = 0; i < paramsCnt; ++i) {
            Type* paramType = getTypeByStr(ctx->funcFParams()->funcFParam(i)->bType()->getText());
            if (!ctx->funcFParams()->funcFParam(i)->L_BRACKT().empty()) {
                vector<antlr4::SysYParser::ExpContext*> v = ctx->funcFParams()->funcFParam(i)->exp();
                for(int j = v.size()-1; j >= 0; j--){
                    auto* size = any_cast<ValueRef*>(MyVisitor::visit(v[j]));
                    assert(size->type == IntConst);
                    paramType = new ArrayType(paramType,stoi(size->get_Ref()));
                }
                paramType = new PointerType(paramType);
            }
            paramsTypes.push_back(paramType);
        }

    }

    //生成函数类型
    Function* function = globalUnit->addFunc(funcName, retType, paramsTypes);
    curFunc = function;
    builder->curFunc = function;

    string retName = funcName + "Ret";
    retBlock = new BasicBlock(retName,function);

    string startName = funcName + "Start";
    BasicBlock* startBlock = builder->AppendBasicBlock(curFunc,startName);
    curFunc->entry = startBlock;

    string entryName = funcName + "Entry";
    BasicBlock* entryBlock = builder->AppendBasicBlock(curFunc,entryName);

    builder->PositionBuilderAtEnd(entryBlock);
    Utils::setTempCounter(0); // 从这里开始进入函数，Counter清零

    curScope = new Scope(curScope, ++scopeCounter);
    string name = "retVal";
    if(retType->type == VOIDTYPE){
        retVal = nullptr;
    }
    else if(retType->type == INT32TYPE){
        retVal = curScope->addVar(builder,retType,name,false,new Int_Const(0));
        globalUnit->addSymbol(curFunc,retVal->name,retVal);
    }
    else{ // float
        retVal = curScope->addVar(builder,retType,name,false,new Float_Const(0.0));
        globalUnit->addSymbol(curFunc,retVal->name,retVal);
    }

    for (int i = 0; i < paramsCnt; ++i) {
        string paramName = ctx->funcFParams()->funcFParam(i)->IDENT()->getText();
        Type* paramType = paramsTypes[i];

        ValueRef* argVal;

        if (paramType->type == INT32TYPE)
            argVal = new Int_Var();
        else if (paramType->type == FLOATTYPE)
            argVal = new Float_Var();
        else //POINTERTYPE
            argVal = new Pointer((PointerType*)paramType);



        Symbol* symbol = curScope->addVar(builder, paramType, paramName, false, argVal);
        globalUnit->addSymbol(curFunc,symbol->name,symbol);

        args += i==0 ? paramType->getTypeName()+" "+argVal->get_Ref() : ", "+paramType->getTypeName()+" "+argVal->get_Ref();
    }

    std::string funcDef1 = "define " + retType->getTypeName() + " @" + funcName + "(" + args + ")" + " {\n";
    function->debugInfo = funcDef1;

    block_ret = false;
    MyVisitor::visitBlock(ctx->block());

    //DONE:buildRet
    if (!block_ret){
        BuildBr(builder,retBlock);
    }

    builder->PositionBuilderAtEnd(startBlock);
    BuildBr(builder,entryBlock);

    builder->PositionBuilderAtEnd(retBlock);
    if(retType->type == VOIDTYPE){
        BuildRet(builder,nullptr);
    }
    else{
        ValueRef * ret = BuildLoad(builder,retVal);
        BuildRet(builder,ret);
    }
    curFunc->block_list.push_back(retBlock);

    curScope = curScope->super;
    curFunc = nullptr;
    builder->PositionBuilderAtEnd(nullptr);

    return "";
}

std::any MyVisitor::visitBlock(antlr4::SysYParser::BlockContext *ctx) {
    block_ret = false;
    curScope = new Scope(curScope, ++scopeCounter);
    SysYParserBaseVisitor::visitBlock(ctx);
    curScope = curScope->super;
    //block_ret = false;
    return nullptr;
}

std::any MyVisitor::visitBlockItem(antlr4::SysYParser::BlockItemContext *ctx) {
    if (block_ret)
        return nullptr;

    return SysYParserBaseVisitor::visitBlockItem(ctx);
}

std::any MyVisitor::visitVarDecl(antlr4::SysYParser::VarDeclContext *ctx){
    for(antlr4::SysYParser::VarDefContext* varDefContext : ctx->varDef()) {
        string varName = varDefContext->IDENT()->getText();
        if (varName.length() > 31) varName = varName.substr(0, 5) + "_";

        string type = ctx->bType()->getText();

        ValueRef* varInitValRef;

        if (varDefContext->L_BRACKT().empty()) {//变量
            if (varDefContext->ASSIGN() != nullptr) {
                varInitValRef = any_cast<ValueRef*>(MyVisitor::visit(varDefContext->initVal()));
            } else {
                varInitValRef = nullptr;
            }

            //Alloca-Init-Def
            if (type == "int") {
                if(varInitValRef != nullptr) varInitValRef = BuildFPTrunc(builder,varInitValRef); // if init != null, convert initVal to int
                Symbol* symbol = curScope->addVar(builder, new Type(INT32TYPE), varName,false, varInitValRef);
                globalUnit->addSymbol(curFunc,symbol->name,symbol);
            } else { // float
                if(varInitValRef != nullptr) varInitValRef = BuildSitofp(builder,varInitValRef); // if init != null, convert initVal to float
                Symbol* symbol = curScope->addVar(builder, new Type(FLOATTYPE), varName,false, varInitValRef);
                globalUnit->addSymbol(curFunc,symbol->name,symbol);
            }
        } else {//数组赋值情况额外考虑
            vector<int> v;
            for(antlr4::SysYParser::ConstExpContext* exp: varDefContext->constExp()){
                auto* ret = any_cast<ValueRef*>(MyVisitor::visitConstExp(exp));
                assert(ret->type == IntConst);
                v.push_back(stoi(ret->get_Ref()));
            }
            Type * arrType = new Type((type == "int")? INT32TYPE:FLOATTYPE);
            for(int i=v.size()-1;i>=0;i--){
                arrType = new ArrayType(arrType,v[i]);
            }

            if (varDefContext->ASSIGN() != nullptr) {
                auto values = any_cast<vector<std::any>>(getArrInitVal(arrType,varDefContext->initVal()));
                varInitValRef = new Array_Const(values);
            } else {
                varInitValRef = nullptr;
            }
            Symbol* symbol = curScope->addVar(builder,arrType,varName,false,varInitValRef);
            globalUnit->addSymbol(curFunc,symbol->name,symbol);
        }
    }
    return nullptr;
}

std::any MyVisitor::visitConstDecl(antlr4::SysYParser::ConstDeclContext *ctx) {
    for(antlr4::SysYParser::ConstDefContext* constDefContext : ctx->constDef()) {
        string constName = constDefContext->IDENT()->getText();
        if (constName.length() > 31) constName = constName.substr(0, 5) + "_";
        string type = ctx->bType()->getText();

        std::any constInitVal;
        ValueRef* constInitValRef;

        if (constDefContext->L_BRACKT().empty()) {//变量
            constInitVal = MyVisitor::visit(constDefContext->constInitVal());
            constInitValRef = any_cast<ValueRef*>(constInitVal);

            if (type == "int") {
                if(constInitValRef != nullptr) constInitValRef = BuildFPTrunc(builder,constInitValRef); // if init != null, try to convert initVal to int
                Symbol* symbol = curScope->addVar(builder, new Type(INT32TYPE), constName,true, constInitValRef);
                globalUnit->addSymbol(curFunc,symbol->name,symbol);
            } else {
                if(constInitValRef != nullptr) constInitValRef = BuildSitofp(builder,constInitValRef); // if init != null, try to convert initVal to float
                Symbol* symbol = curScope->addVar(builder, new Type(FLOATTYPE), constName, true,constInitValRef);
                globalUnit->addSymbol(curFunc,symbol->name,symbol);
            }

        } else {//数组
            vector<int> v;
            for(antlr4::SysYParser::ConstExpContext* exp: constDefContext->constExp()){
                auto* ret = any_cast<ValueRef*>(antlr4::SysYParserBaseVisitor::visitConstExp(exp));
                assert(ret->type == IntConst);
                v.push_back(stoi(ret->get_Ref()));
            }
            Type * arrType = new Type((type == "int")? INT32TYPE:FLOATTYPE);
            for(int i=v.size()-1;i>=0;i--){
                arrType = new ArrayType(arrType,v[i]);
            }

            if (constDefContext->ASSIGN() != nullptr) {
                auto values = any_cast<vector<std::any> >(getArrInitVal(arrType,constDefContext->constInitVal()));
                constInitValRef = new Array_Const(values);
            } else {
                constInitValRef = nullptr;
            }
            Symbol* symbol = curScope->addVar(builder,arrType,constName,true,constInitValRef);
            globalUnit->addSymbol(curFunc,symbol->name,symbol);
        }
    }
    return nullptr;
}

bool hasLogicCond = false;




std::any MyVisitor::visitIfStmt(antlr4::SysYParser::IfStmtContext *ctx) {

    string trueBlkName = "if.then";
    auto* trueBlock = new BasicBlock(trueBlkName, curFunc);
    string nextBlkName = "if.end";
    auto* nextBlock = new BasicBlock(nextBlkName, curFunc);

    TBlock = trueBlock;
    FBlock = nextBlock;


    any condRes = MyVisitor::visit(ctx->cond());

    if (!hasLogicCond) {
        auto* condVal = any_cast<ValueRef*>(condRes);
        ValueRef* condBool = BuildCmp(builder, condVal, zero, NE);
        BuildCondBr(builder, condBool, trueBlock, nextBlock);
    } else {
        hasLogicCond = false;
    }

    //if_true

    curFunc->appendBlock(trueBlock);
    builder->PositionBuilderAtEnd(trueBlock);


    MyVisitor::visit(ctx->stmt());

    if (!block_ret)
        BuildBr(builder, nextBlock);

    block_ret = false;

    //if_next

    curFunc->appendBlock(nextBlock);
    builder->PositionBuilderAtEnd(nextBlock);

    return nullptr;
}

std::any MyVisitor::visitIfElseStmt(antlr4::SysYParser::IfElseStmtContext *ctx) {

    string trueBlkName = "if.then";
    auto* trueBlock = new BasicBlock(trueBlkName, curFunc);
    string falseBlkName = "if.else";
    auto* falseBlock = new BasicBlock(falseBlkName, curFunc);
    string nextBlkName = "if.end";
    BasicBlock* nextBlock = nullptr;

    TBlock = trueBlock;
    FBlock = falseBlock;


    any condRes = MyVisitor::visit(ctx->cond());

    if (!hasLogicCond) {
        auto* condVal = any_cast<ValueRef*>(condRes);
        ValueRef* condBool = BuildCmp(builder, condVal, zero, NE);
        BuildCondBr(builder, condBool, trueBlock, falseBlock);
    } else {
        hasLogicCond = false;
    }


    bool blk_ret1, blk_ret2;
    //if_true

    curFunc->appendBlock(trueBlock);
    builder->PositionBuilderAtEnd(trueBlock);


    MyVisitor::visit(ctx->stmt(0));
    blk_ret1 = block_ret;
    block_ret = false;


    if (!blk_ret1) {
        nextBlock = new BasicBlock(nextBlkName, curFunc);
        BuildBr(builder, nextBlock);
    }


    //if_false
    curFunc->appendBlock(falseBlock);
    builder->PositionBuilderAtEnd(falseBlock);

    MyVisitor::visit(ctx->stmt(1));
    blk_ret2 = block_ret;
    block_ret = false;


    if (!blk_ret2) {
        if (nextBlock == nullptr)
            nextBlock = new BasicBlock(nextBlkName, curFunc);
        BuildBr(builder, nextBlock);
    }

    //if_next
    if (nextBlock != nullptr){
        curFunc->appendBlock(nextBlock);
        builder->PositionBuilderAtEnd(nextBlock);
    }


    block_ret = blk_ret1 && blk_ret2;
    return nullptr;
}

std::any MyVisitor::visitWhileStmt(antlr4::SysYParser::WhileStmtContext *ctx) {

//    string condBlkName = "whileCond";
//    BasicBlock* condBlock = builder->AppendBasicBlock(curFunc, condBlkName);
//    BuildBr(builder, condBlock);
//
//    string bodyBlkName = "whileBody";
//    auto* bodyBlock = new BasicBlock(bodyBlkName, curFunc);
//
//    string nextBlkName = "whileNext";
//    auto* nextBlock = new BasicBlock(nextBlkName, curFunc);
//
//
//    TBlock = bodyBlock;
//    FBlock = nextBlock;
//
//    //while_cond
//    builder->PositionBuilderAtEnd(condBlock);
//    any condRes = MyVisitor::visit(ctx->cond());
//
//    if (!hasLogicCond) {
//        auto* condVal = any_cast<ValueRef*>(condRes);
//        ValueRef* condBool = BuildCmp(builder, condVal, zero, NE);
//        BuildCondBr(builder, condBool, bodyBlock, nextBlock);
//    } else {
//        hasLogicCond = false;
//    }
//
//    //while_body
//    curFunc->appendBlock(bodyBlock);
//    builder->PositionBuilderAtEnd(bodyBlock);
//
//    whileCondStk.push(condBlock);
//    exitWhileStk.push(nextBlock);
//    MyVisitor::visit(ctx->stmt());
//    whileCondStk.pop();
//    exitWhileStk.pop();
//
//    if (!block_ret)
//        BuildBr(builder, condBlock);
//
//    //exit_while
//    curFunc->appendBlock(nextBlock);
//    builder->PositionBuilderAtEnd(nextBlock);
//    block_ret = false;
//    return nullptr;

    // do-while

    string condBlkName = "whileCond";
    BasicBlock* condBlock = builder->AppendBasicBlock(curFunc, condBlkName);
    BuildBr(builder, condBlock);

    string bodyBlkName = "doWhileBody";
    auto bodyBlock = new BasicBlock(bodyBlkName, curFunc);

    string whileCondBlkName = "doWhileCond";
    auto doWhileCondBlock = new BasicBlock(whileCondBlkName, curFunc);

    string nextBlkName = "whileNext";
    auto nextBlock = new BasicBlock(nextBlkName, curFunc);

    TBlock = bodyBlock;
    FBlock = nextBlock;

    //while_cond
    builder->PositionBuilderAtEnd(condBlock);
    any condRes = MyVisitor::visit(ctx->cond());

    if (!hasLogicCond) {
        auto* condVal = any_cast<ValueRef*>(condRes);
        ValueRef* zext = BuildZext(builder,condVal);
        ValueRef* condBool = BuildCmp(builder, zext, zero, NE);
        BuildCondBr(builder, condBool, bodyBlock, nextBlock);
    } else {
        hasLogicCond = false;
    }

    //while_body
    curFunc->appendBlock(bodyBlock);
    builder->PositionBuilderAtEnd(bodyBlock);

    whileCondStk.push(doWhileCondBlock);
    exitWhileStk.push(nextBlock);
    MyVisitor::visit(ctx->stmt());
    whileCondStk.pop();
    exitWhileStk.pop();

    if (!block_ret) {
        BuildBr(builder, doWhileCondBlock);
    }

    //do_while_cond
    curFunc->appendBlock(doWhileCondBlock);
    builder->PositionBuilderAtEnd(doWhileCondBlock);

    TBlock = bodyBlock;
    FBlock = nextBlock;

    any cond = MyVisitor::visit(ctx->cond());
    if (!hasLogicCond) {
        auto condVal = any_cast<ValueRef*>(cond);
        auto condBool = BuildCmp(builder, condVal, zero, NE);
        BuildCondBr(builder, condBool, bodyBlock, nextBlock);
    } else {
        hasLogicCond = false;
    }

    //exit_while
    curFunc->appendBlock(nextBlock);
    builder->PositionBuilderAtEnd(nextBlock);
    block_ret = false;
    return nullptr;
}


std::any MyVisitor::visitBreakStmt(antlr4::SysYParser::BreakStmtContext *ctx) {
    block_ret = true;
    BuildBr(builder, exitWhileStk.top());
    return nullptr;
}

std::any MyVisitor::visitContinueStmt(antlr4::SysYParser::ContinueStmtContext *ctx) {
    block_ret = true;
    BuildBr(builder, whileCondStk.top());
    return nullptr;
}

std::any MyVisitor::visitReturnStmt(antlr4::SysYParser::ReturnStmtContext *ctx) {
    block_ret = true;

    ValueRef* ret = nullptr;
    if (ctx->exp() != nullptr)
        ret = any_cast<ValueRef*>(MyVisitor::visit(ctx->exp()));
    if(curFunc->funcType->retType->type == INT32TYPE){
        ret = BuildFPTrunc(builder,ret);
    }
    else if(curFunc->funcType->retType->type == FLOATTYPE){
        ret = BuildSitofp(builder,ret);
    }
    if(ret != nullptr) {
        BuildStore(builder, retVal, ret);
    }
    BuildBr(builder,retBlock);

    return nullptr;
}

std::any MyVisitor::visitAssignStmt(antlr4::SysYParser::AssignStmtContext *ctx) {
    std::any lVal = MyVisitor::visitLVal(ctx->lVal());
    std::any rVal = MyVisitor::visit(ctx->exp());

    auto rValRef = any_cast<ValueRef*>(rVal);
    if(lVal.type() == typeid(Symbol*)){
        auto lValSymbol = any_cast<Symbol*>(lVal);
        if(lValSymbol->symbolType->type == INT32TYPE){
            rValRef = BuildFPTrunc(builder,rValRef); // (if rVal = float) float -> int
        }
        else if(lValSymbol->symbolType->type == FLOATTYPE){
            rValRef = BuildSitofp(builder,rValRef); // (if rVal = int) int -> float
        }
        BuildStore(builder, lValSymbol,rValRef);
    }
    else{
        auto lValSymbol = Parse_Any(lVal);
        if(((Pointer*)lValSymbol)->pointerType->elementType->type == INT32TYPE){
            rValRef = BuildFPTrunc(builder,rValRef); // (if rVal = float) float -> int
        }
        else if(((Pointer*)lValSymbol)->pointerType->elementType->type == FLOATTYPE){
            rValRef = BuildSitofp(builder,rValRef); // (if rVal = int) int -> float
        }
        BuildStore(builder, ((Pointer*)lValSymbol),rValRef);
    }

    return nullptr;
}

std::any MyVisitor::visitLValExp(antlr4::SysYParser::LValExpContext *ctx) {
    // LVal和LValExp一个是左值一个是右值，考虑分开实现
    string varName = ctx->lVal()->IDENT()->getText();
    if(varName.length() > 31) varName = varName.substr(0, 5) + "_";
    Symbol * symbol = curScope->getVar(varName);

    if (ctx->lVal()->exp().empty()) {
        if(symbol->symbolType->type != ARRAYTYPE){
            if(symbol->is_const){
                return symbol->constVal;
            }
            else{ // not const
                return BuildLoad(builder,symbol);
            }
        }
        else{
            vector<ValueRef*> v;
            return BuildGEP(builder,symbol, v);
        }
    } else {
        vector<ValueRef*> indices;
        bool all_const = symbol->is_const; // 对应数组为const,且所有下标均为const情况

        for(antlr4::SysYParser::ExpContext* exp: ctx->lVal()->exp()) {
            auto* idx = any_cast<ValueRef*>(MyVisitor::visit(exp));
            all_const &= idx->type == IntConst;
            indices.push_back(idx);

        }
        bool get_val = symbol->symbolType->getDimension() == indices.size(); // 用于判断下标个数是否等于数组/指针维度，如果相等则取值，否则返回指针
        if(get_val && all_const){ // 对应const数组取const下标，直接返回相应值
            ValueRef* ret = getConstArrayValue(indices,(Array_Const*)symbol->constVal);
            if(ret != nullptr){
                return ret;
            }
            else{ //返回0
                Type * temp = symbol->symbolType;
                assert(temp->type == ARRAYTYPE);
                for(int i=0;i<indices.size();i++){
                    temp = ((ArrayType*)temp) -> elementType;
                }
                if(temp->type == INT32TYPE)
                    return new Int_Const(0);
                else
                    return new Float_Const(0.0);
            }
        }

        ValueRef *ptr;
        if (symbol->symbolType->type == POINTERTYPE) {
            ValueRef* varPtr = BuildLoad(builder, symbol);
            ptr = Parse_Any(BuildGEP(builder, varPtr, indices));
        } else { //symbol->symbolType->type == ARRAYTYPE
            ptr = Parse_Any(BuildGEP(builder, symbol, indices));
        }

        if(get_val){
            return BuildLoad(builder,dynamic_cast<Pointer*>(ptr));
        }
        else{
            return ptr;
        }
    }
}

std::any MyVisitor::visitLVal(antlr4::SysYParser::LValContext *ctx) {
    string varName = ctx->IDENT()->getText();
    if (varName.length() > 31) varName = varName.substr(0, 5) + "_";
    Symbol * symbol = curScope->getVar(varName);

    if (ctx->exp().empty()) {
        return symbol;
    } else {
        if (symbol->symbolType->type == POINTERTYPE) {
            vector<ValueRef*> indices;
            for(int i = 0; i < ctx->exp().size(); ++i) {
                auto* idx = any_cast<ValueRef*>(MyVisitor::visit(ctx->exp(i)));
                indices.push_back(idx);
            }
            ValueRef* varPtr = BuildLoad(builder, symbol);
            return BuildGEP(builder, varPtr, indices);

        } else { //symbol->symbolType->type == ARRAYTYPE
            vector<ValueRef*> indices;
            for(int i = 0; i < ctx->exp().size(); ++i) {
                auto* idx = any_cast<ValueRef*>(MyVisitor::visit(ctx->exp(i)));
                indices.push_back(idx);
            }
            return BuildGEP(builder, symbol, indices);
        }
    }
}

std::any MyVisitor::visitCallFuncExp(antlr4::SysYParser::CallFuncExpContext *ctx) {
    string funcName = ctx->IDENT()->getText();
    Function *func = globalUnit->getFunc(funcName);

    vector<ValueRef*> args;
    int argsCnt;
    if (ctx->funcRParams() != nullptr) {
        argsCnt = ctx->funcRParams()->param().size();
        auto argsType = func->funcType->arguments;
        // assert(argsType.size() == argsCnt);
        for(int i = 0; i < argsCnt; ++i) {
            ValueRef* arg = Parse_Any(MyVisitor::visit(ctx->funcRParams()->param(i)));

            auto argTy = argsType[i]->type;
            if (argTy == INT32TYPE)
                arg = BuildFPTrunc(builder, arg);
            else if (argTy == FLOATTYPE)
                arg = BuildSitofp(builder, arg);

            Type* type = argsType[i];
            if(type->type == POINTERTYPE && type->getDimension() < arg->getType()->getDimension()){
                vector<ValueRef*> v;
                for(int j=0;j<arg->getType()->getDimension() - type->getDimension(); ++j){
                    v.push_back(new Int_Const(0));
                }
                arg = BuildGEP(builder,arg,v);
            }
            args.push_back(arg);
        }
    }

    if(funcName == "starttime" || funcName == "stoptime"){
        args.push_back(new Int_Const(0));
    }

    return BuildCall(builder, func, args);
}


std::any MyVisitor::visitExpParenthesis(antlr4::SysYParser::ExpParenthesisContext *ctx) {
    return MyVisitor::visit(ctx->exp());
}

std::any MyVisitor::visitNumberExp(antlr4::SysYParser::NumberExpContext *ctx) {
    string valCtx;
    ValueRef* val;

    if (ctx->number()->INTEGER_CONST() != nullptr) {
        int intVal;
        valCtx = ctx->number()->INTEGER_CONST()->getText();
        // if (valCtx.starts_with("0x") || valCtx.starts_with("0X"))
        if (valCtx.substr(0, 2) == "0x" || valCtx.substr(0, 2) == "0X")
            intVal = stoi(valCtx, nullptr, 16);
        else if (valCtx[0] == '0')
            intVal = stoi(valCtx, nullptr, 8);
        else
            intVal = stoi(valCtx);
        val = new Int_Const(intVal);
    } else {
        float floatVal;
        valCtx = ctx->number()->FLOAT_CONST()->getText();
        floatVal = stof(valCtx);
        val = new Float_Const(floatVal);
    }

    return val;
}

std::any MyVisitor::visitUnaryOpExp(antlr4::SysYParser::UnaryOpExpContext *ctx) {

    auto* expVal = any_cast<ValueRef*>(MyVisitor::visit(ctx->exp()));

    ValueRef* res;
    //默认expVal是int类型
    if (ctx->unaryOp()->PLUS() != nullptr)
        res = expVal;
    if (ctx->unaryOp()->MINUS() != nullptr)
        res = BuildNeg(builder, expVal);
    else if (ctx->unaryOp()->NOT() != nullptr)
        res = BuildNot(builder, expVal);


    return res;
}

std::any MyVisitor::visitMulExp(antlr4::SysYParser::MulExpContext *ctx) {
    auto* lhs = any_cast<ValueRef*>(MyVisitor::visit(ctx->exp(0)));
    auto* rhs = any_cast<ValueRef*>(MyVisitor::visit(ctx->exp(1)));
    ValueRef* res;
    //默认lhs和rhs全是int类型
    if (ctx->MUL() != nullptr) {
        res = BuildMul(builder, lhs, rhs);
    } else if (ctx->DIV() != nullptr) {
        res = BuildDiv(builder, lhs, rhs);
    } else if (ctx->MOD() != nullptr) {
        res = BuildMod(builder, lhs, rhs);
    }
    return res;
}

std::any MyVisitor::visitPlusExp(antlr4::SysYParser::PlusExpContext *ctx) {
    //antlr4::SysYParser::ExpContext *exp0 = ctx->exp(0);
    //std :: any ans = MyVisitor::visit(exp0);

    auto* lhs = any_cast<ValueRef*>(MyVisitor::visit(ctx->exp(0)));

    auto* rhs = any_cast<ValueRef*>(MyVisitor::visit(ctx->exp(1)));
    ValueRef* res;
    //cerr<<lhs->get_Ref()<<" "<<rhs->get_Ref()<<endl;

    //默认lhs和rhs全是int类型
    if (ctx->PLUS()!= nullptr) {

        res = BuildAdd(builder, lhs, rhs);

    } else if (ctx->MINUS() != nullptr) {
        res = BuildSub(builder, lhs, rhs);
    }

    return res;
}

bool isExpCond = false;

std::any MyVisitor::visitExpCond(antlr4::SysYParser::ExpCondContext *ctx) {
    isExpCond = true;
    return MyVisitor::visit(ctx->exp());
}

std::any MyVisitor::visitLtCond(antlr4::SysYParser::LtCondContext *ctx) {
    //cerr<<"visitLtCond"<<endl;
    auto* lhs = any_cast<ValueRef*>(MyVisitor::visit(ctx->cond(0)));
    auto* rhs = any_cast<ValueRef*>(MyVisitor::visit(ctx->cond(1)));

    if (ctx->LT() != nullptr)
        return BuildCmp(builder, lhs, rhs,LT);
    else if (ctx->GT() != nullptr)
        return BuildCmp(builder, lhs, rhs,GT);
    else if (ctx->LE() != nullptr)
        return BuildCmp(builder, lhs, rhs,LE);
    else if (ctx->GE() != nullptr)
        return BuildCmp(builder, lhs, rhs,GE);
    else {
        cerr << "you should not arrive here!"<<endl;
        return nullptr;
    }
}

std::any MyVisitor::visitEqCond(antlr4::SysYParser::EqCondContext *ctx) {
    //cerr<<"visitEqCond"<<endl;
    auto* lhs = any_cast<ValueRef*>(MyVisitor::visit(ctx->cond(0)));
    auto* rhs = any_cast<ValueRef*>(MyVisitor::visit(ctx->cond(1)));

    if (ctx->EQ() != nullptr)
        return BuildCmp(builder, lhs, rhs,EQ);
    else if (ctx->NEQ() != nullptr)
        return BuildCmp(builder, lhs, rhs,NE);
    else {
        cerr << "you should not arrive here!"<<endl;
        return nullptr;
    }
}


std::any MyVisitor::visitAndCond(antlr4::SysYParser::AndCondContext *ctx) {
//    //cerr<<"visitAndCond"<<endl;
    hasLogicCond = true;

    string rCondBlkName = "lCondTrue";
    auto* rCondBlock = new BasicBlock(rCondBlkName, curFunc);

    BasicBlock* myTBlock = TBlock;
    BasicBlock* myFBlock = FBlock;
    TBlock = rCondBlock;

    //cerr <<"visitCond0: "<<ctx->cond(0)->getText()<<endl;

    isExpCond = false;
    any lCondRes = MyVisitor::visit(ctx->cond(0));
    if (isExpCond) {
        auto* lCondVal = any_cast<ValueRef*>(lCondRes);
        ValueRef* lCondBool = BuildCmp(builder, lCondVal, zero, NE);
        BuildCondBr(builder, lCondBool, rCondBlock, myFBlock);
    }
    //cerr<<"Cond0 visited"<<endl;

    //rCond
    curFunc->appendBlock(rCondBlock);
    builder->PositionBuilderAtEnd(rCondBlock);

    //cerr <<"visitCond1: "<<ctx->cond(1)->getText()<<endl;

    TBlock = myTBlock;
    isExpCond = false;
    any rCondRes = MyVisitor::visit(ctx->cond(1));
    if (isExpCond) {
        auto* rCondVal = any_cast<ValueRef*>(rCondRes);
        ValueRef* rCondBool = BuildCmp(builder, rCondVal, zero, NE);
        BuildCondBr(builder, rCondBool, myTBlock, myFBlock);
    }

    //cerr<<"Cond1 visited"<<endl;

    isExpCond = false;
    return nullptr;
}

std::any MyVisitor::visitOrCond(antlr4::SysYParser::OrCondContext *ctx) {

    hasLogicCond = true;

    string rCondBlkName = "lCondFalse";
    auto* rCondBlock = new BasicBlock(rCondBlkName, curFunc);

    BasicBlock* myTBlock = TBlock;
    BasicBlock* myFBlock = FBlock;
    FBlock = rCondBlock;

    //cerr <<"visitCond0: "<<ctx->cond(0)->getText()<<endl;

    isExpCond = false;
    any lCondRes = MyVisitor::visit(ctx->cond(0));
    if (isExpCond) {
        auto* lCondVal = any_cast<ValueRef*>(lCondRes);
        ValueRef* lCondBool = BuildCmp(builder, lCondVal, zero, NE);
        BuildCondBr(builder, lCondBool, myTBlock, rCondBlock);
    }

    //cerr<<"cond0 visited"<<endl;

    //rCond
    curFunc->appendBlock(rCondBlock);
    builder->PositionBuilderAtEnd(rCondBlock);

    //cerr << "visitCond1: "<<ctx->cond(1)->getText()<<endl;

    FBlock = myFBlock;
    isExpCond = false;
    any rCondRes = MyVisitor::visit(ctx->cond(1));
    if (isExpCond) {
        FBlock = rCondBlock;
        auto* rCondVal = any_cast<ValueRef*>(rCondRes);
        ValueRef* rCondBool = BuildCmp(builder, rCondVal, zero, NE);
        BuildCondBr(builder, rCondBool, myTBlock, myFBlock);
    }

    //cerr<<"cond1 visited"<<endl;

    isExpCond = false;
    return nullptr;
}


std::any MyVisitor::getArrInitVal(Type* type,antlr4::SysYParser::ConstInitValContext* ctx){
    if(ctx->constExp() != nullptr){
        if(type->type == INT32TYPE){
            return BuildFPTrunc(builder, any_cast<ValueRef *>(this->visit(ctx)));
        }
        else if(type->type == FLOATTYPE){
            return BuildSitofp(builder, any_cast<ValueRef *>(this->visit(ctx)));
        }
        cerr << "can't get here" << endl;
        throw exception();
    }

    // 本层为数组
    std::vector<std::any> v;
    Type* eleType = ((ArrayType*)type)->elementType;
    auto ctxs = ctx->constInitVal();
    for(int i=0;i<ctxs.size();i++){
        if(ctxs[i]->constExp() == nullptr){ //表明下一层还是个数组
            v.push_back(getArrInitVal(eleType,ctxs[i]));
        }
        else{ //下一层是个数字，此时要判断是不是省略括号的情况
            if(eleType->type != ARRAYTYPE) { // 正常到达数组最后一层
                v.push_back(getArrInitVal(eleType,ctxs[i]));
            }
            else{ // 省略括号情况
                v.emplace_back(GiveMeAnArray(ctxs,i,(ArrayType*)eleType));
            }
        }
    }
    return v;
}

std::any MyVisitor::getArrInitVal(Type* type, antlr4::SysYParser::InitValContext* ctx){
    if(ctx->exp() != nullptr){
        if(type->type == INT32TYPE){
            return BuildFPTrunc(builder, any_cast<ValueRef *>(this->visit(ctx)));
        }
        else if(type->type == FLOATTYPE){
            return BuildSitofp(builder, any_cast<ValueRef *>(this->visit(ctx)));
        }
        cerr << "can't get here" << endl;
        throw exception();
    }

    // 本层为数组
    std::vector<std::any> v;
    Type* eleType = ((ArrayType*)type)->elementType;
    auto ctxs = ctx->initVal();
    for(int i=0;i<ctxs.size();){
        if(ctxs[i]->exp() == nullptr){ //表明下一层还是个数组
            v.push_back(getArrInitVal(eleType,ctxs[i++]));
        }
        else{ //下一层是个数字，此时要判断是不是省略括号的情况
            if(eleType->type != ARRAYTYPE) { // 正常到达数组最后一层
                v.push_back(getArrInitVal(eleType,ctxs[i++]));
            }
            else{ // 省略括号情况
                v.emplace_back(GiveMeAnArray(ctxs,i,(ArrayType*)eleType));
            }
        }
    }
    return v;
}

vector<any> MyVisitor::GiveMeAnArray(vector<antlr4::SysYParser::ConstInitValContext*>& vals,int& idx,ArrayType* arr){
    Type * eleType = arr->elementType;
    vector<any> v;
    if(eleType->type == ARRAYTYPE){
        for(int i=0;i<arr->ele_cnt;i++){
            if(idx >= vals.size()) break;
            if(vals[idx]->constExp() != nullptr) //下一个元素为数字，应为省略括号
                v.emplace_back(GiveMeAnArray(vals,idx,(ArrayType*)eleType));
            else
                v.emplace_back(getArrInitVal(eleType,vals[idx++]));
        }
    }
    else{
        for(int i=0;i<arr->ele_cnt;i++){
            if(idx >= vals.size()) break;
            auto ctx = vals[idx];
            assert(ctx->constExp() != nullptr);

            if(ctx->constExp() != nullptr){
                if(eleType->type == INT32TYPE){
                    v.emplace_back(BuildFPTrunc(builder, any_cast<ValueRef *>(this->visit(ctx))));
                }
                else if(eleType->type == FLOATTYPE){
                    v.emplace_back(BuildSitofp(builder, any_cast<ValueRef *>(this->visit(ctx))));
                }
                else {
                    cerr << "can't get here" << endl;
                    throw exception();
                }
            }
            idx ++;
        }
    }
    return v;
}

vector<any> MyVisitor::GiveMeAnArray(vector<antlr4::SysYParser::InitValContext *> &vals, int &idx, ArrayType *arr) {
    Type * eleType = arr->elementType;
    vector<any> v;
    if(eleType->type == ARRAYTYPE){
        for(int i=0;i<arr->ele_cnt;i++){
            if(idx >= vals.size()) break;
            if(vals[idx]->exp() != nullptr) //下一个元素为数字，应为省略括号
                v.emplace_back(GiveMeAnArray(vals,idx,(ArrayType*)eleType));
            else
                v.emplace_back(getArrInitVal(eleType,vals[idx++]));
        }
    }
    else{
        for(int i=0;i<arr->ele_cnt;i++){
            if(idx >= vals.size()) break;
            auto ctx = vals[idx];
            assert(ctx->exp() != nullptr);

            if(ctx->exp() != nullptr){
                if(eleType->type == INT32TYPE){
                    v.emplace_back(BuildFPTrunc(builder, any_cast<ValueRef *>(this->visit(ctx))));
                }
                else if(eleType->type == FLOATTYPE){
                    v.emplace_back(BuildSitofp(builder, any_cast<ValueRef *>(this->visit(ctx))));
                }
                else {
                    cerr << "can't get here" << endl;
                    throw exception();
                }
            }
            idx ++;
        }
    }
    return v;
}
