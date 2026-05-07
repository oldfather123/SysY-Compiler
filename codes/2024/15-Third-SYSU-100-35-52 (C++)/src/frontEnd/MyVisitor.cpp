
#include "CFGAnalysis.h"
#include "IRbuilder.h"
#include "Instruction.h"
#include "MyVisitor.h"
#include "Type.h"
#include "Value.h"
#include "loopParallel.h"
#include <any>
#include <cassert>
#include <cstdio>
#include <memory>
#include <string>

using std::endl;
using std::stoi;
// NOLINTBEGIN
void MyVisitor::print(std::ostream& out) {
    irModule.dump(out);
}

// Run constantCombine and simplifyCFG in a loop until no change
void runComboA(Module& irModule) {
    bool opt = true;
    while(opt) {
        opt = false;
        for(auto& func : irModule.globalFunctions) {
            if(func->isLib)
                continue;
            opt |= constantCombine(func.get());
        }
        for(auto& func : irModule.globalFunctions) {
            if(func->isLib)
                continue;
            opt |= runSimplifyCFG(func.get());
        }
        for(auto& func : irModule.globalFunctions) {
            if(func->isLib)
                continue;
            opt |= constantCombine(func.get());
        }
    }
}

void runComboB(Module& irModule) {
    for(auto& func : irModule.globalFunctions) {
        if(func->isLib)
            continue;
        constStayRight(func.get());
        sub2add(func.get());
    }
}

void runComboC(Module& irModule) {
    for(auto& func : irModule.globalFunctions) {
        if(func->isLib)
            continue;
        bool opt = true;
        while(opt){
            opt = false;
            opt |= simplifyInst(func.get());
            DCE(func.get());
            opt |= cse(func.get());
            DCE(func.get());
            opt |= GVN(func.get());
            DCE(func.get());
            opt |= DAE(func.get());
        }

    }
}

/**
 * Performs optimization on the given IR module.
 * This function analyzes the control flow graph (CFG) of each function in the module and simplifies it.
 * Then, it performs memory-to-register transformation (mem2reg) on each function.
 *
 * @note This function skips any functions marked as library functions.
 *
 * @param irModule The IR module to perform optimization on.
 */
void MyVisitor::opt() {
    bool opt = true;

    runComboA(irModule);
    runGVN(irModule);
    runInline(irModule);
    runDCE(irModule);
    // global2Local(irModule);
    runMem2Reg(irModule);
    runComboA(irModule);
    // splitGEP(irModule);
    runDCE(irModule);
    runComboA(irModule);
    runComboB(irModule);
    runDCE(irModule);
    // loopAnalysis TODO: 如果 pass not changed  不需要重新计算 dom  和 cfg
    for(auto& func : irModule.globalFunctions) {
        if(func->isLib) {
            continue;
        }
        runLoopRotate(func.get());
        constantCombine(func.get());
        runSimplifyCFG(func.get());
        runLoopCanonicalize(func.get());
        auto cfg = runCFGAnalysis(func.get());
        auto dom = runDominateTreeAnalysis(func.get(), cfg);
        runLoopEliminate(func.get(), dom);
    }

    runDCE(irModule);

    //for(auto& func : irModule.globalFunctions) {
    //    if(func->isLib)
    //        continue;
    //    runLoopParallel(func.get(), irModule);
    //}
    //irModule.dump();
    for(auto& func : irModule.globalFunctions) {
        if(func->isLib)
            continue;
        auto cfg = runCFGAnalysis(func.get());
        auto dom = runDominateTreeAnalysis(func.get(), cfg);
        runLICM(func.get(), dom, cfg);
        cfg = runCFGAnalysis(func.get());
        dom = runDominateTreeAnalysis(func.get(), cfg);
        auto loopAnalysisResult = runLoopAnalysis(func.get(), dom);
        runLoopUnroll(func.get(), loopAnalysisResult, cfg);
        std::cout << "hello" << endl;
    }
    runDCE(irModule);
    runComboA(irModule);
    runDCE(irModule);
    for(auto& func : irModule.globalFunctions) {
        if(func->isLib)
            continue;
        auto cfg = runCFGAnalysis(func.get());
        auto dom = runDominateTreeAnalysis(func.get(), cfg);
        runLICM(func.get(), dom, cfg);
    }

    for(auto& func : irModule.globalFunctions) {
        if(func->isLib)
            continue;
        runPeepHole(func.get());
        runPhiEliminate(func.get());
        runLoopGepCombine(func.get());
    }
    runDCE(irModule);
    vector<Function*> funcs;
    for(auto& func : irModule.globalFunctions) {
        funcs.push_back(func.get());
    }

    for(auto func : funcs) {
        if(func->isLib)
            continue;
        runFuncAttrAnalysis(func);
        irModule.shouldAddCacheLookup |= runStateLessCache(func, irModule);
        runUnifyReturn(func);
    }
    runDCE(irModule);
    // runMem2Reg(irModule);
    runComboA(irModule);
    runCSE(irModule);
    runDCE(irModule);

    for(auto& func : irModule.globalFunctions) {
        if(func->isLib)
            continue;
        auto cfg = runCFGAnalysis(func.get());
        auto dom = runDominateTreeAnalysis(func.get(), cfg);
        runLICM(func.get(), dom, cfg);
        cfg = runCFGAnalysis(func.get());
        dom = runDominateTreeAnalysis(func.get(), cfg);
        auto loopAnalysisResult = runLoopAnalysis(func.get(), dom);
        runLoopUnroll(func.get(), loopAnalysisResult, cfg);
        std::cout << "hello" << endl;
    }
    runDCE(irModule);

    runComboC(irModule);
    runDCE(irModule);

    instCombine(irModule);
    runDCE(irModule);
    
    runComboC(irModule);

    constantArrPropagation(irModule);
    runDCE(irModule);

    runDeadArrElemim(irModule);
    runComboA(irModule);
    runDCE(irModule);
    runComboC(irModule);
    runComboA(irModule);

}

// 访问CompUnit，即整个代码
/**
 * Visits the CompUnitContext and returns the result of visiting its children.
 *
 * @param ctx The CompUnitContext to visit.
 * @return The result of visiting the children of the CompUnitContext.
 */
antlrcpp::Any MyVisitor::visitCompUnit(SysY2022Parser::CompUnitContext* ctx) {
    return visitChildren(ctx);
}

bool MyVisitor::dfsInitVal(SysY2022Parser::InitValContext* ctx) {
    int sz = ctx->children.size();  // NOLINT
    if(sz > 1) {
        for(int ind = 1; ind < sz - 1; ind += 2) {
            if(!dfsInitVal(dynamic_cast<SysY2022Parser::InitValContext*>(ctx->children[ind])))
                return false;
        }
        return true;
    }

    try {
        stoi(dynamic_cast<SysY2022Parser::ItemInitValContext*>(ctx)->getText());
        return true;
    } catch(const std::exception& e) {
        return false;
    }
}

antlrcpp::Any MyVisitor::visitItemConstInitVal(SysY2022Parser::ItemConstInitValContext* ctx) {
    return ctx->constExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitItemInitVal(SysY2022Parser::ItemInitValContext* ctx) {
    return ctx->exp()->accept(this);
}

antlrcpp::Any MyVisitor::visitListInitVal(SysY2022Parser::ListInitValContext* ctx) {
    return visitChildren(ctx);
}

/**
 * Visits a function formal parameter context and performs the necessary operations.
 *
 * @param ctx The function formal parameter context to visit.
 * @return The variable representing the formal parameter.
 */
antlrcpp::Any MyVisitor::visitFuncFParam(SysY2022Parser::FuncFParamContext* ctx) {
    auto name = ctx->Identifier()->getText();
    auto addrName = name + "_addr";
    auto type = new Type(ctx->bType()->getText());
    if(ctx->children.size() == 2) {
        assert(type->isInt() || type->isFloat());
        if(type->isInt()) {
            auto allocaInst = myBuilder.createAlloca(Type::getInt(), false, addrName);
            irModule.paramStringTable->insert(allocaInst, name);
            auto var = VariablePtr(new Int(name, false, false));
            myBuilder.createStore(allocaInst, var);
            return var;
        }
        auto allocaInst = myBuilder.createAlloca(Type::getFloat(), false, addrName);
        irModule.paramStringTable->insert(allocaInst, name);
        auto var = VariablePtr(new Float(name, false, false));
        myBuilder.createStore(allocaInst, var);
        return var;
    }
    auto curr = type;
    // FIXME: assert Const
    for(int childInd = ctx->children.size() - 2; childInd >= 5; childInd -= 3) {  // NOLINT
        curr = TypePtr(new ArrType(curr, stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->getStr())));
    }
    curr = TypePtr(new PtrType(curr));
    auto allocaInst = myBuilder.createAlloca(curr, false, addrName);
    irModule.paramStringTable->insert(allocaInst, name);
    auto var = VariablePtr(new Ptr(name, false, false, curr));
    myBuilder.createStore(allocaInst, var);
    return var;
}

/**
 * Visits the FuncFParamsContext and returns a vector of VariablePtr.
 *
 * This function iterates over the children of the FuncFParamsContext and extracts the VariablePtr
 * from each child by calling the accept() function. The extracted VariablePtr is then added to the
 * result vector. Finally, the result vector is returned.
 *
 * @param ctx The FuncFParamsContext to visit.
 * @return A vector of VariablePtr extracted from the children of the FuncFParamsContext.
 */
antlrcpp::Any MyVisitor::visitFuncFParams(SysY2022Parser::FuncFParamsContext* ctx) {
    vector<VariablePtr> rst;
    for(int i = 0; i < ctx->children.size(); i += 2) {
        auto fParam = std::any_cast<VariablePtr>(ctx->children[i]->accept(this));
        rst.emplace_back(fParam);
    }
    return rst;
}

/**
 * Visits a function definition in the SysY2022 language.
 *
 * This function is called when encountering a function definition in the SysY2022 language. It extracts the function type, name,
 * and parameters from the context and creates a new Function object. The function's basic block is set to "entry" and the insert
 * point is set to the basic block. If the function has parameters, they are extracted and added to the function's arguments. The
 * return basic block is set, and if the function's return type is not void, an AllocaInstruction is created for the return value.
 * The function's body is then visited, and the end instruction is solved. Finally, the return basic block and end instruction are
 * solved, and the current basic block is set to null.
 *
 * @param ctx The context of the function definition.
 * @return nullptr.
 */
antlrcpp::Any MyVisitor::visitFuncDef(SysY2022Parser::FuncDefContext* ctx) {
    auto type = new Type(ctx->funcType()->getText());
    auto name = ctx->Identifier()->getText();
    auto func = std::make_unique<Function>(type, name);
    myBuilder.setCurFunc(func.get());
    auto bb = myBuilder.addBasicBlock("entry");
    myBuilder.setInsertPoint(bb);
    auto params = vector<ValuePtr>();
    if(ctx->children.size() == 6) {
        irModule.paramStringTable = std::make_unique<StringTableNode>();
        auto rst = std::any_cast<vector<VariablePtr>>(ctx->funcFParams()->accept(this));
        for(auto var : rst) {
            params.emplace_back(var);
        }
    }
    // FIXME:
    func->args() = params;

    // auto func = std::make_shared<Function>(type, name, params);
    func->setReturnBasicBlock();
    func->returnBasicBlock->setBelongFunc(func.get());
    irModule.pushFunction(std::move(func));

    if(!type->isVoid()) {
        // VariablePtr retVal = VariablePtr(new Int("retval", false, false));
        // func->pushVariable(retVal);
        auto retVal = new AllocaInstruction(type, false);
        retVal->setLabel("retVal");
        retVal->setBasicBlock(myBuilder.getCurBasicBlock());
        myBuilder.getCurFunc()->retVal = retVal;
    } else {
        myBuilder.getCurFunc()->retVal = Void::get();
        assert(myBuilder.getCurFunc()->retVal->type->isVoid());
    }

    ctx->block()->accept(this);
    // TODO: solve end instruction
    if(type->isVoid() || !myBuilder.getCurBasicBlock()->endInstruction) {
        assert(myBuilder.getCurFunc()->returnBasicBlock);
        auto brInst = std::make_unique<BrInstruction>(myBuilder.getCurFunc()->returnBasicBlock);
        myBuilder.getCurBasicBlock()->setEndInstruction(std::move(brInst));
    }

    // auto retBb = myBuilder.addBasicBlock("return");

    // func->allocLocalVariable();
    myBuilder.getCurFunc()->solveEndInstruction();
    // irModule.getBlock()->setBBbelongFunc(irModule.globalFunctions.back());
    // irModule.globalFunctions.back()->block = irModule.popBlock();
    myBuilder.getCurFunc()->solveReturnBasicBlock(myBuilder);
    // irModule.globalFunctions.back()->block->solveEndInstruction();

    myBuilder.setCurBasicBlock(nullptr);

    return nullptr;
}

/**
 * Visits the BlockContext and performs necessary operations.
 *
 * This function pushes a new string table to the IR module and copies the variable table from the parameter string table, if it
 * exists. Then, it visits the children of the BlockContext. Finally, it pops the string table from the IR module.
 *
 * @param ctx The BlockContext to visit.
 * @return nullptr
 */
antlrcpp::Any MyVisitor::visitBlock(SysY2022Parser::BlockContext* ctx) {
    irModule.pushBackStringTable();
    if(irModule.paramStringTable) {
        auto table = irModule.paramStringTable->variableTable;
        for(auto& it : table) {
            irModule.currStringTable->variableTable[it.first] = it.second;
        }
        irModule.paramStringTable = nullptr;
    }
    visitChildren(ctx);
    irModule.popStringTable();
    return nullptr;
}

antlrcpp::Any MyVisitor::visitReturnStmt(SysY2022Parser::ReturnStmtContext* ctx) {
    ValuePtr value;
    if(ctx->exp()) {
        value = std::any_cast<ValuePtr>(ctx->exp()->accept(this));
        if(!value->type->operator==(irModule.globalFunctions.back()->retVal->type)) {
            if(value->type->isFloat() && irModule.globalFunctions.back()->retVal->type->isInt()) {
                if(value->isConst) {
                    value = Const::getConst(Type::getInt(), int(dynamic_cast<Const*>(value)->floatVal));
                } else {
                    value = myBuilder.createFpToSi(value);
                }
            } else if(value->type->isInt() && irModule.globalFunctions.back()->retVal->type->isFloat()) {
                if(value->isConst) {
                    value = Const::getConst(Type::getFloat(), float(dynamic_cast<Const*>(value)->intVal));
                } else {
                    value = myBuilder.createSiToFp(value);
                }
            }
            // to-do
        }
        myBuilder.createStore(myBuilder.getCurFunc()->retVal, value);
        myBuilder.getCurFunc()->retvalSet = true;
    } else {
        value = Void::get();
    }
    auto curBB = myBuilder.getCurBasicBlock();
    assert(myBuilder.getCurFunc()->returnBasicBlock);
    auto brInst = std::make_unique<BrInstruction>(myBuilder.getCurFunc()->returnBasicBlock);
    curBB->setEndInstruction(std::move(brInst));
    return nullptr;
}

antlrcpp::Any MyVisitor::visitVarDefWithOutInitVal(SysY2022Parser::VarDefWithOutInitValContext* ctx) {
    auto name = ctx->Identifier()->getText();
    int childSize = ctx->children.size();  // NOLINT
    // to-do: type
    if(!myBuilder.getCurFunc())  // 全局变量
    {
        if(childSize == 1) {
            if(irModule.declType->isInt())
                irModule.pushVariable(new Int(name, true, false), name);
            else if(irModule.declType->isFloat())
                irModule.pushVariable(new Float(name, true, false), name);
        } else {
            auto any_const = (ctx->children[childSize - 2]->accept(this));
            auto curr = new ArrType(irModule.declType,
                                    stoi(std::any_cast<ValuePtr>(ctx->children[childSize - 2]->accept(this))->getStr()));
            for(int childInd = childSize - 5; childInd > 1; childInd -= 3) {
                curr = new ArrType(curr, stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->getStr()));
            }
            auto variable = new Arr(name, true, false, curr);
            irModule.pushVariable(variable, name);
        }
    } else  // 局部变量
    {
        if(childSize == 1) {
            auto allocaInst = myBuilder.createAlloca(irModule.declType, false, name);
            irModule.currStringTable->insert(allocaInst, name);
        } else {
            auto curr = new ArrType(irModule.declType,
                                    stoi(std::any_cast<ValuePtr>(ctx->children[childSize - 2]->accept(this))->getStr()));
            for(int childInd = childSize - 5; childInd > 1; childInd -= 3) {
                curr = new ArrType(curr, stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->getStr()));
            }
            auto variable = std::make_unique<Arr>(name, false, false, curr);
            auto allocaInst = myBuilder.createAlloca(curr, false, name);
            irModule.currStringTable->insert(allocaInst, name);
        }
    }

    return visitChildren(ctx);
}

/**
 * Function: MyVisitor::arrInitItem
 * Description: This function processes an item initialization value context and returns a VariablePtr object.
 * Parameters:
 *   - ctx: A pointer to the SysY2022Parser::ItemInitValContext object representing the item initialization value context.
 *   - type: A pointer to the Type object representing the type of the variable.
 * Returns: A VariablePtr object representing the initialized variable.
 */
VariablePtr MyVisitor::arrInitItem(SysY2022Parser::ItemInitValContext* ctx, TypePtr type) {  // NOLINT
    auto number = std::any_cast<ValuePtr>(dynamic_cast<SysY2022Parser::ItemInitValContext*>(ctx)->accept(this));
    if(irModule.declType->isInt())
        return VariablePtr(new Int("", false, false, number));
    return VariablePtr(new Float("", false, false, number));
}

/**
 * \brief Initializes an array with a list of values.
 *
 * This function takes a pointer to a `ListInitValContext` object and a `TypePtr` object as parameters.
 * It creates a unique pointer to an `Arr` object and initializes it with the given type.
 * The function then iterates over the children of the `ListInitValContext` object and adds the corresponding values to the array.
 * If a child has more than one child, indicating nested initialization, the function recursively calls itself to initialize the
 * nested array. The function returns the initialized array as a unique pointer.
 *
 * \param ctx A pointer to a `ListInitValContext` object representing the list of values.
 * \param type A `TypePtr` object representing the type of the array.
 * \return A unique pointer to the initialized array.
 */
Arr* MyVisitor::arrInitList(SysY2022Parser::ListInitValContext* ctx, TypePtr type) {
    int sz = ctx->children.size();  // NOLINT
    auto rst = new Arr("__const." + std::to_string(constArr++), true, true, type);
    for(int ind = 1; ind < sz - 1; ind += 2) {
        VariablePtr curr;
        if(ctx->children[ind]->children.size() > 1) {
            auto tmp = arrInitList(dynamic_cast<SysY2022Parser::ListInitValContext*>(ctx->children[ind]), rst->getElementType());
            tmp->fill();
            curr = tmp;
        } else
            curr = arrInitItem(dynamic_cast<SysY2022Parser::ItemInitValContext*>(ctx->children[ind]), rst->getElementType());
        rst->push(curr);
    }
    return rst;
}

/**
 * @brief Initializes an array item with a constant value.
 *
 * This function takes a context object and a type object as parameters and returns a pointer to a Variable object.
 * The context object represents the constant initialization value of the item.
 * The type object represents the type of the array item.
 *
 * @param ctx A pointer to the context object representing the constant initialization value of the item.
 * @param type A pointer to the type object representing the type of the array item.
 * @return A pointer to a Variable object representing the initialized array item.
 */
VariablePtr MyVisitor::arrInitItem(SysY2022Parser::ItemConstInitValContext* ctx, TypePtr type) {  // NOLINT
    string number = dynamic_cast<SysY2022Parser::ItemConstInitValContext*>(ctx)->getText();
    assert(!number.empty());
    return VariablePtr(new Int("", false, false, stoi(number)));
}

/**
 * \brief Initializes an array with a list of constant values.
 *
 * This function takes a parser context and a type pointer as input parameters.
 * It creates and returns a unique pointer to an `Arr` object, which represents an array.
 * The array is initialized with the constant values specified in the given parser context.
 *
 * \param ctx A pointer to the `ListConstInitValContext` object representing the parser context.
 * \param type A pointer to the `Type` object representing the type of the array.
 * \return A unique pointer to the initialized `Arr` object.
 */
Arr* MyVisitor::arrInitList(SysY2022Parser::ListConstInitValContext* ctx, TypePtr type) {
    int sz = ctx->children.size();  // NOLINT
    auto rst = new Arr("", true, true, type);
    for(int ind = 1; ind < sz - 1; ind += 2) {
        VariablePtr curr;
        if(ctx->children[ind]->children.size() > 1) {
            auto tmp =
                arrInitList(dynamic_cast<SysY2022Parser::ListConstInitValContext*>(ctx->children[ind]), rst->getElementType());
            tmp->fill();
            curr = tmp;
        } else
            curr = arrInitItem(dynamic_cast<SysY2022Parser::ItemConstInitValContext*>(ctx->children[ind]), rst->getElementType());
        rst->push(curr);
    }
    return rst;
}

bool indexAdd(vector<ValuePtr>& indexs, int depth, TypePtr type) {  // NOLINT
    if(depth == indexs.size() - 1) {
        int oldIndex = dynamic_cast<Const*>(indexs[depth])->intVal;  // NOLINT
        assert(type->isArr());
        if(dynamic_cast<ArrType*>(type)->length - 1 == oldIndex) {
            return false;
        }
        indexs[depth] = Const::getConst(Type::getInt64(), (long long)(oldIndex + 1));  // NOLINT
        return true;
    }
    if(!indexAdd(indexs, depth + 1, dynamic_cast<ArrType*>(type)->inner)) {
        int oldIndex = dynamic_cast<Const*>(indexs[depth])->intVal;  // NOLINT
        if(dynamic_cast<ArrType*>(type)->length - 1 == oldIndex)
            return false;
        indexs[depth] = Const::getConst(Type::getInt64(), (long long)(oldIndex + 1));  // NOLINT
        indexs[depth + 1] = Const::getConst(Type::getInt64(), (long long)(0));         // 累进可能有bug
        return true;
    }
    return true;
}

void MyVisitor::arrSetList(SysY2022Parser::ListInitValContext* ctx, ValuePtr value) {  // NOLINT
    {                                                                                  // 暴力 set
        vector<ValuePtr> indexs;
        int depth = dynamic_cast<ArrType*>(value->type)->getDepth();
        for(int i = 0; i <= depth; i++)
            indexs.emplace_back(Const::getConst(Type::getInt64(), (long long)(0)));
        // indexs.emplace_back(ValuePtr(new Const(Type::getInt64(), 0)));
        if(ctx->children.size() == 2)
            return;
        for(int i = 1, ind = 0; i < ctx->children.size(); i += 2, ind++) {
            if(ctx->children[i]->children.size() == 1) {  // 值
                auto reg = std::any_cast<ValuePtr>(ctx->children[i]->accept(this));
                auto ins = std::make_unique<GetElementPtrInstruction>(value, indexs);
                ins->setLabel("gep");
                if(!(value->valueId() == ValueID::Instruction) && dynamic_cast<Variable*>(value)->isGlobal) {
                    myBuilder.createStore(ins.get(), reg);
                } else {
                    auto tmp = ins.get();
                    myBuilder.pushInstruction(std::move(ins));
                    if(!tmp->type->operator==(reg->type)) {
                        if(tmp->type->isInt() && reg->type->isFloat()) {
                            reg = myBuilder.createFpToSi(reg);
                        } else if(tmp->type->isFloat() && reg->type->isInt()) {
                            reg = myBuilder.createSiToFp(reg);
                        }
                    }
                    myBuilder.createStore(tmp, reg);
                }

                indexAdd(indexs, 1, value->type);
            } else {  // 列表
                vector<ValuePtr> inner = { indexs[0], indexs[1] };
                auto ins = myBuilder.createGEP(value, inner);
                arrSetList(dynamic_cast<SysY2022Parser::ListInitValContext*>(ctx->children[i]), ins);
                indexs[1] = Const::getConst(Type::getInt64(), (long long)(dynamic_cast<Const*>(indexs[1])->intVal + 1));
                for(int i = 2; i <= depth; i++)
                    indexs[i] = Const::getConst(Type::getInt64(), (long long)(0));
            }
        }
    }
}

antlrcpp::Any MyVisitor::visitVarDefWithInitVal(SysY2022Parser::VarDefWithInitValContext* ctx) {
    auto name = ctx->Identifier()->getText();
    int childSize = ctx->children.size();  // NOLINT

    if(myBuilder.getCurFunc() == nullptr)  // 全局变量
    {
        if(childSize == 3) {
            //* Identifier '=' initVal
            if(irModule.declType->isInt())
                irModule.pushVariable(new Int(name, true, false, std::any_cast<ValuePtr>(ctx->initVal()->accept(this))), name);
            else if(irModule.declType->isFloat())
                irModule.pushVariable(new Float(name, true, false, std::any_cast<ValuePtr>(ctx->initVal()->accept(this))), name);
        } else {
            //* Identifier ('[' constExp ']')* '=' initVal
            auto curr = new ArrType(irModule.declType,
                                    stoi(std::any_cast<ValuePtr>(ctx->children[childSize - 4]->accept(this))->getStr()));
            for(int childInd = childSize - 7; childInd > 1; childInd -= 3) {
                curr = new ArrType(curr, stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->getStr()));
            }
            auto initVal = arrInitList(dynamic_cast<SysY2022Parser::ListInitValContext*>(ctx->initVal()), curr);
            auto variable = new Arr(name, true, false, curr);
            variable->inner = initVal->inner;
            irModule.pushVariable(variable, name);
        }
    } else  // 局部变量
    {
        if(childSize == 3) {
            auto allocaInst = myBuilder.createAlloca(irModule.declType, false, name);
            auto exp = std::any_cast<ValuePtr>(ctx->initVal()->accept(this));
            // auto exp = ctx->initVal()->accept(this).as<ValuePtr>();
            if(((exp->type->id) != (allocaInst->type->id))) {
                // int->float
                if(exp->type->isInt()) {
                    exp = myBuilder.createSiToFp(exp);
                }
                // float->int
                else {
                    exp = myBuilder.createFpToSi(exp);
                }
            }

            myBuilder.createStore(allocaInst, exp);
            irModule.currStringTable->insert(allocaInst, name);

        } else  // 数组
        {
            auto curr = new ArrType(irModule.declType,
                                    stoi(std::any_cast<ValuePtr>(ctx->children[childSize - 4]->accept(this))->getStr()));
            for(int childInd = childSize - 7; childInd > 1; childInd -= 3) {
                curr = new ArrType(curr, stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->getStr()));
            }

            auto allocaInst = myBuilder.createAlloca(curr, false, name);
            irModule.currStringTable->insert(allocaInst, name);
            bool isConstInitVal = dfsInitVal(ctx->initVal());
            if(isConstInitVal) {
                auto arrBitCast = myBuilder.createCast(allocaInst, TypePtr(new PtrType(Type::getInt8())));
                vector<ValuePtr> argv = { arrBitCast, Const::getConst(Type::getInt8(), int8_t(0)),
                                          Const::getConst(
                                              Type::getInt64(),
                                              (long long)(dynamic_cast<ArrType*>(allocaInst->type)->getSize() *  // NOLINT
                                                          4)),                                                   // NOLINT
                                          Const::getConst(Type::getBool(), false) };
                myBuilder.createCall(irModule.globalFunctions[0].get(), argv);
                arrSetList(dynamic_cast<SysY2022Parser::ListInitValContext*>(ctx->initVal()), allocaInst);
            } else {
                auto arrBitCast = myBuilder.createCast(allocaInst, TypePtr(new PtrType(Type::getInt8())));
                vector<ValuePtr> argv = { arrBitCast, Const::getConst(Type::getInt8(), 0),
                                          Const::getConst(Type::getInt64(),
                                                          dynamic_cast<ArrType*>(allocaInst->type)->getSize() * 4),
                                          Const::getConst(Type::getBool(), false) };
                myBuilder.createCall(irModule.globalFunctions[0].get(), argv);
                // 初始值一位一位 set
                arrSetList(dynamic_cast<SysY2022Parser::ListInitValContext*>(ctx->initVal()), allocaInst);
            }
        }
    }

    return nullptr;
}

antlrcpp::Any MyVisitor::visitConstDecl(SysY2022Parser::ConstDeclContext* ctx) {
    return visitChildren(ctx);
}

antlrcpp::Any MyVisitor::visitBType(SysY2022Parser::BTypeContext* ctx) {
    irModule.declType = new Type(ctx->getText());
    return nullptr;
}

antlrcpp::Any MyVisitor::visitConstDef(SysY2022Parser::ConstDefContext* ctx) {
    auto name = ctx->Identifier()->getText();
    int childSize = ctx->children.size();  // NOLINT

    // to-do: type
    if(myBuilder.getCurFunc() == nullptr)  // 全局变量
    {
        if(childSize == 3) {
            //* Identifier '=' constInitVal
            if(irModule.declType->isInt())
                irModule.pushVariable(new Int(name, true, true, std::any_cast<ValuePtr>(ctx->constInitVal()->accept(this))),
                                      name);
            if(irModule.declType->isFloat())
                irModule.pushVariable(new Float(name, true, true, std::any_cast<ValuePtr>(ctx->constInitVal()->accept(this))),
                                      name);
        } else {
            //* Identifier ('[' constExp ']')* '=' constInitVal;
            auto curr = new ArrType(irModule.declType,
                                    stoi(std::any_cast<ValuePtr>(ctx->children[childSize - 4]->accept(this))->getStr()));
            for(int childInd = childSize - 7; childInd > 1; childInd -= 3) {
                curr = new ArrType(curr, stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->getStr()));
            }
            // TODO: 处理数组
            auto initVal = arrInitList(dynamic_cast<SysY2022Parser::ListConstInitValContext*>(ctx->constInitVal()), curr);
            initVal->setLabel(name);
            irModule.pushVariable(initVal, name);
        }
    } else  // 局部变量
    {
        if(childSize == 3) {
            VariablePtr variable;
            if(irModule.declType->isInt())
                variable = VariablePtr(new Int(name, false, true, std::any_cast<ValuePtr>(ctx->constInitVal()->accept(this))));
            else if(irModule.declType->isFloat())
                variable = VariablePtr(new Float(name, false, true, std::any_cast<ValuePtr>(ctx->constInitVal()->accept(this))));
            irModule.currStringTable->insert(variable, name);
        } else  // 数组
        {
            // TODO 数组
            auto curr = new ArrType(irModule.declType,
                                    stoi(std::any_cast<ValuePtr>(ctx->children[childSize - 4]->accept(this))->getStr()));
            for(int childInd = childSize - 7; childInd > 1; childInd -= 3) {
                curr = new ArrType(curr, stoi(std::any_cast<ValuePtr>(ctx->children[childInd]->accept(this))->getStr()));
            }

            auto initVal = arrInitList(dynamic_cast<SysY2022Parser::ListConstInitValContext*>(ctx->constInitVal()), curr);
            initVal->setLabel("__const." + to_string(constArr++));
            irModule.currStringTable->variableTable[name] = initVal;
            irModule.pushVariable(initVal, name);
        }
    }

    return nullptr;
}

antlrcpp::Any MyVisitor::visitAssignStmt(SysY2022Parser::AssignStmtContext* ctx) {
    ValuePtr val = irModule.currStringTable->lookUp(ctx->lVal()->Identifier()->getText());

    auto exp = std::any_cast<ValuePtr>(ctx->exp()->accept(this));
    if(ctx->lVal()->children.size() == 1) {
        if(!val->type->operator==(exp->type)) {
            if(val->type->isInt() && exp->type->isFloat()) {
                exp = myBuilder.createFpToSi(exp);
            } else if(val->type->isFloat() && exp->type->isInt()) {
                exp = myBuilder.createSiToFp(exp);
            }
        }
        myBuilder.createStore(val, exp);
    } else {
        ValuePtr curr = val;
        if(val->type->isPtr()) {
            curr = myBuilder.createLoad(curr, dynamic_cast<PtrType*>(curr->type)->inner);

            // 解引用
            auto ind = std::any_cast<ValuePtr>(ctx->lVal()->children[2]->accept(this));
            if(ind->type != Type::getInt64()) {
                if(ind->isConst) {
                    ind = Const::getConst(Type::getInt64(), dynamic_cast<Const*>(ind)->intVal);
                } else {
                    ind = myBuilder.createExt(ind, Type::getInt64(), true);
                }
            }
            curr = myBuilder.createGEP(curr, { ind });

            // 处理数组
            if(ctx->lVal()->children.size() > 5) {
                vector<ValuePtr> indexs = { Const::getConst(Type::getInt64(), 0) };
                for(int i = 5; i < ctx->lVal()->children.size(); i += 3) {
                    auto ind = std::any_cast<ValuePtr>(ctx->lVal()->children[i]->accept(this));
                    if(ind->type != Type::getInt64()) {
                        if(ind->isConst) {
                            ind = Const::getConst(Type::getInt64(), dynamic_cast<Const*>(ind)->intVal);
                        } else {
                            ind = myBuilder.createExt(ind, Type::getInt64(), true);
                        }
                    }
                    indexs.emplace_back(ind);
                }
                curr = myBuilder.createGEP(curr, indexs);
            }
            if(!curr->type->operator==(exp->type)) {
                if(exp->isConst) {
                    if(exp->type->isInt()) {
                        exp = Const::getConst(Type::getFloat(), float(dynamic_cast<Const*>(exp)->intVal));
                    } else if(exp->type->isFloat()) {
                        exp = Const::getConst(Type::getInt(), int(dynamic_cast<Const*>(exp)->floatVal));
                    }
                } else {
                    if(val->type->isInt() && exp->type->isFloat()) {
                        // TODO: isconst
                        exp = myBuilder.createFpToSi(exp);
                    } else if(val->type->isFloat() && exp->type->isInt()) {
                        exp = myBuilder.createSiToFp(exp);
                    }
                }
            }
            myBuilder.createStore(curr, exp);
        } else {
            vector<ValuePtr> indexs = { Const::getConst(Type::getInt64(), 0) };
            for(int i = 2; i < ctx->lVal()->children.size(); i += 3) {
                auto ind = std::any_cast<ValuePtr>(ctx->lVal()->children[i]->accept(this));
                if(ind->type != Type::getInt64()) {
                    if(ind->isConst) {
                        ind = Const::getConst(Type::getInt64(), dynamic_cast<Const*>(ind)->intVal);
                    } else {
                        ind = myBuilder.createExt(ind, Type::getInt64(), true);
                    }
                }
                indexs.emplace_back(ind);
            }

            auto ins = myBuilder.createGEP(curr, indexs);
            if(!ins->type->operator==(exp->type)) {
                if(ins->type->isInt() && exp->type->isFloat()) {
                    // TODO: isconst
                    exp = myBuilder.createFpToSi(exp);
                } else if(ins->type->isFloat() && exp->type->isInt()) {
                    exp = myBuilder.createSiToFp(exp);
                }
            }
            myBuilder.createStore(ins, exp);
        }
    }
    return exp;  // 返回值
}

antlrcpp::Any MyVisitor::visitExp(SysY2022Parser::ExpContext* ctx) {
    return ctx->addExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitLVal(SysY2022Parser::LValContext* ctx) {
    auto valName = ctx->Identifier()->getText();
    ValuePtr val = irModule.currStringTable->lookUp(valName);
    assert(val && "val not found");
    int sz = ctx->children.size();  // NOLINT

    if(sz > 1)  // 数组
    {
        ValuePtr curr = val;
        vector<ValuePtr> indexs;
        for(int i = 2; i < sz; i += 3) {
            indexs.emplace_back(std::any_cast<ValuePtr>(ctx->children[i]->accept(this)));
        }
        if(val->type->isPtr())  // 函数参数访问
        {
            curr = myBuilder.createLoad(curr, dynamic_cast<PtrType*>(curr->type)->inner);

            // 解引用
            auto ind = indexs[0];
            if(ind->type != Type::getInt64()) {
                if(ind->isConst) {
                    ind = Const::getConst(Type::getInt64(), dynamic_cast<Const*>(ind)->intVal);
                } else {
                    ind = myBuilder.createExt(ind, Type::getInt64(), true);
                }
            }
            auto inst = myBuilder.createGEP(curr, { ind });
            curr = inst;
            indexs.erase(indexs.begin());
            // 处理数组
            if(sz > 5) {
                indexs.insert(indexs.begin(), Const::getConst(Type::getInt64(), 0));
                for(auto& index : indexs) {
                    if(index->type != Type::getInt64()) {
                        if(index->isConst) {
                            index = Const::getConst(Type::getInt64(), dynamic_cast<Const*>(index)->intVal);
                        } else {
                            index = myBuilder.createExt(index, Type::getInt64(), true);
                        }
                    }
                }
                curr = myBuilder.createGEP(curr, indexs);
            }
            return myBuilder.createLoad(curr, curr->type);
        }
        if(val->isConst) {
            bool isConst = true;
            for(auto& index : indexs) {
                if(!index->isConst) {
                    isConst = false;
                    break;
                }
            }
            if(isConst) {
                for(auto& index : indexs) {
                    auto ind = dynamic_cast<Const*>(index)->intVal;
                    curr = dynamic_cast<Arr*>(curr)->inner[ind];
                }
                if(curr->type->isInt())
                    return Const::getConst(Type::getInt(), dynamic_cast<Int*>(curr)->intVal);
                if(curr->type->isFloat())
                    return Const::getConst(Type::getFloat(), dynamic_cast<Float*>(curr)->floatVal);
                assert(0);
            }
        }
        int depth = dynamic_cast<ArrType*>(val->type)->getDepth();
        indexs.insert(indexs.begin(), Const::getConst(Type::getInt64(), 0));
        for(int i = 1; i < indexs.size(); i++) {
            auto ind = indexs[i];
            if(ind->type != Type::getInt64()) {
                if(ind->isConst) {
                    ind = Const::getConst(Type::getInt64(), dynamic_cast<Const*>(ind)->intVal);
                } else {
                    ind = myBuilder.createExt(ind, Type::getInt64(), true);
                }
                indexs[i] = ind;
            }
        }
        if(sz == depth * 3 + 1)  // 访问元素
        {
            shared_ptr<LoadInstruction> inst;
            auto gepInst = myBuilder.createGEP(val, indexs);
            return myBuilder.createLoad(gepInst, gepInst->type);
        }  // 访问行指针
        indexs.emplace_back(Const::getConst(Type::getInt64(), 0));
        auto ins = myBuilder.createGEP(curr, indexs);
        ins->type = TypePtr(new PtrType(ins->type));  // 指针化
        ins->as<GetElementPtrInstruction>()->typeIsImproved = true;
        return ins;
    }
    if(val->isConst) {
        if(val->type->isInt())
            return Const::getConst(Type::getInt(), dynamic_cast<Int*>(val)->intVal);
        return Const::getConst(Type::getFloat(), dynamic_cast<Float*>(val)->floatVal);
    }
    if(val->type->isArr())  // maybe bug
    {
        vector<ValuePtr> index = { Const::getConst(Type::getInt64(), 0), Const::getConst(Type::getInt64(), 0) };
        auto ins = myBuilder.createGEP(val, index);
        ins->type = TypePtr(new PtrType(ins->type));
        ins->as<GetElementPtrInstruction>()->typeIsImproved = true;
        return ins;
    }
    ValuePtr ldInst = myBuilder.createLoad(val, val->type);
    return ldInst;
}

antlrcpp::Any MyVisitor::visitExpPrimaryExp(SysY2022Parser::ExpPrimaryExpContext* ctx) {
    return ctx->exp()->accept(this);
}

antlrcpp::Any MyVisitor::visitLValPrimaryExp(SysY2022Parser::LValPrimaryExpContext* ctx) {
    return ctx->lVal()->accept(this);
}

antlrcpp::Any MyVisitor::visitNumberPrimaryExp(SysY2022Parser::NumberPrimaryExpContext* ctx) {
    return ctx->number()->accept(this);
}

antlrcpp::Any MyVisitor::visitNumber(SysY2022Parser::NumberContext* ctx) {
    if(ctx->getStart()->getType() == SysY2022Lexer::IntConst) {
        return Const::getConst(Type::getInt(), ctx->getText());
    }
    return Const::getConst(Type::getFloat(), ctx->getText());
}

antlrcpp::Any MyVisitor::visitPrimaryUnaryExp(SysY2022Parser::PrimaryUnaryExpContext* ctx) {
    auto primaryExp = std::any_cast<ValuePtr>(ctx->primaryExp()->accept(this));
    if(primaryExp->isConst && primaryExp->type->isInt() && dynamic_cast<Const*>(primaryExp)->intVal == 0x80000000)
        dynamic_cast<Const*>(primaryExp)->intVal = (int)0x80000000;
    return primaryExp;
}

antlrcpp::Any MyVisitor::visitFunctionCallUnaryExp(SysY2022Parser::FunctionCallUnaryExpContext* ctx) {
    vector<ValuePtr> argv;
    if(ctx->children.size() == 4) {
        argv = std::any_cast<vector<ValuePtr>>(ctx->funcRParams()->accept(this));
    }
    auto func = irModule.getFunction(ctx->Identifier()->getText());
    if(!func->isReenterable)
        irModule.globalFunctions.back()->isReenterable = false;
    // TODO: 有识别函数名之嫌
    if(func->getName().substr(0, 6) == "_sysy_") {
        argv.emplace_back(Const::getConst(Type::getInt(), 0));
    }
    int i = 0;
    for(auto& arg : argv) {
        if(!arg->type->operator==(func->formArguments[i]->type)) {
            if(arg->isConst) {
                if(arg->type->isFloat() && func->formArguments[i]->type->isInt()) {
                    arg = Const::getConst(Type::getInt(), int(dynamic_cast<Const*>(arg)->floatVal));
                } else if(arg->type->isInt() && func->formArguments[i]->type->isFloat()) {
                    arg = Const::getConst(Type::getFloat(), float(dynamic_cast<Const*>(arg)->intVal));
                }
                // to-do
            } else {
                if(arg->type->isFloat() && func->formArguments[i]->type->isInt()) {
                    arg = myBuilder.createFpToSi(arg);
                } else if(arg->type->isInt() && func->formArguments[i]->type->isFloat()) {
                    arg = myBuilder.createSiToFp(arg);
                } else {
                    arg = myBuilder.createCast(arg, func->formArguments[i]->type);
                }
            }
        }
        i++;
    }
    return myBuilder.createCall(func, argv);
}

antlrcpp::Any MyVisitor::visitFuncRParams(SysY2022Parser::FuncRParamsContext* ctx) {
    vector<ValuePtr> rst;
    for(int i = 0; i < ctx->children.size(); i += 2) {
        rst.emplace_back(std::any_cast<ValuePtr>(ctx->children[i]->accept(this)));
    }
    return rst;
}

antlrcpp::Any MyVisitor::visitUnaryUnaryExp(SysY2022Parser::UnaryUnaryExpContext* ctx) {
    char op = ctx->unaryOp()->getText()[0];
    auto exp = std::any_cast<ValuePtr>(ctx->unaryExp()->accept(this));

    switch(op) {
        case '-':
            if(exp->isConsantant()) {
                if(exp->type->isInt()) {
                    exp = Const::getConst(Type::getInt(), -1 * dynamic_cast<Const*>(exp)->intVal);
                } else if(exp->type->isFloat()) {
                    exp = Const::getConst(Type::getFloat(), float(-1.0 * dynamic_cast<Const*>(exp)->floatVal));
                }
            } else {
                if(exp->type->isBool()) {
                    exp = myBuilder.createExt(exp, Type::getInt(), false);
                }
                if(exp->type->isInt()) {
                    exp = myBuilder.createBinary(Sub, Const::getConst(Type::getInt(), 0), exp);
                } else if(exp->type->isFloat()) {
                    exp = myBuilder.createFneg(exp);
                }
            }
            break;
        case '+':
            // 处理 int 溢出
            if(exp->isConst && exp->type->isInt() && dynamic_cast<Const*>(exp)->intVal == 0x80000000)
                dynamic_cast<Const*>(exp)->intVal = (int)0x80000000;
            if(exp->type->isBool()) {
                exp = myBuilder.createExt(exp, Type::getInt(), false);
            }
            break;
        case '!':
            if(!exp->type->isBool()) {
                if(exp->type->isFloat()) {
                    exp = myBuilder.createFcmp(IcmpKind::ICmpNE, exp, Const::getConst(Type::getFloat(), 0));
                } else {
                    exp = myBuilder.createIcmp(IcmpKind::ICmpNE, exp, Const::getConst(Type::getInt(), 0));
                }
            };
            if(exp->isConst) {
                if(exp->type->isInt()) {
                    exp = Const::getConst(Type::getBool(), bool(!dynamic_cast<Const*>(exp)->intVal));
                } else if(exp->type->isFloat()) {
                    exp = Const::getConst(Type::getBool(), std::abs(dynamic_cast<Const*>(exp)->floatVal) < 1e-6);
                }
            } else {
                exp = myBuilder.createBinary(Xor, exp, Const::getConst(Type::getBool(), true));
            }
            break;
        default:
            assert(false);
    }

    return exp;
}

antlrcpp::Any MyVisitor::visitMulMulExp(SysY2022Parser::MulMulExpContext* ctx) {
    char op = ctx->children[1]->getText()[0];
    auto a = std::any_cast<ValuePtr>(ctx->mulExp()->accept(this));
    auto b = std::any_cast<ValuePtr>(ctx->unaryExp()->accept(this));

    if(a->isConst && b->isConst) {
        assert(!a->type->isBool() && !b->type->isBool());
        if(a->type->isFloat() || b->type->isFloat()) {
            float valA = a->type->isFloat() ? (dynamic_cast<Const*>(a)->floatVal) : (dynamic_cast<Const*>(a)->intVal);  // NOLINT
            float valB = b->type->isFloat() ? (dynamic_cast<Const*>(b)->floatVal) : (dynamic_cast<Const*>(b)->intVal);  // NOLINT
            float rst = 0;
            switch(op) {
                case '*':
                    rst = valA * valB;
                    break;
                case '/':
                    rst = valA / valB;
                    break;
                case '%':
                    rst = fmod(valA, valB);
                    break;
                default:
                    assert(false);
            }
            return Const::getConst(Type::getFloat(), rst);
        }
        auto valA = dynamic_cast<Const*>(a)->intVal;
        auto valB = dynamic_cast<Const*>(b)->intVal;
        long long rst = 0;
        switch(op) {
            case '*':
                rst = valA * valB;
                break;
            case '/':
                rst = valA / valB;
                break;
            case '%':
                rst = valA % valB;
                break;
            default:
                assert(false);
        }
        return Const::getConst(Type::getInt(), (int)rst);
    }
    if(a->type->isFloat() || b->type->isFloat()) {
        if(!a->type->isFloat()) {
            assert(a->type->isInt());
            if(a->isConst) {
                a = Const::getConst(Type::getFloat(), float(dynamic_cast<Const*>(a)->intVal));
            } else {
                a = myBuilder.createSiToFp(a);
            }
        } else if(!b->type->isFloat()) {
            assert(b->type->isInt());
            if(b->isConst) {
                b = Const::getConst(Type::getFloat(), float(dynamic_cast<Const*>(b)->intVal));
            } else {
                b = myBuilder.createSiToFp(b);
            }
        }
    }
    BinaryInstructionOps bOp;
    switch(op) {
        case '*':
            bOp = Mul;
            break;
        case '/':
            bOp = Div;
            break;
        case '%':
            bOp = Rem;
            break;
        default:
            break;
    }
    return myBuilder.createBinary(bOp, a, b);
}

antlrcpp::Any MyVisitor::visitUnaryMulExp(SysY2022Parser::UnaryMulExpContext* ctx) {
    return ctx->unaryExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitAddAddExp(SysY2022Parser::AddAddExpContext* ctx) {
    char op = ctx->children[1]->getText()[0];
    auto a = std::any_cast<ValuePtr>(ctx->addExp()->accept(this));
    auto b = std::any_cast<ValuePtr>(ctx->mulExp()->accept(this));

    if(a->isConst && b->isConst) {
        assert(!a->type->isBool() && !b->type->isBool());

        if(a->type->isFloat() || b->type->isFloat()) {
            float valA = a->type->isFloat() ? dynamic_cast<Const*>(a)->floatVal : dynamic_cast<Const*>(a)->intVal;
            float valB = b->type->isFloat() ? dynamic_cast<Const*>(b)->floatVal : dynamic_cast<Const*>(b)->intVal;
            float rst = 0;

            if(op == '+')
                rst = valA + valB;
            else if(op == '-')
                rst = valA - valB;

            return Const::getConst(Type::getFloat(), rst);
        }

        auto valA = dynamic_cast<Const*>(a)->intVal;
        auto valB = dynamic_cast<Const*>(b)->intVal;
        long long rst = 0;

        if(op == '+')
            rst = valA + valB;
        else if(op == '-')
            rst = valA - valB;

        return Const::getConst(Type::getInt(), static_cast<int>(rst));
    }

    if(a->type->isFloat() || b->type->isFloat()) {
        if(!a->type->isFloat()) {
            assert(a->type->isInt());

            if(a->isConst) {
                a = Const::getConst(Type::getFloat(), static_cast<float>(dynamic_cast<Const*>(a)->intVal));
            } else {
                a = myBuilder.createSiToFp(a);
            }
        } else if(!b->type->isFloat()) {
            assert(b->type->isInt());

            if(b->isConst) {
                b = Const::getConst(Type::getFloat(), static_cast<float>(dynamic_cast<Const*>(b)->intVal));
            } else {
                b = myBuilder.createSiToFp(b);
            }
        }
    }

    return myBuilder.createBinary(op == '+' ? Add : Sub, a, b);
}

antlrcpp::Any MyVisitor::visitMulAddExp(SysY2022Parser::MulAddExpContext* ctx) {
    return ctx->mulExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitConstExp(SysY2022Parser::ConstExpContext* ctx) {
    return ctx->addExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitIfStmt(SysY2022Parser::IfStmtContext* ctx) {
    auto trueBlock = std::make_unique<BasicBlock>(BrInstruction::getifThenStr());
    auto falseBlock = std::make_unique<BasicBlock>(BrInstruction::getifEndStr());
    trueBlock->setBelongFunc(myBuilder.getCurFunc());
    falseBlock->setBelongFunc(myBuilder.getCurFunc());
    irModule.trueBlockStack.emplace(trueBlock.get());
    irModule.falseBlockStack.emplace(falseBlock.get());
    auto cond = ctx->cond()->accept(this);
    irModule.trueBlockStack.pop();
    irModule.falseBlockStack.pop();

    auto tmpTrue = trueBlock.get();
    auto tmpFalse = falseBlock.get();
    myBuilder.getCurFunc()->addBasicBlock(std::move(trueBlock));
    myBuilder.setInsertPoint(tmpTrue);
    ctx->stmt()->accept(this);
    assert(falseBlock.get());
    auto brIns = std::make_unique<BrInstruction>(falseBlock.get());
    myBuilder.getCurBasicBlock()->setEndInstruction(std::move(brIns));
    myBuilder.getCurFunc()->addBasicBlock(std::move(falseBlock));
    myBuilder.setInsertPoint(tmpFalse);
    return nullptr;
}

antlrcpp::Any MyVisitor::visitIfElseStmt(SysY2022Parser::IfElseStmtContext* ctx) {
    auto trueBlock = std::make_unique<BasicBlock>(BrInstruction::getifThenStr());
    auto falseBlock = std::make_unique<BasicBlock>(BrInstruction::getifElseStr());
    auto endBlock = std::make_unique<BasicBlock>(BrInstruction::getifEndStr());
    auto tmpTrue = trueBlock.get();
    auto tmpFalse = falseBlock.get();
    auto tmpEnd = endBlock.get();
    trueBlock->setBelongFunc(myBuilder.getCurFunc());
    falseBlock->setBelongFunc(myBuilder.getCurFunc());
    endBlock->setBelongFunc(myBuilder.getCurFunc());
    irModule.trueBlockStack.emplace(trueBlock.get());
    irModule.falseBlockStack.emplace(falseBlock.get());
    ctx->cond()->accept(this);
    irModule.trueBlockStack.pop();
    irModule.falseBlockStack.pop();
    myBuilder.getCurFunc()->addBasicBlock(std::move(trueBlock));
    myBuilder.setInsertPoint(tmpTrue);
    ctx->children[4]->accept(this);
    myBuilder.getCurBasicBlock()->setEndInstruction(std::make_unique<BrInstruction>(endBlock.get()));
    myBuilder.getCurFunc()->addBasicBlock(std::move(falseBlock));
    myBuilder.setInsertPoint(tmpFalse);
    ctx->children[6]->accept(this);
    myBuilder.getCurBasicBlock()->setEndInstruction(std::make_unique<BrInstruction>(endBlock.get()));
    myBuilder.getCurFunc()->addBasicBlock(std::move(endBlock));
    myBuilder.setInsertPoint(tmpEnd);
    return nullptr;
}

antlrcpp::Any MyVisitor::visitWhileStmt(SysY2022Parser::WhileStmtContext* ctx) {
    auto trueBlock = std::make_unique<BasicBlock>(BrInstruction::getwhileBodyStr());
    auto falseBlock = std::make_unique<BasicBlock>(BrInstruction::getwhileEndStr());
    auto condBlock = std::make_unique<BasicBlock>(BrInstruction::getWhileCondStr());
    trueBlock->setBelongFunc(myBuilder.getCurFunc());
    falseBlock->setBelongFunc(myBuilder.getCurFunc());
    condBlock->setBelongFunc(myBuilder.getCurFunc());

    auto tmpTrue = trueBlock.get();
    auto tmpFalse = falseBlock.get();
    auto tmpcond = condBlock.get();
    myBuilder.getCurBasicBlock()->setEndInstruction(std::make_unique<BrInstruction>(condBlock.get()));
    myBuilder.getCurFunc()->addBasicBlock(std::move(condBlock));
    irModule.whileCondBlockStack.emplace(myBuilder.getCurFunc()->getBasicBlocks().back().get());
    irModule.whileEndBlockStack.emplace(falseBlock.get());
    irModule.trueBlockStack.emplace(trueBlock.get());
    irModule.falseBlockStack.emplace(falseBlock.get());
    myBuilder.setInsertPoint(tmpcond);
    ctx->cond()->accept(this);
    irModule.trueBlockStack.pop();
    irModule.falseBlockStack.pop();
    myBuilder.getCurFunc()->addBasicBlock(std::move(trueBlock));
    myBuilder.setInsertPoint(tmpTrue);
    ctx->stmt()->accept(this);
    myBuilder.getCurBasicBlock()->setEndInstruction(std::make_unique<BrInstruction>(tmpcond));
    myBuilder.getCurFunc()->addBasicBlock(std::move(falseBlock));
    myBuilder.setInsertPoint(tmpFalse);
    irModule.whileEndBlockStack.pop();
    irModule.whileCondBlockStack.pop();
    return nullptr;
}

antlrcpp::Any MyVisitor::visitBreakStmt(SysY2022Parser::BreakStmtContext* ctx) {  // NOLINT
    auto brInst = std::make_unique<BrInstruction>(irModule.whileEndBlockStack.top());
    myBuilder.getCurBasicBlock()->setEndInstruction(std::move(brInst));
    return nullptr;
}

antlrcpp::Any MyVisitor::visitContinueStmt(SysY2022Parser::ContinueStmtContext* ctx) {  // NOLINT
    auto brInst = std::make_unique<BrInstruction>(irModule.whileCondBlockStack.top());
    myBuilder.getCurBasicBlock()->setEndInstruction(std::move(brInst));
    return nullptr;
}

antlrcpp::Any MyVisitor::visitRelRelExp(SysY2022Parser::RelRelExpContext* ctx) {
    string op = ctx->children[1]->getText();
    auto a = std::any_cast<ValuePtr>(ctx->relExp()->accept(this));
    // auto a = ctx->relExp()->accept(this).as<ValuePtr>();
    auto b = std::any_cast<ValuePtr>(ctx->addExp()->accept(this));
    // auto b = ctx->addExp()->accept(this).as<ValuePtr>();
    map<string, IcmpKind> opMap = {
        { "<", IcmpKind::ICmpSLT },
        { "<=", IcmpKind::ICmpSLE },
        { ">", IcmpKind::ICmpSGT },
        { ">=", IcmpKind::ICmpSGE },
    };
    IcmpKind icmpKind = opMap[op];
    if(a->isConsantant() && b->isConsantant()) {
        auto constA = dynamic_cast<Const*>(a);
        auto constB = dynamic_cast<Const*>(b);
        double valA = a->type->isFloat() ? constA->floatVal : a->type->isInt() ? constA->intVal : constA->boolVal;
        double valB = b->type->isFloat() ? constB->floatVal : b->type->isInt() ? constB->intVal : constB->boolVal;
        switch(icmpKind) {
            case IcmpKind::ICmpSLT:
                return Const::getConst(Type::getBool(), valA < valB);
                break;
            case IcmpKind::ICmpSLE:
                return Const::getConst(Type::getBool(), valA <= valB);
                break;
            case IcmpKind::ICmpSGT:
                return Const::getConst(Type::getBool(), valA > valB);
                break;
            case IcmpKind::ICmpSGE:
                return Const::getConst(Type::getBool(), valA >= valB);
                break;
            default:
                throw std::runtime_error("unknown icmp kind");
                break;
        }
    }
    if(a->type->isFloat() || b->type->isFloat()) {
        if(!a->type->isFloat()) {
            if(a->type->isInt()) {
                a = myBuilder.createSiToFp(a);
            } else if(a->type->isBool()) {
                a = myBuilder.createExt(a, Type::getInt(), false);
                a = myBuilder.createExt(a, Type::getFloat(), false);
            }
        }
        if(!b->type->isFloat()) {
            if(b->type->isInt()) {
                b = myBuilder.createSiToFp(b);
            } else if(b->type->isBool()) {
                b = myBuilder.createExt(b, Type::getInt(), false);
                b = myBuilder.createExt(b, Type::getFloat(), false);
            }
        }
        auto ins = myBuilder.createFcmp(icmpKind, a, b);
        return ins;
    }
    return myBuilder.createIcmp(icmpKind, a, b);
}

antlrcpp::Any MyVisitor::visitAddRelExp(SysY2022Parser::AddRelExpContext* ctx) {
    return visitChildren(ctx);
}

antlrcpp::Any MyVisitor::visitEqEqExp(SysY2022Parser::EqEqExpContext* ctx) {
    string op = ctx->children[1]->getText();
    auto a = std::any_cast<ValuePtr>(ctx->eqExp()->accept(this));
    auto b = std::any_cast<ValuePtr>(ctx->relExp()->accept(this));
    map<string, IcmpKind> opMap = {
        { "==", IcmpKind::ICmpEQ },
        { "!=", IcmpKind::ICmpNE },
    };
    IcmpKind icmpKind = opMap[op];
    if(a->isConst && b->isConst) {
        auto constA = dynamic_cast<Const*>(a);
        auto constB = dynamic_cast<Const*>(b);
        // TODO: might consist problem in longlong->double
        double valA = a->type->isFloat() ? constA->floatVal : a->type->isInt() ? constA->intVal : constA->boolVal;
        double valB = b->type->isFloat() ? constB->floatVal : b->type->isInt() ? constB->intVal : constB->boolVal;
        if(op == "==") {
            return Const::getConst(Type::getBool(), valA == valB);
        }
        return Const::getConst(Type::getBool(), valA != valB);
    }
    if(a->type->id != b->type->id) {

        if(a->type->isBool()) {
            if(b->type->isInt()) {
                a = myBuilder.createExt(a, Type::getInt(), false);
                return myBuilder.createIcmp(icmpKind, a, b);
            }
            //* b is float
            a = myBuilder.createExt(a, Type::getInt(), false);
            a = myBuilder.createSiToFp(a);
            return myBuilder.createFcmp(icmpKind, a, b);
        }
        if(b->type->isBool()) {
            if(a->type->isInt()) {
                b = myBuilder.createExt(b, Type::getInt(), false);
                return myBuilder.createIcmp(icmpKind, a, b);
            }
            //* a is float
            b = myBuilder.createExt(b, Type::getInt(), false);
            b = myBuilder.createSiToFp(b);
            return myBuilder.createFcmp(icmpKind, a, b);
        }
        if(a->type->isFloat() || b->type->isFloat()) {
            if(a->type->isFloat()) {
                if(b->isConst) {
                    b = Const::getConst(Type::getFloat(), float(dynamic_cast<Const*>(b)->intVal));
                    return myBuilder.createFcmp(icmpKind, a, b);
                }  // to-do
                assert(false && "can not handle ");

            } else {
                if(a->isConst) {
                    a = Const::getConst(Type::getFloat(), float(dynamic_cast<Const*>(a)->intVal));
                    return myBuilder.createFcmp(icmpKind, a, b);
                }
                assert(false && "can not handle a->isConst");
            }
        } else {
            b = myBuilder.createExt(b, Type::getInt(), false);
            return myBuilder.createIcmp(icmpKind, a, b);
        }
    }
    // both float
    if(a->type->isFloat()) {
        return myBuilder.createFcmp(icmpKind, a, b);
    }
    // both int
    return myBuilder.createIcmp(icmpKind, a, b);
}

antlrcpp::Any MyVisitor::visitRelEqExp(SysY2022Parser::RelEqExpContext* ctx) {
    return visitChildren(ctx);
}

antlrcpp::Any MyVisitor::visitEqLAndExp(SysY2022Parser::EqLAndExpContext* ctx) {
    auto trueBlock = irModule.trueBlockStack.top();
    auto falseBlock = irModule.falseBlockStack.top();

    auto val = std::any_cast<ValuePtr>(ctx->eqExp()->accept(this));

    if(!val->type->isBool()) {
        if(val->type->isFloat()) {
            val = myBuilder.createFcmp(IcmpKind::ICmpNE, val, Const::getConst(Type::getFloat(), 0));
        } else {
            val = myBuilder.createIcmp(IcmpKind::ICmpNE, val, Const::getConst(Type::getInt(), 0));
        }
    }
    auto brInst = std::make_unique<BrInstruction>(val, trueBlock, falseBlock);
    myBuilder.getCurBasicBlock()->setEndInstruction(std::move(brInst));
    if(val->isConst)
        return val;
    return nullptr;
}

antlrcpp::Any MyVisitor::visitLAndLAndExp(SysY2022Parser::LAndLAndExpContext* ctx) {
    auto trueBlock = irModule.trueBlockStack.top();
    auto falseBlock = irModule.falseBlockStack.top();
    string andLabel = BrInstruction::getAndStr();
    auto andBlock = std::make_unique<BasicBlock>(andLabel);
    irModule.trueBlockStack.emplace(andBlock.get());
    auto lAnd = ctx->lAndExp()->accept(this);
    irModule.trueBlockStack.pop();
    myBuilder.pushBasicBlock(std::move(andBlock));
    myBuilder.setInsertPoint(myBuilder.getCurFunc()->getBasicBlocks().back().get());
    auto eqVal = std::any_cast<ValuePtr>(ctx->eqExp()->accept(this));
    if(!eqVal->type->isBool()) {
        eqVal = myBuilder.createIcmp(IcmpKind::ICmpNE, eqVal, Const::getConst(Type::getInt(), 0));
    }
    auto brInst = std::make_unique<BrInstruction>(eqVal, trueBlock, falseBlock);
    myBuilder.getCurBasicBlock()->setEndInstruction(std::move(brInst));

    if(lAnd.has_value() && lAnd.type() != typeid(nullptr) && eqVal->isConst) {
        auto land = std::any_cast<ValuePtr>(lAnd);
        bool val1 = dynamic_cast<Const*>(land)->boolVal;
        bool val2 = dynamic_cast<Const*>(eqVal)->boolVal;
        bool rst = val1 && val2;
        myBuilder.getCurFunc()->basicBlocks.pop_back();
        myBuilder.setInsertPoint(myBuilder.getCurFunc()->basicBlocks.back().get());
        if(rst) {
            myBuilder.getCurBasicBlock()->forceSetEndInst(std::make_unique<BrInstruction>(trueBlock));
        } else {
            myBuilder.getCurBasicBlock()->forceSetEndInst(std::make_unique<BrInstruction>(falseBlock));
        }
        return Const::getConst(Type::getBool(), rst);
    }
    return nullptr;
}

antlrcpp::Any MyVisitor::visitLAndLOrExp(SysY2022Parser::LAndLOrExpContext* ctx) {
    return ctx->lAndExp()->accept(this);
}

antlrcpp::Any MyVisitor::visitLOrLOrExp(SysY2022Parser::LOrLOrExpContext* ctx) {
    auto trueBlock = irModule.trueBlockStack.top();
    auto falseBlock = irModule.falseBlockStack.top();
    string orLabel = BrInstruction::getOrStr();
    auto orBlock = std::make_unique<BasicBlock>(orLabel);
    irModule.falseBlockStack.emplace(orBlock.get());
    auto lOr = ctx->lOrExp()->accept(this);
    irModule.falseBlockStack.pop();
    myBuilder.pushBasicBlock(std::move(orBlock));
    myBuilder.setInsertPoint(myBuilder.getCurFunc()->getBasicBlocks().back().get());
    auto lAnd = ctx->lAndExp()->accept(this);
    if(lAnd.has_value() && lAnd.type() != typeid(nullptr) && lOr.has_value() && lOr.type() == typeid(nullptr)) {
        bool val1 = false, val2 = false;
        val1 = dynamic_cast<Const*>(std::any_cast<ValuePtr>(lAnd))->boolVal;
        val2 = dynamic_cast<Const*>(std::any_cast<ValuePtr>(lOr))->boolVal;
        bool rst = val1 || val2;
        myBuilder.getCurFunc()->basicBlocks.pop_back();
        myBuilder.setInsertPoint(myBuilder.getCurFunc()->basicBlocks.back().get());
        if(rst) {
            myBuilder.getCurBasicBlock()->forceSetEndInst(std::make_unique<BrInstruction>(falseBlock));
        } else {
            // FIXME: 与hq原来不同
            myBuilder.getCurBasicBlock()->forceSetEndInst(std::make_unique<BrInstruction>(falseBlock));
        }
        // return ValuePtr(new Const(Type::getBool(), rst));
        return Const::getConst(Type::getBool(), rst);
    }
    return nullptr;
}
// NOLINTEND