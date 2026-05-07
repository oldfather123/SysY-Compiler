#ifndef TCA_IR_H
#define TCA_IR_H

#include <string>
#include <vector>

namespace ir {
    enum class Type { Int, Float, IntLiteral, FloatLiteral, IntPtr, FloatPtr, null };
    std::string toString(Type t);

    enum class Operator { _return, _goto, call, alloc, store, load, getptr, def, fdef, mov, fmov, cvt_i2f, cvt_f2i, add, addi, fadd, sub, subi, fsub, mul, fmul, div, fdiv, mod, lss, flss, leq, fleq, gtr, fgtr, geq, fgeq, eq, feq, neq, fneq, _not, _and, _or, __unuse__ };
    std::string toString(Operator t);

    struct Operand {
        std::string name;
        Type type;
        Operand(std::string name = "null", Type type = Type::null);
    };

    struct Instruction {
        Operand op1;
        Operand op2;
        Operand des;
        Operator op;

        Instruction();
        Instruction(const Operand& op1, const Operand& op2, const Operand& des, const Operator& op);
        virtual std::string draw() const;
    };

    struct CallInst : public Instruction {
        std::vector<Operand> argumentList;
        CallInst(const Operand& op1, std::vector<Operand> paraList, const Operand& des);
        CallInst(const Operand& op1, const Operand& des);  // 无参数情况
        std::string draw() const override;
    };

    struct Function {
        std::string name;
        Type returnType;
        std::vector<Operand> ParameterList;
        std::vector<Instruction*> InstVec;

        Function();
        Function(const std::string& name, const Type& returnType);
        Function(const std::string& name, const std::vector<Operand>& pl, const Type& returnType);

        void addInst(Instruction* inst);
        std::string draw();
    };

    struct GlobalVal {
        Operand val;
        int maxlen = 0;
        GlobalVal(Operand va);
        GlobalVal(Operand va, int len);
    };

    struct Program {
        std::vector<Function> functions;
        std::vector<GlobalVal> globalVal;

        Program();
        void addFunction(const Function& proc);
        std::string draw();
    };

}  // namespace ir

#endif  // TCA_IR_H