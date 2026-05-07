#pragma once
#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "DeadCode.hpp"
#include "FuncInfo.hpp"
#include "Function.hpp"
#include "IRprinter.hpp"
#include "Instruction.hpp"
#include "Module.hpp"
#include "PassManager.hpp"
#include "Value.hpp"
#include "ilist.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

namespace GVNExpression {

    // fold the constant value 常量折叠
    class ConstFolder {
    public:
        explicit ConstFolder(Module *m) : module_(m) {}
        Constant *compute(Instruction *instr, Constant *value1, Constant *value2);
        Constant *compute(Instruction *instr, Constant *value1);

    private:
        Module *module_;
    };

    /**
     * for constructor of class derived from `Expression`, we make it public
     * because `std::make_shared` needs the constructor to be publicly available,
     * but you should call the static factory method `create` instead the constructor itself to get
     * the desired data
     */
    class Expression {
    public:
        // TODO: you need to extend expression types according to testcases
        enum gvn_expr_t {
            e_bin,
            e_const,
            e_var,
            e_cmp,
            e_phi,
            e_unary,
            e_gep,
            e_load
        };
        explicit Expression(gvn_expr_t t) : expr_type(t) {}
        virtual ~Expression()       = default;
        virtual std::string print() const = 0;
        virtual bool equiv(const Expression *other) const = 0;
        [[nodiscard]] gvn_expr_t get_expr_type() const {
            return expr_type;
        }

    private:
        gvn_expr_t expr_type;
    };

    bool operator==(const std::shared_ptr<Expression> &lhs, const std::shared_ptr<Expression> &rhs);
    bool operator==(const GVNExpression::Expression &lhs, const GVNExpression::Expression &rhs);

    // arithmetic expression
    class BinaryExpression : public Expression {
    public:
        static std::shared_ptr<BinaryExpression> create(Instruction::OpID op,
                                                        std::shared_ptr<Expression> lhs,
                                                        std::shared_ptr<Expression> rhs) {
            return std::make_shared<BinaryExpression>(op, lhs, rhs);
        }
        std::string print() const override {
            return "(" + print_instr_op_name(op_) + " " + lhs_->print() + " " + rhs_->print() + ")";
        }

        bool equiv(const Expression *other) const override {
            if (other->get_expr_type() != e_bin)
                return false;
            auto *o = static_cast<const BinaryExpression *>(other);
            return op_ == o->op_ && *lhs_ == *o->lhs_ && *rhs_ == *o->rhs_;
        }


        BinaryExpression(Instruction::OpID op,
                         std::shared_ptr<Expression> lhs,
                         std::shared_ptr<Expression> rhs)
            : Expression(e_bin), op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

        bool both_phi() {
            // TODO: determine whether both operands are phi functions
            return lhs_->get_expr_type()==e_phi && rhs_->get_expr_type()==e_phi;
        }

        std::shared_ptr<Expression> get_lhs() const {
            return lhs_;
        }

        std::shared_ptr<Expression> get_rhs() const {
            return rhs_;
        }

        Instruction::OpID get_op() const {
            return op_;
        }

    private:
        Instruction::OpID op_;
        std::shared_ptr<Expression> lhs_, rhs_;
    };

    class ConstantExpression : public Expression {
    public:
        static std::shared_ptr<ConstantExpression> create(Constant *c) {
            
            return std::make_shared<ConstantExpression>(c);
        }

        ConstantExpression(Constant *c) : Expression(e_const), const_(c) {}

        std::string print() const override {
            return const_->print();  // 假设 Constant 类已有打印函数
        }

        bool equiv(const Expression *other) const override {
            if (other->get_expr_type() != e_const)
                return false;
            auto *o = static_cast<const ConstantExpression *>(other);
            return const_ == o->const_;
        }

        Constant *get_constant() const { return const_; }

    private:
        Constant *const_;
    };

    class VariableExpression : public Expression {
    public:
        static std::shared_ptr<VariableExpression> create(Value *v) {
            return std::make_shared<VariableExpression>(v);
        }

        VariableExpression(Value *v) : Expression(e_var), var_(v) {}

        std::string print() const override {
            return var_->get_name();  // 假设 Value 有 get_name()
        }

        bool equiv(const Expression *other) const override {
            if (other->get_expr_type() != e_var)
                return false;
            auto *o = static_cast<const VariableExpression *>(other);
            return var_ == o->var_;
        }

        Value *get_var() const { return var_; }

    private:
        Value *var_;
    };

    class CmpExpression : public Expression {
    public:
        static std::shared_ptr<CmpExpression> create(Instruction::OpID op,
                                                    std::shared_ptr<Expression> lhs,
                                                    std::shared_ptr<Expression> rhs) {
            return std::make_shared<CmpExpression>(op, lhs, rhs);
        }

        CmpExpression(Instruction::OpID op,
                    std::shared_ptr<Expression> lhs,
                    std::shared_ptr<Expression> rhs)
            : Expression(e_cmp), op_(op), lhs_(std::move(lhs)), rhs_(std::move(rhs)) {}

        std::string print() const override {
            return "(" + print_instr_op_name(op_) + " " + lhs_->print() + " " + rhs_->print() + ")";
        }

        bool equiv(const Expression *other) const override {
            if (other->get_expr_type() != e_cmp)
                return false;
            auto *o = static_cast<const CmpExpression *>(other);
            return op_ == o->op_ && *lhs_ == *o->lhs_ && *rhs_ == *o->rhs_;
        }

        // ✅ Getter functions
        Instruction::OpID get_op() const { return op_; }
        std::shared_ptr<Expression> get_lhs() const { return lhs_; }
        std::shared_ptr<Expression> get_rhs() const { return rhs_; }

    private:
        Instruction::OpID op_;
        std::shared_ptr<Expression> lhs_, rhs_;
    };

    class PhiExpression : public Expression {
    public:
        static std::shared_ptr<PhiExpression> create(const std::vector<std::shared_ptr<Expression>> &operands) {
            return std::make_shared<PhiExpression>(operands);
        }

        PhiExpression(const std::vector<std::shared_ptr<Expression>> &operands)
            : Expression(e_phi), operands_(operands) {}

        std::string print() const override {
            std::string s = "phi(";
            for (size_t i = 0; i < operands_.size(); ++i) {
                s += operands_[i]->print();
                if (i != operands_.size() - 1)
                    s += ", ";
            }
            s += ")";
            return s;
        }

        bool equiv(const Expression *other) const override {
            if (other->get_expr_type() != e_phi)
                return false;
            auto *o = static_cast<const PhiExpression *>(other);
            if (operands_.size() != o->operands_.size())
                return false;
            for (size_t i = 0; i < operands_.size(); ++i) {
                if (!(*operands_[i] == *o->operands_[i]))
                    return false;
            }
            return true;
        }

        const std::vector<std::shared_ptr<Expression>> &get_operands() const {
            return operands_;
        }

    private:
        std::vector<std::shared_ptr<Expression>> operands_;
    };

    class UnaryExpression : public Expression {
    public:
        static std::shared_ptr<UnaryExpression> create(Instruction::OpID op, std::shared_ptr<Expression> operand) {
            return std::make_shared<UnaryExpression>(op, operand);
        }
        std::string print() const override {
            return "(" + print_instr_op_name(op_) + " " + operand_->print() + ")";
        }

        bool equiv(const Expression *other) const override {
            if (other->get_expr_type() != e_unary)
                return false;
            auto *o = static_cast<const UnaryExpression *>(other);
            return op_ == o->op_ && *operand_ == *o->operand_;
        }

        UnaryExpression(Instruction::OpID op, std::shared_ptr<Expression> operand)
            : Expression(e_unary), op_(op), operand_(std::move(operand)) {}

        Instruction::OpID get_op() const { return op_; }
        std::shared_ptr<Expression> get_operand() const { return operand_; }

    private:
        Instruction::OpID op_;
        std::shared_ptr<Expression> operand_;
    };

    // GEP (GetElementPtr) expression for array access
    class GEPExpression : public Expression {
    public:
        static std::shared_ptr<GEPExpression> create(Value *base_ptr, 
                                                    const std::vector<std::shared_ptr<Expression>> &indices,
                                                    bool after_call = false) {
            return std::make_shared<GEPExpression>(base_ptr, indices, after_call);
        }
        
        std::string print() const override {
            std::string result = "gep(" + base_ptr_->get_name() + ", ";
            for (size_t i = 0; i < indices_.size(); ++i) {
                if (i > 0) result += ", ";
                result += indices_[i]->print();
            }
            result += ")";
            if (after_call_) result += "_after_call";
            return result;
        }

        bool equiv(const Expression *other) const override {
            if (other->get_expr_type() != e_gep)
                return false;
            auto *o = static_cast<const GEPExpression *>(other);
            
            // 新增：如果两个GEP指令的执行时机不同，不能合并
            if (after_call_ != o->after_call_) {
                return false;
            }
            
            if (base_ptr_ != o->base_ptr_ || indices_.size() != o->indices_.size())
                return false;
            for (size_t i = 0; i < indices_.size(); ++i) {
                if (!(*indices_[i] == *o->indices_[i]))
                    return false;
            }
            return true;
        }

        GEPExpression(Value *base_ptr, const std::vector<std::shared_ptr<Expression>> &indices, bool after_call = false)
            : Expression(e_gep), base_ptr_(base_ptr), indices_(indices), after_call_(after_call) {}

        Value *get_base_ptr() const { return base_ptr_; }
        const std::vector<std::shared_ptr<Expression>> &get_indices() const { return indices_; }
        bool is_after_call() const { return after_call_; }

    private:
        Value *base_ptr_;
        std::vector<std::shared_ptr<Expression>> indices_;
        bool after_call_;  // 新增：标记是否在函数调用之后
    };

    // Load expression for load instructions
    class LoadExpression : public Expression {
    public:
        static std::shared_ptr<LoadExpression> create(std::shared_ptr<Expression> ptr_expr, size_t load_id = 0) {
            return std::make_shared<LoadExpression>(ptr_expr, load_id);
        }
        
        std::string print() const override {
            return "load(" + ptr_expr_->print() + ")_" + std::to_string(load_id_);
        }

        bool equiv(const Expression *other) const override {
            if (other->get_expr_type() != e_load)
                return false;
            auto *o = static_cast<const LoadExpression *>(other);
            
            // 检查指针表达式是否相同
            bool ptr_equal = *ptr_expr_ == *o->ptr_expr_;
            if (!ptr_equal) return false;
            
            // 如果指针相同，检查load_id
            // 如果两个load的ID都为0（默认值），说明它们可能来自同一个基本块或没有冲突
            // 这种情况下允许合并，因为GVN会通过其他机制确保安全性
            if (load_id_ == 0 && o->load_id_ == 0) {
                return true;
            }
            
            // 如果ID不同且都不为0，则不允许合并
            if (load_id_ != o->load_id_) {
                return false;
            }
            
            return true;
        }

        LoadExpression(std::shared_ptr<Expression> ptr_expr, size_t load_id = 0)
            : Expression(e_load), ptr_expr_(std::move(ptr_expr)), load_id_(load_id) {}

        const std::shared_ptr<Expression> &get_ptr_expr() const { return ptr_expr_; }
        size_t get_load_id() const { return load_id_; }

    private:
        std::shared_ptr<Expression> ptr_expr_;
        size_t load_id_;  // 新增：load的唯一标识符
    };

    // TODO: add other expression subclasses here
} // namespace GVNExpression

/**
 * Congruence class in each partitions
 * Note: for constant propagation, you might need to add other fields
 */
struct CongruenceClass {
    size_t index_;
    // representative of the congruence class, used to replace all the members (except itself) when
    // analysis is done
    Value *leader_{};
    // value expression in congruence class
    // Note: type of value_expr_ and value_phi_ is shared_ptr<Expression>, which correspond to the
    // return type of valueExpr and valuePhiFunc function, if you want to design your own value
    // expression, you need to modify the type of value_expr_, value_phi_ and the return type of
    // valueExpr and valuePhiFunc
    std::shared_ptr<GVNExpression::Expression> value_expr_;
    // value φ-function is an annotation of the congruence class
    std::shared_ptr<GVNExpression::Expression> value_phi_;
    // equivalent variables in one congruence class
    std::set<Value *> members_;

    explicit CongruenceClass(size_t index) : index_(index) {}

    bool operator<(const CongruenceClass &other) const {
        return this->index_ < other.index_;
    }
    bool operator==(const CongruenceClass &other) const;
};

namespace std {
    template<>
    // overload std::less for std::shared_ptr<CongruenceClass>, i.e. how to sort the congruence
    // classes
    struct less<std::shared_ptr<CongruenceClass>> {
        bool operator()(const std::shared_ptr<CongruenceClass> &a,
                        const std::shared_ptr<CongruenceClass> &b) const {
            // nullptrs should never appear in partitions, so we just dereference it
            return *a < *b;
        }
    };
} // namespace std

class GVN : public Pass {
public:
    using partitions = std::set<std::shared_ptr<CongruenceClass>>;
    static bool isTop(const partitions &p) {
        return (*p.begin())->index_ == 0;
    }
    GVN(Module *m, bool dump_json) : Pass(m), dump_json_(dump_json) {}
    // pass start
    void run() override;
    // init for pass metadata;
    void initPerFunction();

    // fill the following functions according to Pseudocode, **you might need to add more
    // arguments**
    void detectEquivalences(ilist<GlobalVariable> *global_list);
    partitions join(const partitions &P1, const partitions &P2, BasicBlock *lbb, BasicBlock *rbb);
    std::shared_ptr<CongruenceClass> intersect(const std::shared_ptr<CongruenceClass> &,
                                               const std::shared_ptr<CongruenceClass> &,
                                               BasicBlock *lbb,
                                               BasicBlock *rbb);
    partitions transferFunction(Instruction *x, Value *e, const partitions &pin);
    std::shared_ptr<GVNExpression::Expression>
    valuePhiFunc(const std::shared_ptr<GVNExpression::Expression> &, const partitions &);
    std::shared_ptr<GVNExpression::Expression> valueExpr(const partitions &pout, Value *v);
    std::shared_ptr<GVNExpression::Expression> valueExprHelper(const partitions &pout, Value *v, std::set<Value*> visited, int depth = 0);

    static std::shared_ptr<GVNExpression::Expression>
    getVN(const partitions &pout, std::shared_ptr<GVNExpression::Expression> ve);

    // 尝试常量折叠二元表达式
    std::shared_ptr<GVNExpression::Expression> tryConstantFold(
        const GVNExpression::BinaryExpression* bin_expr);
    
    // 尝试重用现有表达式
    std::shared_ptr<GVNExpression::Expression> tryReuseExpression(
        std::shared_ptr<GVNExpression::Expression> ve,
        const partitions& pin,
        const partitions& pout);

    // 新增：表达式缓存和规范化相关函数
    std::shared_ptr<GVNExpression::Expression> getOrCreateExpression(std::shared_ptr<GVNExpression::Expression> expr);
    std::shared_ptr<GVNExpression::Expression> canonicalizeExpression(std::shared_ptr<GVNExpression::Expression> expr);
    
    // 新增：GEP表达式专门规范化函数
    std::shared_ptr<GVNExpression::Expression> canonicalizeGEPExpression(
        std::shared_ptr<GVNExpression::GEPExpression> gep_expr);
    
    // 新增：检查指令是否在函数调用之后
    bool isAfterFunctionCall(Instruction* instr);
    
    // 新增：检查两个GEP指令是否可以安全合并
    bool canSafelyMergeGEP(Instruction* gep1, Instruction* gep2);
    
    void clearExpressionCache();
    
    // 新增：内存状态跟踪相关函数
    void markMemoryModified(Value* memory_location);
    bool isMemoryModified(Value* memory_location);
    void clearMemoryState();
    bool shouldPreventMerge(Instruction* instr, std::shared_ptr<GVNExpression::Expression> ve, 
                           std::shared_ptr<CongruenceClass> cc);

    // replace cc members with leader
    void replace_cc_members();

    // note: be careful when to use copy constructor or clone
    static partitions clone(const partitions &p);

    // create congruence class helper
    static std::shared_ptr<CongruenceClass> createCongruenceClass(size_t index = 0) {
        return std::make_shared<CongruenceClass>(index);
    }

private:
    bool dump_json_;
    std::uint64_t next_value_number_ = 1;
    Function *func_{};
    std::map<BasicBlock *, partitions> pin_, pout_;
    std::unique_ptr<FuncInfo> func_info_;
    std::unique_ptr<GVNExpression::ConstFolder> folder_;
    std::unique_ptr<DeadCode> dce_;
    
    // 新增：表达式缓存相关成员
    static constexpr int MAX_EXPRESSION_DEPTH = 10; // 限制表达式深度
    
    // 新增：内存状态跟踪相关成员
    std::set<Value*> modified_memory_locations_; // 记录被修改的内存位置
    std::set<Value*> modified_global_vars_;     // 记录被修改的全局变量
    
    // 新增：LoadExpression唯一ID计数器
    size_t load_counter_ = 0;
};

bool operator==(const GVN::partitions &p1, const GVN::partitions &p2);
