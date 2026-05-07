#include "AsmFunction.h"
#include "AsmCode.h"
#include "GraphColoringAllocator.h"
#include "LifeTimeController.h"
#include "AsmPassController.h"

using namespace Backend;

AsmFunction::AsmFunction(AsmCode *asmCode, IR::Function *irFunction)
    : asmCode(asmCode), irFunction(irFunction)
{
    this->stackAllocator = new AsmStackAllocator();
    this->registerAllocator = new AsmRegisterAllocator();
    this->addressAllocator = new AsmAddressAllocator();
    this->lifetimeController = new LifeTimeController(this);
    this->passController = new AsmPassController();

    this->labelName = irFunction->getIRName().substr(1); // remove the first character '@'
    returnLabel = new AsmOperandLabel("return_" + labelName, false);
    initReturnReg();

    initFoamalParams();
}

void AsmFunction::genFunction()
{
    if (irFunction->isBuiltinFunction())
        return;

    // std::cerr << ">> Generating function: " << labelName << std::endl;

    prepareBBs();

    buildArgs();

    for (auto bb : bbList)
        bb->preGenInstsPhi();

    for (auto bb : bbList)
        bb->genBB();
    addInst(1, asmArgInstList);
    addInst(new AsmInstLabel(returnLabel));

    instList.setList(passController->runBeforeRegisterAlloc(instList.getList()));

    lifetimeController->getAllRegLifeTime(instList);
    std::cout << 1 << std::endl;
    allocateRegisters();

    buildStackInsts();

    instList.setList(passController->runAfterRegisterAlloc(instList.getList()));
}

std::string AsmFunction::emit()
{
    std::string s;

    s += "\t.text\n";
    s += "\t.align 1\n";
    s += "\t.globl " + labelName + "\n";
    s += "\t.type " + labelName + ", @function\n";
    s += labelName + ":\n";
    for (auto inst : instList.getList())
        s += inst->emit();
    s += "\t.size " + labelName + ", .-" + labelName + "\n";

    return s;
}

AsmCode *AsmFunction::getAsmCode() { return asmCode; }

IR::Function *AsmFunction::getIRFunction() { return irFunction; }

std::string AsmFunction::getLabelName() { return labelName; }

void AsmFunction::addInst(AsmInstBasic *inst) { instList.addInst(inst); }

void AsmFunction::addInst(AsmInstBasicList insts) { instList.addInst(insts); }

void AsmFunction::addInst(int index, AsmInstBasic *inst) { instList.addInst(index, inst); }

void AsmFunction::addInst(int index, AsmInstBasicList insts) { instList.addInst(index, insts); }

AsmBasicBlock *AsmFunction::getBB(IR::BasicBlock *bb)
{
    if (bbMap.find(bb) == bbMap.end())
        Error::Error(__PRETTY_FUNCTION__, "Basic block not found, bb: " + bb->getIRName());
    return bbMap[bb];
}

AsmBasicBlock *AsmFunction::getBB(AsmOperandLabel *label)
{
    if (bbLabelMap.find(label) == bbLabelMap.end())
        Error::Error(__PRETTY_FUNCTION__, "Basic block not found, label: " + label->toString());
    return bbLabelMap[label];
}

AsmBasicBlock *AsmFunction::getBB(AsmInstLabel *label)
{
    AsmOperandLabel *labelOperand = label->getAsmOperandLabel();
    return getBB(labelOperand);
}

AsmOperandRegister *AsmFunction::getReturnReg() { return returnReg; }

AsmOperandLabel *AsmFunction::getReturnLabel() { return returnLabel; }

AsmRegisterAllocator *AsmFunction::getRegisterAllocator() { return registerAllocator; }

AsmStackAllocator *AsmFunction::getStackAllocator() { return stackAllocator; }

AsmAddressAllocator *AsmFunction::getAddressAllocator() { return addressAllocator; }

AsmOperandBasic *AsmFunction::getArg(IR::Value *value)
{
    if (asmArgMap.find(value) == asmArgMap.end())
        Error::Error(__PRETTY_FUNCTION__, "Argument not found, value: " + value->getIRName());
    return asmArgMap[value];
}

AsmOperandRegister *AsmFunction::getArgReg(IR::Value *value)
{
    if (asmArgRegMap.find(value) == asmArgRegMap.end())
        Error::Error(__PRETTY_FUNCTION__, "Argument not found, value: " + value->getIRName());
    return asmArgRegMap[value];
}

AsmOperandLabel *AsmFunction::getBBLabel(IR::BasicBlock *bb)
{
    if (bbMap.find(bb) == bbMap.end())
        Error::Error(__PRETTY_FUNCTION__, "Basic block not found, bb: " + bb->getIRName());
    return bbMap[bb]->getBlockOperandLabel();
}

/**
 * @brief Get the Asm Operand object of the given IR Value object
 * @details GlobalVariable, argument, instruction, constant
 */
AsmOperandBasic *AsmFunction::getAsmOperand(IR::Value *value)
{
    if (value->isGlobalVariable())
    {
        // Get global variable asm operand,
        // allocate a int register to store the address of the global variable,
        // load the address of the global variable to the register,
        // return a AsmOperandMemoryAddress object containing the register.
        // i.e. return a memory address containing the global variable
        AsmGlobalVariable *globalVar = new AsmGlobalVariable(dynamic_cast<IR::GlobalVariable *>(value));
        AsmOperandRegisterInt *reg = registerAllocator->allocateIntRegister();
        addInst(new AsmInstLoad(reg, globalVar->getAsmOperandLabel()));
        return new AsmOperandMemoryAddress(reg, 0);
    }
    else if (value->isArgument())
        return getArgReg(value);
    else if (value->isInstruction())
    {
        // check the three allocator to see if the value is already allocated,
        //   if allocated, return the allocated operand(register, stack variable, memory address),
        //   if not found, raise error (should be allocated in the previous pass)
        IR::Instruction *inst = dynamic_cast<IR::Instruction *>(value);
        if (registerAllocator->contains(inst))
            return registerAllocator->get(inst);
        else if (stackAllocator->contains(inst))
            return transformStackVar(stackAllocator->get(inst), instList);
        else if (addressAllocator->contains(inst))
            return addressAllocator->get(inst);
        else
            Error::Error(__PRETTY_FUNCTION__, "Value not found in any allocator, value: " + value->getIRName());
    }
    else if (isIRConstant(value))
    {
        // return the Asm Operand object of the constant value
        return getConstant(value, this->instList);
    }
    else
    {
        // TODO: constant array, constant pointer are not supported
        Error::Error(__PRETTY_FUNCTION__, "Unknown value type, value: " + value->getIRName());
    }
    return nullptr;
}

/**
 * @brief Check if the given IR Value object is a constant
 * @details int32, float
 * @note array, pointer are not supported
 */
bool AsmFunction::isIRConstant(IR::Value *value)
{
    return value->isConstantInt32() || value->isConstantFloat();
}

/**
 * @brief Get the Asm Operand object of the given IR Value object
 * @param value IR Value object
 * @param appendInstList AsmInstBasicList object to store the generated instructions
 * @return AsmOperandBasic*
 * @details int32, float
 * @note array, pointer are not supported
 */
AsmOperandBasic *AsmFunction::getConstant(IR::Value *value, AsmInstBasicList &appendInstList)
{
    if (value->isConstantInt32())
    {
        // return a AsmOperandImmediate if the constant is within 12 bits,
        //   otherwise, return a AsmOperandRegister containing the constant value
        int val = dynamic_cast<IR::ConstantInt32 *>(value)->getValue();
        if (ImmediateWithin12Bits(val))
        {
            return new AsmOperandImmediate(val);
        }
        else
        {
            AsmOperandRegisterInt *reg = registerAllocator->allocateIntRegister();
            appendInstList.addInst(new AsmInstLoad(reg, new AsmOperandImmediate(val)));
            return reg;
        }
    }
    else if (value->isConstantFloat())
    {
        // load the address(label) of the float constant to a register,
        //   then load the float value from the address to another register,
        //   return the register containing the float value
        float val = dynamic_cast<IR::ConstantFloat *>(value)->getValue();
        AsmConstantFloat *AsmConstantFloat = asmCode->getConstantFloat(val);
        AsmOperandRegisterInt *addressReg = registerAllocator->allocateIntRegister();
        AsmOperandRegisterFloat *floatReg = registerAllocator->allocateFloatRegister();
        appendInstList.addInst(new AsmInstLoad(addressReg, AsmConstantFloat->getAsmOperandLabel()));
        appendInstList.addInst(new AsmInstLoad(floatReg, new AsmOperandMemoryAddress(addressReg, 0), 32));
        return floatReg;
    }
    else
    {
        Error::Error(__PRETTY_FUNCTION__, "Unsupported constant type");
        return nullptr;
    }
}

AsmOperandBasic *AsmFunction::getValueByType(IR::Value *value, IR::BasicType *type)
{
    AsmOperandBasic *asmOperand = getAsmOperand(value);
    // if (type->isPoint() && asmOperand->isMemoryAddress())
    if (type->isPoint())
    {
        AsmOperandMemoryAddress *address = getOperandToAddress(asmOperand);
        return getAddressToIntRegister(address);
    }
    else
    {
        return asmOperand;
    }
}

LifeTimeController *AsmFunction::getLifeTimeController() { return lifetimeController; }

void AsmFunction::initReturnReg()
{
    IR::FunctionType *funcType = dynamic_cast<IR::FunctionType *>(irFunction->getType());
    IR::BasicType *returnType = funcType->getBaseType();
    if (returnType->isInt32())
        returnReg = AsmOperandRegisterInt::getParamReg(0);
    else if (returnType->isFloat())
        returnReg = AsmOperandRegisterFloat::getParamReg(0);
    else if (returnType->isVoid())
        returnReg = nullptr;
    else
        Error::Error(__PRETTY_FUNCTION__, "Unknown return type");
}

void AsmFunction::initFoamalParams()
{
    int stackOffset = 0;
    int intParamsCounter = 0;
    int floatParamsCounter = 0;

    for (auto arg : irFunction->args())
    {
        IR::BasicType *argType = dynamic_cast<IR::Argument *>(arg)->getType();

        if (argType->isInt32() || argType->isPoint())
        {
            int argSize = argType->isInt32() ? 4 : 8;
            if (intParamsCounter < 8)
            {
                this->asmArgList.push_back(AsmOperandRegisterInt::getParamReg(intParamsCounter));
            }
            else
            {
                // store the argument in the stack variable at stackOffset(%s0)
                this->asmArgList.push_back(new AsmOperandStackVariable(false, stackOffset, argSize));
                stackOffset += argSize;
            }
            intParamsCounter++;
        }
        else if (argType->isFloat())
        {
            if (floatParamsCounter < 8)
            {
                this->asmArgList.push_back(AsmOperandRegisterFloat::getParamReg(floatParamsCounter));
            }
            else
            {
                this->asmArgList.push_back(new AsmOperandStackVariable(false, stackOffset, 4));
                stackOffset += 4;
            }
            floatParamsCounter++;
        }
        else
        {
            Error::Error(__PRETTY_FUNCTION__, "Unknown argument type");
        }

        this->asmArgMap[arg] = this->asmArgList.back();
    }
}

/**
 * @brief AsmBasicBlocks from IR::BasicBlocks
 * TODO: use bfs to get a better order
 */
void AsmFunction::prepareBBs()
{
    for (auto bb : irFunction->getVectorBlocks())
    {
        AsmBasicBlock *asmBB = new AsmBasicBlock(this, bb);
        bbList.push_back(asmBB);
        bbMap[bb] = asmBB;
        bbLabelMap[asmBB->getBlockOperandLabel()] = asmBB;
    }
    bbLabelMap[returnLabel] = bbLabelMap[bbList.back()->getBlockOperandLabel()];
}

/**
 * @brief Allocate registers and move the arguments to the registers
 * @details use move instruction if the argument is in the parameter register,
 *      use load instruction if the argument is in the stack variable
 */
void AsmFunction::buildArgs()
{
    if (irFunction->isBuiltinFunction())
        return;
    for (auto arg : irFunction->args())
    {
        AsmOperandRegister *argReg = registerAllocator->allocateRegister(arg->getType());
        asmArgRegMap[arg] = argReg;
        AsmOperandBasic *asmOperand = getFormalParamAsmOperand(arg, asmArgInstList);
        if (asmOperand->isRegister())
            asmArgInstList.addInst(new AsmInstMove(argReg, asmOperand));
        else if (asmOperand->isStackVariable())
            asmArgInstList.addInst(new AsmInstLoad(argReg, asmOperand));
        else
            Error::Error(__PRETTY_FUNCTION__, "Unknown argument type");
    }
}
/**
 * @brief Allocate registers for the instructions
 * @details use register allocator to allocate registers for the instructions
 */
void AsmFunction::allocateRegisters()
{
    RegisterControl *rc = new GraphColoringAllocator(this, getStackAllocator());
    // RegisterControl *rc = new LinearScanRegisterControl(this, getStackAllocator());
    
    AsmInstBasicList newInstList = rc->work(instList);

    for (auto inst : newInstList.getList())
    {
        for (int i = 0; i < 3; i++)
        {
            AsmOperandBasic *operand = inst->getOperand(i);
            if (operand && operand->isEXStackVarAdd())
            {
                ExStackVarAdd *exStackVarAdd = dynamic_cast<ExStackVarAdd *>(operand);
                inst->setOperand(i, exStackVarAdd->add(-stackAllocator->getRegSizeMax()));
            }
        }
    }

    instList.clear();
    instList.addInst(rc->emitPrologue());
    instList.addInst(newInstList);
    instList.addInst(rc->emitEpilogue());
}

/**
 * @brief Build stack instructions
 * @details use stack allocator to allocate stack variables for the instructions
 */
void AsmFunction::buildStackInsts()
{
    AsmInstBasicList newInstList;
    newInstList.addInst(stackAllocator->emitPrologue());
    newInstList.addInst(instList);
    // newInstList.addInst(new AsmInstLabel(returnLabel));
    newInstList.addInst(stackAllocator->emitEpilogue());
    instList = newInstList;
}

/**
 * @brief merge offset and baseaddress of a stack variable, gen relavanet instructions
 */
ExStackVarContent *AsmFunction::transformStackVar(AsmOperandStackVariable *src, AsmInstBasicList &instList)
{
    AsmOperandMemoryAddress *stackVarAddress = src->getAddress();

    AsmOperandRegisterInt *offsetReg = registerAllocator->allocateIntRegister();
    ExStackVarOffset *offset = ExStackVarOffset::transform(stackVarAddress->getOffset());
    instList.addInst(new AsmInstLoad(offsetReg, offset));

    AsmOperandRegisterInt *destReg = registerAllocator->allocateIntRegister();
    instList.addInst(new AsmInstAdd(destReg, stackVarAddress->getRegister(), offsetReg, 64));

    AsmOperandMemoryAddress *newAddress = new AsmOperandMemoryAddress(destReg, 0);

    return ExStackVarContent::transform(src, newAddress);
}

ExStackVarContent *AsmFunction::transformStackVar(AsmOperandStackVariable *src)
{
    return transformStackVar(src, instList);
}

/**
 * @brief transform stack variables if its offset is bigger than 12 bits
 */
AsmOperandStackVariable *AsmFunction::getOperableStackVariable(AsmOperandStackVariable *src, AsmInstBasicList &instList)
{
    if (ImmediateWithin12Bits(src->getAddress()->getOffset()))
        return src;
    else
        return transformStackVar(src, instList);
}

/**
 * @brief get the AsmOperand of IR formal parameter
 */
AsmOperandBasic *AsmFunction::getFormalParamAsmOperand(IR::Value *irValue, AsmInstBasicList &instList)
{
    if (asmArgMap.find(irValue) == asmArgMap.end())
        Error::Error(__PRETTY_FUNCTION__, "Formal parameter not found, value: " + irValue->getIRName());

    AsmOperandBasic *asmOperand = asmArgMap[irValue];
    if (asmOperand->isStackVariable())
        // is stack variable (need to transform when immediate value out of range)
        return getOperableStackVariable(dynamic_cast<AsmOperandStackVariable *>(asmOperand)->flip(), instList);
    else
        // is physical parameter register
        return asmOperand;
}

/**
 * @brief get the AsmOperand of IR operand
 * @details if the operand is a memory address, return the register containing the address;
 *  if the operand is a register, return the register;
 *  otherwise, load the operand to a register and return the register
 */
AsmOperandRegisterInt *AsmFunction::getOperandToIntRegister(AsmOperandBasic *operand)
{
    if (operand->isRegisterInt())
        return dynamic_cast<AsmOperandRegisterInt *>(operand);
    else if (operand->isMemoryAddress())
        return getAddressToIntRegister(dynamic_cast<AsmOperandMemoryAddress *>(operand));
    else
    {
        AsmOperandRegisterInt *reg = registerAllocator->allocateIntRegister();
        addInst(new AsmInstLoad(reg, operand));
        return reg;
    }
}

/**
 * @brief get the AsmOperand of IR memory address
 * @details if the offset is 0, return the base register;
 * otherwise, load the offset to a register and add the offset to the base register
 */
AsmOperandRegisterInt *AsmFunction::getAddressToIntRegister(AsmOperandMemoryAddress *address)
{
    if (address->getOffset() == 0)
        return address->getBaseRegister();
    else
    {
        int offset = address->getOffset();
        AsmOperandRegisterInt *dstReg = registerAllocator->allocateIntRegister();
        if (ImmediateWithin12Bits(offset))
            addInst(new AsmInstAdd(dstReg, address->getBaseRegister(), new AsmOperandImmediate(offset), 64));
        else
        {
            AsmOperandRegisterInt *offsetReg = registerAllocator->allocateIntRegister();
            addInst(new AsmInstLoad(offsetReg, new AsmOperandImmediate(offset)));
            addInst(new AsmInstAdd(dstReg, address->getBaseRegister(), offsetReg, 64));
        }
        return dstReg;
    }
}

/**
 * @brief get the AsmOperand of IR operand
 * @details if the operand is a memory address, return the register containing the address;
 * if the operand is a register, return the register;
 */
AsmOperandRegisterFloat *AsmFunction::getOperandToFloatRegister(AsmOperandBasic *operand)
{
    if (operand->isRegisterFloat())
        return dynamic_cast<AsmOperandRegisterFloat *>(operand);
    else
    {
        AsmOperandRegisterFloat *reg = registerAllocator->allocateFloatRegister();
        addInst(new AsmInstLoad(reg, operand));
        return reg;
    }
}

/**
 * @brief get the address of the stack variable, so that the stack variable can be used as a memory address
 */
AsmOperandMemoryAddress *AsmFunction::transformStackVarToAddress(AsmOperandStackVariable *src)
{
    AsmOperandRegisterInt *destReg = registerAllocator->allocateIntRegister();
    AsmOperandMemoryAddress *stackVarAddress = src->getAddress();
    AsmOperandRegisterInt *offsetReg = registerAllocator->allocateIntRegister();

    addInst(new AsmInstLoad(offsetReg, ExStackVarOffset::transform(stackVarAddress->getOffset())));
    addInst(new AsmInstAdd(destReg, offsetReg, stackVarAddress->getRegister(), 64));

    return stackVarAddress->withBaseRegister(destReg)->setOffset(0)->getAddress();
}

AsmOperandMemoryAddress *AsmFunction::getOperandToAddress(AsmOperandBasic *operand)
{
    if (operand->isMemoryAddress())
    {
        AsmOperandMemoryAddress *address = dynamic_cast<AsmOperandMemoryAddress *>(operand);
        int offset = address->getOffset();
        if (ImmediateWithin12Bits(offset))
            return address;
        else
        {
            AsmOperandRegisterInt *baseReg = getAddressToIntRegister(address);
            return new AsmOperandMemoryAddress(baseReg, 0);
        }
    }
    else if (operand->isStackVariable())
    {
        if (operand->isExStackVarContent())
            return dynamic_cast<ExStackVarContent *>(operand)->getAddress()->getAddress();
        return transformStackVarToAddress(dynamic_cast<AsmOperandStackVariable *>(operand));
    }
    else if (operand->isRegisterInt())
    {
        AsmOperandRegisterInt *reg = dynamic_cast<AsmOperandRegisterInt *>(operand);
        return new AsmOperandMemoryAddress(reg, 0);
    }
    else
    {
        Error::Error(__PRETTY_FUNCTION__, "Unknown operand type");
        return nullptr;
    }
}

std::vector<AsmOperandRegister *> AsmFunction::getArgRegs()
{
    std::vector<AsmOperandRegister *> argRegs;
    for (auto arg : asmArgList)
        if (arg->isRegister())
            argRegs.push_back(dynamic_cast<AsmOperandRegister *>(arg));
    return argRegs;
}

AsmInstBasicList AsmFunction::call(AsmFunction *callee, std::vector<AsmOperandBasic *> args, AsmOperandRegister *returnReg)
{
    if (args.size() != callee->asmArgList.size())
        Error::Error(__PRETTY_FUNCTION__, "Argument number mismatch");

    stackAllocator->prepareForFunctionCall();

    AsmInstBasicList newInstList;
    for (int i = 0; i < (int)args.size(); i++)
    {
        AsmOperandBasic *formalArgOperand = callee->asmArgList[i];
        if (formalArgOperand->isStackVariable())
        {
            // transform the stack variable to merge its offset and base address
            formalArgOperand = transformStackVar(dynamic_cast<AsmOperandStackVariable *>(formalArgOperand), newInstList);

            if (!formalArgOperand->isExStackVarContent())
                Error::Error(__PRETTY_FUNCTION__, "Formal argument is not ExStackVarContent");
            // allocate stack space for the argument to be passed
            stackAllocator->pushBack(dynamic_cast<AsmOperandStackVariable *>(formalArgOperand));
        }

        AsmOperandBasic *realArgOperand = args[i];
        if (realArgOperand->isRegister())
        {
            if (formalArgOperand->isRegister())
                newInstList.addInst(new AsmInstMove(dynamic_cast<AsmOperandRegister *>(formalArgOperand), realArgOperand));
            else
                newInstList.addInst(new AsmInstStore(dynamic_cast<AsmOperandRegister *>(realArgOperand), dynamic_cast<AsmOperandStackVariable *>(formalArgOperand)));
        }
        else
        {
            if (formalArgOperand->isRegister())
                newInstList.addInst(new AsmInstLoad(dynamic_cast<AsmOperandRegister *>(formalArgOperand), realArgOperand));
            else
            {
                AsmOperandRegisterInt *reg = registerAllocator->allocateIntRegister();
                newInstList.addInst(new AsmInstLoad(reg, realArgOperand));
                newInstList.addInst(new AsmInstStore(reg, dynamic_cast<AsmOperandStackVariable *>(formalArgOperand)));
            }
        }
    }

    newInstList.addInst(new AsmInstCall(
        new AsmOperandLabel(callee->getLabelName(), true),
        callee->getArgRegs(),
        callee->getReturnReg()));

    if (returnReg != nullptr)
        newInstList.addInst(new AsmInstMove(returnReg, callee->getReturnReg()));

    return newInstList;
}

AsmOperandRegister *AsmFunction::getValueToRegister(IR::Value *value)
{
    AsmOperandBasic *op = getValueByType(value, value->getType());
    AsmOperandRegister *dstReg;

    if (op->isRegister())
    {
        AsmOperandRegister *reg = dynamic_cast<AsmOperandRegister *>(op);
        dstReg = registerAllocator->allocateRegister(reg);
        addInst(new AsmInstMove(dstReg, reg));
    }
    else
    {
        if (value->getType()->isFloat())
        {
            AsmOperandRegisterFloat *reg = registerAllocator->allocateFloatRegister();
            addInst(new AsmInstLoad(reg, op));
            dstReg = reg;
        }
        else
        {
            AsmOperandRegisterInt *reg = registerAllocator->allocateIntRegister();
            addInst(new AsmInstLoad(reg, op));
            dstReg = reg;
        }
    }
    return dstReg;
}

std::vector<AsmInstBasic *> AsmFunction::getInsts() { return instList.getList(); }

void AsmFunction::setInsts(std::vector<AsmInstBasic *> insts) { instList.setList(insts); }