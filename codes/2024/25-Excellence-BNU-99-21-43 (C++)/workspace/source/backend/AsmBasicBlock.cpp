#include "AsmBasicBlock.h"
#include "AsmFunction.h"
#include "AsmCode.h"
#include "AsmOperandHeaders.h"
#include "ConstantMultiplyPlanner.h"
#include "AsmUtility.h"

using namespace Backend;

void AsmBasicBlock::genBB()
{
    // std::cerr << "\n\n>>>>>>>>>> genBB : " << irBlock->getIRName() << std::endl;

    asmFunction->addInst(blockInstLabel);
    for (auto inst : irBlock->getVectorInstructions())
        genInst(inst);
    asmFunction->addInst(new AsmInstBlockEnd());
}

void AsmBasicBlock::genInst(IR::Instruction *inst)
{
    // std::cerr << "              > genInst: ";
    // inst->emitIR(std::cerr);

    int opcode = inst->getOpcode();
    if (opcode >= IR::Instruction::BinaryBegin && opcode < IR::Instruction::BinaryEnd)
        genInstBinary(static_cast<IR::BinaryInstruction *>(inst));
    else if (opcode >= IR::Instruction::UnaryBegin && opcode < IR::Instruction::UnaryEnd)
        genInstUnary(static_cast<IR::UnaryInstruction *>(inst));
    else if (opcode >= IR::Instruction::CastBegin && opcode < IR::Instruction::CastEnd)
        genInstCast(static_cast<IR::CastInstruction *>(inst));
    else if (opcode == IR::Instruction::Alloca)
        genInstAlloca(static_cast<IR::AllocaInstruction *>(inst));
    else if (opcode == IR::Instruction::Load)
        genInstLoad(static_cast<IR::LoadInstruction *>(inst));
    else if (opcode == IR::Instruction::Store)
        genInstStore(static_cast<IR::StoreInstruction *>(inst));
    else if (opcode == IR::Instruction::GEP)
        genInstGetElementPtr(static_cast<IR::GetElementPtrInstruction *>(inst));
    else if (opcode == IR::Instruction::BR)
        genInstBranch(static_cast<IR::BranchInstruction *>(inst));
    else if (opcode == IR::Instruction::Return)
        genInstReturn(static_cast<IR::ReturnInstruction *>(inst));
    else if (opcode == IR::Instruction::Call)
        genInstCall(static_cast<IR::CallInstruction *>(inst));
    else if (opcode == IR::Instruction::Phi)
        genInstPhi(static_cast<IR::PhiInstruction *>(inst));
    else if (opcode >= IR::Instruction::CmpBegin && opcode < IR::Instruction::CmpEnd)
        genInstCmp(static_cast<IR::CmpInstruction *>(inst));
    else
        Error::Error(__PRETTY_FUNCTION__, "Unknown instruction, opcode: " + std::to_string(opcode));
}

void AsmBasicBlock::genInstBinary(IR::BinaryInstruction *inst)
{
    switch (inst->getOpcode())
    {
    case IR::Instruction::Add:
        genInstBinary_IntAdd(inst);
        break;
    case IR::Instruction::Sub:
        genInstBinary_IntSub(inst);
        break;
    case IR::Instruction::Mul:
        genInstBinary_IntMul(inst);
        break;
    case IR::Instruction::Div:
        genInstBinary_IntDiv(inst);
        break;
    case IR::Instruction::Rem:
        genInstBinary_IntRem(inst);
        break;
    case IR::Instruction::Xor:
        genInstBinary_IntXor(inst);
        break;
    case IR::Instruction::Or:
        genInstBinary_IntOr(inst);
        break;
    case IR::Instruction::And:
        genInstBinary_IntAnd(inst);
        break;
    case IR::Instruction::FAdd:
        genInstBinary_FPAdd(inst);
        break;
    case IR::Instruction::FSub:
        genInstBinary_FPSub(inst);
        break;
    case IR::Instruction::FMul:
        genInstBinary_FPMul(inst);
        break;
    case IR::Instruction::FDiv:
        genInstBinary_FPDiv(inst);
        break;
    default:
        Error::Error(__PRETTY_FUNCTION__, "Unknown binary instruction");
    }
}

void AsmBasicBlock::genInstBinary_IntAdd(IR::BinaryInstruction *inst) // opt done
{
    int bitLength = 32;
    AsmOperandRegisterInt *rhs1 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandBasic *rhs2 = asmFunction->getAsmOperand(inst->getOperand(1));
    AsmOperandRegisterInt *lhs = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    asmFunction->addInst(new AsmInstAdd(lhs, rhs1, rhs2, bitLength)); // NOTE: 32 is the bit length maybe need to be changed (and the following methods)
}

void AsmBasicBlock::genInstBinary_IntSub(IR::BinaryInstruction *inst) // opt done
{
    int bitLength = 32;
    AsmOperandRegisterInt *rhs1 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandBasic *rhs2 = asmFunction->getAsmOperand(inst->getOperand(1));
    AsmOperandRegisterInt *lhs = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    if (rhs2->isImmediate()) // NOTE: is it possible that rhs2 is out of 12 bits?
        asmFunction->addInst(new AsmInstAdd(lhs, rhs1, new AsmOperandImmediate(-dynamic_cast<AsmOperandImmediate *>(rhs2)->getValue()), 64));
    else
        asmFunction->addInst(new AsmInstSub(lhs, rhs1, dynamic_cast<AsmOperandRegisterInt *>(rhs2), bitLength));
}

void AsmBasicBlock::genInstBinary_IntMul(IR::BinaryInstruction *inst) // opt done
{
    IR::Value *rhs1Value = inst->getOperand(0);
    IR::Value *rhs2Value = inst->getOperand(1);

    if (false && (rhs1Value->isConstantInt32() || rhs2Value->isConstantInt32()))
    { // try to optimize the mul instruction when one of the operands is a constant
        AsmOperandRegisterInt *opRegister;
        long long opConstant;
        if (rhs1Value->isConstantInt32())
        {
            opRegister = dynamic_cast<AsmOperandRegisterInt *>(asmFunction->getValueToRegister(rhs2Value));
            opConstant = dynamic_cast<IR::ConstantInt32 *>(rhs1Value)->getValue();
        }
        else
        {
            opRegister = dynamic_cast<AsmOperandRegisterInt *>(asmFunction->getValueToRegister(rhs1Value));
            opConstant = dynamic_cast<IR::ConstantInt32 *>(rhs2Value)->getValue();
        }

        // try to use the constant multiply planner to optimize the mul instruction,
        //   replace the mul instruction with a series of add, sub, shl instructions
        MulPlannerOperand *plan = ConstantMultiplyPlanner::makePlan(opConstant);
        if (plan != MulPlannerNotReducible::getInstance())
        {
            AsmOperandBasic *planReg = genConstantMultiplyPlan(opRegister, plan, inst->getType()->getSize() * 8);
            AsmOperandRegisterInt *lhs = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
            if (planReg->isRegister())
                asmFunction->addInst(new AsmInstMove(lhs, planReg));
            else if (planReg->isImmediate())
                asmFunction->addInst(new AsmInstLoad(lhs, planReg));
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid plan operand type");
            return;
        }
    }
    // normal mul instruction
    int bitLength = 32;
    AsmOperandRegisterInt *rhs1 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(rhs1Value));
    AsmOperandRegisterInt *rhs2 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(rhs2Value));
    AsmOperandRegisterInt *lhs = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    asmFunction->addInst(new AsmInstMul(lhs, rhs1, rhs2, bitLength));
}

AsmOperandBasic *AsmBasicBlock::genConstantMultiplyPlan(AsmOperandRegisterInt *opRegister, MulPlannerOperand *plan, int bitLength)
{
    if (dynamic_cast<MulPlannerVariableOperand *>(plan))
        return opRegister;
    else if (dynamic_cast<MulPlannerConstantOperand *>(plan))
        return asmFunction->getAsmOperand(IR::ConstantInt32::get(dynamic_cast<MulPlannerConstantOperand *>(plan)->value));
    else if (dynamic_cast<MulPlannerOperation *>(plan))
    {
        // parse recursive operations
        MulPlannerOperation *operation = dynamic_cast<MulPlannerOperation *>(plan);

        AsmOperandRegisterInt *reg1 = asmFunction->getOperandToIntRegister(genConstantMultiplyPlan(opRegister, operation->operand1, bitLength));
        AsmOperandBasic *operand2 = genConstantMultiplyPlan(opRegister, operation->operand2, bitLength);
        AsmOperandRegisterInt *dstReg = asmFunction->getRegisterAllocator()->allocateIntRegister();
        AsmInstBasic *inst = nullptr;
        switch (operation->type)
        {
        case MulPlannerOperationType::ADD:
            inst = new AsmInstAdd(dstReg, reg1, operand2, bitLength);
            break;
        case MulPlannerOperationType::SUB:
            inst = new AsmInstSub(dstReg, reg1, dynamic_cast<AsmOperandRegisterInt *>(operand2), bitLength);
            break;
        case MulPlannerOperationType::SHL:
            inst = new AsmInstShiftLeft(dstReg, reg1, operand2, bitLength);
            break;
        default:
            Error::Error(__PRETTY_FUNCTION__, "Unknown operation type");
            break;
        }

        asmFunction->addInst(inst);
        return dstReg;
    }
    else
    {
        Error::Error(__PRETTY_FUNCTION__, "Unknown plan type");
        return nullptr;
    }
}

void AsmBasicBlock::genInstBinary_IntDiv(IR::BinaryInstruction *inst) // opt done
{
    // IR::Value *rhs1Value = inst->getOperand(0);
    IR::Value *rhs2Value = inst->getOperand(1);

    if (rhs2Value->isConstantInt32())
    {
        genInstSignedConstIntDivide(inst, dynamic_cast<IR::ConstantInt32 *>(rhs2Value));
        return;
    }
    int bitLength = 32;
    AsmOperandRegisterInt *rhs1 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterInt *rhs2 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterInt *lhs = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    asmFunction->addInst(new AsmInstSignedIntDivide(lhs, rhs1, rhs2, bitLength));
}

void AsmBasicBlock::genInstSignedConstIntDivide(IR::BinaryInstruction *inst, IR::ConstantInt32 *constDivisor)
{
    // 获取二元指令的两个操作数
    IR::Value *rhs1Value = inst->getOperand(0);
    // IR::Value *rhs2Value = inst->getOperand(1);

    // 获取除数的绝对值
    int divisor = std::abs(constDivisor->getValue());
    AsmOperandRegisterInt *regAns;

    // 如果除数为负，分配一个额外的寄存器来保存最终答案
    if (constDivisor->getValue() < 0)
    {
        regAns = asmFunction->getRegisterAllocator()->allocateIntRegister();
    }
    else
    {
        regAns = static_cast<AsmOperandRegisterInt *>(asmFunction->getRegisterAllocator()->allocateRegister(inst));
    }

    // 判断除数是否为2的幂
    if (isPowerOf2(divisor))
    {
        int x = log2Floor(divisor);

        // 如果 x 为 0，直接加载寄存器值
        if (x == 0)
        {
            auto tmp = asmFunction->getValueToRegister(rhs1Value);
            auto result = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
            asmFunction->addInst(new AsmInstLoad(result, tmp));
        }
        else if (x >= 1 && x <= 30)
        {
            // 除数为2的幂时的处理
            // %1 = srli %src, #(64-x)
            // %2 = addw %src, %1
            // %ans = sraiw %2, #x
            auto src = static_cast<AsmOperandRegisterInt *>(asmFunction->getValueToRegister(rhs1Value));
            auto reg1 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            auto imm64SubX = asmFunction->getAsmOperand(IR::ConstantInt32::get(64 - x));
            asmFunction->addInst(new AsmInstShiftRightLogical(reg1, src, imm64SubX, 64));

            auto reg2 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            asmFunction->addInst(new AsmInstAdd(reg2, src, reg1, 32));

            auto immX = asmFunction->getAsmOperand(IR::ConstantInt32::get(x));
            asmFunction->addInst(new AsmInstShiftRightArithmetic(regAns, reg2, immX, 32));
        }
    }
    else
    {
        // 对于非2的幂的除数进行处理
        int l = log2Floor(divisor);
        int sh = l;

        // 使用 __int128 进行高精度整数运算
        __int128 temp = 1;
        __int128 low = (temp << (32 + l)) / divisor;
        __int128 high = ((temp << (32 + l)) + (temp << (l + 1))) / divisor;

        // 调整 low 和 high 的值以便于位移操作
        while (((low / 2) < (high / 2)) && sh > 0)
        {
            low /= 2;
            high /= 2;
            sh--;
        }

        if (high < (1L << 31))
        {
            // 对于 high 较小的情况
            // %1 = mul %src, #high
            // %2 = srai %1, #(32+sh)
            // %3 = sraiw %src, #31
            // %ans = subw %2, %3
            auto src = static_cast<AsmOperandRegisterInt *>(asmFunction->getValueToRegister(rhs1Value));
            auto reg1 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            auto immHigh = asmFunction->getAsmOperand(IR::ConstantInt32::get(high)); // NOTE: 可能需要使用 long long
            asmFunction->addInst(new AsmInstMul(reg1, src, asmFunction->getOperandToIntRegister(immHigh), 64));

            auto reg2 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            auto imm32PlusSh = asmFunction->getAsmOperand(IR::ConstantInt32::get(32 + sh));
            asmFunction->addInst(new AsmInstShiftRightArithmetic(reg2, reg1, imm32PlusSh, 64));

            auto reg3 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            auto imm31 = asmFunction->getAsmOperand(IR::ConstantInt32::get(31));
            asmFunction->addInst(new AsmInstShiftRightArithmetic(reg3, src, imm31, 32));

            asmFunction->addInst(new AsmInstSub(regAns, reg2, reg3, 32));
        }
        else
        {
            // 对于 high 较大的情况
            high = high - (1L << 32);
            // %1 = mul %src, #high
            // %2 = srai %1, #32
            // %3 = addw %2, %src
            // %4 = sariw %3, #sh
            // %5 = sariw %src, #31
            // %ans = subw %4, %5
            auto reg1 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            auto src = static_cast<AsmOperandRegisterInt *>(asmFunction->getValueToRegister(rhs1Value));
            auto immHigh = asmFunction->getAsmOperand(IR::ConstantInt32::get(high)); // NOTE 可能需要使用 long long
            asmFunction->addInst(new AsmInstMul(reg1, src, asmFunction->getOperandToIntRegister(immHigh), 64));

            auto reg2 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            auto imm32 = asmFunction->getAsmOperand(IR::ConstantInt32::get(32));
            asmFunction->addInst(new AsmInstShiftRightArithmetic(reg2, reg1, imm32, 64));

            auto reg3 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            asmFunction->addInst(new AsmInstAdd(reg3, reg2, src, 32));

            auto reg4 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            auto immSh = asmFunction->getAsmOperand(IR::ConstantInt32::get(sh));
            asmFunction->addInst(new AsmInstShiftRightArithmetic(reg4, reg3, immSh, 32));

            auto reg5 = asmFunction->getRegisterAllocator()->allocateIntRegister();
            auto imm31 = asmFunction->getAsmOperand(IR::ConstantInt32::get(31));
            asmFunction->addInst(new AsmInstShiftRightArithmetic(reg5, src, imm31, 32));

            asmFunction->addInst(new AsmInstSub(regAns, reg4, reg5, 32));
        }
    }

    // 如果除数为负，进行最终的结果修正
    if (constDivisor->getValue() < 0)
    {
        auto regActualAns = static_cast<AsmOperandRegisterInt *>(asmFunction->getRegisterAllocator()->allocateRegister(inst));
        auto imm0 = asmFunction->getAsmOperand(IR::ConstantInt32::get(0));
        asmFunction->addInst(new AsmInstSub(regActualAns, asmFunction->getOperandToIntRegister(imm0), regAns, 32));
    }
}

void AsmBasicBlock::genInstBinary_IntRem(IR::BinaryInstruction *inst)
{
    if (inst->getOperand(1)->isConstantInt32())
    {
        IR::Value *rhs1Value = inst->getOperand(0);
        IR::Value *rhs2Value = inst->getOperand(1);

        IR::Instruction *irDiv = IR::BinaryInstruction::createDiv(IR::BasicType::getInt32Type(), rhs1Value, rhs2Value);
        IR::Instruction *irMul = IR::BinaryInstruction::createMul(IR::BasicType::getInt32Type(), irDiv, rhs2Value);
        IR::Instruction *irSub = IR::BinaryInstruction::createSub(IR::BasicType::getInt32Type(), rhs1Value, irMul);
        genInst(irDiv);
        genInst(irMul);
        genInst(irSub);
        AsmOperandRegisterInt *tmpReg = dynamic_cast<AsmOperandRegisterInt *>(asmFunction->getRegisterAllocator()->get(irSub));
        AsmOperandRegisterInt *tarReg = dynamic_cast<AsmOperandRegisterInt *>(asmFunction->getRegisterAllocator()->allocateRegister(inst));
        asmFunction->addInst(new AsmInstMove(tarReg, tmpReg));
    }
    else
    {
        int bitLength = 32;
        AsmOperandRegisterInt *rhs1 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
        AsmOperandRegisterInt *rhs2 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
        AsmOperandRegisterInt *lhs = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
        asmFunction->addInst(new AsmInstSignedIntRemainder(lhs, rhs1, rhs2, bitLength));
    }
}

void AsmBasicBlock::genInstBinary_IntXor(IR::BinaryInstruction *inst)
{
    // TODO:CHECK there is not a xor instruction in examples
    AsmOperandRegisterInt *rhs1 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterInt *rhs2 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterInt *lhs = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    asmFunction->addInst(new AsmInstXor(lhs, rhs1, rhs2, 64));
}

void AsmBasicBlock::genInstBinary_IntOr(IR::BinaryInstruction *inst)
{
    // TODO:CHECK there is not a or instruction in examples
    AsmOperandRegisterInt *rhs1 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterInt *rhs2 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterInt *lhs = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    asmFunction->addInst(new AsmInstOr(lhs, rhs1, rhs2, 64));
}

void AsmBasicBlock::genInstBinary_IntAnd(IR::BinaryInstruction *inst)
{
    // TODO:CHECK there is not a and instruction in examples
    AsmOperandRegisterInt *rhs1 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterInt *rhs2 = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterInt *lhs = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    asmFunction->addInst(new AsmInstAnd(lhs, rhs1, rhs2));
}

void AsmBasicBlock::genInstBinary_FPAdd(IR::BinaryInstruction *inst)
{
    AsmOperandRegisterFloat *rhs1 = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterFloat *rhs2 = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterFloat *lhs = asmFunction->getRegisterAllocator()->allocateFloatRegister(inst);
    asmFunction->addInst(new AsmInstAdd(lhs, rhs1, rhs2));
}

void AsmBasicBlock::genInstBinary_FPSub(IR::BinaryInstruction *inst)
{
    AsmOperandRegisterFloat *rhs1 = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterFloat *rhs2 = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterFloat *lhs = asmFunction->getRegisterAllocator()->allocateFloatRegister(inst);
    asmFunction->addInst(new AsmInstSub(lhs, rhs1, rhs2));
}

void AsmBasicBlock::genInstBinary_FPMul(IR::BinaryInstruction *inst)
{
    AsmOperandRegisterFloat *rhs1 = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterFloat *rhs2 = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterFloat *lhs = asmFunction->getRegisterAllocator()->allocateFloatRegister(inst);
    asmFunction->addInst(new AsmInstMul(lhs, rhs1, rhs2));
}

void AsmBasicBlock::genInstBinary_FPDiv(IR::BinaryInstruction *inst)
{
    AsmOperandRegisterFloat *rhs1 = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterFloat *rhs2 = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterFloat *lhs = asmFunction->getRegisterAllocator()->allocateFloatRegister(inst);
    asmFunction->addInst(new AsmInstFloatDivide(lhs, rhs1, rhs2));
}

void AsmBasicBlock::genInstAlloca(IR::AllocaInstruction *inst)
{
    IR::BasicType *allocType = inst->getType()->getBaseType();
    int allocSize = allocType->getSize();
    allocSize = (allocSize + 3) / 4 * 4; // align to 4 bytes, NOTE: maybe 8 is needed
    asmFunction->getStackAllocator()->allocate(inst, allocSize);
}

void AsmBasicBlock::genInstLoad(IR::LoadInstruction *inst)
{
    AsmOperandBasic *srcOperand = asmFunction->getAsmOperand(inst->getOperand(0));
    AsmOperandRegister *dstReg = asmFunction->getRegisterAllocator()->allocateRegister(inst);

    if (!inst->getOperand(0)->getType()->isPoint())
        Error::Error(__PRETTY_FUNCTION__, "Operand must be a pointer type");
    IR::PointerType *srcPointerType = dynamic_cast<IR::PointerType *>(inst->getOperand(0)->getType());
    IR::BasicType *srcElementType = srcPointerType->getBaseType();

    if (srcElementType->isInt32())
    {
        if (srcOperand->isImmediate())
            asmFunction->addInst(new AsmInstLoad(dstReg, srcOperand));
        else if (srcOperand->isRegister())
            asmFunction->addInst(new AsmInstMove(dstReg, srcOperand));
        else if (srcOperand->isMemoryAddress())
            asmFunction->addInst(new AsmInstLoad(dstReg, dynamic_cast<AsmOperandMemoryAddress *>(srcOperand), 32));
        else if (srcOperand->isExStackVarContent())
        {
            AsmOperandMemoryAddress *address = asmFunction->getOperandToAddress(srcOperand);
            asmFunction->addInst(new AsmInstLoad(dstReg, address, 32));
        }
        else
            Error::Error(__PRETTY_FUNCTION__, "Invalid source operand type");
    }
    else if (srcElementType->isPoint())
    {
        if (srcOperand->isImmediate())
            asmFunction->addInst(new AsmInstLoad(dstReg, srcOperand));
        else if (srcOperand->isRegister())
            asmFunction->addInst(new AsmInstMove(dstReg, srcOperand));
        else if (srcOperand->isMemoryAddress())
            asmFunction->addInst(new AsmInstLoad(dstReg, dynamic_cast<AsmOperandMemoryAddress *>(srcOperand), 64));
        else if (srcOperand->isExStackVarContent())
        {
            AsmOperandMemoryAddress *address = asmFunction->getOperandToAddress(srcOperand);
            asmFunction->addInst(new AsmInstLoad(dstReg, address, 64));
        }
        else
            Error::Error(__PRETTY_FUNCTION__, "Invalid source operand type");
    }
    else
    {
        if (srcOperand->isImmediate())
            asmFunction->addInst(new AsmInstLoad(dstReg, srcOperand));
        else if (srcOperand->isRegister())
            asmFunction->addInst(new AsmInstMove(dstReg, srcOperand));
        else if (srcOperand->isMemoryAddress())
            asmFunction->addInst(new AsmInstLoad(dstReg, dynamic_cast<AsmOperandMemoryAddress *>(srcOperand), 32));
        else if (srcOperand->isExStackVarContent())
        {
            AsmOperandMemoryAddress *address = asmFunction->getOperandToAddress(srcOperand);
            asmFunction->addInst(new AsmInstLoad(dstReg, address, 32));
        }
        else
            Error::Error(__PRETTY_FUNCTION__, "Invalid source operand type");
    }
}

void AsmBasicBlock::genInstStore(IR::StoreInstruction *inst)
{
    IR::Value *srcValue = inst->getOperand(0);
    AsmOperandBasic *dstOperand = asmFunction->getAsmOperand(inst->getOperand(1));

    // NOTE: const array exits in example, but we think it is not necessary(abanonded)
    if (srcValue->isConstantArray())
    {
        Error::Error(__PRETTY_FUNCTION__, "Constant array not implemented");
    }
    else
    {
        AsmOperandBasic *srcOperand = asmFunction->getAsmOperand(srcValue);
        int bitLength = 32;

        // if (srcOperand->isMemoryAddress() && srcValue->getType()->isPoint())
        // {
        //     AsmOperandRegisterInt *srcReg = dynamic_cast<AsmOperandMemoryAddress *>(srcOperand)->getBaseRegister();
        //     if (dstOperand->isRegister())
        //         asmFunction->addInst(new AsmInstLoad(dynamic_cast<AsmOperandRegister *>(dstOperand), dynamic_cast<AsmOperandRegister *>(srcReg)));
        //     else if (dstOperand->isMemoryAddress())
        //         asmFunction->addInst(new AsmInstStore(dynamic_cast<AsmOperandRegister *>(srcReg), dynamic_cast<AsmOperandMemoryAddress *>(dstOperand), bitLength));
        //     else if (dstOperand->isStackVariable())
        //         asmFunction->addInst(new AsmInstStore(dynamic_cast<AsmOperandRegister *>(srcReg), dynamic_cast<AsmOperandStackVariable *>(dstOperand)));
        //     else
        //         Error::Error(__PRETTY_FUNCTION__, "Invalid destination operand type");
        // }
        // else
        if (srcOperand->isRegister())
        { // the source operand is a register
            if (dstOperand->isRegister())
                asmFunction->addInst(new AsmInstLoad(dynamic_cast<AsmOperandRegister *>(dstOperand), dynamic_cast<AsmOperandRegister *>(srcOperand)));
            else if (dstOperand->isMemoryAddress())
                asmFunction->addInst(new AsmInstStore(dynamic_cast<AsmOperandRegister *>(srcOperand), dynamic_cast<AsmOperandMemoryAddress *>(dstOperand), bitLength));
            else if (dstOperand->isStackVariable())
                asmFunction->addInst(new AsmInstStore(dynamic_cast<AsmOperandRegister *>(srcOperand), dynamic_cast<AsmOperandStackVariable *>(dstOperand)));
            else
                Error::Error(__PRETTY_FUNCTION__, "Invalid destination operand type");
        }
        else
        { // the source operand is not a register
            AsmOperandRegister *srcReg = asmFunction->getRegisterAllocator()->allocateRegister(srcValue->getType());
            if (srcOperand->isMemoryAddress())
            { // the source operand is a memory address
                asmFunction->addInst(new AsmInstLoad(srcReg, dynamic_cast<AsmOperandMemoryAddress *>(srcOperand), bitLength));
                if (dstOperand->isRegister())
                    asmFunction->addInst(new AsmInstLoad(dynamic_cast<AsmOperandRegister *>(dstOperand), srcReg));
                else if (dstOperand->isMemoryAddress())
                    asmFunction->addInst(new AsmInstStore(srcReg, dynamic_cast<AsmOperandMemoryAddress *>(dstOperand), bitLength));
                else if (dstOperand->isStackVariable())
                    asmFunction->addInst(new AsmInstStore(srcReg, dynamic_cast<AsmOperandStackVariable *>(dstOperand)));
                else
                    Error::Error(__PRETTY_FUNCTION__, "Invalid destination operand type");
            }
            else
            { // the source operand is a immediate value
                asmFunction->addInst(new AsmInstLoad(srcReg, srcOperand));
                if (dstOperand->isRegister())
                    asmFunction->addInst(new AsmInstLoad(dynamic_cast<AsmOperandRegister *>(dstOperand), srcReg));
                else if (dstOperand->isMemoryAddress())
                    asmFunction->addInst(new AsmInstStore(srcReg, dynamic_cast<AsmOperandMemoryAddress *>(dstOperand), bitLength));
                else if (dstOperand->isStackVariable())
                    asmFunction->addInst(new AsmInstStore(srcReg, dynamic_cast<AsmOperandStackVariable *>(dstOperand)));
                else
                    Error::Error(__PRETTY_FUNCTION__, "Invalid destination operand type");
            }
        }
    }
}

void AsmBasicBlock::genInstGetElementPtr(IR::GetElementPtrInstruction *inst)
{
    // get the base address (offset and base register)
    AsmOperandMemoryAddress *baseAddress = asmFunction->getOperandToAddress(asmFunction->getAsmOperand(inst->getOperand(0)));
    int offset = baseAddress->getOffset();
    AsmOperandRegisterInt *baseReg = baseAddress->getBaseRegister();

    // get the index - get the sum of immediate indices first, and then register indices
    std::vector<IR::Value *> indices = inst->getIndices();
    int indexNum = indices.size();

    // 1. get the sum of immediate indices (calculate directly)
    IR::BasicType *baseType = inst->getOperand(0)->getType();
    for (int i = 0; i < indexNum; i++)
    {
        AsmOperandBasic *index = asmFunction->getAsmOperand(indices[i]);
        baseType = baseType->getBaseType();
        if (index->isImmediate())
            offset += dynamic_cast<AsmOperandImmediate *>(index)->getValue() * baseType->getSize();
    }

    // allocate a new register to store the offset
    AsmOperandRegisterInt *offsetReg = asmFunction->getRegisterAllocator()->allocateIntRegister();
    asmFunction->addInst(new AsmInstLoad(offsetReg, new AsmOperandImmediate(offset)));

    // 2. get the sum of register indices (calculate using insts)
    baseType = inst->getOperand(0)->getType();
    for (int i = 0; i < indexNum; i++)
    {
        AsmOperandBasic *index = asmFunction->getAsmOperand(indices[i]);
        baseType = baseType->getBaseType();
        if (!index->isImmediate())
        {
            AsmOperandRegisterInt *tmpIndexReg = asmFunction->getRegisterAllocator()->allocateIntRegister();
            AsmOperandRegisterInt *tmpDeltaReg = asmFunction->getRegisterAllocator()->allocateIntRegister();
            AsmOperandRegisterInt *tmpOffsetReg = asmFunction->getRegisterAllocator()->allocateIntRegister();

            asmFunction->addInst(new AsmInstMove(tmpIndexReg, asmFunction->getOperandToIntRegister(index)));
            AsmOperandRegisterInt *tmpSizeReg = asmFunction->getOperandToIntRegister(new AsmOperandImmediate(baseType->getSize()));

            asmFunction->addInst(new AsmInstMul(tmpDeltaReg, tmpIndexReg, tmpSizeReg, 64));
            asmFunction->addInst(new AsmInstAdd(tmpOffsetReg, offsetReg, tmpDeltaReg, 64));

            offsetReg = tmpOffsetReg;
        }
    }

    // 3. add the offset to the base register, return new memory address (resPtrReg, 0)
    AsmOperandRegisterInt *resPtrReg = asmFunction->getRegisterAllocator()->allocateIntRegister();
    asmFunction->addInst(new AsmInstAdd(resPtrReg, offsetReg, baseReg, 64));
    asmFunction->getAddressAllocator()->allocate(inst, new AsmOperandMemoryAddress(resPtrReg, 0));
}

void AsmBasicBlock::genInstUnary(IR::UnaryInstruction *inst)
{
    switch (inst->getOpcode())
    {
    case IR::Instruction::Neg:
        genInstUnary_Neg(inst);
        break;
    case IR::Instruction::FNeg:
        genInstUnary_FNeg(inst);
        break;
    case IR::Instruction::Not:
        genInstUnary_Not(inst);
        break;
    default:
        Error::Error(__PRETTY_FUNCTION__, "Unknown unary instruction, opcode: " + std::to_string(inst->getOpcode()));
    }
}

void AsmBasicBlock::genInstUnary_Neg(IR::UnaryInstruction *inst)
{
    // TODO:OPT
    // implement neg instruction by sub 0 - src
    AsmOperandRegisterInt *dstReg = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    AsmOperandRegisterInt *srcReg = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterInt *zeroReg = AsmOperandRegisterInt::ZERO;
    asmFunction->addInst(new AsmInstSub(dstReg, zeroReg, srcReg, 64));
}

void AsmBasicBlock::genInstUnary_FNeg(IR::UnaryInstruction *inst)
{
    AsmOperandRegisterFloat *dstReg = asmFunction->getRegisterAllocator()->allocateFloatRegister(inst);
    AsmOperandRegisterFloat *srcReg = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    asmFunction->addInst(new AsmInstFloatNegate(dstReg, srcReg));
}

void AsmBasicBlock::genInstUnary_Not(IR::UnaryInstruction *inst)
{
    // use seqz when the source is int, use fmv.s.x and feq.s when the source is float
    IR::Value *srcValue = inst->getOperand(0);
    if (srcValue->getType()->isInt32())
    {
        AsmOperandRegisterInt *dstReg = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
        AsmOperandRegisterInt *srcReg = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(srcValue));
        asmFunction->addInst(AsmInstIntCompare::createEQZ(dstReg, srcReg));
    }
    else if (srcValue->getType()->isFloat())
    {
        AsmOperandRegisterInt *dstReg = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
        AsmOperandRegisterFloat *srcReg = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(srcValue));
        AsmOperandRegisterFloat *zeroReg = asmFunction->getRegisterAllocator()->allocateFloatRegister();
        asmFunction->addInst(new AsmInstMove(zeroReg, AsmOperandRegisterInt::ZERO));
        asmFunction->addInst(AsmInstFloatCompare::createEQ(dstReg, srcReg, zeroReg));
    }
    else
        Error::Error(__PRETTY_FUNCTION__, "Invalid source operand type");
}

void AsmBasicBlock::genInstCast(IR::CastInstruction *inst)
{
    switch (inst->getOpcode())
    {
    case IR::Instruction::FPtoSI:
        genInstCast_FPtoSI(inst);
        break;
    case IR::Instruction::SItoFP:
        genInstCast_SItoFP(inst);
        break;
    default:
        Error::Error(__PRETTY_FUNCTION__, "Unknown cast instruction");
    }
}

void AsmBasicBlock::genInstCast_FPtoSI(IR::CastInstruction *inst)
{
    IR::Value *srcValue = inst->getOperand(0);
    AsmOperandRegisterFloat *srcReg = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(srcValue));
    AsmOperandRegisterInt *dstReg = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    asmFunction->addInst(new AsmInstConvertFloatInt(dstReg, srcReg));
}

void AsmBasicBlock::genInstCast_SItoFP(IR::CastInstruction *inst)
{
    IR::Value *srcValue = inst->getOperand(0);
    AsmOperandRegisterInt *srcReg = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(srcValue));
    AsmOperandRegisterFloat *dstReg = asmFunction->getRegisterAllocator()->allocateFloatRegister(inst);
    asmFunction->addInst(new AsmInstConvertFloatInt(dstReg, srcReg));
}

void AsmBasicBlock::genInstBranch(IR::BranchInstruction *inst)
{
    sufGenInstsPhi();
    // TODO:OPT Take advantage of a variety of conditional jump instructions, refer to example code
    if (inst->isUnconditional())
    {
        IR::BasicBlock *irUnconditionalBlock = inst->getUnconditionalBlock();
        AsmOperandLabel *unconditionalLabel = asmFunction->getBBLabel(irUnconditionalBlock);
        asmFunction->addInst(AsmInstJump::createUnconditional(unconditionalLabel));
    }
    else
    {
        IR::Value *condition = inst->getCondition();
        IR::BasicBlock *irTrueBlock = inst->getTrueBlock();
        IR::BasicBlock *irFalseBlock = inst->getFalseBlock();
        AsmOperandLabel *trueLabel = asmFunction->getBBLabel(irTrueBlock);
        AsmOperandLabel *falseLabel = asmFunction->getBBLabel(irFalseBlock);

        if (condition->isInstruction() && dynamic_cast<IR::Instruction *>(condition)->getOpcode() > IR::Instruction::ICmp && dynamic_cast<IR::Instruction *>(condition)->getOpcode() < IR::Instruction::FCmp)
        {
            IR::Instruction *icmpInst = dynamic_cast<IR::Instruction *>(condition);
            IR::Value *icmpOp1 = icmpInst->getOperand(0);
            IR::Value *icmpOp2 = icmpInst->getOperand(1);
            IR::Instruction::CmpOp icmpCondition = static_cast<IR::Instruction::CmpOp>(dynamic_cast<IR::ICmpInstruction *>(icmpInst)->getCmpCode());

            if (icmpOp1->isConstantInt32() && dynamic_cast<IR::ConstantInt32 *>(icmpOp1)->getValue() == 0)
                genInstBranchWithZero(icmpOp2, icmpCondition, trueLabel);
            else if (icmpOp2->isConstantInt32() && dynamic_cast<IR::ConstantInt32 *>(icmpOp2)->getValue() == 0)
                genInstBranchWithZero(icmpOp1, icmpCondition, trueLabel);
            else
            {
                AsmOperandRegisterInt *icmpOp1Reg = dynamic_cast<AsmOperandRegisterInt *>(asmFunction->getValueToRegister(icmpOp1));
                AsmOperandRegisterInt *icmpOp2Reg = dynamic_cast<AsmOperandRegisterInt *>(asmFunction->getValueToRegister(icmpOp2));

                AsmInstJump::Condition cond;
                switch (icmpCondition)
                {
                case IR::Instruction::Eq:
                    cond = AsmInstJump::Condition::EQ;
                    break;
                case IR::Instruction::Ne:
                    cond = AsmInstJump::Condition::NE;
                    break;
                case IR::Instruction::Lt:
                    cond = AsmInstJump::Condition::LT;
                    break;
                case IR::Instruction::Le:
                    cond = AsmInstJump::Condition::LE;
                    break;
                case IR::Instruction::Gt:
                    cond = AsmInstJump::Condition::GT;
                    break;
                case IR::Instruction::Ge:
                    cond = AsmInstJump::Condition::GE;
                    break;
                default:
                    cond = AsmInstJump::Condition::DEFAULT;
                    Error::Error(__PRETTY_FUNCTION__, "invalid INT CMP CONDITION");
                    break;
                }

                asmFunction->addInst(AsmInstJump::createBinary(cond, trueLabel, icmpOp1Reg, icmpOp2Reg));
            }
        }
        else
        {
            AsmOperandRegisterInt *conditionReg = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(condition));
            asmFunction->addInst(AsmInstJump::createNEZ(trueLabel, conditionReg));
        }

        asmFunction->addInst(AsmInstJump::createUnconditional(falseLabel));
    }
}

// j[condition]z cmpValue, trueLabel
void AsmBasicBlock::genInstBranchWithZero(IR::Value *cmpValue, IR::Instruction::CmpOp condition, AsmOperandLabel *trueLabel)
{
    AsmOperandRegisterInt *cmpReg = static_cast<AsmOperandRegisterInt *>(asmFunction->getValueToRegister(cmpValue));

    AsmInstJump::Condition cond;
    switch (condition)
    {
    case IR::Instruction::Eq:
        cond = AsmInstJump::Condition::EQZ;
        break;
    case IR::Instruction::Ne:
        cond = AsmInstJump::Condition::NEZ;
        break;
    case IR::Instruction::Lt:
        cond = AsmInstJump::Condition::LTZ;
        break;
    case IR::Instruction::Le:
        cond = AsmInstJump::Condition::LEZ;
        break;
    case IR::Instruction::Gt:
        cond = AsmInstJump::Condition::GTZ;
        break;
    case IR::Instruction::Ge:
        cond = AsmInstJump::Condition::GEZ;
        break;
    default:
        cond = AsmInstJump::Condition::DEFAULT;
        Error::Error(__PRETTY_FUNCTION__, "invalid INT CMP CONDITION");
        break;
    }

    asmFunction->addInst(AsmInstJump::createUnary(cond, trueLabel, cmpReg));
}

void AsmBasicBlock::genInstReturn(IR::ReturnInstruction *inst)
{
    bool isVoid = inst->operands.empty();
    if (isVoid)
    {
    }
    else
    {
        IR::Value *retValue = inst->getOperand(0);
        IR::BasicType *retType = retValue->getType();
        if (retType->isInt32())
        {
            AsmOperandBasic *retOperand = asmFunction->getAsmOperand(retValue);
            if (retOperand->isRegister())
                asmFunction->addInst(new AsmInstMove(asmFunction->getReturnReg(), dynamic_cast<AsmOperandRegisterInt *>(retOperand)));
            else
                asmFunction->addInst(new AsmInstLoad(asmFunction->getReturnReg(), retOperand));
        }
        else if (retType->isFloat())
        {
            AsmOperandRegisterFloat *retReg = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(retValue));
            asmFunction->addInst(new AsmInstMove(asmFunction->getReturnReg(), retReg));
        }
        else
            Error::Error(__PRETTY_FUNCTION__, "Invalid return type");
    }

    // NOTE: why add a jump instruction here? Question
    asmFunction->addInst(AsmInstJump::createUnconditional(asmFunction->getReturnLabel()));
}

void AsmBasicBlock::genInstCall(IR::CallInstruction *inst)
{
    IR::Function *irCallee = dynamic_cast<IR::Function *>(inst->getOperand(0));

    std::string calleeName = irCallee->getIRName();
    // NOTE: ignore @memset temporarily
    // if (calleeName == "@memset")
    //     return;

    AsmFunction *asmCallee = asmFunction->getAsmCode()->getFunction(irCallee);

    std::vector<AsmOperandBasic *> asmRealArgs;
    int argNum = irCallee->args().size();
    for (int i = 0; i < argNum; i++)
    {
        IR::Value *realArgValue = inst->getOperand(i + 1);
        IR::Value *formalArgValue = irCallee->args()[i];
        IR::BasicType *formalArgType = formalArgValue->getType();

        // NOTE: array pointer as argument is within the function getValueByType
        asmRealArgs.push_back(asmFunction->getValueByType(realArgValue, formalArgType));
    }

    AsmOperandRegister *returnReg = nullptr;
    if (asmCallee->getReturnReg() != nullptr)
        returnReg = asmFunction->getRegisterAllocator()->allocateRegister(inst);

    asmFunction->addInst(asmFunction->call(asmCallee, asmRealArgs, returnReg));
}

void AsmBasicBlock::genInstPhi(IR::PhiInstruction *inst)
{
    AsmOperandRegister *src = dynamic_cast<AsmOperandRegister *>(asmFunction->getAsmOperand(inst));
    AsmOperandRegister *dst = dynamic_cast<AsmOperandRegister *>(asmFunction->getRegisterAllocator()->allocateRegister(inst));
    asmFunction->addInst(new AsmInstMove(dst, src));
}

void AsmBasicBlock::genInstCmp(IR::CmpInstruction *inst)
{
    std::vector<IR::User *> users = inst->getUsers();
    if (users.size() == 0)
        return;

    // if the cmp instruction is the only use of the result, and the user is a branch instruction,
    //   no need to generate cmp instruction, will be optimized in the branch instruction
    if (inst->getOpcode() > IR::Instruction::ICmp && inst->getOpcode() < IR::Instruction::FCmp && users.size() == 1)
    {
        IR::User *user = users[0];
        if (user->isInstruction() && dynamic_cast<IR::Instruction *>(user)->getOpcode() == IR::Instruction::BR)
            return;
    }

    if (inst->getOpcode() > IR::Instruction::ICmp && inst->getOpcode() < IR::Instruction::FCmp)
        genInstCmp_ICmp(static_cast<IR::ICmpInstruction *>(inst));
    else if (inst->getOpcode() > IR::Instruction::FCmp && inst->getOpcode() < IR::Instruction::CmpEnd)
        genInstCmp_FCmp(static_cast<IR::FCmpInstruction *>(inst));
    else
        Error::Error(__PRETTY_FUNCTION__, "Unknown cmp instruction, opcode: " + std::to_string(inst->getOpcode()));
}

void AsmBasicBlock::genInstCmp_ICmp(IR::ICmpInstruction *inst)
{
    AsmOperandRegisterInt *lhs = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterInt *rhs = asmFunction->getOperandToIntRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterInt *dst = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);

    int opcode = inst->cmpCode;

    if (opcode == IR::ICmpInstruction::Gt)
    {
        asmFunction->addInst(AsmInstIntCompare::createGT(dst, lhs, rhs));
    }
    else if (opcode == IR::ICmpInstruction::Ge)
    {
        // slt t0, t1, t2
        // xori t0, t0, 1
        // NOTE: maybe need signed extension and truncation
        asmFunction->addInst(AsmInstIntCompare::createLT(dst, lhs, rhs));
        asmFunction->addInst(new AsmInstXor(dst, dst, new AsmOperandImmediate(1), 64));
    }
    else if (opcode == IR::ICmpInstruction::Lt)
    {
        asmFunction->addInst(AsmInstIntCompare::createLT(dst, lhs, rhs));
    }
    else if (opcode == IR::ICmpInstruction::Le)
    {
        // sext.w t1, t1
        // sext.w t2, t2
        // sgt t0, t1, t2
        // xori t0, t0, 1
        // andi	t0,t0,0xff
        // sext.w	t0,t0
        // NOTE: maybe need signed extension and truncation
        asmFunction->addInst(AsmInstIntCompare::createGT(dst, lhs, rhs));
        asmFunction->addInst(new AsmInstXor(dst, dst, new AsmOperandImmediate(1), 64));
    }
    else if (opcode == IR::ICmpInstruction::Eq)
    {
        // sub t0, t1, t2
        // seqz t0, t0
        asmFunction->addInst(new AsmInstSub(dst, lhs, rhs, 64));
        asmFunction->addInst(AsmInstIntCompare::createEQZ(dst, dst));
    }
    else if (opcode == IR::ICmpInstruction::Ne)
    {
        // sub t0, t1, t2
        // snez t0, t0
        asmFunction->addInst(new AsmInstSub(dst, lhs, rhs, 64));
        asmFunction->addInst(AsmInstIntCompare::createNEZ(dst, dst));
    }
    else
        Error::Error(__PRETTY_FUNCTION__, "Unknown icmp instruction, opcode: " + std::to_string(opcode));
}

void AsmBasicBlock::genInstCmp_FCmp(IR::FCmpInstruction *inst)
{
    int opcode = inst->cmpCode;

    AsmOperandRegisterFloat *lhs = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(0)));
    AsmOperandRegisterFloat *rhs = asmFunction->getOperandToFloatRegister(asmFunction->getAsmOperand(inst->getOperand(1)));
    AsmOperandRegisterInt *dst = asmFunction->getRegisterAllocator()->allocateIntRegister(inst);
    if (opcode == IR::FCmpInstruction::FGt)
    {
        asmFunction->addInst(AsmInstFloatCompare::createLT(dst, rhs, lhs));
    }
    else if (opcode == IR::FCmpInstruction::FGe)
    {
        asmFunction->addInst(AsmInstFloatCompare::createLE(dst, rhs, lhs));
    }
    else if (opcode == IR::FCmpInstruction::FLt)
    {
        asmFunction->addInst(AsmInstFloatCompare::createLT(dst, lhs, rhs));
    }
    else if (opcode == IR::FCmpInstruction::FLe)
    {
        asmFunction->addInst(AsmInstFloatCompare::createLE(dst, lhs, rhs));
    }
    else if (opcode == IR::FCmpInstruction::FEq)
    {
        asmFunction->addInst(AsmInstFloatCompare::createEQ(dst, lhs, rhs));
    }
    else if (opcode == IR::FCmpInstruction::FNe)
    {
        asmFunction->addInst(AsmInstFloatCompare::createEQ(dst, lhs, rhs));
        asmFunction->addInst(AsmInstIntCompare::createEQZ(dst, dst));
    }
    else
        Error::Error(__PRETTY_FUNCTION__, "Unknown fcmp instruction, opcode: " + std::to_string(opcode));
}

AsmOperandLabel *AsmBasicBlock::getBlockOperandLabel() { return blockOperandLabel; }

AsmInstLabel *AsmBasicBlock::getBlockInstLabel() { return blockInstLabel; }

IR::BasicBlock *AsmBasicBlock::getIRBB() { return irBlock; }

void AsmBasicBlock::preGenInstsPhi()
{
    std::vector<IR::Instruction *> phiInstList;

    for (auto irInst : irBlock->getVectorInstructions())
    {
        if (irInst->getOpcode() == IR::Instruction::Phi)
            phiInstList.push_back(irInst);
        else
            break;
    }

    for (auto irInst : phiInstList)
    {
        IR::PhiInstruction *phiInst = dynamic_cast<IR::PhiInstruction *>(irInst);
        AsmOperandRegister *tmp = asmFunction->getRegisterAllocator()->allocateRegister(irInst);

        std::vector<PVB> incomingValues = phiInst->getIncomingValue();
        int incomingValuesNum = incomingValues.size();

        for (int i = 0; i < incomingValuesNum; i++)
        {
            auto [incomingValue, incomingBlock] = incomingValues[i];

            if (phiOperation.find(incomingBlock) == phiOperation.end())
            {
                phiOperation[incomingBlock] = AsmInstBasicList();
                phiValueMap[incomingBlock] = std::map<IR::Value *, std::vector<AsmOperandRegister *>>();
            }

            if (incomingValue->isConstantInt32() || incomingValue->isConstantFloat())
            {

                AsmOperandBasic *constantVar = asmFunction->getConstant(incomingValue, phiOperation[incomingBlock]);

                if (constantVar->isRegister())
                {
                    phiOperation[incomingBlock].addInst(new AsmInstMove(tmp, dynamic_cast<AsmOperandRegister *>(constantVar)));
                }
                else
                {
                    phiOperation[incomingBlock].addInst(new AsmInstLoad(tmp, constantVar));
                }
            }
            else
            {
                if (phiValueMap[incomingBlock].find(incomingValue) == phiValueMap[incomingBlock].end())
                {
                    phiValueMap[incomingBlock][incomingValue] = std::vector<AsmOperandRegister *>();
                }
                phiValueMap[incomingBlock][incomingValue].push_back(tmp);
            }
        }
    }
}

void AsmBasicBlock::sufGenInstsPhi()
{
    std::set<IR::BasicBlock *> exitBlocks = irBlock->getSuccBlock();

    for (auto block : exitBlocks)
    {
        AsmBasicBlock *nextBlock = asmFunction->getBB(block);
        if (nextBlock->phiValueMap.find(irBlock) != nextBlock->phiValueMap.end())
        {
            asmFunction->addInst(nextBlock->phiOperation[irBlock]);

            for (auto [value, regs] : nextBlock->phiValueMap[irBlock])
            {
                AsmOperandRegister *reg = asmFunction->getValueToRegister(value);

                for (auto target : nextBlock->phiValueMap[irBlock][value])
                {
                    asmFunction->addInst(new AsmInstMove(target, reg));
                }
            }
        }
    }
}