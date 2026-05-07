#include "passes.h"
namespace pass {

bool ConstProp::onFunction(ir::Function *function) {
  bool changed = false;
  for (auto &blockPtr : *function) {
    auto block = static_cast<ir::BlockBase *>(blockPtr.get());
    for (int i = 0; i < block->size(); i++) {
      const auto inst = block->getInstruction(i);
      if (const auto binaryOperator = dynamic_cast<ir::BinaryInst *>(inst)) {
        const auto value1 =
            dynamic_cast<ir::ConstantNumber *>(binaryOperator->getOperand(0));
        if (value1) {
          const auto value2 =
              dynamic_cast<ir::ConstantNumber *>(binaryOperator->getOperand(1));
          if (value2) {
            ir::ConstantNumber *newValue = nullptr;
            switch (binaryOperator->getOp()) {
            case ir::BinaryOp::ADD:
            case ir::BinaryOp::FADD:
              newValue = new ir::ConstantNumber((*value1) + (*value2));
              break;
            case ir::BinaryOp::SUB:
            case ir::BinaryOp::FSUB:
              newValue = new ir::ConstantNumber((*value1) - (*value2));
              break;
            case ir::BinaryOp::MUL:
            case ir::BinaryOp::FMUL:
              newValue = new ir::ConstantNumber((*value1) * (*value2));
              break;
            case ir::BinaryOp::SDIV:
            case ir::BinaryOp::FDIV:
              newValue = new ir::ConstantNumber((*value1) / (*value2));
              break;
            case ir::BinaryOp::SREM:
              newValue = new ir::ConstantNumber((*value1) % (*value2));
              break;
            case ir::BinaryOp::XOR:
              newValue = new ir::ConstantNumber((*value1) ^ (*value2));
            }
            if (!newValue) {
              throw std::runtime_error("Unsupported binary operator in "
                                       "ConstPropPass::runOnFunction");
            }
            binaryOperator->replaceAllUsesWith(newValue);
            block->eraseInstruction(i);
            i--;
            changed = true;
          }
        }
      } else if (const auto cmpInst = dynamic_cast<ir::CmpInst *>(inst)) {
        const auto value1 =
            dynamic_cast<ir::ConstantNumber *>(cmpInst->getOperand(0));
        if (value1) {
          const auto value2 =
              dynamic_cast<ir::ConstantNumber *>(cmpInst->getOperand(1));
          if (value2) {
            ir::ConstantNumber *newValue = nullptr;
            switch (cmpInst->getOp()) {
            case ir::CmpOp::EQ:
            case ir::CmpOp::OEQ:
              newValue = new ir::ConstantNumber((*value1) == (*value2));
              break;
            case ir::CmpOp::NE:
            case ir::CmpOp::UNE:
              newValue = new ir::ConstantNumber((*value1) != (*value2));
              break;
            case ir::CmpOp::SGE:
            case ir::CmpOp::OGE:
              newValue = new ir::ConstantNumber((*value1) >= (*value2));
              break;
            case ir::CmpOp::SGT:
            case ir::CmpOp::OGT:
              newValue = new ir::ConstantNumber((*value1) > (*value2));
              break;
            case ir::CmpOp::SLE:
            case ir::CmpOp::OLE:
              newValue = new ir::ConstantNumber((*value1) <= (*value2));
              break;
            case ir::CmpOp::SLT:
            case ir::CmpOp::OLT:
              newValue = new ir::ConstantNumber((*value1) < (*value2));
              break;
            }
            if (!newValue) {
              throw std::runtime_error("Unsupported cmp instruction in "
                                       "ConstPropPass::runOnFunction");
            }
            cmpInst->replaceAllUsesWith(newValue);
            block->eraseInstruction(i);
            i--;
            changed = true;
          }
        }
      }
    }
  }
  return changed;
}

} // namespace pass
