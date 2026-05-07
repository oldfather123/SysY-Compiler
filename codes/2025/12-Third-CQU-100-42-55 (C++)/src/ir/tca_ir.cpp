#include "./tca_ir.h"

#include <iostream>

namespace ir {

    std::string toString(Type t) {
        switch (t) {
        case Type::Int:
            return "int";
        case Type::IntPtr:
            return "int*";
        case Type::Float:
            return "float";
        case Type::FloatPtr:
            return "float*";
        case Type::null:
            return "void";
        case Type::FloatLiteral:
            return "floatLiteral";
        case Type::IntLiteral:
            return "intLiteral";
        default:
            return "TypeError:" + std::to_string(static_cast<int>(t));
        }
    }
    std::string toString(Operator t) {
        switch (t) {
        case Operator::_return:
            return "return";
        case Operator::_goto:
            return "goto";
        case Operator::call:
            return "call";
        case Operator::alloc:
            return "alloc";
        case Operator::store:
            return "store";
        case Operator::getptr:
            return "getptr";
        case Operator::load:
            return "load";
        case Operator::def:
            return "def";
        case Operator::fdef:
            return "fdef";
        case Operator::mov:
            return "mov";
        case Operator::fmov:
            return "fmov";
        case Operator::add:
            return "add";
        case Operator::addi:
            return "addi";
        case Operator::fadd:
            return "fadd";
        case Operator::sub:
            return "sub";
        case Operator::subi:
            return "subi";
        case Operator::fsub:
            return "fsub";
        case Operator::mul:
            return "mul";
        case Operator::fmul:
            return "fmul";
        case Operator::div:
            return "div";
        case Operator::fdiv:
            return "fdiv";
        case Operator::mod:
            return "mod";
        case Operator::lss:
            return "lss";
        case Operator::flss:
            return "flss";
        case Operator::leq:
            return "leq";
        case Operator::fleq:
            return "fleq";
        case Operator::gtr:
            return "gtr";
        case Operator::fgtr:
            return "fgtr";
        case Operator::geq:
            return "geq";
        case Operator::fgeq:
            return "fgeq";
        case Operator::eq:
            return "eq";
        case Operator::feq:
            return "feq";
        case Operator::neq:
            return "neq";
        case Operator::fneq:
            return "fneq";
        case Operator::_not:
            return "not";
        case Operator::_or:
            return "or";
        case Operator::_and:
            return "and";
        case Operator::cvt_i2f:
            return "cvt_i2f";
        case Operator::cvt_f2i:
            return "cvt_f2i";
        case Operator::__unuse__:
            return "__unuse__";
        default:
            return "OpError:" + std::to_string(static_cast<int>(t));
        }
    }

    Operand::Operand(std::string n, Type t) : name(std::move(n)), type(t) {}

    Instruction::Instruction() = default;

    Instruction::Instruction(const Operand& op1, const Operand& op2, const Operand& des, const Operator& op) : op1(op1), op2(op2), des(des), op(op) {}

    CallInst::CallInst(const Operand& op1, std::vector<Operand> paraList, const Operand& des) : Instruction(op1, Operand(), des, Operator::call), argumentList(std::move(paraList)) {}

    CallInst::CallInst(const Operand& op1, const Operand& des) : Instruction(op1, Operand(), des, Operator::call) {}

    std::string CallInst::draw() const {
        std::string res = "call " + des.name + ", " + op1.name + "(";
        for (const auto& arg : argumentList) res += arg.name + ", ";
        if (!argumentList.empty())
            res.erase(res.size() - 2);
        return res + ")";
    }

    std::string Instruction::draw() const {
        switch (op) {
        case Operator::_return:
            return "return " + op1.name;
        case Operator::_goto:
            return (op1.name != "null") ? "if " + op1.name + " goto [pc, " + des.name + "]" : "goto [pc, " + des.name + "]";
        case Operator::alloc:
            return "alloc " + des.name + ", " + op1.name;
        case Operator::mov:
            return "mov " + des.name + ", " + op1.name;
        case Operator::fmov:
            return "fmov " + des.name + ", " + op1.name;
        case Operator::cvt_f2i:
            return "cvt_f2i " + des.name + ", " + op1.name;
        case Operator::cvt_i2f:
            return "cvt_i2f " + des.name + ", " + op1.name;
        case Operator::def:
            return "def " + des.name + ", " + op1.name;
        case Operator::fdef:
            return "fdef " + des.name + ", " + op1.name;
        case Operator::_not:
            return "not " + des.name + ", " + op1.name;
        default:
            return toString(op) + " " + des.name + ", " + op1.name + ", " + op2.name;
        }
    }

    Function::Function() : name("null"), returnType(Type::null) {}

    Function::Function(const std::string& n, const Type& type) : name(n), returnType(type) {}

    Function::Function(const std::string& n, const std::vector<Operand>& pl, const Type& type) : name(n), returnType(type), ParameterList(pl) {}

    void Function::addInst(Instruction* inst) { InstVec.push_back(inst); }

    std::string Function::draw() {
        std::string res = toString(returnType) + " " + name + "(";
        for (const auto& it : ParameterList) res += toString(it.type) + " " + it.name + ",";
        if (!ParameterList.empty())
            res.pop_back();  // 移除末尾逗号
        res += ")\n";

        for (size_t i = 0; i < InstVec.size(); ++i) res += "\t" + std::to_string(i) + ": " + InstVec[i]->draw() + "\n";
        return res + "end\n\n";
    }

    GlobalVal::GlobalVal(Operand va) : val(std::move(va)) {}

    GlobalVal::GlobalVal(Operand va, int len) : val(std::move(va)), maxlen(len) {}

    Program::Program() = default;

    void Program::addFunction(const Function& proc) { functions.push_back(proc); }

    std::string Program::draw() {
        std::string ret;
        for (auto& func : functions) ret += func.draw();
        ret += "GVT:\n";
        for (auto& gval : globalVal) {
            ret += "\t" + gval.val.name + " " + toString(gval.val.type) + " " + std::to_string(gval.maxlen) + "\n";
        }
        return ret;
    }

}  // namespace ir