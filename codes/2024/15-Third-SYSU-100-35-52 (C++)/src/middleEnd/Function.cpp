#include "Function.h"
#include "Instruction.h"
#include "Type.h"
#include "Value.h"
#include "Variable.h"

#include <IRbuilder.h>
#include <cassert>
#include <memory>
#include <utility>

void Function::pushVariable(VariablePtr variable) {  // NOLINT
    variables[variable->getName()] = variable;
}

void Function::setReturnBasicBlock() {
    returnBasicBlock = new BasicBlock("return");  // NOLINT
}

void Function::solveReturnBasicBlock(IRbuilder& builder) {
    // IRbuilder builder(returnBasicBlock);
    builder.setInsertPoint(returnBasicBlock);
    if(retVal->type->isVoid()) {
        auto retInst = builder.createRet(retVal);
        // 创建一个 uniqu_ptr<Basicblock> 获取 returnBasicBlock
        auto retBB = std::unique_ptr<BasicBlock>(returnBasicBlock);
        // retInst->setBasicBlock(returnBasicBlock);
        returnBasicBlock = retBB.get();
        this->basicBlocks.push_back(std::move(retBB));
    } else {
        // FIXME:
        auto ld = builder.createLoad(retVal, retVal->type);
        auto ret = builder.createRet(ld);
        auto retBB = std::unique_ptr<BasicBlock>(returnBasicBlock);
        // retInst->setBasicBlock(returnBasicBlock);
        returnBasicBlock = retBB.get();
        this->basicBlocks.push_back(std::move(retBB));
    }
}

// FIXME:
void Function::solveEndInstruction() {
    int notEntry = 0;
    for(auto& basicBlock : basicBlocks) {
        vector<unique_ptr<Instruction>> newIns;
        int i = 0;
        for(; i < basicBlock->instructionsRef().size(); i++) {
            // cerr<<i<<"  "<<basicBlock->instructions[i]->type<<endl;
            auto curIns = basicBlock->instructionsRef()[i].get();
            // newIns.push_back(curIns);
            if(basicBlock->instructionsRef()[i]->insId == InsID::Store) {
                auto storeIns = dynamic_cast<StoreInstruction*>(basicBlock->instructionsRef()[i].get());
                assert(storeIns);
                // auto ptr = dynamic_cast<User*>(storeIns->getPtr().get());
                auto ptr = storeIns->getPtr();
                assert(ptr);
                if(ptr == retVal) {
                    newIns.push_back(std::move(basicBlock->instructionsRef()[i]));
                    break;
                }
            }
            if(curIns->insId != InsID::Br && curIns->insId != InsID::Return)
                newIns.push_back(std::move(basicBlock->instructionsRef()[i]));
            if(curIns == basicBlock->beforeEndIns) {
                break;
            }
        }

        // 假设endInstruction是一个裸指针，我们需要找到它在原来的vector中的unique_ptr，并转移所有权
        auto& instructions = basicBlock->instructionsRef();
        std::unique_ptr<Instruction> endInsPtr;
        // 查找并移除endInstruction的所有权
        for(auto it = instructions.begin(); it != instructions.end(); ++it) {
            if(it->get() == basicBlock->endInstruction) {
                endInsPtr = std::move(*it);  // 转移所有权
                break;
            }
        }

        // 确保我们成功获取了所有权
        if(endInsPtr) {

            if(!notEntry && !retVal->isVoid()) {
                std::unique_ptr<Instruction> retIns = unique_ptr<Instruction>(retVal->as<Instruction>());
                retVal = retIns.get();
                newIns.insert(newIns.begin(), std::move(retIns));
            }
            
            instructions = std::move(newIns);
            // 将endInstruction的所有权重新添加到instructions中
            instructions.push_back(std::move(endInsPtr));
        } else {
            assert(false);
        }
        notEntry = 1;
    }
}

void Function::dump(std::ostream& out) {
    if(isLib) {
        out << "declare " << getTypeAndStr() << "(";
        for(int i = 0; i < formArguments.size(); i++) {
            if(i)
                out << ", ";
            out << formArguments[i]->type->getStr();
        }
        out << ")" << endl;
    } else {
        // std::cerr<<name<<": "<<isReenterable<<endl;
        out << "define " << getTypeStr() << " @" << getName() << "(";
        for(int i = 0; i < formArguments.size(); i++) {
            if(i)
                out << ", ";
            out << formArguments[i]->getTypeAndStr();
        }
        out << ") {" << endl;
        // block->print(out);
        for(auto& bb : basicBlocks) {
            bb->dump(out);
        }
        out << "}" << endl << endl;
    }
}

ValuePtr Function::addArg(TypePtr type) {
    auto arg = new Variable(type, "");
    formArguments.push_back(arg);
    return arg;
}
