#ifndef RISCV_H
#define RISCV_H

#include "backend_utils.h"
#include "emit_vi.h"
#include "build_asm.hpp"
#include <chrono>
#include <ctime>

#define ASM_FUNC_PASS(pass_name) void pass_name(Func_Asm *func_asm)
typedef ASM_FUNC_PASS(Asm_Func_Pass);

#define ASM_PASS(pass_name) void pass_name(Program_Asm *program_asm)
typedef ASM_PASS(Asm_Pass);

inline void run_on_every_function(Program_Asm *program_asm, Asm_Func_Pass pass);

Program_Asm *emit_vis(Module program_IR);
void backendOpt(Program_Asm *program_asm, bool opt = false);
void build_program_asm(Asm_Buffer *s, Program_Asm *pro, vector<VariablePtr> & globalVariables);

void replace_uses(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func);
void replace_defs(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func);
void s_replace_uses(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func);
void s_replace_defs(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func);
vector<VOper> get_defs(VI *I);
vector<VOper> get_uses(VI *I);
vector<VOper> s_get_defs(VI *I);
vector<VOper> s_get_uses(VI *I);
vector<VOper *> get_defs_ptr(VI *I);
vector<VOper *> get_uses_ptr(VI *I);
vector<VOper *> s_get_defs_ptr(VI *I);
vector<VOper *> s_get_uses_ptr(VI *I);

void replace_operand_by_operand(VOper *opr, VOper *new_opr, Func_Asm *func);
void delete_user(VOper *opr, Func_Asm *func);

bool rm_instruction_use(VI* I, VOper *v, Func_Asm *func);
bool s_rm_instruction_use(VI* I, VOper *v, Func_Asm *func);
void clear_use_head_map();
void clear_def_I_map();
std::string get_current_file_directory();

#endif
