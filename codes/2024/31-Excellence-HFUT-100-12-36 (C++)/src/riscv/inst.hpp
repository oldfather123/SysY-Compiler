#pragma once

// #include <cassert>
#include <cstdint>
#include <iostream>
// #include <list>
#include <map>
// #include <memory>
#include <set>
// #include <sstream>
#include <vector>

#include "arch.hpp"
#include "code_gen.hpp"
#include "../ir/ir.hpp"



namespace LoongArch {
class RookieAllocator;
inline std::ostream &operator<<(std::ostream &os, const Reg &reg) {
  if(reg.is_float()) {
    os << "f" << reg.id;
  }
  // else if(reg.type == FBOOL) {
  //   os << "f" << reg.id;
  // }
  else {
    os << "x" << reg.id;
  }
  return os;
}

class AsmContext;

struct Inst {
  virtual ~Inst() = default;

  virtual std::vector<Reg> def_reg() { return {}; }
  virtual std::vector<Reg> use_reg() { return {}; }
  virtual std::vector<Reg *> regs() { return {}; }
  virtual bool side_effect() {
    return false;
  } 
  virtual void gen_asm(std::ostream &out) = 0;  //生成汇编
  virtual void print(std::ostream &out) { gen_asm(out); } //打印

  template <class T> T *as() {
    return dynamic_cast<T *>(this);
  }
  void update_live(std::set<Reg> &live) {
    for (Reg i : def_reg())
      if (i.is_virtual() || integer_allocable(i.id)) live.erase(i);
    for (Reg i : use_reg())
      if (i.is_virtual() || integer_allocable(i.id)) live.insert(i);
  }
  bool def(Reg reg) {
    for (Reg r : def_reg())
      if (r == reg) return true;
    return false;
  }
  bool use(Reg reg) {
    for (Reg r : use_reg())
      if (r == reg) return true;
    return false;
  }
  bool relate(Reg reg) { return def(reg) || use(reg); }
  void replace_reg(Reg before, Reg after) {
    for (Reg *i : regs())
      if ((*i).id == before.id) (*i) = after;
  }

  LoongArch::Reg big_imm = LoongArch::Reg{31};
};

//modified
struct RegRegInst : Inst {      //R-Type Instruction like add r1, r2 ——> r3
  enum Type {
    add_w,add_d, fadd_f,
    sub_w,sub_d, fsub_f,
    mul_w, mul_d, fmul_f,
    div_w, fdiv_f,

    andw,orw,
    slt
    , mod_w, fmod_f                 // 我加一个：取余
  } op;
  Reg dst, lhs, rhs;
  RegRegInst(Type _op, Reg _dst, Reg _lhs, Reg _rhs)
      :  dst(_dst), lhs(_lhs), rhs(_rhs), op(_op) 
      {
      }

  virtual std::vector<Reg> def_reg() override { return {dst}; }
  virtual std::vector<Reg> use_reg() override { return {lhs, rhs}; }
  virtual std::vector<Reg *> regs() override { return {&dst, &lhs, &rhs}; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name
    {
        //Integer
        {add_w, "add"}, {add_d, "add"}, {fadd_f, "fadd.s"}, {sub_w, "sub"}, {fsub_f, "fsub.s"}, {mul_w, "mul"}, {fmul_f, "fmul.s"},  {div_w, "div"},  {fdiv_f, "fdiv.s"},{andw, "and"}, {orw, "or"},
        {slt, "slt"}, {mul_d, "mul"}, {sub_d, "sub"}
        ,{mod_w, "remw"}
    };
    out << asm_name.find(op)->second << ' ' << dst << ", " << lhs << ", " << rhs<< '\n';
  }
};

struct RegImmInst : Inst {
  enum Type { 
    addi_d , addi_w , slli_w, slli_d, srli, srai, andi, ori, slti
    , xori
    , sltui
  } op;
  Reg dst, lhs;
  int32_t rhs;
  RegImmInst(Type _op, Reg _dst, Reg _lhs, int32_t _rhs)
      : op(_op), dst(_dst), lhs(_lhs), rhs(_rhs) {
  }

  virtual std::vector<Reg> def_reg() override { return {dst}; }
  virtual std::vector<Reg> use_reg() override { return {lhs}; }
  virtual std::vector<Reg *> regs() override { return {&dst, &lhs}; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name 
    {
        {addi_d, "addi"},{addi_w, "addi"}, {andi, "andi"}, {ori, "ori"},
        {slti, "slti"}, {slli_w, "slli"}, {slli_d, "slli"}
        , {xori, "xori"}, {sltui, "sltui"}
    };
    static const std::map<Type, string> rr_name {
      {addi_d, "add"},{addi_w, "add"}, {andi, "and"}, {ori, "or"},
      {slti, "slt"}, {slli_w, "sll"}, {slli_d, "sll"},
      {xori, "xor"}, {sltui, "slt"}
    };
      if((uint32_t)rhs >> 11) {
        // out << "lu12i.w " << big_imm << ", " << rhs << ">>12\n\t";
        // out << "ori " << big_imm << ", " << big_imm << ", " << (rhs & 0xFFF) << "\n\t";
        // out << "addi.d " << big << ", " << base << ", " << big << "\n\t";
        out << "la " << big_imm << ", " << rhs << "\n\t";
        out << rr_name.find(op)->second << ' ' << dst << ", " << lhs << ", " << big_imm<< '\n';
      }
      else
        out << asm_name.find(op)->second << ' ' << dst << ", " << lhs << ", " << rhs<< '\n';
      
  }
};

struct LoadImm : Inst { //用于将立即数加载到寄存器的汇编代码。
  Reg dst;
  int32_t value;
  LoadImm(Reg _dst, int32_t _value) : dst(_dst), value(_value) {}

  virtual std::vector<Reg> def_reg() override { return {dst}; }
  virtual std::vector<Reg *> regs() override { return {&dst}; }
  virtual void gen_asm(std::ostream &out) override {
    // out << "ori " << dst << ", " << "$r0" << ", " << value << '\n';        // ori无法加载(unsigned)-1这类在无符号上过大的数
    // uint32_t u_value = (uint32_t)value;
    // if(u_value >> 11) {
      // int32_t low = value & 0xFFF;
      // std::stringstream high;
      // high << std::hex << std::showbase << (u_value >> 11);
      // out << "lu12i.w " << dst << ", " << value << ">>12\n\t";
      // out << "ori " << dst << ", " << dst << ", " << low << '\n';
      out << "la " << dst << ", " << value << '\n';
    // }
    // else {
      // out << "ori " << dst << ", " << "$r0" << ", " << value << '\n';
    // }
  }
};

struct LoadFloat: Inst {
  Reg dst;
  int value_at;
  LoadFloat(Reg _dst, int _value) : dst(_dst), value_at(_value) {
    assert(dst.is_float());
  }

  virtual std::vector<Reg> def_reg() override { return {dst}; }
  virtual std::vector<Reg *> regs() override { return {&dst}; }
  virtual void gen_asm(std::ostream &out) override {
    out << "lui " << big_imm << ", %hi(.LC" << value_at << ")\n";
    out << "\tflw " << dst << ", %lo(.LC" << value_at << ")(" << big_imm << ")\n";           // TODO
  }
};

struct Jump : Inst {  //用于生成无条件跳转指令的汇编代码。
  LoongArch::Block *target;
  Jump(Block *_target) : target(_target) { target->label_used = true; }

  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    out << "j " << target->name << '\n';
  }
};

struct Br : Inst {
  enum Type {
    beqz, bnez, bceqz, bcnez
  };
  Type op;
  LoongArch::Reg cond;
  LoongArch::Block *target;
  Br(Type _op, Reg _cond, Block *_target) : op(_op), cond(_cond), target(_target) { target->label_used = true; }

  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    // static const std::map<Type, std::string> asm_name {
    //   {beqz, "beqz"}, {bnez, "bnez"}, {bceqz, "bceqz"}, {bcnez, "bcnez"}
    // };
    // auto what = asm_name.find(op)->second;
    // out << asm_name.find(op)->second << " " << cond << ", " << target->name << '\n';
    switch(op) {
      case bceqz:
      case beqz: {
        out << "beq " << cond << ", zero, " << target->name << "\n";
        break;
      }
      case bcnez:
      case bnez: {
        out << "bne " << cond << ", zero, " << target->name << "\n";
        break;
      }
    };
    // auto what = asm_name.find(op)->second;
    // out << asm_name.find(op)->second << " " << cond << ", " << target->name << '\n';
  }
};

struct Bl : Inst {
  string name;
  Bl(string name) : name(name) {}

  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    out << "jal " << name << '\n';
  }
};

struct jr : Inst { //用于生成函数返回指令的汇编代码。
  bool has_return_value;
  jr(bool _has_return_value) : has_return_value(_has_return_value) {}

  virtual std::vector<Reg> use_reg() override {
    if (has_return_value)
      return {Reg{ra}};
    else
      return {};
  }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    // ctx->epilogue(out);
    out << "jr\t" << "ra" << '\n'; 
  }
  virtual void print(std::ostream &out) override { out << "jr\t" << '$' << "ra"; out << "ret\n"; }
};


struct st : Inst {
  enum Type {
    st_d,
    st_w,
    fst_f,
    fst_d
  } op;
  Reg src, base;
  int32_t offset;
  st(Reg _src, Reg _base, int32_t _offset, Type _op = st_d)
      : op(_op), src(_src), base(_base), offset(_offset) {
    // assert(is_imm12(offset));
  }

  virtual std::vector<Reg> use_reg() override { return {src, base}; }
  virtual std::vector<Reg *> regs() override { return {&src, &base}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name {
        {st_d, "sd"}, {st_w, "sw"}, {fst_f, "fsw"}, {fst_d, "fsd"}
    };
      if((uint32_t)offset >> 11) {
        // out << "lu12i.w " << big_imm << ", " << offset << ">>12\n\t";
        // out << "ori " << big_imm << ", " << big_imm << ", " << (offset & 0xFFF) << "\n\t";
        // out << "add.d " << big_imm << ", " << base << ", " << big_imm << "\n\t";
        // out << asm_name.find(op)->second << ' ' << src << ", " << big_imm <<  ", 0" <<"\n";
        out << "la " << big_imm << ", " << offset << "\n\t";
        out << "add " << big_imm << ", " << base << ", " << big_imm << "\n\t";
        out << asm_name.find(op)->second << " " << src << ", 0(" << big_imm << ")\n";
      }
      else
        out << asm_name.find(op)->second << " " << src << ", " << offset << "(" << base << ")\n";
  }
};

struct ld : Inst {
  enum Type {
    ld_d,
    ld_w,
    fld_f,
    fld_d
  } op;
  Reg src, base;
  int32_t offset;
  ld(Reg _src, Reg _base, int32_t _offset, Type _op = ld_d)
      : op(_op), src(_src), base(_base), offset(_offset) {
    // assert(is_imm12(offset));
  }

  virtual std::vector<Reg> use_reg() override { return {src, base}; }
  virtual std::vector<Reg *> regs() override { return {&src, &base}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name {
        {ld_d, "ld"}, {ld_w, "lw"}, {fld_f, "flw"}, {fld_d, "fld"}
    };
      if((uint32_t)offset >> 11) {
        // out << "lu12i.w " << big_imm << ", " << offset << ">>12\n\t";
        // out << "ori " << big_imm << ", " << big_imm << ", " << (offset & 0xFFF) << "\n\t";
        // out << "add.d " << big_imm << ", " << base << ", " << big_imm << "\n\t";
        // out << asm_name.find(op)->second << ' ' << src << ", " << big_imm <<  ", 0" <<"\n";
        out << "la " << big_imm << ", " << offset << "\n\t";
        out << "add " << big_imm << ", " << base << ", " << big_imm << "\n\t";
        out << asm_name.find(op)->second << " " << src << ", 0(" << big_imm << ")\n";
      }
      else
        out << asm_name.find(op)->second << " " << src << ", " << offset << "(" << base << ")\n";
  }
};

struct ldptr : Inst {
  enum Type {
    ld_d,
    ld_w,
    fld_f,
    fld_d
  } op;
  Reg src, base;
  int32_t offset;
  ldptr(Reg _src, Reg _base, int32_t _offset, Type _op = ld_d)
      : op(_op), src(_src), base(_base), offset(_offset) {
    // assert(is_imm12(offset));
  }

  virtual std::vector<Reg> use_reg() override { return {src, base}; }
  virtual std::vector<Reg *> regs() override { return {&src, &base}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    // static const std::map<Type, std::string> asm_name {
    //     {ld_d, "ldptr.d"}, {ld_w, "ldptr.w"}, {fld_f, "fld.s"}, {fld_d, "fld.s"}
    // };
    //   if((uint32_t)offset >> 11) {
    //     out << "lu12i.w " << big_imm << ", " << offset << ">>12\n\t";
    //     out << "ori " << big_imm << ", " << big_imm << ", " << (offset & 0xFFF) << "\n\t";
    //     out << "add.d " << big_imm << ", " << base << ", " << big_imm << "\n\t";
    //     out << asm_name.find(op)->second << ' ' << src << ", " << big_imm <<  ", 0" <<"\n";
    //   }
    //   else
    //     out << asm_name.find(op)->second << ' ' << src << ", "  << base << "," << this->offset << "\n";
    static const std::map<Type, std::string> asm_name {
        {ld_d, "ld"}, {ld_w, "lw"}, {fld_f, "flw"}, {fld_d, "fld"}
    };
      if((uint32_t)offset >> 11) {
        // out << "lu12i.w " << big_imm << ", " << offset << ">>12\n\t";
        // out << "ori " << big_imm << ", " << big_imm << ", " << (offset & 0xFFF) << "\n\t";
        // out << "add.d " << big_imm << ", " << base << ", " << big_imm << "\n\t";
        // out << asm_name.find(op)->second << ' ' << src << ", " << big_imm <<  ", 0" <<"\n";
        out << "la " << big_imm << ", " << offset << "\n\t";
        out << "add " << big_imm << ", " << base << ", " << big_imm << "\n\t";
        out << asm_name.find(op)->second << " " << src << ", 0(" << big_imm << ")\n";
      }
      else
        out << asm_name.find(op)->second << " " << src << ", " << offset << "(" << base << ")\n";
  }
};

struct stptr : Inst {
  enum Type {
    st_d,
    st_w,
    fst_f,
    fst_d
  } op;
  Reg src, base;
  int32_t offset;
  stptr(Reg _src, Reg _base, int32_t _offset, Type _op = st_d)
      : op(_op), src(_src), base(_base), offset(_offset) {
    // assert(is_imm12(offset));
  }

  virtual std::vector<Reg> use_reg() override { return {src, base}; }
  virtual std::vector<Reg *> regs() override { return {&src, &base}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    // static const std::map<Type, std::string> asm_name {
    //     {st_d, "stptr.d"}, {st_w, "stptr.w"}, {fst_f, "fst.s"}, {fst_d, "fst.s"}
    // };
    //   if((uint32_t)offset >> 11) {
    //     out << "lu12i.w " << big_imm << ", " << offset << ">>12\n\t";
    //     out << "ori " << big_imm << ", " << big_imm << ", " << (offset & 0xFFF) << "\n\t";
    //     out << "add.d " << big_imm << ", " << base << ", " << big_imm << "\n\t";
    //     out << asm_name.find(op)->second << ' ' << src << ", " << big_imm <<  ", 0" <<"\n";
    //   }
    //   else
    //     out << asm_name.find(op)->second << ' ' << src << ", "  << base << "," << this->offset << "\n";
    static const std::map<Type, std::string> asm_name {
        {st_d, "sd"}, {st_w, "sw"}, {fst_f, "fsw"}, {fst_d, "fsd"}
    };
      if((uint32_t)offset >> 11) {
        // out << "lu12i.w " << big_imm << ", " << offset << ">>12\n\t";
        // out << "ori " << big_imm << ", " << big_imm << ", " << (offset & 0xFFF) << "\n\t";
        // out << "add.d " << big_imm << ", " << base << ", " << big_imm << "\n\t";
        // out << asm_name.find(op)->second << ' ' << src << ", " << big_imm <<  ", 0" <<"\n";
        out << "la " << big_imm << ", " << offset << "\n\t";
        out << "add " << big_imm << ", " << base << ", " << big_imm << "\n\t";
        out << asm_name.find(op)->second << " " << src << ", 0(" << big_imm << ")\n";
      }
      else
        out << asm_name.find(op)->second << " " << src << ", " << offset << "(" << base << ")\n";
  }
};

struct ldglob : Inst {
  // enum Type {
  //   sti,
  //   stf
  // } op;
  ptr<ir::ir_reg> src;
  Reg dst;
  ldglob(ptr<ir::ir_reg> _src, Reg dst)
      : src(_src), dst(dst) {
    // assert(is_imm12(offset));
  }

  virtual std::vector<Reg> use_reg() override { return {dst}; }
  virtual std::vector<Reg *> regs() override { return {&dst}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    out << "lui " << big_imm << ", %hi(" << src->get_name() << ")\n\t";
    if(dst.is_float()) {
      out << "flw " << dst << ", %lo(" << src->get_name() << ")(" << big_imm << ")\n";
    }
    else {
      out << "lw " << dst << ", %lo(" << src->get_name() << ")(" << big_imm << ")\n";
    }
  }
};

struct stglob : Inst {
  // enum Type {
  //   sti,
  //   stf
  // } op;
  ptr<ir::ir_reg> src;
  Reg dst;
  stglob(ptr<ir::ir_reg> _src, Reg dst)
      : src(_src), dst(dst) {
    // assert(is_imm12(offset));
  }

  virtual std::vector<Reg> use_reg() override { return {dst}; }
  virtual std::vector<Reg *> regs() override { return {&dst}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    out << "lui " << big_imm << ", %hi(" << src->get_name() << ")\n\t";
    if(dst.is_float()) {
      out << "fsw " << dst << ", %lo(" << src->get_name() << ")(" << big_imm << ")\n";
    }
    else {
      out << "sw " << dst << ", %lo(" << src->get_name() << ")(" << big_imm << ")\n";
    }
  }
};

struct la : Inst {
  enum Type {
    local,
  } op;
  Reg dst;
  ptr<ir::ir_reg> src;
  la(ptr<ir::ir_reg> src, Reg dst, Type op = local) : src(src), dst(dst), op(op) {}
  virtual std::vector<Reg> use_reg() override { return {dst}; }
  virtual std::vector<Reg *> regs() override { return {&dst}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name {
        {local, "la"},
    };
      out << asm_name.find(op)->second << ' ' << dst << ", "  << src->get_name() << "\n";
  }
};

struct CMP : Inst {
    enum Type {
    eq, ne,
    gt, ge,
    lt, le
  } op;
  Reg dst, src1, src2;
  CMP(Reg dst, Reg src1, Reg src2, Type op) : dst(dst), src1(src1), src2(src2), op(op) {}
  virtual std::vector<Reg> use_reg() override { return {src1, src2}; }
  virtual std::vector<Reg *> regs() override { return {&src1, &src2}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name {
        {eq, "sub"}, {ne, "sub"}, {gt, "sgt"}, {ge, "slt"}, {lt, "slt"}, {le, "sgt"}
    };
      out << "sext.w " << src1 << ", " << src1 << "\n\t";
      out << "sext.w " << src2 << ", " << src2 << "\n\t";
      out << asm_name.find(op)->second << ' ' << dst << ", " << src1 << ", "  << src2 << "\n";
      if(op == eq || op == ge || op == le) {
        out << "\tseqz " << dst << ", " << dst << "\n";
      }
      else if(op == ne) {
        out << "\tsnez " << dst << ", " << dst << "\n";
      }
  }
};

struct FCMP : Inst {
    enum Type {
    eq, ne,
    gt, ge,
    lt, le
  } op;
  Reg dst, src1, src2;
  FCMP(Reg dst, Reg src1, Reg src2, Type op) : dst(dst), src1(src1), src2(src2), op(op) {}
  virtual std::vector<Reg> use_reg() override { return {src1, src2}; }
  virtual std::vector<Reg *> regs() override { return {&src1, &src2}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name {
        {eq, "feq.s"}, {ne, "feq.s"}, {gt, "fgt.s"}, {ge, "fge.s"}, {lt, "flt.s"}, {le, "fle.s"}
    };
      out << asm_name.find(op)->second << ' ' << dst << ", " << src1 << ", "  << src2 << "\n";
      if(op == ne) {
        out << "\tseqz " << dst << ", " << dst << "\n";           // CHECK
      }
  }
};

struct trans : Inst {
    enum Type {
    fti, itf
  } op;
  Reg dst, src;
  trans(Reg dst, Reg src, Type op) : dst(dst), src(src), op(op) {}
  virtual std::vector<Reg> use_reg() override { return {src}; }
  virtual std::vector<Reg *> regs() override { return {&src}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name {
        {fti, "fcvt.w.s"}, {itf, "fcvt.s.w"}
    };
      out << asm_name.find(op)->second << " " << dst << ", " << src << (op == fti ? ", rtz" : "") << "\n";
  }
};

struct mov : Inst {
    enum Type {
    ftg, gtf, ftf_f
  } op;
  Reg dst, src;
  mov(Reg dst, Reg src, Type op) : dst(dst), src(src), op(op) {}
  virtual std::vector<Reg> use_reg() override { return {src}; }
  virtual std::vector<Reg *> regs() override { return {&src}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name {
        {ftg, "fmv.x.s"}, {gtf, "fmv.s.x"}, {ftf_f, "fmv.s"}
    };
      out << asm_name.find(op)->second << ' ' << dst << ", " << src << "\n";
  }
};

struct funary : Inst {
    enum Type {
    neg_f
  } op;
  Reg dst, src;
  funary(Reg dst, Reg src, Type op) : dst(dst), src(src), op(op) {}
  virtual std::vector<Reg> use_reg() override { return {src}; }
  virtual std::vector<Reg *> regs() override { return {&src}; }
  virtual bool side_effect() override { return true; }
  virtual void gen_asm(std::ostream &out) override {
    static const std::map<Type, std::string> asm_name {
        {neg_f, "fneg.s"}
    };
      out << asm_name.find(op)->second << ' ' << dst << ", " << src << "\n";
  }
};


}  // namespace Archriscv
