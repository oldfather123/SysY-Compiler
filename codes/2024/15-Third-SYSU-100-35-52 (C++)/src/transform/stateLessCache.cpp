#include "Block.h"
#include "Function.h"
#include "IRbuilder.h"
#include "Instruction.h"
#include "Module.h"
#include "Type.h"
#include "Value.h"
#include "Variable.h"
#include "stateLessCache.h"
#include <memory>

static Function* getLookupLibFunc(Module& mod, ArrType* table, Type* i32) {
    // TODO:
    for(auto& func : mod.globalFunctions) {
        if(func->getName().find("yatccCacheLookup") != std::string::npos) {
            return func.get()->as<Function>();  // NOLINT
        }
    }
    // FIXME:??
    //  const auto signature = make<FunctionType>(PointerType::get(i32), Vector<const Type*>{ PointerType::get(table), i32, i32
    //  }); const auto func = make<Function>(String::get("cmmcCacheLookup"), signature);
    auto func = std::make_unique<Function>(i32, "yatccCacheLookup");  // NOLINT
    func->isLib = true;
    vector<Value*> cacheLookupArgv = { new Variable(table, ""), new Variable(i32, ""), new Variable(i32, "") };
    func->formArguments = std::move(cacheLookupArgv);
    mod.pushFunction(std::move(func));

    return mod.globalFunctions.back().get();
}

bool runStateLessCache(FunctionPtr func, Module& mod) {

    if(!func->stateless)
        return false;
    auto hasRecursive = [&] {
        uint32_t count = 0;
        for(auto& block : func->basicBlocks)
            for(auto& inst : block->instructionsRef()) {
                if(inst->insId == InsID::Call) {
                    auto callee = inst->getOperand(inst->getNumOperands() - 1);
                    if(callee == func)
                        ++count;
                    else
                        return false;
                }
            }
        return count >= 2;
    };

    if(!hasRecursive())
        return false;

    auto i32 = Type::getInt();
    auto f32 = Type::getFloat();
    auto ret = func->getType();

    // FIXME:
    if(!(ret->id == i32->id || ret->id == f32->id))  // NOLINT
        return false;
    std::vector<Value*> args;
    for(auto val : func->formArguments)
        if(val->getType()->id == i32->id || val->getType()->id == f32->id) {
            if(args.size() >= 2)
                return false;
            args.push_back(val);
        } else
            return false;
    if(args.empty())
        return false;

    IRbuilder builder{};
    BasicBlock* evalBlock = nullptr;
    for(auto& inst : func->getEntryBlock()->instructionsRef()) {
        if(inst->insId != InsID::Alloca) {
            if(inst.get() == func->getEntryBlock()->instructionsRef()[0].get()) {
                evalBlock = func->getEntryBlock();
                func->basicBlocks.insert(func->basicBlocks.begin(), make_unique<BasicBlock>("entry_new"));
                func->getEntryBlock()->belongfunc = func;
            } else
                evalBlock = splitBasicBlock(inst.get());
            break;
        }
    }

    builder.setCurBasicBlock(func->getEntryBlock());
    constexpr uint32_t tableSize = 1021;

    auto arrayType = new ArrType(i32, tableSize * 4);  // NOLINT

    auto lookup = getLookupLibFunc(mod, arrayType, i32);
    // auto lut = make<GlobalVariable>(String::get("lut_" + std::string(func.getSymbol().prefix())), arrayType,
    //                                       dataLyaout.getStorageAlignment());
    auto lut = new Arr("lut_" + func->getName(), true, false, arrayType);  // NOLINT

    mod.pushVariable(lut, "lut_"+ func->getName());

    // const auto ptr = builder.makeOp<FunctionCallInst>(lookup, std::move(argVal));
    // auto arg1 = builder.createCast(lut, lut->type);  // NOLINT
    auto arg1 = builder.createGEP(lut, { Const::getConst(Type::getInt64(), 0), Const::getConst(Type::getInt64(), 0) });
    arg1->type = new PtrType(arg1->type);  // NOLINT

    std::vector<Value*> argVal;
    argVal.reserve(3);
    argVal.push_back(arg1);
    for(auto arg : args) {
        if(arg->getType()->id == i32->id) {
            argVal.push_back(arg);
        } else {
            auto cst = builder.createFpToSi(arg);
            argVal.push_back(cst);
        }
    }
    while(argVal.size() < 3)
        argVal.push_back(Const::getConst(i32, 0));
    auto ptr = builder.createCall(lookup, std::move(argVal));
    // ptr->type = Type::getInt64();


    auto index1 = builder.createBinary(BinaryInstructionOps::Add, ptr, Const::getConst(Type::getInt64(), 2));
    //auto ext1 = builder.createExt(index1, Type::getInt64(), true);
    auto index2 = builder.createBinary(BinaryInstructionOps::Add, ptr, Const::getConst(Type::getInt64(), 3));
    //auto ext2 = builder.createExt(index2, Type::getInt64(), true);
    // auto base = builder.createGEP(lut, { ptr });
    //  Value* valPtr = builder.makeOp<GetElementPtrInst>(ptr, std::vector<Value*>{ ConstantInteger::get(i32, 2) });
    Value* valPtr = builder.createGEP(
        lut, { Const::getConst(Type::getInt64(), (long long)0), index1 /*Const::getConst(Type::getInt64(), (long long)2)*/ });

    // FIXME: 指针类型 !!!
    if(valPtr->getType() != ret)
        // valPtr = builder.makeOp<PtrCastInst>(valPtr, PointerType::get(ret));
        valPtr = builder.createCast(valPtr, ret);  // NOLINT

    // const auto hasValPtr = builder.makeOp<GetElementPtrInst>(ptr, std::vector<Value*>{ ConstantInteger::get(i32, 3) });
    // const auto hasVal = builder.makeOp<CompareInst>(InstructionID::ICmp, CompareOp::ICmpNotEqual,
    //                                                 builder.makeOp<LoadInst>(hasValPtr), ConstantInteger::get(i32, 0));
    auto hasValPtr = builder.createGEP(lut, { Const::getConst(Type::getInt64(), (long long)0), index2 });
    auto hasVal = builder.createIcmp(IcmpKind::ICmpNE, builder.createLoad(hasValPtr, hasValPtr->type), Const::getConst(i32, 0));
    // const auto earlyExit = builder.addBlock();
    auto earlyExit = builder.addBasicBlock("b");

    // builder.makeOp<BranchInst>(hasVal, 0.9, earlyExit, evalBlock);
    // builder.setCurrentBlock(earlyExit);
    // builder.makeOp<ReturnInst>(builder.makeOp<LoadInst>(valPtr));

    builder.createBranch(earlyExit, evalBlock, hasVal);
    builder.setCurBasicBlock(earlyExit);
    // builder.createCall(mod.globalFunctions[2].get(), {Const::getConst(Type::getInt(), 112)});
    builder.createRet(builder.createLoad(valPtr, valPtr->getType()));

    for(auto& block : func->basicBlocks) {
        if(block.get() == earlyExit)
            continue;
        const auto terminator = block->getTerminator();
        if(terminator->is<ReturnInstruction>()) {
            const auto retVal = terminator->getOperand(0);
            builder.setInsertPoint(terminator);
            builder.createStore(hasValPtr, Const::getConst(i32, 1));
            builder.createStore(valPtr, retVal);  // NOLINT
        }
    }
    fixPhiNode(func->getEntryBlock(), evalBlock);

    return true;
}
