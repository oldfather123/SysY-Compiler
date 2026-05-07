#include "DivStrengthReduction.h"
#include <cmath>
#include <cstdint>

namespace sysy {

char DivStrengthReduction::ID = 0;

bool DivStrengthReduction::runOnFunction(Function *F, AnalysisManager& AM) {
    // This pass works on MachineFunction level, not IR level
    return false;
}

void DivStrengthReduction::runOnMachineFunction(MachineFunction *mfunc) {
    if (!mfunc)
        return;

    bool debug = false; // Set to true for debugging
    if (debug)
        std::cout << "Running DivStrengthReduction optimization..." << std::endl;

    int next_temp_reg = 1000;
    auto createTempReg = [&]() -> int {
        return next_temp_reg++;
    };

    struct MagicInfo {
        int64_t magic;
        int shift;
    };
    
    auto computeMagic = [](int64_t d, bool is_32bit) -> MagicInfo {
        int word_size = is_32bit ? 32 : 64;
        uint64_t ad = std::abs(d);
        
        if (ad == 0) return {0, 0};
        
        int l = std::floor(std::log2(ad));
        if ((ad & (ad - 1)) == 0) { // power of 2
             l = 0; // special case for power of 2, shift will be calculated differently
        }

        __int128_t one = 1;
        __int128_t num;
        int total_shift;

        if (is_32bit) {
            total_shift = 31 + l;
            num = one << total_shift;
        } else {
            total_shift = 63 + l;
            num = one << total_shift;
        }
        
        __int128_t den = ad;
        int64_t magic = (num / den) + 1;
        
        return {magic, total_shift};
    };

    auto isPowerOfTwo = [](int64_t n) -> bool {
        return n > 0 && (n & (n - 1)) == 0;
    };

    auto getPowerOfTwoExponent = [](int64_t n) -> int {
        if (n <= 0 || (n & (n - 1)) != 0) return -1;
        int shift = 0;
        while (n > 1) {
            n >>= 1;
            shift++;
        }
        return shift;
    };

    struct InstructionReplacement {
        size_t index;
        size_t count_to_erase;
        std::vector<std::unique_ptr<MachineInstr>> newInstrs;
    };
    
    for (auto &mbb_uptr : mfunc->getBlocks()) {
        auto &mbb = *mbb_uptr;
        auto &instrs = mbb.getInstructions();
        std::vector<InstructionReplacement> replacements;
        
        for (size_t i = 0; i < instrs.size(); ++i) {
            auto *instr = instrs[i].get();
            
            bool is_32bit = (instr->getOpcode() == RVOpcodes::DIVW);
            
            if (instr->getOpcode() != RVOpcodes::DIV && !is_32bit) {
                continue;
            }
            
            if (instr->getOperands().size() != 3) {
                continue;
            }
            
            auto *dst_op = instr->getOperands()[0].get();
            auto *src1_op = instr->getOperands()[1].get();
            auto *src2_op = instr->getOperands()[2].get();

            int64_t divisor = 0;
            bool const_divisor_found = false;
            size_t instructions_to_replace = 1;

            if (src2_op->getKind() == MachineOperand::KIND_IMM) {
                divisor = static_cast<ImmOperand *>(src2_op)->getValue();
                const_divisor_found = true;
            } else if (src2_op->getKind() == MachineOperand::KIND_REG) {
                if (i > 0) {
                    auto *prev_instr = instrs[i - 1].get();
                    if (prev_instr->getOpcode() == RVOpcodes::LI && prev_instr->getOperands().size() == 2) {
                        auto *li_dst_op = prev_instr->getOperands()[0].get();
                        auto *li_imm_op = prev_instr->getOperands()[1].get();
                        if (li_dst_op->getKind() == MachineOperand::KIND_REG && li_imm_op->getKind() == MachineOperand::KIND_IMM) {
                            auto *div_reg_op = static_cast<RegOperand *>(src2_op);
                            auto *li_dst_reg_op = static_cast<RegOperand *>(li_dst_op);
                            if (div_reg_op->isVirtual() && li_dst_reg_op->isVirtual() &&
                                div_reg_op->getVRegNum() == li_dst_reg_op->getVRegNum()) {
                                divisor = static_cast<ImmOperand *>(li_imm_op)->getValue();
                                const_divisor_found = true;
                                instructions_to_replace = 2;
                            }
                        }
                    }
                }
            }

            if (!const_divisor_found) {
                continue;
            }
            
            auto *dst_reg = static_cast<RegOperand *>(dst_op);
            auto *src1_reg = static_cast<RegOperand *>(src1_op);
            
            if (divisor == 0) continue;
            
            std::vector<std::unique_ptr<MachineInstr>> newInstrs;
            
            if (divisor == 1) {
                auto moveInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::ADDW : RVOpcodes::ADD);
                moveInstr->addOperand(std::make_unique<RegOperand>(*dst_reg));
                moveInstr->addOperand(std::make_unique<RegOperand>(*src1_reg));
                moveInstr->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                newInstrs.push_back(std::move(moveInstr));
            }
            else if (divisor == -1) {
                auto negInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::SUBW : RVOpcodes::SUB);
                negInstr->addOperand(std::make_unique<RegOperand>(*dst_reg));
                negInstr->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                negInstr->addOperand(std::make_unique<RegOperand>(*src1_reg));
                newInstrs.push_back(std::move(negInstr));
            }
            else if (isPowerOfTwo(std::abs(divisor))) {
                int shift = getPowerOfTwoExponent(std::abs(divisor));
                int temp_reg = createTempReg();
                
                auto sraSignInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::SRAIW : RVOpcodes::SRAI);
                sraSignInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                sraSignInstr->addOperand(std::make_unique<RegOperand>(*src1_reg));
                sraSignInstr->addOperand(std::make_unique<ImmOperand>(is_32bit ? 31 : 63));
                newInstrs.push_back(std::move(sraSignInstr));
                
                auto srlInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::SRLIW : RVOpcodes::SRLI);
                srlInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                srlInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                srlInstr->addOperand(std::make_unique<ImmOperand>((is_32bit ? 32 : 64) - shift));
                newInstrs.push_back(std::move(srlInstr));
                
                auto addInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::ADDW : RVOpcodes::ADD);
                addInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                addInstr->addOperand(std::make_unique<RegOperand>(*src1_reg));
                addInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                newInstrs.push_back(std::move(addInstr));
                
                auto sraInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::SRAIW : RVOpcodes::SRAI);
                sraInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                sraInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                sraInstr->addOperand(std::make_unique<ImmOperand>(shift));
                newInstrs.push_back(std::move(sraInstr));

                if (divisor < 0) {
                    auto negInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::SUBW : RVOpcodes::SUB);
                    negInstr->addOperand(std::make_unique<RegOperand>(*dst_reg));
                    negInstr->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                    negInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                    newInstrs.push_back(std::move(negInstr));
                } else {
                    auto moveInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::ADDW : RVOpcodes::ADD);
                    moveInstr->addOperand(std::make_unique<RegOperand>(*dst_reg));
                    moveInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                    moveInstr->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                    newInstrs.push_back(std::move(moveInstr));
                }
            }
            else {
                auto magic_info = computeMagic(divisor, is_32bit);
                int magic_reg = createTempReg();
                int temp_reg = createTempReg();

                auto loadInstr = std::make_unique<MachineInstr>(RVOpcodes::LI);
                loadInstr->addOperand(std::make_unique<RegOperand>(magic_reg));
                loadInstr->addOperand(std::make_unique<ImmOperand>(magic_info.magic));
                newInstrs.push_back(std::move(loadInstr));

                if (is_32bit) {
                    auto mulInstr = std::make_unique<MachineInstr>(RVOpcodes::MUL);
                    mulInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                    mulInstr->addOperand(std::make_unique<RegOperand>(*src1_reg));
                    mulInstr->addOperand(std::make_unique<RegOperand>(magic_reg));
                    newInstrs.push_back(std::move(mulInstr));

                    auto sraInstr = std::make_unique<MachineInstr>(RVOpcodes::SRAI);
                    sraInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                    sraInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                    sraInstr->addOperand(std::make_unique<ImmOperand>(magic_info.shift));
                    newInstrs.push_back(std::move(sraInstr));
                } else {
                    auto mulhInstr = std::make_unique<MachineInstr>(RVOpcodes::MULH);
                    mulhInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                    mulhInstr->addOperand(std::make_unique<RegOperand>(*src1_reg));
                    mulhInstr->addOperand(std::make_unique<RegOperand>(magic_reg));
                    newInstrs.push_back(std::move(mulhInstr));
                    
                    int post_shift = magic_info.shift - 63;
                    if (post_shift > 0) {
                        auto sraInstr = std::make_unique<MachineInstr>(RVOpcodes::SRAI);
                        sraInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                        sraInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                        sraInstr->addOperand(std::make_unique<ImmOperand>(post_shift));
                        newInstrs.push_back(std::move(sraInstr));
                    }
                }
                
                int sign_reg = createTempReg();
                auto sraSignInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::SRAIW : RVOpcodes::SRAI);
                sraSignInstr->addOperand(std::make_unique<RegOperand>(sign_reg));
                sraSignInstr->addOperand(std::make_unique<RegOperand>(*src1_reg));
                sraSignInstr->addOperand(std::make_unique<ImmOperand>(is_32bit ? 31 : 63));
                newInstrs.push_back(std::move(sraSignInstr));

                auto subInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::SUBW : RVOpcodes::SUB);
                subInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                subInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                subInstr->addOperand(std::make_unique<RegOperand>(sign_reg));
                newInstrs.push_back(std::move(subInstr));

                if (divisor < 0) {
                    auto negInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::SUBW : RVOpcodes::SUB);
                    negInstr->addOperand(std::make_unique<RegOperand>(*dst_reg));
                    negInstr->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                    negInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                    newInstrs.push_back(std::move(negInstr));
                } else {
                    auto moveInstr = std::make_unique<MachineInstr>(is_32bit ? RVOpcodes::ADDW : RVOpcodes::ADD);
                    moveInstr->addOperand(std::make_unique<RegOperand>(*dst_reg));
                    moveInstr->addOperand(std::make_unique<RegOperand>(temp_reg));
                    moveInstr->addOperand(std::make_unique<RegOperand>(PhysicalReg::ZERO));
                    newInstrs.push_back(std::move(moveInstr));
                }
            }
            
            if (!newInstrs.empty()) {
                size_t start_index = i;
                if (instructions_to_replace == 2) {
                    start_index = i - 1;
                }
                replacements.push_back({start_index, instructions_to_replace, std::move(newInstrs)});
            }
        }
        
        for (auto it = replacements.rbegin(); it != replacements.rend(); ++it) {
            instrs.erase(instrs.begin() + it->index, instrs.begin() + it->index + it->count_to_erase);
            instrs.insert(instrs.begin() + it->index, 
                         std::make_move_iterator(it->newInstrs.begin()),
                         std::make_move_iterator(it->newInstrs.end()));
        }
    }
}

} // namespace sysy
