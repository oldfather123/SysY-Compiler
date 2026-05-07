#pragma once

#include <functional>
#include <map>
#include <sstream>
#include <unordered_map>
#include <vector>
#include <memory>
#include <set>
#include <iostream>

#include "../ir/ir.hpp"
#include "arch.hpp"
#include "register_allocator.hpp"
#include "../parser/SyntaxTree.hpp"

namespace LoongArch {


class Program;
class Function;
class Block;
class ProgramBuilder;
class IrMapping;
struct Inst;
struct Jump;
struct Br;
struct Bl;
struct RegImmInst;


class Program {
    friend LoongArch::ProgramBuilder;
protected:
    std::stringstream asm_code;
    std::vector<std::shared_ptr<Function>> functions;
    std::vector<std::shared_ptr<Function>> lib_funcs;
    ptr_list<ir::global_def> global_var;
    std::vector<string> float_nums;
    int block_n;
public:
    Program();
    void get_asm(std::ostream &out);
    void global_ini(ptr<int> pointer, ptr_list<ir::ir_value> init_val, ptr_list<ast::expr_syntax> dimensions, std::ostream& out);
};

class Function : public std::enable_shared_from_this<Function> {
    friend LoongArch::ProgramBuilder;
    friend LoongArch::Bl;
protected:
    int regn; 
    std::string name;
    std::shared_ptr<LoongArch::Block> entry,ret_block;
    std::vector<std::shared_ptr<LoongArch::Block>> blocks;
    std::vector<std::shared_ptr<LoongArch::Block>> waiting_blocks;
    std::stringstream asm_code;
    int stack_size;                                         //用来存储使用栈的字节数
    int cur_pos;                                            //用来存储当前的变量的栈帧
    int bitalign(int value, int align);                     //用来进行字节对齐
public:
    Function(std::string name);
    void get_asm(std::ostream& out);
    string get_name();
};

class Block {
    friend LoongArch::ProgramBuilder;
    friend LoongArch::Jump;
    friend LoongArch::Br;
protected:
    bool label_used;
    std::string name;
    std::list<std::shared_ptr<LoongArch::Inst>> instructions;
public:
    Block(std::string name);
    void insert_before_jump(std::shared_ptr<Inst> inst);  //用来在跳转指令的前面添加一条别的指令
    void get_asm(std::ostream &out);
};

class IrMapping //关键，用来完成中间语言到具体的架构的映射
{
    friend LoongArch::ProgramBuilder;
    int regn;
protected:
    //完成block之间的映射
    std::unordered_map<std::shared_ptr<ir::ir_basicblock>,std::shared_ptr<LoongArch::Block>> blockmapping;
    //block之间的逆映射
    std::unordered_map<std::shared_ptr<LoongArch::Block>,std::shared_ptr<ir::ir_basicblock>> rev_blockmapping;
    //中端reg到后端reg的映射，寄存器分配之后关系可以放在这里
    std::unordered_map<int,LoongArch::Reg> reg_mapping;
    //内存变量和栈中的偏移之间的映射
    std::unordered_map<std::shared_ptr<ir::ir_memobj>,int> objoffset_mapping;
    //中端地址寄存器和内存变量之间的映射
    std::unordered_map<std::shared_ptr<ir::ir_reg>,std::shared_ptr<ir::ir_memobj>> addrMemObj;
    //上面的逆映射
    std::unordered_map<std::shared_ptr<ir::ir_memobj>,std::shared_ptr<ir::ir_reg>> revAddMemObj;

    // std::vector<std::shared_ptr<ir::ir_reg>> spill_vec;
    // std::vector<ir::ir_reg> spill_vec;
    std::vector<int> spill_vec;
    std::unordered_map<int, LoongArch::Reg> spill_mapping;
    // ptr_list<ir::ir_memobj> mem_var;
    std::unordered_map<int, int> mem_var;
    std::unordered_map<int, int> spill_offset;
    int used_mem = 0;
    int call_mem = 0;
public:
    //申请一个被预留的寄存器
    Reg new_reg();
    //对寄存器进行转换
    Reg transfer_reg(ir::ir_reg irReg);
    IrMapping();
};



} // namespace ArchLA
