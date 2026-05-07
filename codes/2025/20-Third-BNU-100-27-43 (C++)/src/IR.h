#ifndef IR_STRUCTURE_H
#define IR_STRUCTURE_H

#include <iostream>
#include <string>
#include <vector>
#include <list>
#include <memory>
#include <variant>
#include <map>
#include <optional>
#include <utility>

namespace MyIR
{

    class Type;
    class Value;
    class User;
    class Instruction;
    class BasicBlock;
    class Function;
    class Constant;
    class GlobalVariable;
    class Argument;

    enum class TypeID
    {
        Void,
        Label,
        Integer,
        Float,
        Pointer,
        Array,
        Function
    };

    class Type
    {
    public:
        virtual ~Type() = default;
        TypeID get_type_id() const { return id; }

        static bool is_integer_ty(const Type *ty) { return ty && ty->get_type_id() == TypeID::Integer; }
        static bool is_float_ty(const Type *ty) { return ty && ty->get_type_id() == TypeID::Float; }
        static bool is_pointer_ty(const Type *ty) { return ty && ty->get_type_id() == TypeID::Pointer; }
        static bool is_array_ty(const Type *ty) { return ty && ty->get_type_id() == TypeID::Array; }
        static bool is_function_ty(const Type *ty) { return ty && ty->get_type_id() == TypeID::Function; }
        virtual size_t get_size() const = 0;
        // virtual std::shared_ptr<Type> get_element_type() const;
    protected:
        Type(TypeID id) : id(id) {}

    private:
        TypeID id;
    };

    class IntegerType : public Type
    {
    public:
        explicit IntegerType(unsigned bitwidth) : Type(TypeID::Integer), bitwidth(bitwidth) {}
        unsigned get_bitwidth() const { return bitwidth; }
        size_t get_size() const override { return (bitwidth + 7) / 8; } // 向上取整
    private:
        unsigned bitwidth;
    };

    class FloatType : public Type
    {
    public:
        FloatType() : Type(TypeID::Float) {}
        size_t get_size() const override { return sizeof(float); }
    };

    class VoidType : public Type
    {
    public:
        VoidType() : Type(TypeID::Void) {}
        size_t get_size() const override { return 0; }
    };

    class LabelType : public Type
    {
    public:
        LabelType() : Type(TypeID::Label) {}
        size_t get_size() const override { return 0; }
    };

    class PointerType : public Type
    {
    public:
        explicit PointerType(std::shared_ptr<Type> element_type) : Type(TypeID::Pointer), element_type(std::move(element_type)) {}
        std::shared_ptr<Type> get_element_type() const { return element_type; }
        size_t get_size() const override { return sizeof(void*); } // 假设指针大小为平台默认大小

    private:
        std::shared_ptr<Type> element_type;
    };

    class ArrayType : public Type
    {
    public:
        ArrayType(std::shared_ptr<Type> element_type, size_t num_elements)
            : Type(TypeID::Array), element_type(std::move(element_type)), num_elements(num_elements) {}
        std::shared_ptr<Type> get_element_type() const { return element_type; }
        size_t get_num_elements() const { return num_elements; }
        size_t get_size() const override {
            return get_num_elements() * get_element_type()->get_size();
        }
        
    private:
        std::shared_ptr<Type> element_type;
        size_t num_elements;
    };

    class FunctionType : public Type
    {
    public:
        FunctionType(std::shared_ptr<Type> return_type, std::vector<std::shared_ptr<Type>> param_types, bool is_var_arg = false)
            : Type(TypeID::Function), return_type(std::move(return_type)), param_types(std::move(param_types)), is_var_arg_(is_var_arg) {}

        std::shared_ptr<Type> get_return_type() const { return return_type; }
        const std::vector<std::shared_ptr<Type>> &get_param_types() const { return param_types; }
        bool is_var_arg() const { return is_var_arg_; }
        size_t get_size() const override { return 0; } // 函数类型的大小在这里没有实际意义
    private:
        std::shared_ptr<Type> return_type;
        std::vector<std::shared_ptr<Type>> param_types;
        bool is_var_arg_;
    };

    class Value
    {
    public:
        Value(std::shared_ptr<Type> type, std::string name = "")
            : type(std::move(type)), name(std::move(name)) {}
        virtual ~Value() = default;

        std::shared_ptr<Type> get_type() const { return type; }
        const std::string &get_name() const { return name; }
        void set_name(const std::string &new_name) { name = new_name; }

    protected:
        std::shared_ptr<Type> type;
        std::string name;
    };

    class User : public Value
    {
    public:
        User(std::shared_ptr<Type> type, size_t num_operands = 0, const std::string &name = "")
            : Value(std::move(type), name)
        {
            operands.resize(num_operands);
        }

        Value *get_operand(size_t i) const { return operands[i]; }
        void set_operand(size_t i, Value *v) { operands[i] = v; }
        size_t get_num_operands() const { return operands.size(); }

        void add_operand(Value *v) { operands.push_back(v); }
        void remove_operands(size_t index, size_t count)
        {
            if (index + count > operands.size())
                return;
            operands.erase(operands.begin() + index, operands.begin() + index + count);
        }

    protected:
        std::vector<Value *> operands;
    };

    class Constant : public User
    {
    public:
        Constant(std::shared_ptr<Type> type) : User(std::move(type)) {}
        static std::shared_ptr<Constant> get_zero_initializer(std::shared_ptr<Type> type);
    };

    class ConstantPointerNull : public Constant
    {
    public:
        explicit ConstantPointerNull(std::shared_ptr<PointerType> type)
            : Constant(type) {}
    };

    class ConstantInt : public Constant
    {
    public:
        ConstantInt(std::shared_ptr<IntegerType> type, int64_t value)
            : Constant(type), value(value) {}
        int64_t get_value() const { return value; }

    private:
        int64_t value;
    };

    class ConstantFloat : public Constant
    {
    public:
        ConstantFloat(std::shared_ptr<FloatType> type, double value)
            : Constant(type), value(value) {}
        double get_value() const { return value; }

    private:
        double value;
    };

    class ConstantArray : public Constant
    {
    public:
        ConstantArray(std::shared_ptr<ArrayType> type, std::vector<std::shared_ptr<Constant>> elements)
            : Constant(type), elements(std::move(elements)) {}
        bool is_zero_initializer() const { return elements.empty(); }
        const std::vector<std::shared_ptr<Constant>> &get_elements() const { return elements; }

    private:
        std::vector<std::shared_ptr<Constant>> elements;
    };

    enum class LinkageType
    {
        Global,
        Constant,
        Common,
        Private,
    };

    class GlobalVariable : public Value
    {
    public:
        GlobalVariable(const std::string &name, std::shared_ptr<Type> value_type, LinkageType linkage,
                       std::shared_ptr<Constant> initializer, bool is_dso_local, bool is_unnamed_addr,
                       std::optional<unsigned> align = std::nullopt)
            : Value(std::make_shared<PointerType>(value_type), name),
              value_type(value_type), linkage(linkage),
              initializer(std::move(initializer)), is_dso_local_(is_dso_local),
              is_unnamed_addr_(is_unnamed_addr), align(align) {}

        LinkageType get_linkage() const { return linkage; }
        std::shared_ptr<Constant> get_initializer() const { return initializer; }
        bool is_dso_local() const { return is_dso_local_; }
        bool is_unnamed_addr() const { return is_unnamed_addr_; }
        std::optional<unsigned> get_align() const { return align; }

    private:
        std::shared_ptr<Type> value_type;
        LinkageType linkage;
        std::shared_ptr<Constant> initializer;
        bool is_dso_local_;
        bool is_unnamed_addr_;
        std::optional<unsigned> align;
    };

    enum class Opcode
    {
        Ret,
        Br,
        Add,
        FAdd,
        Sub,
        FSub,
        Mul,
        FMul,
        SDiv,
        FDiv,
        SRem,
        And,
        Or,
        Shl,
        AShr,
        Alloca,
        Load,
        Store,
        ICmp,
        FCmp,
        GEP,
        SIToFP,
        FPToSI,
        ZExt,
        Bitcast,
        Call,
        PHI
    };
    ;

    enum class CmpPredicate
    {
        EQ,
        NE,
        SGT,
        SGE,
        SLT,
        SLE,
        OEQ,
        ONE,
        OGT,
        OGE,
        OLT,
        OLE,
    };

    class Instruction : public User
    {
    public:
        Instruction(std::shared_ptr<Type> type, Opcode op, size_t num_operands, const std::string &name = "", BasicBlock *parent = nullptr)
            : User(std::move(type), num_operands, name), op(op), parent(parent) {}

        Opcode get_opcode() const { return op; }
        BasicBlock *get_parent() const { return parent; }
        void set_parent(BasicBlock *p) { parent = p; }
        bool is_terminator() const { return op == Opcode::Ret || op == Opcode::Br; }

    private:
        Opcode op;
        BasicBlock *parent;
    };

    class AllocaInst : public Instruction
    {
    public:
        AllocaInst(std::shared_ptr<Type> allocated_type, const std::string &name = "", BasicBlock *parent = nullptr, std::optional<unsigned> align = std::nullopt)
            : Instruction(std::make_shared<PointerType>(allocated_type), Opcode::Alloca, 0, name, parent),
              allocated_type(allocated_type), align(align) {}
        std::shared_ptr<Type> get_allocated_type() const { return allocated_type; }
        std::optional<unsigned> get_align() const { return align; }

    private:
        std::shared_ptr<Type> allocated_type;
        std::optional<unsigned> align;
    };

    class LoadInst : public Instruction
    {
    public:
        LoadInst(Value *ptr, const std::string &name = "", BasicBlock *parent = nullptr, std::optional<unsigned> align = std::nullopt)
            : Instruction(std::static_pointer_cast<PointerType>(ptr->get_type())->get_element_type(), Opcode::Load, 1, name, parent),
              align(align)
        {
            set_operand(0, ptr);
        }
        std::optional<unsigned> get_align() const { return align; }

    private:
        std::optional<unsigned> align;
    };

    class StoreInst : public Instruction
    {
    public:
        StoreInst(Value *val, Value *ptr, BasicBlock *parent = nullptr, std::optional<unsigned> align = std::nullopt)
            : Instruction(std::make_shared<VoidType>(), Opcode::Store, 2, "", parent), align(align)
        {
            set_operand(0, val);
            set_operand(1, ptr);
        }
        std::optional<unsigned> get_align() const { return align; }

    private:
        std::optional<unsigned> align;
    };

    class BinaryInst : public Instruction
    {
    public:
        BinaryInst(Opcode op, Value *lhs, Value *rhs, const std::string &name = "", BasicBlock *parent = nullptr)
            : Instruction(lhs->get_type(), op, 2, name, parent)
        {
            set_operand(0, lhs);
            set_operand(1, rhs);
        }
    };

    class CmpInst : public Instruction
    {
    public:
        CmpInst(std::shared_ptr<Type> type, CmpPredicate pred, Value *lhs, Value *rhs, const std::string &name = "", BasicBlock *parent = nullptr)
            : Instruction(type, (Type::is_integer_ty(lhs->get_type().get()) ? Opcode::ICmp : Opcode::FCmp), 2, name, parent), pred(pred)
        {
            set_operand(0, lhs);
            set_operand(1, rhs);
        }
        CmpPredicate get_predicate() const { return pred; }

    private:
        CmpPredicate pred;
    };

    class CastInst : public Instruction
    {
    public:
        CastInst(Opcode op, Value *src, std::shared_ptr<Type> dest_ty, const std::string &name = "", BasicBlock *parent = nullptr)
            : Instruction(std::move(dest_ty), op, 1, name, parent)
        {
            set_operand(0, src);
        }
    };

    class GetElementPtrInst : public Instruction
    {
    public:
        GetElementPtrInst(Value *ptr, const std::vector<Value *> &indices_param, bool inbounds, const std::string &name = "", BasicBlock *parent = nullptr);
        bool is_inbounds() const { return inbounds; }
        const std::vector<Value *> &get_indices() const { return indices; } // 改为const
        // std::shared_ptr<Type> get_source_element_type() const { return type; } // 改为const
    private:
        bool inbounds;
        std::vector<Value *> indices; // 改为普通成员而不是引用
    };

    class ReturnInst : public Instruction
    {
    public:
        explicit ReturnInst(Value *ret_val, BasicBlock *parent = nullptr) : Instruction(std::make_shared<VoidType>(), Opcode::Ret, 1, "", parent) { set_operand(0, ret_val); }
        explicit ReturnInst(BasicBlock *parent = nullptr) : Instruction(std::make_shared<VoidType>(), Opcode::Ret, 0, "", parent) {}
        bool has_return_value() const { return get_num_operands() > 0; }
    };

    class BranchInst : public Instruction
    {
    public:
        BranchInst(Value *cond, BasicBlock *true_dest, BasicBlock *false_dest, BasicBlock *parent = nullptr)
            : Instruction(std::make_shared<VoidType>(), Opcode::Br, 3, "", parent)
        {
            set_operand(0, cond);
            set_operand(1, reinterpret_cast<Value *>(true_dest));
            set_operand(2, reinterpret_cast<Value *>(false_dest));
        }
        explicit BranchInst(BasicBlock *dest, BasicBlock *parent = nullptr)
            : Instruction(std::make_shared<VoidType>(), Opcode::Br, 1, "", parent) { set_operand(0, reinterpret_cast<Value *>(dest)); }
        bool is_conditional() const { return get_num_operands() == 3; }
        BasicBlock *get_true_dest() const { return reinterpret_cast<BasicBlock *>(get_operand(1)); }
        BasicBlock *get_false_dest() const { return reinterpret_cast<BasicBlock *>(get_operand(2)); }
        BasicBlock *get_uncond_dest() const { return reinterpret_cast<BasicBlock *>(get_operand(0)); }
    };

    class CallInst : public Instruction
    {
    public:
        CallInst(Value *callee, const std::vector<Value *> &args, const std::string &name = "", BasicBlock *parent = nullptr);
        Value *get_callee() const { return get_operand(0); }
    };

    class BasicBlock : public Value
    {
    public:
        using InstructionList = std::list<std::shared_ptr<Instruction>>;
        BasicBlock(const std::string &name = "", Function *parent = nullptr)
            : Value(std::make_shared<LabelType>(), name), parent(parent) {}
        InstructionList &get_instructions() { return instructions; }
        const InstructionList &get_instructions() const { return instructions; }
        Function *get_parent() const { return parent; }
        void set_parent(Function *p) { parent = p; }

    private:
        InstructionList instructions;
        Function *parent;
    };

    class Argument : public Value
    {
    public:
        Argument(std::shared_ptr<Type> type, const std::string &name = "", Function *parent = nullptr, size_t arg_no = 0)
            : Value(type, name), parent(parent), arg_no_(arg_no) {}
        
        Function *get_parent() const { return parent; }
        void add_attribute(const std::string &attr) { attributes.push_back(attr); }
        const std::vector<std::string> &get_attributes() const { return attributes; }
        size_t get_arg_no() const { return arg_no_; } // 新增：获取参数序号

    private:
        Function *parent;
        std::vector<std::string> attributes;
        size_t arg_no_;  // 新增：参数序号
    };

    class Function : public Value
    {
    public:
        using BasicBlockList = std::list<std::shared_ptr<BasicBlock>>;
        using ArgumentList = std::vector<std::shared_ptr<Argument>>;

        Function(std::shared_ptr<FunctionType> func_type, const std::string &name, bool is_dso_local = false, bool is_declaration_only = false)
            : Value(std::make_shared<PointerType>(func_type), name), is_dso_local_(is_dso_local), is_declaration_only(is_declaration_only)
        {
            const auto param_types = func_type->get_param_types();
            for (size_t i = 0; i < param_types.size(); i++)
            {
                arguments.push_back(std::make_shared<Argument>(param_types[i], "", this, i));  // 添加参数序号
            }
        }

        bool is_declaration() const { return is_declaration_only; }
        bool is_dso_local() const { return is_dso_local_; }
        std::shared_ptr<FunctionType> get_function_type() const
        {
            auto ptr_type = std::dynamic_pointer_cast<PointerType>(this->get_type());
            return std::static_pointer_cast<FunctionType>(ptr_type->get_element_type());
        }
        std::shared_ptr<Type> get_return_type() const { return get_function_type()->get_return_type(); }
        BasicBlockList &get_basic_blocks() { return basic_blocks; }
        const BasicBlockList &get_basic_blocks() const { return basic_blocks; }
        ArgumentList &get_arguments() { return arguments; }
        const ArgumentList &get_arguments() const { return arguments; }

    private:
        bool is_dso_local_;
        bool is_declaration_only;
        ArgumentList arguments;
        BasicBlockList basic_blocks;
    };

    inline GetElementPtrInst::GetElementPtrInst(Value *ptr, 
        const std::vector<Value *> &indices_param, bool inbounds, 
        const std::string &name, BasicBlock *parent)
        : Instruction(nullptr, Opcode::GEP, 1 + indices_param.size(), name, parent), 
          inbounds(inbounds),
          indices(indices_param)  // 直接复制构造
    {
        set_operand(0, ptr);
        for (size_t i = 0; i < indices.size(); ++i) {
            set_operand(i + 1, indices[i]);
        }
        
        auto current_type = ptr->get_type();
        for (size_t i = 0; i < indices.size(); ++i) {
            if (auto pt = std::dynamic_pointer_cast<PointerType>(current_type)) {
                current_type = pt->get_element_type();
            } else if (auto at = std::dynamic_pointer_cast<ArrayType>(current_type)) {
                current_type = at->get_element_type();
            }
        }
        this->type = std::make_shared<PointerType>(current_type);
    }

    inline CallInst::CallInst(Value *callee, const std::vector<Value *> &args, const std::string &name, BasicBlock *parent)
        : Instruction(nullptr, Opcode::Call, 1 + args.size(), name, parent)
    {
        set_operand(0, callee);
        for (size_t i = 0; i < args.size(); ++i)
        {
            set_operand(i + 1, args[i]);
        }
        auto callee_ptr_type = std::dynamic_pointer_cast<PointerType>(callee->get_type());
        auto callee_func_type = std::dynamic_pointer_cast<FunctionType>(callee_ptr_type->get_element_type());
        this->type = callee_func_type->get_return_type();
    }

    class IRUnit
    {
    public:
        std::vector<std::shared_ptr<Function>> functions;
        std::vector<std::shared_ptr<GlobalVariable>> global_variables;

        std::map<std::string, Function *> function_map;
        std::map<std::string, GlobalVariable *> global_variable_map;
    };

}

#endif // IR_STRUCTURE_Ha