#include "generate_mir.h"

namespace riscv {

using ir::InstKind;
using ir::Instruction;
using std::make_unique;
using std::unique_ptr;
using std::unordered_map;

void GenerateMIR::makeMachineBlocks(MachineFunc *mFunc) {
  auto original = mFunc->getOriginalFunc();
  for (auto &block : *original) {
    auto machineBlock =
        make_unique<MachineBlock>(static_cast<ir::BasicBlock *>(block.get()));
    mFunc->blockMap[static_cast<ir::BasicBlock *>(block.get())] =
        machineBlock.get();
    mFunc->pushBlock(std::move(machineBlock));
  }
}

unique_ptr<MachineFunc> GenerateMIR::funcToMIR(ir::Function *func) {
  auto machineFunc = make_unique<MachineFunc>(func);
  auto exitBBlock = make_unique<ir::BasicBlock>();
  auto exitMBlock = make_unique<MachineBlock>(exitBBlock.get());
  machineFunc->blockMap[exitBBlock.get()] = exitMBlock.get();
  makeMachineBlocks(machineFunc.get());

  for (auto &block : *func) {
    for (size_t i = 0; i < block->size(); i++) {
      auto inst = static_cast<ir::Instruction *>(block->getInstruction(i));
      auto instKind = inst->getInstKind();
      if (instKind == InstKind::Phi) {
        auto phiInst = static_cast<ir::PhiInst *>(inst);

        // Add phiNode to hold the virtual register
        auto phiNode = machineFunc->pushPhiNode(
            make_unique<PhiNode>(phiInst->makeRegType()));
        machineFunc->addInstPair(phiInst, phiNode);

        // Add move instructions for each incoming value
        for (size_t j = 0; j < phiInst->getNumOperands(); j++) {
          auto incomingBlock = phiInst->getIncomingBlock(j);
          auto value = phiInst->getOperand(j);
          incomingBlock->insertInstruction(
              static_cast<int>(incomingBlock->size()) - 1,
              make_unique<ir::MoveInst>(phiInst, value));
        }

        // // Remove the phi instruction from the block
        // block->eraseInstruction(i);
        // i--; // Adjust index after erasing the instruction
      }
    }
  }

  for (auto &block : *func) {
    auto mBlock =
        machineFunc->blockMap[static_cast<ir::BasicBlock *>(block.get())];
    for (auto &instPtr : *block) {
      auto inst = static_cast<Instruction *>(instPtr.get());
      switch (inst->getInstKind()) {
      case InstKind::Alloca:
      case InstKind::Phi:
        continue;
      case InstKind::Binary:
        machineFunc->binary(static_cast<ir::BinaryInst *>(inst), mBlock);
        continue;
      case InstKind::Branch:
        machineFunc->branch(static_cast<ir::BranchInst *>(inst), mBlock);
        continue;
      case InstKind::Call: {
        int paramNum =
            machineFunc->call(static_cast<ir::CallInst *>(inst), mBlock);
        machineFunc->maxFuncParamNum =
            std::max(machineFunc->maxFuncParamNum, paramNum);
        continue;
      }
      case InstKind::GEP:
        machineFunc->gep(static_cast<ir::GetElementPtrInst *>(inst), mBlock);
        continue;
      case InstKind::Load:
        machineFunc->load(static_cast<ir::LoadInst *>(inst), mBlock);
        continue;
      case InstKind::Ret:
        machineFunc->ret(static_cast<ir::RetInst *>(inst), mBlock,
                         exitMBlock.get());
        continue;
      case InstKind::Store:
        machineFunc->store(static_cast<ir::StoreInst *>(inst), mBlock);
        continue;
      case InstKind::ICmp:
        machineFunc->icmp(static_cast<ir::CmpInst *>(inst), mBlock);
        continue;
      case InstKind::FCmp:
        machineFunc->fcmp(static_cast<ir::CmpInst *>(inst), mBlock);
        continue;
      case InstKind::BitCast:
        machineFunc->bitcast(static_cast<ir::CastInst *>(inst), mBlock);
        continue;
      case InstKind::ZExt:
        machineFunc->zext(static_cast<ir::CastInst *>(inst), mBlock);
        continue;
      case InstKind::SExt:
        machineFunc->sext(static_cast<ir::CastInst *>(inst), mBlock);
        continue;
      case InstKind::SIToFP:
        machineFunc->sitofp(static_cast<ir::CastInst *>(inst), mBlock);
        continue;
      case InstKind::FPToSI:
        machineFunc->fptosi(static_cast<ir::CastInst *>(inst), mBlock);
        continue;
      case InstKind::Move:
        machineFunc->move(static_cast<ir::MoveInst *>(inst), mBlock);
        continue;
      default:
        throw std::runtime_error(
            "Unsupported instruction kind in MIR generation: " +
            std::to_string(static_cast<int>(inst->getInstKind())));
      }
    }
  }
  machineFunc->pushBlock(std::move(exitMBlock));
  func->pushBlock(std::move(exitBBlock));
  return std::move(machineFunc);
}

} // namespace riscv