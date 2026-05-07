#pragma once

#include <cassert>
#include <cstdint>
#include "../backend/Riscv.h"

/**
 * @file RiscvBuilder.h
 *
 * @brief 定义RISC-V构建器相关的类型与操作的头文件
 */

namespace riscv {
/**
 * @brief riscv汇编指令构建器
 *
 */
class RiscvBuilder {
 private:
  using InstKind = Instruction::InstKind;
  BasicBlock *block;
  BasicBlock::iterator position;

 public:
  RiscvBuilder() = default;
  explicit RiscvBuilder(BasicBlock *block) : block(block), position(block->end()) {}
  RiscvBuilder(BasicBlock *block, BasicBlock::iterator position) : block(block), position(position) {}

 public:
  void setPosition(BasicBlock *block) {
    this->block = block;
    position = block->end();
  }
  void setPosition(BasicBlock *block, BasicBlock::iterator position) {
    this->block = block;
    this->position = position;
  }
  auto getCurBlock() const -> BasicBlock * { return block; }
  auto getPosition() const -> BasicBlock::iterator { return position; }

 public:
  auto createRInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2, InstKind kind) -> RInst * {
    auto inst = new RInst(rd, rs1, rs2, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createSllInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSll);
  }
  auto createSrlInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSrl);
  }
  auto createSraInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSra);
  }
  auto createSllwInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSllw);
  }
  auto createSrlwInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSrlw);
  }
  auto createSrawInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSraw);
  }
  auto createAddInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kAdd);
  }
  auto createSubInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSub);
  }
  auto createAddwInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kAddw);
  }
  auto createSubwInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSubw);
  }
  auto createXorInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kXor);
  }
  auto createOrInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kOr);
  }
  auto createAndInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kAnd);
  }
  auto createSltInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSlt);
  }
  auto createSltuInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kSltu);
  }
  auto createMulInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kMul);
  }
  auto createMulhInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kMulh);
  }
  auto createMulhsuInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kMulhsu);
  }
  auto createMulhuInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kMulhu);
  }
  auto createMulwInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kMulw);
  }
  auto createDivInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kDiv);
  }
  auto createDivuInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kDivu);
  }
  auto createDivwInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kDivw);
  }
  auto createRemInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kRem);
  }
  auto createRemuInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kRemu);
  }
  auto createRemwInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kRemw);
  }
  auto createRemuwInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kRemuw);
  }
  auto createAmoMinInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kAmoMin);
  }
  auto createAmoMaxInst(IntRegister *rd, IntRegister *rs1, IntRegister *rs2) -> RInst * {
    return createRInst(rd, rs1, rs2, InstKind::kAmoMax);
  }

  auto createUInst(IntRegister *rd, unsigned imm, InstKind kind) -> UInst * {
    auto inst = new UInst(rd, imm, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createLuiInst(IntRegister *rd, unsigned imm, InstKind kind) -> UInst * {
    assert(imm < 1048576);
    return createUInst(rd, imm, InstKind::kLui);
  }
  auto createAuipcInst(IntRegister *rd, unsigned imm, InstKind kind) -> UInst * {
    assert(imm < 1048576);
    return createUInst(rd, imm, InstKind::kAuipc);
  }

  auto createIInst(IntRegister *rd, IntRegister *rs1, int imm, InstKind kind) -> IInst * {
    auto inst = new IInst(rd, rs1, imm, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createSlliInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kSlli);
  }
  auto createSrliInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kSrli);
  }
  auto createSraiInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kSrai);
  }
  auto createSlliwInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kSlliw);
  }
  auto createSrliwInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kSrliw);
  }
  auto createSraiwInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kSraiw);
  }
  auto createAddiInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kAddi);
  }
  auto createAddiwInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kAddiw);
  }
  auto createXoriInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kXori);
  }
  auto createOriInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kOri);
  }
  auto createAndiInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kAndi);
  }
  auto createSltiInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kSlti);
  }
  auto createSltiuInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kSltiu);
  }
  auto createJalrInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kJalr);
  }
  auto createLbInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kLb);
  }
  auto createLhInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kLh);
  }
  auto createLbuInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kLbu);
  }
  auto createLhuInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kLhu);
  }
  auto createLwInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kLw);
  }
  auto createLwuInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kLwu);
  }
  auto createLdInst(IntRegister *rd, IntRegister *rs1, int imm) -> IInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createIInst(rd, rs1, imm, InstKind::kLd);
  }

  auto createSInst(IntRegister *rs1, IntRegister *rs2, int imm, InstKind kind) -> SInst * {
    auto inst = new SInst(rs1, rs2, imm, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createSbInst(IntRegister *rs1, IntRegister *rs2, int imm) -> SInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createSInst(rs1, rs2, imm, InstKind::kSb);
  }
  auto createShInst(IntRegister *rs1, IntRegister *rs2, int imm) -> SInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createSInst(rs1, rs2, imm, InstKind::kSh);
  }
  auto createSwInst(IntRegister *rs1, IntRegister *rs2, int imm) -> SInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createSInst(rs1, rs2, imm, InstKind::kSw);
  }
  auto createSdInst(IntRegister *rs1, IntRegister *rs2, int imm) -> SInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createSInst(rs1, rs2, imm, InstKind::kSd);
  }

  auto createBInst(IntRegister *rs1, IntRegister *rs2, BasicBlock *thenBlock, InstKind kind) -> BInst * {
    auto inst = new BInst(rs1, rs2, thenBlock, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createBeq(IntRegister *rs1, IntRegister *rs2, BasicBlock *thenBlock) -> BInst * {
    return createBInst(rs1, rs2, thenBlock, InstKind::kBeq);
  }
  auto createBne(IntRegister *rs1, IntRegister *rs2, BasicBlock *thenBlock) -> BInst * {
    return createBInst(rs1, rs2, thenBlock, InstKind::kBne);
  }
  auto createBlt(IntRegister *rs1, IntRegister *rs2, BasicBlock *thenBlock) -> BInst * {
    return createBInst(rs1, rs2, thenBlock, InstKind::kBlt);
  }
  auto createBge(IntRegister *rs1, IntRegister *rs2, BasicBlock *thenBlock) -> BInst * {
    return createBInst(rs1, rs2, thenBlock, InstKind::kBge);
  }
  auto createBltu(IntRegister *rs1, IntRegister *rs2, BasicBlock *thenBlock) -> BInst * {
    return createBInst(rs1, rs2, thenBlock, InstKind::kBltu);
  }
  auto createBgeu(IntRegister *rs1, IntRegister *rs2, BasicBlock *thenBlock) -> BInst * {
    return createBInst(rs1, rs2, thenBlock, InstKind::kBgeu);
  }

  auto createJInst(IntRegister *rd, Label *label, InstKind kind) -> JInst * {
    auto inst = new JInst(rd, label, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createJalInst(IntRegister *rd, Label *label) { return createJInst(rd, label, InstKind::kJal); }

  auto createI2FInst(FloatRegister *rd, IntRegister *rs1, InstKind kind) -> I2FInst * {
    auto inst = new I2FInst(rd, rs1, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createFmv_w_xInst(FloatRegister *rd, IntRegister *rs1) -> I2FInst * {
    return createI2FInst(rd, rs1, InstKind::kFmv_w_x);
  }
  auto createFmv_d_xInst(FloatRegister *rd, IntRegister *rs1) -> I2FInst * {
    return createI2FInst(rd, rs1, InstKind::kFmv_d_x);
  }
  auto createFcvt_s_wInst(FloatRegister *rd, IntRegister *rs1) -> I2FInst * {
    return createI2FInst(rd, rs1, InstKind::kFcvt_s_w);
  }
  auto createFcvt_s_wuInst(FloatRegister *rd, IntRegister *rs1) -> I2FInst * {
    return createI2FInst(rd, rs1, InstKind::kFcvt_s_wu);
  }
  auto createFcvt_s_lInst(FloatRegister *rd, IntRegister *rs1) -> I2FInst * {
    return createI2FInst(rd, rs1, InstKind::kFcvt_s_l);
  }
  auto createFcvt_s_luInst(FloatRegister *rd, IntRegister *rs1) -> I2FInst * {
    return createI2FInst(rd, rs1, InstKind::kFcvt_s_lu);
  }

  auto createF2IInst(IntRegister *rd, FloatRegister *rs1, InstKind kind) -> F2IInst * {
    auto inst = new F2IInst(rd, rs1, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createFmv_x_wInst(IntRegister *rd, FloatRegister *rs1) -> F2IInst * {
    return createF2IInst(rd, rs1, InstKind::kFmv_x_w);
  }
  auto createFmv_x_dInst(IntRegister *rd, FloatRegister *rs1) -> F2IInst * {
    return createF2IInst(rd, rs1, InstKind::kFmv_x_d);
  }
  auto createFcvt_w_sInst(IntRegister *rd, FloatRegister *rs1) -> F2IInst * {
    return createF2IInst(rd, rs1, InstKind::kFcvt_w_s);
  }
  auto createFcvt_wu_sInst(IntRegister *rd, FloatRegister *rs1) -> F2IInst * {
    return createF2IInst(rd, rs1, InstKind::kFcvt_wu_s);
  }
  auto createFcvt_l_sInst(IntRegister *rd, FloatRegister *rs1) -> F2IInst * {
    return createF2IInst(rd, rs1, InstKind::kFcvt_l_s);
  }
  auto createFcvt_lu_sInst(IntRegister *rd, FloatRegister *rs1) -> F2IInst * {
    return createF2IInst(rd, rs1, InstKind::kFcvt_lu_s);
  }
  auto createFclass_sInst(IntRegister *rd, FloatRegister *rs1) -> F2IInst * {
    return createF2IInst(rd, rs1, InstKind::kFclass_s);
  }

  auto createFRInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2, InstKind kind) -> FRInst * {
    auto inst = new FRInst(rd, rs1, rs2, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createFadd_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FRInst * {
    return createFRInst(rd, rs1, rs2, InstKind::kFadd_s);
  }
  auto createFsub_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FRInst * {
    return createFRInst(rd, rs1, rs2, InstKind::kFsub_s);
  }
  auto createFmul_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FRInst * {
    return createFRInst(rd, rs1, rs2, InstKind::kFmul_s);
  }
  auto createFdiv_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FRInst * {
    return createFRInst(rd, rs1, rs2, InstKind::kFdiv_s);
  }
  auto createFsgnj_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FRInst * {
    return createFRInst(rd, rs1, rs2, InstKind::kFsgnj_s);
  }
  auto createFsgnjn_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FRInst * {
    return createFRInst(rd, rs1, rs2, InstKind::kFsgnjn_s);
  }
  auto createFsgnjx_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FRInst * {
    return createFRInst(rd, rs1, rs2, InstKind::kFsgnjx_s);
  }
  auto createFmin_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FRInst * {
    return createFRInst(rd, rs1, rs2, InstKind::kFmin_s);
  }
  auto createFmax_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FRInst * {
    return createFRInst(rd, rs1, rs2, InstKind::kFmax_s);
  }

  auto createFR2IInst(IntRegister *rd, FloatRegister *rs1, FloatRegister *rs2, InstKind kind) -> FR2IInst * {
    auto inst = new FR2IInst(rd, rs1, rs2, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createFeq_sInst(IntRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FR2IInst * {
    return createFR2IInst(rd, rs1, rs2, InstKind::kFeq_s);
  }
  auto createFlt_sInst(IntRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FR2IInst * {
    return createFR2IInst(rd, rs1, rs2, InstKind::kFlt_s);
  }
  auto createFle_sInst(IntRegister *rd, FloatRegister *rs1, FloatRegister *rs2) -> FR2IInst * {
    return createFR2IInst(rd, rs1, rs2, InstKind::kFle_s);
  }

  auto createFLongIInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2, FloatRegister *rs3, InstKind kind)
      -> FLongInst * {
    auto inst = new FLongInst(rd, rs1, rs2, rs3, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createFmadd_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2, FloatRegister *rs3) -> FLongInst * {
    return createFLongIInst(rd, rs1, rs2, rs3, InstKind::kFmadd_s);
  }
  auto createFmsub_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2, FloatRegister *rs3) -> FLongInst * {
    return createFLongIInst(rd, rs1, rs2, rs3, InstKind::kFmsub_s);
  }
  auto createFnmadd_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2, FloatRegister *rs3)
      -> FLongInst * {
    return createFLongIInst(rd, rs1, rs2, rs3, InstKind::kFnmadd_s);
  }
  auto createFnmsub_sInst(FloatRegister *rd, FloatRegister *rs1, FloatRegister *rs2, FloatRegister *rs3)
      -> FLongInst * {
    return createFLongIInst(rd, rs1, rs2, rs3, InstKind::kFnmsub_s);
  }

  auto createFLInst(FloatRegister *rd, IntRegister *rs1, int imm, InstKind kind) -> FLInst * {
    auto inst = new FLInst(rd, rs1, imm, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createFlwInst(FloatRegister *rd, IntRegister *rs1, int imm) -> FLInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createFLInst(rd, rs1, imm, InstKind::kFlw);
  }

  auto createFSInst(FloatRegister *rs1, IntRegister *rs2, int imm, InstKind kind) -> FSInst * {
    auto inst = new FSInst(rs1, rs2, imm, kind, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createFswInst(FloatRegister *rs1, IntRegister *rs2, int imm) -> FSInst * {
    assert(imm >= -2048 && imm <= 2047);
    return createFSInst(rs1, rs2, imm, InstKind::kFsw);
  }

  auto createLlaInst(IntRegister *rd, Global *symbol) -> LlaInst * {
    auto inst = new LlaInst(rd, symbol, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createLiInst(IntRegister *rd, uint64_t imm) -> LiInst * {
    auto inst = new LiInst(rd, imm, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
  auto createSext_wInst(IntRegister *rd, IntRegister *rs1) -> Sext_wInst * {
    auto inst = new Sext_wInst(rd, rs1, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }

  auto createCallInst(Label *label) -> CallInst * {
    auto inst = new CallInst(label, block);
    assert(inst);
    block->instructions.emplace(position, inst);
    return inst;
  }
};
}  // namespace riscv
