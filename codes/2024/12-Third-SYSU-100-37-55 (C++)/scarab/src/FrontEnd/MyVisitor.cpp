#include "MyVisitor.h"
#include <string>
#include <any>
using std::endl;
using std::stoi;
using std::cerr;

void MyVisitor::print()
{
    irModule.print();
}

antlrcpp::Any MyVisitor::visitCompUnit(SysY2022Parser::CompUnitContext *ctx)
{
    return visitChildren(ctx);
}

bool MyVisitor::parseInitValue(SysY2022Parser::InitValContext *ctx)
{
    int numChildren = ctx->children.size();

    if (numChildren > 1) {
        for (int index = 1; index < numChildren - 1; index += 2) {
            auto childContext = dynamic_cast<SysY2022Parser::InitValContext *>(ctx->children[index]);
            if (childContext == nullptr || !parseInitValue(childContext))
                return false;
        }
        return true;
    }
    else {
        auto itemContext = dynamic_cast<SysY2022Parser::ItemInitValContext *>(ctx);
        if (itemContext != nullptr) {
            try {
                stoi(itemContext->getText());
                return true;
            }
            catch (const std::invalid_argument &) {
                return false;
            }
            catch (const std::out_of_range &) {
                return false;
            }
        }
        return false;
    }
}

antlrcpp::Any MyVisitor::visitItemConstInitVal(SysY2022Parser::ItemConstInitValContext *ctx)
{
    return ctx->constExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitItemInitVal(SysY2022Parser::ItemInitValContext *ctx)
{
    return ctx->exp()->accept(this);
}

antlrcpp::Any MyVisitor::visitListInitVal(SysY2022Parser::ListInitValContext *ctx)
{
    return visitChildren(ctx);
}

antlrcpp::Any MyVisitor::visitFuncFParam(SysY2022Parser::FuncFParamContext *ctx)
{
    auto name = ctx->Identifier()->getText();
    auto type = TypePtr(new Type(ctx->bType()->getText()));
    if (ctx->children.size() == 2) {
        assert(type->isInt() || type->isFloat());
        if (type->isInt())
            return VariablePtr(new Int(name, false, false));
        else
            return VariablePtr(new Float(name, false, false));
    }
    else {
        auto currentType = type;
        for (int childIndex = ctx->children.size() - 2; childIndex >= 5; childIndex -= 3) {
            currentType = TypePtr(new ArrType(currentType, stoi(std::any_cast<ValuePtr>(ctx->children[childIndex]->accept(this))->toString())));
        }
        currentType = TypePtr(new PtrType(currentType));
        return VariablePtr(new Ptr(name, false, false, currentType));
    }
}

antlrcpp::Any MyVisitor::visitFuncFParams(SysY2022Parser::FuncFParamsContext *ctx)
{
    vector<VariablePtr> variables;
    for (int i = 0; i < ctx->children.size(); i += 2) {
        variables.emplace_back(std::any_cast<VariablePtr>(ctx->children[i]->accept(this)));
    }
    return variables;
}

antlrcpp::Any MyVisitor::visitFuncDef(SysY2022Parser::FuncDefContext *ctx)
{
    auto name = ctx->Identifier()->getText();    
    auto type = TypePtr(new Type(ctx->funcType()->getText()));

    irModule.pushFunctionToStack(FunctionPtr(new Function()));
    vector<ValuePtr> params;

    if (ctx->children.size() == 6) {
        auto rst = std::any_cast<vector<VariablePtr>>(ctx->funcFParams()->accept(this));
        irModule.parameterStringTable = StringTableNodePtr(new StringTableNode());
        for (const auto& var : rst) {
            params.emplace_back(var);
            auto addr = Variable::copy(var);
            addr->name += ".addr";
            irModule.parameterStringTable->varMap[var->name] = addr;
            irModule.getCurrentFunction()->addGlobalVariable(addr);
            irModule.getCurrentBasicBlock()->appendInstruction(InstructionPtr(new StoreInstruction(addr, var, irModule.getCurrentBasicBlock())));
        }
    }
    auto func = FunctionPtr(new Function(type, name, params));
    func->initializeReturnBlock();
    irModule.addFunction(func);
    if (type->isInt()) {
        VariablePtr returnValue = VariablePtr(new Int("retval", false, false));
        irModule.getCurrentFunction()->addGlobalVariable(returnValue);
        func->returnValue = returnValue;
    }
    else if (type->isFloat()) {
        VariablePtr returnValue = VariablePtr(new Float("retval", false, false));
        irModule.getCurrentFunction()->addGlobalVariable(returnValue);
        func->returnValue = returnValue;
    }
    else {
        func->returnValue = Void::get();
    }
    
    ctx->block()->accept(this);

    if (type->isVoid() || !irModule.getCurrentBasicBlock()->terminator)
        irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr(new BrInstruction(irModule.globalFunctions.back()->returnBasicBlock->label, irModule.getCurrentBasicBlock())));
    
    irModule.getCurrentFunction()->allocateLocalVariables();
    irModule.getCurrentFunction()->assignBlocksToFunction();

    auto temp_func = irModule.popFunctionFromStack();
    auto& globalFunc = irModule.globalFunctions.back();
    globalFunc->registerCount = temp_func->registerCount;
    globalFunc->basicBlocks = temp_func->basicBlocks;
    globalFunc->variableMap = temp_func->variableMap;
    globalFunc->LabelToBBMap = temp_func->LabelToBBMap;
    globalFunc->loopList = temp_func->loopList;
    globalFunc->bbToLoopMap = temp_func->bbToLoopMap;
    globalFunc->outerLoops = temp_func->outerLoops;

    globalFunc->processReturnBlock();
    globalFunc->processEndInstructions();
    return nullptr;
}


antlrcpp::Any MyVisitor::visitBlock(SysY2022Parser::BlockContext *ctx)
{
    irModule.restorePreviousStringTable();
    if (irModule.parameterStringTable) {
        auto table = irModule.parameterStringTable->varMap;
        for (auto it = table.begin(); it != table.end(); it++) {
            irModule.currentScopeStringTable->varMap[it->first] = it->second;
        }
        irModule.parameterStringTable.reset();
    }

    visitChildren(ctx);
    irModule.popStringTable();
    return nullptr;
}

antlrcpp::Any MyVisitor::visitReturnStmt(SysY2022Parser::ReturnStmtContext *ctx)
{
    ValuePtr value = Void::get();
    if (ctx->exp()) {
        value = std::any_cast<ValuePtr>(ctx->exp()->accept(this));
        auto returnType = irModule.globalFunctions.back()->returnValue->type;
        if (!value->type->operator==(returnType)) {
            if (value->type->isFloat() && returnType->isInt()) {
                if (value->isConst) {
                    value = Const::createConstant(Type::getInt(),int(dynamic_cast<Const *>(value.get())->floatVal), value->name);
                }
                else {
                    auto instruction = shared_ptr<FloatToSignInstruction>(new FloatToSignInstruction(value, irModule.getCurrentBasicBlock()));
                    irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                    value = instruction->reg;
                }
            }
            else if (value->type->isInt() && returnType->isFloat()) {
                if(value->isConst) {
                    value = Const::createConstant(Type::getFloat(),float(dynamic_cast<Const *>(value.get())->intVal), value->name);
                }
                else {
                    auto instruction = shared_ptr<SignToFloatInstruction>(new SignToFloatInstruction(value, irModule.getCurrentBasicBlock()));
                    irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                    value = instruction->reg;
                }
            }
        }
        irModule.getCurrentBasicBlock()->appendInstruction(InstructionPtr(new StoreInstruction(irModule.globalFunctions.back()->returnValue, value, irModule.getCurrentBasicBlock())));
    }

    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr(new BrInstruction(irModule.globalFunctions.back()->returnBasicBlock->label, irModule.getCurrentBasicBlock())));

    return nullptr;
}


antlrcpp::Any MyVisitor::visitVarDefWithOutInitVal(SysY2022Parser::VarDefWithOutInitValContext *ctx)
{
    auto name = ctx->Identifier()->getText();
    int childSize = ctx->children.size();

    bool isGlobal = irModule.isFunctionStackEmpty();

    // 处理基本类型变量
    if (childSize == 1) {
        VariablePtr variable;
        if (irModule.declaredType->isInt()) {
            variable = std::make_shared<Int>(name, isGlobal, false);
        }
        else if (irModule.declaredType->isFloat()) {
            variable = std::make_shared<Float>(name, isGlobal, false);
        }

        if (isGlobal) {
            irModule.addGlobalVariable(variable);
        }
        else {
            irModule.registerGlobalVariable(variable);
        }
    }
    // 处理数组类型变量
    else {
        auto curr = std::make_shared<ArrType>(irModule.declaredType, 
            std::stoi(std::any_cast<ValuePtr>(ctx->children[childSize - 2]->accept(this))->toString()));

        for (int childInd = childSize - 5; childInd > 1; childInd -= 3) {
            curr = std::make_shared<ArrType>(curr, 
                std::stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->toString()));
        }

        VariablePtr variable = std::make_shared<Arr>(name, isGlobal, false, curr);

        if (isGlobal) {
            irModule.addGlobalVariable(variable);
        }
        else {
            irModule.registerGlobalVariable(variable);
        }
    }

    return visitChildren(ctx);
}

VariablePtr MyVisitor::initializeArrayItem(SysY2022Parser::ItemInitValContext *ctx, TypePtr type)
{
    auto number = std::any_cast<ValuePtr>(ctx->accept(this));
    if (irModule.declaredType->isInt())
        return VariablePtr(new Int("", false, false, number));
    else
        return VariablePtr(new Float("", false, false, number));
}

shared_ptr<Arr> MyVisitor::initializeArrayList(SysY2022Parser::ListInitValContext *ctx, TypePtr type)
{
    int numChildren = ctx->children.size();
    auto rst = shared_ptr<Arr>(new Arr(string("__const.") + to_string(constArrayCounter++), true, true, type));
    for (int index = 1; index < numChildren - 1; index += 2) {
        VariablePtr curr;
        if (ctx->children[index]->children.size() > 1) {
            auto tmp = initializeArrayList(dynamic_cast<SysY2022Parser::ListInitValContext *>(ctx->children[index]), rst->getElementType());
            tmp->fill();
            curr = tmp;
        }
        else {
            curr = initializeArrayItem(dynamic_cast<SysY2022Parser::ItemInitValContext *>(ctx->children[index]), rst->getElementType());
        }
        rst->push(curr);
    }
    return rst;
}

VariablePtr MyVisitor::initializeArrayItem(SysY2022Parser::ItemConstInitValContext *ctx, TypePtr type)
{
    string number = dynamic_cast<SysY2022Parser::ItemConstInitValContext *>(ctx)->getText();
    assert(!number.empty());
    return VariablePtr(new Int("", false, false, stoi(number)));
}

shared_ptr<Arr> MyVisitor::initializeArrayList(SysY2022Parser::ListConstInitValContext *ctx, TypePtr type)
{
    int numChildren = ctx->children.size();
    auto rst = shared_ptr<Arr>(new Arr("", true, true, type));
    for (int index = 1; index < numChildren - 1; index += 2) {
        VariablePtr curr;
        if (ctx->children[index]->children.size() > 1) {
            auto tmp = initializeArrayList(dynamic_cast<SysY2022Parser::ListConstInitValContext *>(ctx->children[index]), rst->getElementType());
            tmp->fill();
            curr = tmp;
        }
        else {
            curr = initializeArrayItem(dynamic_cast<SysY2022Parser::ItemConstInitValContext *>(ctx->children[index]), rst->getElementType());
        }
        rst->push(curr);
    }
    return rst;
}

bool indexAdd(std::vector<ValuePtr> &indexs, int depth, TypePtr type)
{
    auto arrType = dynamic_cast<ArrType *>(type.get());
    if (!arrType) {
        throw std::runtime_error("Invalid type: Expected ArrType.");
    }

    int arrayLength = arrType->size;
    int oldIndex = dynamic_cast<Const *>(indexs[depth].get())->intVal;

    // 递归基底：处理最内层索引
    if (depth == indexs.size() - 1) {
        if (oldIndex == arrayLength - 1) {
            return false;
        }
        else {
            indexs[depth] = Const::createConstant(Type::getInt64(), static_cast<long long>(oldIndex + 1));
            return true;
        }
    }
    else {
        if (!indexAdd(indexs, depth + 1, arrType->inner)) {
            if (oldIndex == arrayLength - 1) {
                return false;
            }
            else {
                indexs[depth] = Const::createConstant(Type::getInt64(), static_cast<long long>(oldIndex + 1));
                indexs[depth + 1] = Const::createConstant(Type::getInt64(), static_cast<long long>(0));
                return true;
            }
        }
        return true;
    }
}

void MyVisitor::setArrayList(SysY2022Parser::ListInitValContext *ctx, ValuePtr value)
{
    vector<ValuePtr> indexs;
    auto arrType = dynamic_cast<ArrType *>(value->type.get());
    if (!arrType) {
        throw std::runtime_error("Invalid type: Expected ArrType.");
    }

    int depth = arrType->getDepth();
    for (int i = 0; i <= depth; i++) {
        indexs.emplace_back(Const::createConstant(Type::getInt64(), (long long)(0)));
    }
    if (ctx->children.size() == 2)
    {
        return;
    }
    for (int i = 1, index = 0; i < ctx->children.size(); i += 2, index++) {
        if (ctx->children[i]->children.size() == 1) {
            auto reg = std::any_cast<ValuePtr>(ctx->children[i]->accept(this));
            auto ins = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(value, indexs, irModule.getCurrentBasicBlock()));
            if (!value->isReg && dynamic_cast<Variable *>(value.get())->isGlobal) {
                irModule.getCurrentBasicBlock()->appendInstruction(InstructionPtr(new StoreInstruction(ins, reg, irModule.getCurrentBasicBlock())));
            }
            else {
                irModule.getCurrentBasicBlock()->appendInstruction(ins);
                auto val = ins->reg;
                if (!val->type->operator==(reg->type)) {
                    if (val->type->isInt() && reg->type->isFloat()) {
                        auto ins = shared_ptr<FloatToSignInstruction>(new FloatToSignInstruction(reg, irModule.getCurrentBasicBlock()));
                        irModule.getCurrentBasicBlock()->appendInstruction(ins);
                        reg = ins->reg;
                    }
                    else if (val->type->isFloat() && reg->type->isInt()) {
                        auto ins = shared_ptr<SignToFloatInstruction>(new SignToFloatInstruction(reg, irModule.getCurrentBasicBlock()));
                        irModule.getCurrentBasicBlock()->appendInstruction(ins);
                        reg = ins->reg;
                    }
                }
                irModule.getCurrentBasicBlock()->appendInstruction(InstructionPtr(new StoreInstruction(val, reg, irModule.getCurrentBasicBlock())));
            }

            indexAdd(indexs, 1, value->type);
        }
        else {
            vector<ValuePtr> inner = {indexs[0], indexs[1]};
            auto ins = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(value, inner, irModule.getCurrentBasicBlock()));
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            setArrayList(dynamic_cast<SysY2022Parser::ListInitValContext *>(ctx->children[i]), ins->reg);
            indexs[1] = Const::createConstant(Type::getInt64(),(long long)( dynamic_cast<Const *>(indexs[1].get())->intVal + 1));
            for (int j = 2; j <= depth; j++) {
                indexs[j] = Const::createConstant(Type::getInt64(), 0);
            }
        }
    }
}

antlrcpp::Any MyVisitor::visitVarDefWithInitVal(SysY2022Parser::VarDefWithInitValContext *ctx)
{
    auto name = ctx->Identifier()->getText();
    int childSize = ctx->children.size();

    // lambda 函数用于处理基本类型变量
    auto processBasicType = [&](bool isGlobal) {
        VariablePtr variable;
        auto initVal = std::any_cast<ValuePtr>(ctx->initVal()->accept(this));

        if (isGlobal) {
            if (irModule.declaredType->isInt()) {
                variable = std::make_shared<Int>(name, isGlobal, false, initVal);
            }
            else if (irModule.declaredType->isFloat()) {
                variable = std::make_shared<Float>(name, isGlobal, false, initVal);
            }
            irModule.addGlobalVariable(variable);
        }
        else {
            if (irModule.declaredType->isInt()) {
                variable = std::make_shared<Int>(name, isGlobal, false);
            }
            else if (irModule.declaredType->isFloat()) {
                variable = std::make_shared<Float>(name, isGlobal, false);
            }

            if (initVal->type->ID != variable->type->ID) {
                if (initVal->type->isInt()) {
                    auto instruction = std::make_shared<SignToFloatInstruction>(initVal, irModule.getCurrentBasicBlock());
                    irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                    initVal = instruction->reg;
                }
                else {
                    auto instruction = std::make_shared<FloatToSignInstruction>(initVal, irModule.getCurrentBasicBlock());
                    irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                    initVal = instruction->reg;
                }
            }
            irModule.getCurrentBasicBlock()->appendInstruction(std::make_shared<StoreInstruction>(variable, initVal, irModule.getCurrentBasicBlock()));
            irModule.registerGlobalVariable(variable);
        }
    };

    // lambda 函数用于处理数组类型变量
    auto processArrayType = [&](bool isGlobal) {
        auto parseArrayType = [&]() {
            auto curr = std::make_shared<ArrType>(irModule.declaredType, std::stoi(std::any_cast<ValuePtr>(ctx->children[childSize - 4]->accept(this))->toString()));
            for (int childInd = childSize - 7; childInd > 1; childInd -= 3) {
                curr = std::make_shared<ArrType>(curr, std::stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->toString()));
            }
            return curr;
        };

        auto curr = parseArrayType();
        auto variable = std::make_shared<Arr>(name, isGlobal, false, curr);
        if (isGlobal) {
            auto initVal = initializeArrayList(dynamic_cast<SysY2022Parser::ListInitValContext *>(ctx->initVal()), curr);
            variable->inner = initVal->inner;
            irModule.addGlobalVariable(variable);
        }
        else {
            irModule.registerGlobalVariable(variable);
            bool isConstInitVal = parseInitValue(ctx->initVal());

            auto arrBitCast = std::make_shared<BitCastInstruction>(variable, irModule.getCurrentFunction()->createRegister(std::make_shared<PtrType>(Type::getInt8())), irModule.getCurrentBasicBlock());
            irModule.getCurrentBasicBlock()->appendInstruction(arrBitCast);
            std::vector<ValuePtr> argv = {arrBitCast->reg,
                                          Const::createConstant(Type::getInt8(), 0),
                                          Const::createConstant(Type::getInt64(), static_cast<long long>(dynamic_cast<ArrType *>(variable->type.get())->getSize() * 4)),
                                          Const::createConstant(Type::getBool(), false)};
            irModule.getCurrentBasicBlock()->appendInstruction(std::make_shared<CallInstruction>(irModule.globalFunctions[0], argv, irModule.getCurrentBasicBlock()));
            setArrayList(dynamic_cast<SysY2022Parser::ListInitValContext *>(ctx->initVal()), variable);
        }
    };

    if (irModule.isFunctionStackEmpty()) {
        if (childSize == 3) {
            processBasicType(true);
        }
        else {
            processArrayType(true);
        }
    }
    else {
        if (childSize == 3) {
            processBasicType(false);
        }
        else {
            processArrayType(false);
        }
    }

    return nullptr;
}


antlrcpp::Any MyVisitor::visitConstDecl(SysY2022Parser::ConstDeclContext *ctx)
{
    return visitChildren(ctx);
}

antlrcpp::Any MyVisitor::visitBType(SysY2022Parser::BTypeContext *ctx)
{
    irModule.declaredType = TypePtr(new Type(ctx->getText()));
    return nullptr;
}

antlrcpp::Any MyVisitor::visitConstDef(SysY2022Parser::ConstDefContext *ctx)
{
    auto name = ctx->Identifier()->getText();
    int childSize = ctx->children.size();

    // lambda 函数用于处理基本类型常量
    auto processBasicType = [&](bool isGlobal) {
        VariablePtr variable;
        auto initVal = std::any_cast<ValuePtr>(ctx->constInitVal()->accept(this));

        if (irModule.declaredType->isInt()) {
            variable = std::make_shared<Int>(name, isGlobal, true, initVal);
        }
        else if (irModule.declaredType->isFloat()) {
            variable = std::make_shared<Float>(name, isGlobal, true, initVal);
        }

        if (isGlobal) {
            irModule.addGlobalVariable(variable);
        }
        else {
            irModule.currentScopeStringTable->varMap[variable->name] = variable;
        }
    };

    // lambda 函数用于处理数组类型常量
    auto processArrayType = [&](bool isGlobal) {
        auto parseArrayType = [&]() {
            auto curr = std::make_shared<ArrType>(irModule.declaredType, std::stoi(std::any_cast<ValuePtr>(ctx->children[childSize - 4]->accept(this))->toString()));
            for (int childInd = childSize - 7; childInd > 1; childInd -= 3) {
                curr = std::make_shared<ArrType>(curr, std::stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->toString()));
            }
            return curr;
        };

        auto curr = parseArrayType();
        auto initVal = initializeArrayList(dynamic_cast<SysY2022Parser::ListConstInitValContext *>(ctx->constInitVal()), curr);

        if (isGlobal) {
            initVal->name = name;
            irModule.addGlobalVariable(initVal);
        }
        else {
            initVal->name = "__const." + std::to_string(constArrayCounter++);
            irModule.addGlobalVariable(initVal);
            irModule.currentScopeStringTable->varMap[name] = initVal;
        }
    };

    if (irModule.isFunctionStackEmpty()) {
        if (childSize == 3) {
            processBasicType(true);
        }
        else {
            processArrayType(true);
        }
    }
    else {
        if (childSize == 3) {
            processBasicType(false);
        }
        else {
            processArrayType(false);
        }
    }

    return nullptr;
}

antlrcpp::Any MyVisitor::visitAssignStmt(SysY2022Parser::AssignStmtContext *ctx)
{
    VariablePtr val = irModule.currentScopeStringTable->lookUp(ctx->lVal()->Identifier()->getText());
    if (val->isGlobal || val->type->isPtr())
        irModule.globalFunctions.back()->isReentrant = false;

    ValuePtr exp = std::any_cast<ValuePtr>(ctx->exp()->accept(this));

    auto performTypeConversion = [&](ValuePtr &src, TypePtr targetType) {
        if (targetType->isInt() && src->type->isFloat()) {
            auto ins = std::make_shared<FloatToSignInstruction>(src, irModule.getCurrentBasicBlock());
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            src = ins->reg;
        }
        else if (targetType->isFloat() && src->type->isInt()) {
            auto ins = std::make_shared<SignToFloatInstruction>(src, irModule.getCurrentBasicBlock());
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            src = ins->reg;
        }
    };

    auto resolveIndex = [&](int startIdx) {
        std::vector<ValuePtr> indexs = {Const::createConstant(Type::getInt64(), 0)};
        for (int i = startIdx; i < ctx->lVal()->children.size(); i += 3) {
            ValuePtr ind = std::any_cast<ValuePtr>(ctx->lVal()->children[i]->accept(this));
            if (ind->type != Type::getInt64()) {
                if (ind->isConst) {
                    ind = Const::createConstant(Type::getInt64(), dynamic_cast<Const*>(ind.get())->intVal, ind->name);
                }
                else {
                    auto sextIns = std::make_shared<ExtInstruction>(ind, Type::getInt64(), true, irModule.getCurrentBasicBlock());
                    irModule.getCurrentBasicBlock()->appendInstruction(sextIns);
                    ind = sextIns->reg;
                }
            }
            indexs.emplace_back(ind);
        }
        return indexs;
    };

    if (ctx->lVal()->children.size() == 1) {
        if (!val->type->operator==(exp->type)) {
            performTypeConversion(exp, val->type);
        }
        irModule.getCurrentBasicBlock()->appendInstruction(shared_ptr<StoreInstruction>(new StoreInstruction(val, exp, irModule.getCurrentBasicBlock())));
    }
    else {
        ValuePtr curr = val;
        if (val->type->isPtr()) {
            auto ins = shared_ptr<LoadInstruction>(new LoadInstruction(curr, irModule.getCurrentFunction()->createRegister(dynamic_cast<PtrType *>(curr->type.get())->inner), irModule.getCurrentBasicBlock()));
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            curr = ins->to;

            ValuePtr ind = std::any_cast<ValuePtr>(ctx->lVal()->children[2]->accept(this));
            if (ind->type != Type::getInt64()) {
                if (ind->isConst) {
                    ind = Const::createConstant(Type::getInt64(), dynamic_cast<Const*>(ind.get())->intVal,ind->name);
                }
                else {
                    auto sextIns = shared_ptr<ExtInstruction>(new ExtInstruction(ind, Type::getInt64(), true, irModule.getCurrentBasicBlock()));
                    irModule.getCurrentBasicBlock()->appendInstruction(sextIns);
                    ind = sextIns->reg;
                }
            }
            auto inst = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(curr, vector<ValuePtr>({ind}), irModule.getCurrentBasicBlock()));
            irModule.getCurrentBasicBlock()->appendInstruction(inst);
            curr = inst->reg;

            // 处理数组
            if (ctx->lVal()->children.size() > 5) {
                auto indexs = resolveIndex(5);
                inst = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(curr, indexs, irModule.getCurrentBasicBlock()));
                irModule.getCurrentBasicBlock()->appendInstruction(inst);
                curr = inst->reg;
            }
            if (!curr->type->operator==(exp->type)) {
                    if(exp->isConst) {
                        if(exp->type->isInt()) {
                            exp = Const::createConstant(Type::getFloat(),float(dynamic_cast<Const *>(exp.get())->intVal), exp->name);
                        }
                        else if(exp->type->isFloat()) {
                            exp = Const::createConstant(Type::getInt(),int(dynamic_cast<Const *>(exp.get())->floatVal), exp->name);
                        }
                    }
                    else {
                        performTypeConversion(exp, curr->type);
                    }
                }
            irModule.getCurrentBasicBlock()->appendInstruction(shared_ptr<StoreInstruction>(new StoreInstruction(curr, exp, irModule.getCurrentBasicBlock())));
        }
        else {
            auto indexs = resolveIndex(2);

            auto inst = shared_ptr<GetElementPtrInstruction>(new GetElementPtrInstruction(curr, indexs, irModule.getCurrentBasicBlock()));
            curr = inst->reg;
            if (!curr->type->operator==(exp->type)) {
                performTypeConversion(exp, curr->type);
            }
            irModule.getCurrentBasicBlock()->appendInstruction(inst);
            irModule.getCurrentBasicBlock()->appendInstruction(shared_ptr<StoreInstruction>(new StoreInstruction(curr, exp, irModule.getCurrentBasicBlock())));
        }
    }
    return exp;
}

antlrcpp::Any MyVisitor::visitExp(SysY2022Parser::ExpContext *ctx)
{
    return ctx->addExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitLVal(SysY2022Parser::LValContext *ctx)
{
    VariablePtr val = irModule.currentScopeStringTable->lookUp(ctx->Identifier()->getText());
    int numChildren = ctx->children.size();

    auto convertToInt64 = [&](ValuePtr ind) -> ValuePtr {
        if (ind->type != Type::getInt64()) {
            if (ind->isConst) {
                ind = Const::createConstant(Type::getInt64(), dynamic_cast<Const*>(ind.get())->intVal, ind->name);
            }
            else {
                auto sextIns = std::make_shared<ExtInstruction>(ind, Type::getInt64(), true, irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(sextIns);
                ind = sextIns->reg;
            }
        }
        return ind;
    };

    if (numChildren > 1) {
        ValuePtr curr = val;
        std::vector<ValuePtr> indexs;

        for (int i = 2; i < numChildren; i += 3)
            indexs.emplace_back(std::any_cast<ValuePtr>(ctx->children[i]->accept(this)));

        if (val->type->isPtr()) {
            auto ins = std::make_shared<LoadInstruction>(curr, irModule.getCurrentFunction()->createRegister(dynamic_cast<PtrType *>(curr->type.get())->inner), irModule.getCurrentBasicBlock());
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            curr = ins->to;

            auto ind = convertToInt64(indexs[0]);
            auto inst = std::make_shared<GetElementPtrInstruction>(curr, std::vector<ValuePtr>({ind}), irModule.getCurrentBasicBlock());
            irModule.getCurrentBasicBlock()->appendInstruction(inst);
            curr = inst->reg;
            indexs.erase(indexs.begin());

            if (numChildren > 5) {
                indexs.insert(indexs.begin(), Const::createConstant(Type::getInt64(), 0));
                for (int i = 1; i < indexs.size(); i++) {
                    indexs[i] = convertToInt64(indexs[i]);
                }
                inst = std::make_shared<GetElementPtrInstruction>(curr, indexs, irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(inst);
                curr = inst->reg;
            }
            ins = std::make_shared<LoadInstruction>(curr, irModule.getCurrentFunction()->createRegister(curr->type), irModule.getCurrentBasicBlock());
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            return ins->to;
        }
        else {
            if (val->isConst) {
                bool isConst = true;
                for (auto &index : indexs) {
                    if (!index->isConst) {
                        isConst = false;
                        break;
                    }
                }
                if (isConst) {
                    for (auto &index : indexs) {
                        auto ind = dynamic_cast<Const *>(index.get())->intVal;
                        curr = dynamic_cast<Arr *>(curr.get())->inner[ind];
                    }
                    if (curr->type->isInt())
                        return Const::createConstant(Type::getInt(), dynamic_cast<Int *>(curr.get())->intVal);
                    else if (curr->type->isFloat())
                        return Const::createConstant(Type::getFloat(), dynamic_cast<Float *>(curr.get())->floatVal);
                    else
                        assert(0);
                }
            }
            int depth = dynamic_cast<ArrType *>(val->type.get())->getDepth();
            indexs.insert(indexs.begin(), Const::createConstant(Type::getInt64(), 0));
            for (int i = 1; i < indexs.size(); i++) {
                indexs[i] = convertToInt64(indexs[i]);
            }
            if (numChildren == depth * 3 + 1) {
                auto ins = std::make_shared<GetElementPtrInstruction>(val, indexs, irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(ins);
                auto inst = std::make_shared<LoadInstruction>(ins->reg, irModule.getCurrentFunction()->createRegister(ins->reg->type), irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(inst);
                return inst->to;
            }
            else {
                indexs.emplace_back(Const::createConstant(Type::getInt64(), 0));
                auto ins = std::make_shared<GetElementPtrInstruction>(curr, indexs, irModule.getCurrentBasicBlock());
                ins->reg->type = TypePtr(new PtrType(ins->reg->type));
                irModule.getCurrentBasicBlock()->appendInstruction(ins);
                return ins->reg;
            }
        }
    }
    else {
        if (val->isConst) {
            if (val->type->isInt())
                return Const::createConstant(Type::getInt(), dynamic_cast<Int *>(val.get())->intVal);
            else
                return Const::createConstant(Type::getFloat(), dynamic_cast<Float *>(val.get())->floatVal);
        }
        else {
            if (val->type->isArr()) {
                std::vector<ValuePtr> index = {
                    Const::createConstant(Type::getInt64(), 0),
                    Const::createConstant(Type::getInt64(), 0)};
                auto ins = std::make_shared<GetElementPtrInstruction>(val, index, irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(ins);
                ins->reg->type = TypePtr(new PtrType(ins->reg->type));
                return ins->reg;
            }
            else {
                auto ins = std::make_shared<LoadInstruction>(val, irModule.getCurrentFunction()->createRegister(val->type), irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(ins);
                return ins->to;
            }
        }
    }
}

antlrcpp::Any MyVisitor::visitExpPrimaryExp(SysY2022Parser::ExpPrimaryExpContext *ctx)
{
    return ctx->exp()->accept(this);
}

antlrcpp::Any MyVisitor::visitLValPrimaryExp(SysY2022Parser::LValPrimaryExpContext *ctx)
{
    return ctx->lVal()->accept(this);
}

antlrcpp::Any MyVisitor::visitNumberPrimaryExp(SysY2022Parser::NumberPrimaryExpContext *ctx)
{
    return ctx->number()->accept(this);
}

antlrcpp::Any MyVisitor::visitNumber(SysY2022Parser::NumberContext *ctx)
{
    if (ctx->getStart()->getType() == SysY2022Lexer::IntConst){
        return Const::createConstant(Type::getInt(), ctx->getText());
    }
    else
        return Const::createConstant(Type::getFloat(), ctx->getText());
}

antlrcpp::Any MyVisitor::visitPrimaryUnaryExp(SysY2022Parser::PrimaryUnaryExpContext *ctx)
{
    auto primaryExp = std::any_cast<ValuePtr>(ctx->primaryExp()->accept(this));
    if (primaryExp->isConst && primaryExp->type->isInt() && dynamic_cast<Const *>(primaryExp.get())->intVal == 0x80000000)
        dynamic_cast<Const *>(primaryExp.get())->intVal = (int)0x80000000;
    return primaryExp;
}

antlrcpp::Any MyVisitor::visitFunctionCallUnaryExp(SysY2022Parser::FunctionCallUnaryExpContext *ctx)
{
    vector<ValuePtr> argv;
    if (ctx->children.size() == 4) {
        argv = std::any_cast<vector<ValuePtr>>(ctx->funcRParams()->accept(this));
    }

    auto func = irModule.findFunctionByName(ctx->Identifier()->getText());
    if (!func->isReentrant) {
        irModule.globalFunctions.back()->isReentrant = false;
    }

    if (func->name.substr(0, 6) == "_sysy_") {
        argv.emplace_back(Const::createConstant(Type::getInt(), 0));
    }

    auto convertType = [&](ValuePtr arg, TypePtr targetType) -> ValuePtr {
        if (arg->isConst) {
            auto constArg = std::dynamic_pointer_cast<Const>(arg);
            if (arg->type->isFloat() && targetType->isInt()) {
                return Const::createConstant(Type::getInt(), static_cast<int>(constArg->floatVal), constArg->name);
            }
            else if (arg->type->isInt() && targetType->isFloat()) {
                return Const::createConstant(Type::getFloat(), static_cast<float>(constArg->intVal), constArg->name);
            }
        }
        else {
            if (arg->type->isFloat() && targetType->isInt()) {
                auto ins = std::make_shared<FloatToSignInstruction>(arg, irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(ins);
                return ins->reg;
            }
            else if (arg->type->isInt() && targetType->isFloat()) {
                auto ins = std::make_shared<SignToFloatInstruction>(arg, irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(ins);
                return ins->reg;
            }
            else {
                auto ins = std::make_shared<BitCastInstruction>(arg, irModule.getCurrentFunction()->createRegister(targetType), irModule.getCurrentBasicBlock(), targetType);
                irModule.getCurrentBasicBlock()->appendInstruction(ins);
                return ins->reg;
            }
        }
        return arg;
    };

    for (int i = 0; i < argv.size(); i++) {
        if (!argv[i]->type->operator==(func->formalArguments[i]->type)) {
            argv[i] = convertType(argv[i], func->formalArguments[i]->type);
        }
    }

    auto ins = std::make_shared<CallInstruction>(func, argv, irModule.getCurrentBasicBlock());
    irModule.getCurrentBasicBlock()->appendInstruction(ins);
    return ins->reg;
}

antlrcpp::Any MyVisitor::visitFuncRParams(SysY2022Parser::FuncRParamsContext *ctx)
{
    vector<ValuePtr> rst;
    for (int i = 0; i < ctx->children.size(); i += 2) {
        rst.emplace_back(std::any_cast<ValuePtr>(ctx->children[i]->accept(this)));
    }
    return rst;
}

antlrcpp::Any MyVisitor::visitUnaryUnaryExp(SysY2022Parser::UnaryUnaryExpContext *ctx)
{
    char op = ctx->unaryOp()->getText()[0];
    auto exp = std::any_cast<ValuePtr>(ctx->unaryExp()->accept(this));

    switch (op) {
    case '-':
        if (exp->isConst) {
            if (exp->type->isInt()){
                exp = Const::createConstant(Type::getInt(),-1*dynamic_cast<Const*>(exp.get())->intVal,exp->name);
            }
            else if (exp->type->isFloat()){
                exp = Const::createConstant(Type::getFloat(),float(-1.0*dynamic_cast<Const*>(exp.get())->floatVal),exp->name);
            }
        }
        else {
            if (exp->type->isBool()) {
                auto ins = shared_ptr<ExtInstruction>(new ExtInstruction(exp, Type::getInt(), false, irModule.getCurrentBasicBlock()));
                irModule.getCurrentBasicBlock()->appendInstruction(InstructionPtr(ins));
                exp = ins->reg;
            }
            if (exp->type->isInt()) {
                auto ins = shared_ptr<BinaryInstruction>(new BinaryInstruction(Const::createConstant(Type::getInt(),0), exp, op, irModule.getCurrentBasicBlock()));
                irModule.getCurrentBasicBlock()->appendInstruction(InstructionPtr(ins));
                exp = ins->reg;
            }
            else if (exp->type->isFloat()) {
                auto ins = shared_ptr<FnegInstruction>(new FnegInstruction(exp, irModule.getCurrentBasicBlock()));
                irModule.getCurrentBasicBlock()->appendInstruction(ins);
                exp = ins->reg;
            }
        }
        break;
    case '+':
        if (exp->isConst && exp->type->isInt() && dynamic_cast<Const *>(exp.get())->intVal == 0x80000000)
            dynamic_cast<Const *>(exp.get())->intVal = (int)0x80000000;
        if (exp->type->isBool()) {
            auto ins = shared_ptr<ExtInstruction>(new ExtInstruction(exp, Type::getInt(), false, irModule.getCurrentBasicBlock()));
            irModule.getCurrentBasicBlock()->appendInstruction(InstructionPtr(ins));
            exp = ins->reg;
        }
        break;
    case '!':
        if (!exp->type->isBool()) {
            if (exp->type->isFloat()) {
                auto instruction = shared_ptr<FcmpInstruction>(new FcmpInstruction(irModule.getCurrentBasicBlock(), exp));
                irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                exp = instruction->reg;
            }
            else {
                auto instruction = shared_ptr<IcmpInstruction>(new IcmpInstruction(irModule.getCurrentBasicBlock(), exp));
                irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                exp = instruction->reg;
            }
        }
        if (exp->isConst) {
            if (exp->type->isInt()){
                exp = Const::createConstant(Type::getBool(),bool(!dynamic_cast<Const*>(exp.get())->intVal),exp->name);
            }
            else if (exp->type->isFloat()){
                exp = Const::createConstant(Type::getBool(),std::abs(dynamic_cast<Const *>(exp.get())->floatVal) < 1e-6,exp->name);
            }
        }
        else {
            auto ins = shared_ptr<BinaryInstruction>(new BinaryInstruction(exp, Const::createConstant(Type::getBool(),true), op, irModule.getCurrentBasicBlock()));
            irModule.getCurrentBasicBlock()->appendInstruction(InstructionPtr(ins));
            exp = ins->reg;
        }
        break;
    default:
        break;
    }

    return exp;
}

antlrcpp::Any MyVisitor::visitMulMulExp(SysY2022Parser::MulMulExpContext *ctx)
{
    char op = ctx->children[1]->getText()[0];
    auto a = std::any_cast<ValuePtr>(ctx->mulExp()->accept(this));
    auto b = std::any_cast<ValuePtr>(ctx->unaryExp()->accept(this));

    if (a->isConst && b->isConst) {
        assert(!a->type->isBool() && !b->type->isBool());
        if (a->type->isFloat() || b->type->isFloat()) {
            float valA = a->type->isFloat() ? (dynamic_cast<Const *>(a.get())->floatVal) : (dynamic_cast<Const *>(a.get())->intVal);
            float valB = b->type->isFloat() ? (dynamic_cast<Const *>(b.get())->floatVal) : (dynamic_cast<Const *>(b.get())->intVal);
            float rst = 0;
            if (op == '*')
                rst = valA * valB;
            else if (op == '/')
                rst = valA / valB;
            return Const::createConstant(Type::getFloat(), rst);
        }
        else {
            auto valA = dynamic_cast<Const *>(a.get())->intVal;
            auto valB = dynamic_cast<Const *>(b.get())->intVal;
            long long rst = 0;
            if (op == '*')
                rst = valA * valB;
            else if (op == '/')
                rst = valA / valB;
            else
                rst = valA % valB;
            return Const::createConstant(Type::getInt(), (int)rst);
        }
    }
    auto convertToFloat = [&](ValuePtr val) -> ValuePtr {
        if (!val->type->isFloat()) {
            if (val->isConst) {
                int intVal = dynamic_cast<Const *>(val.get())->intVal;
                return Const::createConstant(Type::getFloat(), static_cast<float>(intVal), val->name);
            }
            else {
                auto instruction = std::make_shared<SignToFloatInstruction>(val, irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                return instruction->reg;
            }
        }
        return val;
    };

    if (a->type->isFloat() || b->type->isFloat()) {
        a = convertToFloat(a);
        b = convertToFloat(b);
    }
    auto instruction = shared_ptr<BinaryInstruction>(new BinaryInstruction(a, b, op, irModule.getCurrentBasicBlock()));
    irModule.getCurrentBasicBlock()->appendInstruction(instruction);
    return instruction->reg;
}

antlrcpp::Any MyVisitor::visitUnaryMulExp(SysY2022Parser::UnaryMulExpContext *ctx)
{
    return ctx->unaryExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitAddAddExp(SysY2022Parser::AddAddExpContext *ctx)
{
    char op = ctx->children[1]->getText()[0];
    auto a = std::any_cast<ValuePtr>(ctx->addExp()->accept(this));
    auto b = std::any_cast<ValuePtr>(ctx->mulExp()->accept(this));
    if (a->isConst && b->isConst) {
        assert(!a->type->isBool() && !b->type->isBool());
        if (a->type->isFloat() || b->type->isFloat()) {
            float valA = a->type->isFloat() ? (dynamic_cast<Const *>(a.get())->floatVal) : (dynamic_cast<Const *>(a.get())->intVal);
            float valB = b->type->isFloat() ? (dynamic_cast<Const *>(b.get())->floatVal) : (dynamic_cast<Const *>(b.get())->intVal);
            float rst = 0;
            if (op == '+')
                rst = valA + valB;
            else if (op == '-')
                rst = valA - valB;
            return Const::createConstant(Type::getFloat(), rst);
        }
        else {
            auto valA = dynamic_cast<Const *>(a.get())->intVal;
            auto valB = dynamic_cast<Const *>(b.get())->intVal;
            long long rst = 0;
            if (op == '+')
                rst = valA + valB;
            else if (op == '-')
                rst = valA - valB;
            return Const::createConstant(Type::getInt(), (int)rst);
        }
    }
    auto convertToFloat = [&](ValuePtr val) -> ValuePtr {
        if (!val->type->isFloat()) {
            if (val->isConst) {
                int intVal = dynamic_cast<Const *>(val.get())->intVal;
                return Const::createConstant(Type::getFloat(), static_cast<float>(intVal), val->name);
            }
            else {
                auto instruction = std::make_shared<SignToFloatInstruction>(val, irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                return instruction->reg;
            }
        }
        return val;
    };

    if (a->type->isFloat() || b->type->isFloat()) {
        a = convertToFloat(a);
        b = convertToFloat(b);
    }

    auto instruction = shared_ptr<BinaryInstruction>(new BinaryInstruction(a, b, op, irModule.getCurrentBasicBlock()));
    irModule.getCurrentBasicBlock()->appendInstruction(instruction);

    return instruction->reg;
}

antlrcpp::Any MyVisitor::visitMulAddExp(SysY2022Parser::MulAddExpContext *ctx)
{
    return ctx->mulExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitConstExp(SysY2022Parser::ConstExpContext *ctx)
{
    return ctx->addExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitIfStmt(SysY2022Parser::IfStmtContext *ctx)
{
    auto trueLabel = LabelPtr(new Label(BrInstruction::getifThenStr()));
    auto falseLabel = LabelPtr(new Label(BrInstruction::getifEndStr()));
    irModule.trueLabelStack.emplace(trueLabel);
    irModule.falseLabelStack.emplace(falseLabel);
    auto cond = ctx->cond()->accept(this);
    irModule.trueLabelStack.pop();
    irModule.falseLabelStack.pop();

    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(trueLabel)));
    ctx->stmt()->accept(this);
    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr(new BrInstruction(falseLabel, irModule.getCurrentBasicBlock())));
    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(falseLabel)));
    return nullptr;
}

antlrcpp::Any MyVisitor::visitIfElseStmt(SysY2022Parser::IfElseStmtContext *ctx)
{
    auto trueLabel = LabelPtr(new Label(BrInstruction::getifThenStr()));
    auto falseLabel = LabelPtr(new Label(BrInstruction::getifElseStr()));
    auto endLabel = LabelPtr(new Label(BrInstruction::getifEndStr()));
    irModule.trueLabelStack.emplace(trueLabel);
    irModule.falseLabelStack.emplace(falseLabel);
    ctx->cond()->accept(this);
    irModule.trueLabelStack.pop();
    irModule.falseLabelStack.pop();

    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(trueLabel)));
    ctx->children[4]->accept(this);
    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr(new BrInstruction(endLabel, irModule.getCurrentBasicBlock())));
    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(falseLabel)));
    ctx->children[6]->accept(this);
    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr(new BrInstruction(endLabel, irModule.getCurrentBasicBlock())));
    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(endLabel)));

    return nullptr;
}

antlrcpp::Any MyVisitor::visitWhileStmt(SysY2022Parser::WhileStmtContext *ctx)
{
    auto trueLabel = LabelPtr(new Label(BrInstruction::getwhileBodyStr()));
    auto falseLabel = LabelPtr(new Label(BrInstruction::getwhileEndStr()));
    auto condLabel = LabelPtr(new Label(BrInstruction::getWhileCondStr()));

    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr(new BrInstruction(condLabel, irModule.getCurrentBasicBlock())));
    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(condLabel)));
    irModule.loopConditionLabelStack.emplace(condLabel);
    irModule.loopEndLabelStack.emplace(falseLabel);
    irModule.trueLabelStack.emplace(trueLabel);
    irModule.falseLabelStack.emplace(falseLabel);
    ctx->cond()->accept(this);
    irModule.trueLabelStack.pop();
    irModule.falseLabelStack.pop();
    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(trueLabel)));
    ctx->stmt()->accept(this);
    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr(new BrInstruction(condLabel, irModule.getCurrentBasicBlock())));
    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(falseLabel)));
    irModule.loopEndLabelStack.pop();
    irModule.loopConditionLabelStack.pop();

    return nullptr;
}

antlrcpp::Any MyVisitor::visitBreakStmt(SysY2022Parser::BreakStmtContext *ctx)
{
    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr(new BrInstruction(irModule.loopEndLabelStack.top(), irModule.getCurrentBasicBlock())));
    return nullptr;
}

antlrcpp::Any MyVisitor::visitContinueStmt(SysY2022Parser::ContinueStmtContext *ctx)
{
    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr(new BrInstruction(irModule.loopConditionLabelStack.top(), irModule.getCurrentBasicBlock())));
    return nullptr;
}

antlrcpp::Any MyVisitor::visitRelRelExp(SysY2022Parser::RelRelExpContext *ctx)
{
    string op = ctx->children[1]->getText();
    auto a = std::any_cast<ValuePtr>(ctx->relExp()->accept(this));
    auto b = std::any_cast<ValuePtr>(ctx->addExp()->accept(this));
    if (a->isConst && b->isConst) {
        auto constA = dynamic_cast<Const *>(a.get());
        auto constB = dynamic_cast<Const *>(b.get());
        long long valA = constA->type->isBool() ? constA->boolVal : constA->intVal;
        long long valB = constB->type->isBool() ? constB->boolVal : constB->intVal;
        bool result = false;
        if (op == ">") {
            result = valA > valB;
        }
        else if (op == ">=") {
            result = valA >= valB;
        }
        else if (op == "<") {
            result = valA < valB;
        }
        else if (op == "<=") {
            result = valA <= valB;
        }
        return Const::createConstant(Type::getBool(), result);
    }
    else {
        if (a->type->isFloat() || b->type->isFloat()) {
            if (!b->type->isFloat()) {
                assert(b->type->isInt());
                if (b->isConst) {
                    b = Const::createConstant(a->type,float(dynamic_cast<Const *>(b.get())->intVal), b->name);
                }
            }
            auto ins = shared_ptr<FcmpInstruction>(new FcmpInstruction(irModule.getCurrentBasicBlock(), a, b, op));
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            return ins->reg;
        }
        else {
            auto ins = shared_ptr<IcmpInstruction>(new IcmpInstruction(irModule.getCurrentBasicBlock(), a, b, op));
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            return ins->reg;
        }
    }
}

antlrcpp::Any MyVisitor::visitAddRelExp(SysY2022Parser::AddRelExpContext *ctx)
{
    return visitChildren(ctx);
}

antlrcpp::Any MyVisitor::visitEqEqExp(SysY2022Parser::EqEqExpContext *ctx)
{
    string op = ctx->children[1]->getText();
    auto a = std::any_cast<ValuePtr>(ctx->eqExp()->accept(this));
    auto b = std::any_cast<ValuePtr>(ctx->relExp()->accept(this));

    if (a->isConst && b->isConst) {
        auto constA = dynamic_cast<Const *>(a.get());
        auto constB = dynamic_cast<Const *>(b.get());
        long long valA = constA->type->isBool() ? constA->boolVal : constA->intVal;
        long long valB = constB->type->isBool() ? constB->boolVal : constB->intVal;
        if (op == "==") {
            return Const::createConstant(Type::getBool(), valA == valB);
        }
        else {
            return Const::createConstant(Type::getBool(), valA != valB);
        }
    }
    else {
        auto convertToFloat = [&](ValuePtr val) -> ValuePtr {
            if (!val->type->isFloat()) {
                if (val->isConst) {
                    int intVal = dynamic_cast<Const *>(val.get())->intVal;
                    return Const::createConstant(Type::getFloat(), static_cast<float>(intVal), val->name);
                }
                else {
                    auto instruction = std::make_shared<SignToFloatInstruction>(val, irModule.getCurrentBasicBlock());
                    irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                    return instruction->reg;
                }
            }
            return val;
        };

        auto convertToInt = [&](ValuePtr val) -> ValuePtr {
            if (!val->type->isInt()) {
                auto instruction = std::make_shared<ExtInstruction>(val, Type::getInt(), false, irModule.getCurrentBasicBlock());
                irModule.getCurrentBasicBlock()->appendInstruction(instruction);
                return instruction->reg;
            }
            return val;
        };

        if (a->type->isFloat() || b->type->isFloat()) {
            a = convertToFloat(a);
            b = convertToFloat(b);
            auto ins = std::make_shared<FcmpInstruction>(irModule.getCurrentBasicBlock(), a, b, op);
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            return ins->reg;
        }
        else if (a->type->isBool() || b->type->isBool()) {
            a = convertToInt(a);
            b = convertToInt(b);
            auto ins = std::make_shared<IcmpInstruction>(irModule.getCurrentBasicBlock(), a, b, op);
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            return ins->reg;
        }
        else {
            auto ins = std::make_shared<IcmpInstruction>(irModule.getCurrentBasicBlock(), a, b, op);
            irModule.getCurrentBasicBlock()->appendInstruction(ins);
            return ins->reg;
        }
    }
}

antlrcpp::Any MyVisitor::visitRelEqExp(SysY2022Parser::RelEqExpContext *ctx)
{
    return visitChildren(ctx);
}

antlrcpp::Any MyVisitor::visitEqLAndExp(SysY2022Parser::EqLAndExpContext *ctx)
{
    auto trueLabel = irModule.trueLabelStack.top();
    auto falseLabel = irModule.falseLabelStack.top();

    auto val = std::any_cast<ValuePtr>(ctx->eqExp()->accept(this));

    if (!val->type->isBool()) {
        if (val->type->isFloat()) {
            auto instruction = shared_ptr<FcmpInstruction>(new FcmpInstruction(irModule.getCurrentBasicBlock(), val));
            irModule.getCurrentBasicBlock()->appendInstruction(instruction);
            val = instruction->reg;
        }
        else {
            auto instruction = shared_ptr<IcmpInstruction>(new IcmpInstruction(irModule.getCurrentBasicBlock(), val));
            irModule.getCurrentBasicBlock()->appendInstruction(instruction);
            val = instruction->reg;
        }
    }
    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr((new BrInstruction(val, trueLabel, falseLabel, irModule.getCurrentBasicBlock()))));
    if (val->isConst)
        return val;
    else
        return nullptr;
}

antlrcpp::Any MyVisitor::visitLAndLAndExp(SysY2022Parser::LAndLAndExpContext *ctx)
{
    auto trueLabel = irModule.trueLabelStack.top();
    auto falseLabel = irModule.falseLabelStack.top();
    auto andLabel = LabelPtr(new Label(BrInstruction::getAndStr()));
    irModule.trueLabelStack.emplace(andLabel);
    auto lAnd = ctx->lAndExp()->accept(this);
    irModule.trueLabelStack.pop();
    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(andLabel)));
    auto val = std::any_cast<ValuePtr>(ctx->eqExp()->accept(this));
    if (!val->type->isBool()) {
        auto instruction = shared_ptr<IcmpInstruction>(new IcmpInstruction(irModule.getCurrentBasicBlock(), val));
        irModule.getCurrentBasicBlock()->appendInstruction(instruction);
        val = instruction->reg;
    }
    irModule.getCurrentBasicBlock()->setTerminator(InstructionPtr((new BrInstruction(val, trueLabel, falseLabel, irModule.getCurrentBasicBlock()))));

    if (lAnd.has_value()&& lAnd.type() != typeid(nullptr)&& val->isConst) {
        auto land = std::any_cast<ValuePtr>(lAnd);
        bool val1 = dynamic_cast<Const *>(land.get())->boolVal;
        bool val2 = dynamic_cast<Const *>(val.get())->boolVal;
        bool rst = val1 && val2;
        irModule.getCurrentFunction()->basicBlocks.pop_back();
        auto terminator = std::make_shared<BrInstruction>(rst ? trueLabel : falseLabel, irModule.getCurrentBasicBlock());
        irModule.getCurrentBasicBlock()->terminator = InstructionPtr(terminator);
        return Const::createConstant(Type::getBool(), rst);
    }
    return nullptr;
}

antlrcpp::Any MyVisitor::visitLAndLOrExp(SysY2022Parser::LAndLOrExpContext *ctx)
{
    return ctx->lAndExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitLOrLOrExp(SysY2022Parser::LOrLOrExpContext *ctx)
{
    auto trueLabel = irModule.trueLabelStack.top();
    auto falseLabel = irModule.falseLabelStack.top();
    auto orLabel = LabelPtr(new Label(BrInstruction::getOrStr()));
    irModule.falseLabelStack.emplace(orLabel);
    auto lOr = ctx->lOrExp()->accept(this);
    irModule.falseLabelStack.pop();
    irModule.getCurrentFunction()->addBasicBlock(BasicBlockPtr(new BasicBlock(orLabel)));
    auto lAnd = ctx->lAndExp()->accept(this);
    if (lAnd.has_value()&& lAnd.type() != typeid(nullptr) && lOr.has_value()&& lOr.type() == typeid(nullptr)) {
        bool val1=false,val2=false;

        auto extractBoolVal = [](const std::any& expr) -> bool {
            try {
                return dynamic_cast<Const *>(std::any_cast<ValuePtr>(expr).get())->boolVal;
            } catch (const std::bad_any_cast& e) {
                std::cerr << "Bad any cast caught: " << e.what() << std::endl;
                return false;
            }
        };

        val1 = extractBoolVal(lAnd);
        val2 = extractBoolVal(lOr);
        bool rst = val1 || val2;
        irModule.getCurrentFunction()->basicBlocks.pop_back();
        irModule.getCurrentBasicBlock()->terminator = InstructionPtr(new BrInstruction(rst ? trueLabel : falseLabel, irModule.getCurrentBasicBlock()));
        return Const::createConstant(Type::getBool(), rst);
    }
    return nullptr;
}