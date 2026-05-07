#pragma once

#include "Block.h"
// #include "Instruction.h"
#include "IRbuilder.h"
#include "StringTable.h"
#include "Type.h"
#include "Value.h"
#include <Label.h>
#include <list>
#include <memory>
#include <ostream>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

struct BasicBlock;
struct Instruction;
struct Module;
struct Function;
using FunctionPtr = Function*;
using std::unique_ptr;

struct Function : User {
    Value* retVal = nullptr;
    BasicBlock* returnBasicBlock = nullptr;
    // 参数泄露就泄露吧
    vector<Value*> formArguments;
    // 注意使用的时候 用 auto&  必须引用
    vector<unique_ptr<BasicBlock>> basicBlocks;
    std::unordered_map<string, VariablePtr> variables;
    bool noMemoryRead = false;
    bool noMemoryWrite = false;
    bool noRecurse = false;
    bool stateless = false;
    bool noSideEffect = false;
    bool noReturn = false;
    bool loopBody = false;
    bool parallelBody = true;
    bool isLib;
    bool isReenterable;
    bool isParallelbody = false;
    bool retvalSet = false;
    // // 记录该函数的调用者以及调用次数
    // unordered_map<Function*, int> caller;
    // // 记录该函数调用的函数以及调用次数
    // unordered_map<Function*, int> callee;

    // std::unordered_set<Instruction*> callerIns;

    // TODO: add more constructor, yatcc 都做了 retVal 的初始化
    Function(TypePtr returnType, const string& name, vector<Value*> formArguments = vector<Value*>())
        : User{ returnType, name }, formArguments{ std::move(formArguments) }, isLib{ false }, isReenterable{ false } {
        // setReturnBasicBlock();
    }
    // FIXME: 目前只看到库函数使用了这个构造函数
    Function(TypePtr returnType, const string& name, bool isReenterable, vector<Value*> formArguments = vector<Value*>())
        : User{ returnType, name }, formArguments{ std::move(formArguments) }, isLib{ true }, isReenterable{ isReenterable } {
        // setReturnBasicBlock();
    }
    // Function(TypePtr returnType, string name, bool isReenterable, vector<ValuePtr> formArguments = vector<ValuePtr>());

    void setReturnBasicBlock();
    // notRef
    vector<unique_ptr<BasicBlock>>& getBasicBlocks() {
        return basicBlocks;
    }

    BasicBlock* getBasicBlock(int i) {
        return basicBlocks[i].get();
    }

    void solveReturnBasicBlock(IRbuilder& builder);

    void solveEndInstruction();

    ValuePtr addArg(TypePtr type);
    ValuePtr getArg(int i) {
        return formArguments[i];
    }

    auto& args() {
        return formArguments;
    }

    size_t getArgCount() {
        return formArguments.size();
    }
    ValuePtr getReturnValue() {
        return retVal;
    }
    BasicBlockPtr getEntryBlock() {
        return basicBlocks.front().get();
    }

    void dump(std::ostream& out = std::cout) override;
    [[nodiscard]] ValueID valueId() const override {
        return ValueID::Function;
    }

    void addBasicBlock(unique_ptr<BasicBlock> bb) {
        bb->setBelongFunc(this);
        basicBlocks.push_back(std::move(bb));
    }

    void pushVariable(VariablePtr variable);
    VariablePtr findVariable(string name);
    string getStr() override {
        return "@" + label.name();
    }

    bool deleteBasicBlock(BasicBlock* bb) {
        for(auto iter = basicBlocks.begin(); iter != basicBlocks.end(); iter++) {
            if((*iter).get() == bb) {
                basicBlocks.erase(iter);
                return true;
            }
        }
        return false;
    }

    vector<unique_ptr<BasicBlock>>::iterator begin() {
        return basicBlocks.begin();
    }

    vector<unique_ptr<BasicBlock>>::iterator end() {
        return basicBlocks.end();
    }

    vector<unique_ptr<BasicBlock>>::iterator getIterator(BasicBlockPtr bb) {
        assert(bb->belongfunc == this);
        auto it = std::find_if(basicBlocks.begin(), basicBlocks.end(),
                               [bb](const unique_ptr<BasicBlock>& ptr) -> bool { return ptr.get() == bb; });
        return it;
    }

    vector<std::unique_ptr<BasicBlock>>::iterator insertBefore(BasicBlockPtr before, unique_ptr<BasicBlock> bb) {
        auto it = getIterator(before);
        return basicBlocks.insert(it, std::move(bb));
    }

    vector<std::unique_ptr<BasicBlock>>::iterator insertBefore(vector<unique_ptr<BasicBlock>>::iterator before,
                                                               unique_ptr<BasicBlock> bb) {
        return basicBlocks.insert(before, std::move(bb));
    }

    vector<std::unique_ptr<BasicBlock>>::iterator insertAfter(BasicBlockPtr after, unique_ptr<BasicBlock> bb) {
        auto it = getIterator(after);
        return basicBlocks.insert(++it, std::move(bb));
    }

    vector<std::unique_ptr<BasicBlock>>::iterator insertAfter(vector<unique_ptr<BasicBlock>>::iterator after,
                                                              unique_ptr<BasicBlock> bb) {

        return basicBlocks.insert(++after, std::move(bb));
    }

    //~Function() override {
    //    for(auto& bb : basicBlocks) {
    //        bb.release();
    //    }
    //}
};
// using FunctionPtr = shared_ptr<Function>;
