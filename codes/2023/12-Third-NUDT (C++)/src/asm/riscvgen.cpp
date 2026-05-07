// #include "riscvgen.hpp"

// #include "ir.hpp"

// RISCV::Gener::Gener(IR::CompileUnit *comp) : _comp(comp) {}

// void RISCV::Gener::gen(LogLevel log_level) {
//   configuration.saveLogLevel();
//   configuration.setLogLevel(log_level);
//   gen(_comp);
//   configuration.loadLogLevel();
// }

// void RISCV::Gener::gen(IR::CompileUnit *comp) {
//   info << IDENT << ".text" << '\n';
//   info << IDENT << ".file \"" << configuration.filename() << '\"' << '\n';
//   std::set<string> builtins({
//       "getint",
//       "getch",
//       "getfloat",
//       "getarray",
//       "getfarray",
//       "putint",
//       "putch",
//       "putfloat",
//       "putarray",
//       "putfarray",
//       "putf",
//       "starttime",
//       "stoptime",
//   });
//   for (auto &[name, func] : *comp->functionTable())
//     if (!builtins.count(name)) gen(func.get());
// }

// // [temp, offset] TODO performance issue
// static std::map<std::string, int> asm_var_tab;

// void RISCV::Gener::gen(IR::Function *func) {
//   asm_var_tab.clear();
//   if (func->name() != ".init") {
//     info << IDENT << ".globl " << func->name() << ':' << '\n';
//     info << IDENT << ".p2align " << 1 << '\n';
//     info << IDENT << ".type " << func->name() << ", @function" << '\n';
//     info << func->name() << ':' << '\n';
//     size_t offset = 0;  // from fp
//     offset += 8;
//     offset += 8;
//     bool leaf_func = true;
//     size_t spilling_args = 0;
//     for (auto &block : func->blocks()) {
//       for (auto &inst : block->instructions()) {
//         if (inst->itype() == IR::ALLOCA)
//           offset += inst->type()->deref(1)->sizeOf();
//         else
//           offset += 4;
//         asm_var_tab[inst->name()] = offset;
//         if (inst->itype() == IR::CALL) {
//           if (leaf_func) leaf_func = false;
//           size_t param_num = inst->operand(0)->type()->paramTypes().size();
//           if (param_num > 8)
//             spilling_args = std::max(spilling_args, param_num - 8);
//         }
//       }
//     }
//     offset += spilling_args * 4;
//     info << IDENT << "addi "
//          << "sp, "
//          << "sp, " << '-' << offset << '\n';
//     info << IDENT << "sd "
//          << "ra, " << offset - 8 << "(sp)" << '\n';
//     info << IDENT << "sd "
//          << "fp, " << offset - 16 << "(sp)" << '\n';
//     info << IDENT << "addi "
//          << "fp, "
//          << "sp, " << offset << '\n';
//     for (auto &block : func->blocks()) gen(block.get());
//     info << IDENT << "ld "
//          << "ra, " << offset - 8 << "(sp)" << '\n';
//     info << IDENT << "ld "
//          << "fp, " << offset - 16 << "(sp)" << '\n';
//     info << IDENT << "addi "
//          << "sp, "
//          << "sp, " << offset << '\n';
//     info << IDENT << "ret" << '\n';
//     info << '\n';
//   } else {
//     for (auto &block : func->blocks()) gen(block.get());
//     info << '\n';
//   }
// }

// void RISCV::Gener::gen(IR::BasicBlock *block) {
//   if (block->name() != "entry") info << '.' << block->name() << ":\n";
//   for (auto &inst : block->instructions()) gen(inst.get());
// }

// void RISCV::Gener::gen(IR::Instruction *inst) {
//   if (inst->itype() == IR::INVALID) {
//     throw InvalidInst();
//     return;
//   } else if (inst->itype() == IR::INEG) {
//     return;
//   } else if (inst->itype() == IR::INOT) {
//     return;
//   } else if (inst->itype() == IR::IADD) {
//     return;
//   } else if (inst->itype() == IR::ISUB) {
//     return;
//   } else if (inst->itype() == IR::IMUL) {
//     return;
//   } else if (inst->itype() == IR::IDIV) {
//     return;
//   } else if (inst->itype() == IR::IMOD) {
//     return;
//   } else if (inst->itype() == IR::IEQ) {
//     return;
//   } else if (inst->itype() == IR::INE) {
//     return;
//   } else if (inst->itype() == IR::ILT) {
//     return;
//   } else if (inst->itype() == IR::IGT) {
//     return;
//   } else if (inst->itype() == IR::ILE) {
//     return;
//   } else if (inst->itype() == IR::IGE) {
//     return;
//   } else if (inst->itype() == IR::IAND) {
//     return;
//   } else if (inst->itype() == IR::IOR) {
//     return;
//   } else if (inst->itype() == IR::FNEG) {
//     return;
//   } else if (inst->itype() == IR::FNOT) {
//     return;
//   } else if (inst->itype() == IR::FADD) {
//     return;
//   } else if (inst->itype() == IR::FSUB) {
//     return;
//   } else if (inst->itype() == IR::FMUL) {
//     return;
//   } else if (inst->itype() == IR::FDIV) {
//     return;
//   } else if (inst->itype() == IR::FEQ) {
//     return;
//   } else if (inst->itype() == IR::FNE) {
//     return;
//   } else if (inst->itype() == IR::FLT) {
//     return;
//   } else if (inst->itype() == IR::FGT) {
//     return;
//   } else if (inst->itype() == IR::FLE) {
//     return;
//   } else if (inst->itype() == IR::FGE) {
//     return;
//   } else if (inst->itype() == IR::FAND) {
//     return;
//   } else if (inst->itype() == IR::FOR) {
//     return;
//   } else if (inst->itype() == IR::FTOI) {
//     return;
//   } else if (inst->itype() == IR::ITOF) {
//     return;
//   } else if (inst->itype() == IR::CALL) {
//     return;
//   } else if (inst->itype() == IR::JMP) {
//     return;
//   } else if (inst->itype() == IR::BR) {
//     return;
//   } else if (inst->itype() == IR::RET) {
//     return;
//   // } else if (inst->itype() == IR::GALLOCA) {
//   //   return;
//   } else if (inst->itype() == IR::ALLOCA) {
//     return;
//   } else if (inst->itype() == IR::LOAD) {
//     return;
//   } else if (inst->itype() == IR::STORE) {
//     return;
//   } else if (inst->itype() == IR::GPTR) {
//     info << IDENT << "lw "
//          << "t0, " << asm_var_tab[inst->operand(0)->name()] << "(fp)" << '\n';
//     info << IDENT << "slli " << "t1, t0, 2" << '\n';
//     info << IDENT << "addi t0, fp, " << asm_var_tab[inst->name()] << '\n';
//     info << IDENT << "add t0, t0, t1" << '\n';
//     info << IDENT << "sd t0\n";
//     // info << IDENT << "ld " << "t0, " << asm_var_tab[inst->name()] << "(fp)"
//     // << '\n'; info << IDENT << "addi " << "t1, " << "t0, " <<
//     // inst->type()->deref(1)->sizeOf() << '\n'; info << IDENT << "sd " << "t1,
//     // " << asm_var_tab[inst->name()] << "(fp)" << '\n';
//     return;
//   } else if (inst->itype() == IR::GEPTR) {
//     return;
//   }
// }
