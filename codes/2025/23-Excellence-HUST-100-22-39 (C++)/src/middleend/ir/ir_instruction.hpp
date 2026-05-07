#ifndef __IR_INSTRUCTION_HPP__
#define __IR_INSTRUCTION_HPP__

#include <vector>
#include <list>
#include <string>
#include "ir.hpp"
#include "ir_value.hpp"

// IR 指令类型
enum IRInstID {
    // 二元操作符
    Add, Sub, Mul, Div, Rem, // 整型
    FAdd, FSub, FMul, FDiv,  // 浮点型
    Shl, AShr, LShr,         // 移位
    // 比较操作符
    ICmp, FCmp,
    // 内存操作符
    Alloca, GetElementPtr, Load, Store,
    // 一元操作符，仅包含类型转换
    ZExt, FpToSi, SiToFp, BitCast,
    // 控制流操作符
    Call, Br, Ret,
    Phi // 特殊：Phi 指令
};

// 整型比较符类型
enum ICmpOp {
    EQ, NE, GT, GE, LT, LE
};

// 浮点型比较符类型
enum FCmpOp {
    FEQ, FNE, FGT, FGE, FLT, FLE
};

// 创建指令时插入基本块的位置
enum class InsertPos {
    None,   // 不插入
    Head,   // 插入头部
    Tail    // 插入尾部
};

class Instruction: public Value {
public:
    IRInstID iid;               // 指令类型
    BasicBlock* parent;         // 所属基本块
    vector<Value*> operands;    // 操作数列表
    vector<list<Use>::iterator> use_pos;            // 对应操作数的 Use 迭代器列表
    vector<list<Instruction*>::iterator> pos_in_bb; // 对应操作数的指令迭代器列表

    /// @brief 构造函数
    /// @param type 类型
    /// @param iid 指令 ID
    /// @param num_ops 操作数数量
    /// @param parent 所属基本块
    /// @param position 插入位置
    explicit Instruction(Type* type, IRInstID iid, int num_ops, BasicBlock* parent, InsertPos position);
    virtual void print(ostream& out) = 0;

    /// @brief 克隆指令
    /// @param parent 克隆出来的指令所属的基本块
    /// @return 克隆出来的指令
    virtual Instruction* clone(BasicBlock* parent) = 0;

    /// @brief 获取操作数
    /// @param index 操作数索引
    /// @return 操作数
    Value* get_op(int index);

    /// @brief 设置操作数
    /// @param index 操作数索引
    /// @param val 操作数
    void set_op(int index, Value* val);

    /// @brief 替换操作数
    /// @note 替换操作数后，原来的操作数的 Use 关系会被更新
    /// @param index 操作数索引
    /// @param new_val 新的操作数
    void replace_op(int index, Value* new_val);

    /// @brief 移除所有操作数的 Use 关系
    /// @note 该函数会清除所有操作数的 Use 链，但不会从基本块中删除指令
    void remove_use_of_ops();

    // 快速类型判断
    bool is_add();  bool is_sub();  bool is_mul();  bool is_div();  bool is_rem();
    bool is_fadd(); bool is_fsub(); bool is_fmul(); bool is_fdiv();
    bool is_shl();  bool is_ashr(); bool is_lshr();
    bool is_icmp(); bool is_fcmp();
    bool is_alloca(); bool is_gep(); bool is_load(); bool is_store();
    bool is_zext(); bool is_fptosi(); bool is_sitofp(); bool is_bitcast();
    bool is_call(); bool is_br(); bool is_ret();
    bool is_phi();
    bool is_binary(); bool is_terminator(); // br, ret
};

class BinaryInst: public Instruction {
public:
    BinaryInst(Type* type, IRInstID iid, Value* val1, Value* val2, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class ICmpInst: public Instruction {
public:
    ICmpOp icmp_op; // 整型比较操作符
    ICmpInst(ICmpOp icmp_op, Value* val1, Value* val2, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class FCmpInst: public Instruction {
public:
    FCmpOp fcmp_op; // 浮点型比较操作符
    FCmpInst(FCmpOp fcmp_op, Value* val1, Value* val2, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class AllocaInst: public Instruction {
public:
    Type* alloca_type; // 分配的类型

    /// @brief 分配内存指令
    /// @param type 分配的类型
    /// @param bb 所属基本块
    /// @param position 插入位置，默认为头部插入
    AllocaInst(Type* type, BasicBlock* bb, InsertPos position = InsertPos::Head, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class GetElementPtrInst: public Instruction {
public:
    bool has_base;
    GetElementPtrInst* base;
    int offset;
    GetElementPtrInst(Value* ptr, vector<Value*> idxs, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;

    /// @brief 获取 GEP 指令的返回类型
    /// @param ptr 指向的指针类型
    /// @param idxs_size 
    /// @return 
    Type* get_return_type(Value* ptr, int idxs_size);
};

class LoadInst: public Instruction {
public:
    LoadInst(Value* ptr, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class StoreInst: public Instruction {
public:
    StoreInst(Value* val, Value* ptr, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class UnaryInst: public Instruction {
public:
    Type* target_type;
    UnaryInst(Type* type, IRInstID iid, Value* val, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class CallInst: public Instruction {
public:
    CallInst(Function* callee, vector<Value*> args, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class BranchInst: public Instruction {
public:
    BranchInst(BasicBlock* target_bb, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    BranchInst(Value* cond, BasicBlock* true_bb, BasicBlock* false_bb, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class ReturnInst: public Instruction {
public:
    ReturnInst(BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    ReturnInst(Value* val, BasicBlock* bb, InsertPos position = InsertPos::Tail, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;
};

class PhiInst: public Instruction {
public:
    /// @brief Phi 指令构造函数
    /// @param type Phi 指令的返回类型
    /// @param vals 操作数列表
    /// @param val_bbs 前驱基本块列表
    /// @param bb 所属基本块
    /// @param position 插入位置，默认为头部
    PhiInst(Type* type, vector<Value*> vals, vector<BasicBlock*> val_bbs, BasicBlock* bb, InsertPos position = InsertPos::Head, bool is_clone = false);
    void print(ostream& out) override;
    Instruction* clone(BasicBlock* parent) override;

    /// @brief 添加操作数对 [val, pre_bb] 到 Phi 指令
    /// @param val 操作数
    /// @param pre_bb 前驱基本块
    void add_ops(Value* val, BasicBlock* pre_bb);

    /// @brief 移除指定索引的操作数对
    /// @param index_1 操作数索引
    /// @param index_2 前驱基本块索引
    void remove_ops(int index_1, int index_2);
};

#endif // __IR_INSTRUCTION_HPP__