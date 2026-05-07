#pragma once

#include "PassManager.hpp"
#include "FuncInfo.hpp"

class GVN : public Pass<Module>{
  public:
    //GVN(Module *m) : Pass(m), func_info(std::make_shared<FuncInfo>(m)) {}
    ~GVN() = default;
    void run(Module* module, AnalysisPassManager &APM) override;

    template <class T, typename... Args>
    std::shared_ptr<T> create_expr(Args... args) {
        return std::make_shared<T>(std::forward<Args>(args)...);
    }

    class Expression {
      public:
        enum class expr_type {
            e_const,
            e_unique,
            e_call,
            e_unit,
            e_ibin,
            e_fbin,
            e_icmp,
            e_fcmp,
            e_gep,
            e_phi,
            e_load,
            e_store
        };
        Expression(expr_type op) : _op(op) {}
        expr_type get_op() const { return _op; }
        virtual std::string print() = 0;
        template <typename T>
        static bool equal_as(const Expression *lhs, const Expression *rhs) {
            auto lval = dynamic_cast<const T*>(lhs);
            auto rval = dynamic_cast<const T*>(rhs);
            return *lval == *rval;
        }

        bool operator==(const Expression &other) const {
            if (this->get_op() != other.get_op())
                return false;
            if (this == &other)
                return true;
            bool cmp_resu = true;
            switch (_op) {
            case expr_type::e_const:
                cmp_resu = equal_as<ConstExpr>(this, &other);
                break;
            case expr_type::e_unique:
                cmp_resu = equal_as<UniqueExpr>(this, &other);
                break;
            case expr_type::e_call:
                cmp_resu = equal_as<CallExpr>(this, &other);
                break;
            case expr_type::e_unit:
                cmp_resu = equal_as<UnitExpr>(this, &other);
                break;
            case expr_type::e_ibin:
                cmp_resu = equal_as<IBinExpr>(this, &other);
                break;
            case expr_type::e_fbin:
                cmp_resu = equal_as<FBinExpr>(this, &other);
                break;
            case expr_type::e_icmp:
                cmp_resu = equal_as<ICmpExpr>(this, &other);
                break;
            case expr_type::e_fcmp:
                cmp_resu = equal_as<FCmpExpr>(this, &other);
                break;
            case expr_type::e_gep:
                cmp_resu = equal_as<GepExpr>(this, &other);
                break;
            case expr_type::e_phi:
                cmp_resu = equal_as<PhiExpr>(this, &other);
                break;
            case expr_type::e_load:
                cmp_resu = equal_as<LoadExpr>(this, &other);
                break;
            case expr_type::e_store:
                cmp_resu = equal_as<StoreExpr>(this, &other);
                break;
            }
            return cmp_resu;
        }

      private:
        expr_type _op;
    };
    friend Expression;

    class ConstExpr final : public Expression {
      public:
        ConstExpr(Value *con)
            : Expression(expr_type::e_const),
              _const(dynamic_cast<Constant*>(con)) {}

        bool operator==(const ConstExpr &other) const {
            return _const == other._const;
        }

        virtual std::string print() {
            return "ConstExpr:" + _const->get_name() + "";
        }

      private:
        Constant *_const;
    };

    class UniqueExpr final : public Expression { // load/store/alloca/ptr2int/int2ptr
      public:
        UniqueExpr(Value *val)
            : Expression(expr_type::e_unique), _val(val) {}

        bool operator==(const UniqueExpr &other) const {
            return _val == other._val;
        }

        virtual std::string print() {
            return "UniqueExpr:" + _val->get_name() + "\n";
        }

      private:
        Value *_val;
    };

    class CallExpr final : public Expression {
      public:
        CallExpr(Instruction *inst)
            : Expression(expr_type::e_call), _inst(inst) {} // non-pure function
        CallExpr(Function *func,
                 std::vector<std::shared_ptr<Expression>> &&params)
            : Expression(expr_type::e_call), _func(func), _params(params) {
        } // pure function

        bool operator==(const CallExpr &other) const {
            if (_func == nullptr)
                return _inst == other._inst;
            else if (_func != other._func ||
                     _params.size() != other._params.size())
                return false;
            else
                for (unsigned i = 0; i < _params.size(); i++) {
                    if (not(*_params[i] == *other._params[i]))
                        return false;
                }
            return true;
        }

        virtual std::string print() {
            if (_func != nullptr) {
                return "CallExpr:{\n" + _func->get_name() + "}";
            }
            return "CallExpr:{\n" + _inst->get_name() + "}";
        }

      private:
        Function *_func{};
        Instruction *_inst{};
        std::vector<std::shared_ptr<Expression>> _params{};
    };

    class LoadExpr final : public Expression {
      public:
        LoadExpr(std::shared_ptr<Expression> addr)
            : Expression(expr_type::e_load), _addr(addr) {}

        bool operator==(const LoadExpr &other) const {
            return false && *_addr == *other._addr;
        }

        virtual std::string print() {
            return "LoadExpr:{\n" + _addr->print() + "}";
        }

      private:
        std::shared_ptr<Expression> _addr;
    };

    class StoreExpr final : public Expression {
      public:
        StoreExpr(std::shared_ptr<Expression> val,
                  std::shared_ptr<Expression> addr)
            : Expression(expr_type::e_store), _val(val), _addr(addr) {}

        bool operator==(const StoreExpr &other) const {
            return false && *_addr == *other._addr &&
                   *_val == *other._val;
        }

        virtual std::string print() {
            return "StoreExpr:{\n" + _addr->print() + "," + _val->print() + "}";
        }

      private:
        std::shared_ptr<Expression> _val;
        std::shared_ptr<Expression> _addr;
    };

    class UnitExpr final : public Expression { // zext/fp2si/si2fp/sext/trunc/bitcast
      public:
        UnitExpr(std::shared_ptr<Expression> oper)
            : Expression(expr_type::e_unit), _unit(oper) {}

        bool operator==(const UnitExpr &other) const {
            return *_unit == *other._unit;
        }

        virtual std::string print() {
            return "UnitExpr:{\n" + _unit->print() + "}";
        }

      private:
        std::shared_ptr<Expression> _unit;
    };

    class BinExpr : public Expression {
      public:
        BinExpr(expr_type ty, std::shared_ptr<Expression> lhs,
                std::shared_ptr<Expression> rhs)
            : Expression(ty), _lhs(lhs), _rhs(rhs) {}
        std::shared_ptr<Expression> get_lhs() const { return _lhs; }
        std::shared_ptr<Expression> get_rhs() const { return _rhs; }

      private:
        std::shared_ptr<Expression> _lhs, _rhs;
    };

    class IBinExpr final : public BinExpr {
      public:
        IBinExpr(IBinaryInst::OpID op, std::shared_ptr<Expression> lhs,
                 std::shared_ptr<Expression> rhs)
            : BinExpr(expr_type::e_ibin, lhs, rhs), _op(op) {}

        bool operator==(const IBinExpr &other) const {
            if (_op == IBinaryInst::add or _op == IBinaryInst::mul)
                return _op == other._op && ((*get_lhs() == *other.get_lhs() &&
                                             *get_rhs() == *other.get_rhs()) ||
                                            (*get_lhs() == *other.get_rhs() &&
                                             *get_rhs() == *other.get_lhs()));
            return _op == other._op && *get_lhs() == *other.get_lhs() &&
                   *get_rhs() == *other.get_rhs();
        }

        IBinaryInst::OpID get_ibin_op() { return _op; }

        virtual std::string print() {
            std::string lhs_s = "[" + get_lhs()->print() + "]\n";
            std::string rhs_s = "[" + get_rhs()->print() + "]\n";
            return "IBin:{\n" + lhs_s + rhs_s + "}\n";
        }

      private:
        IBinaryInst::OpID _op;
    };

    class FBinExpr final : public BinExpr {
      public:
        FBinExpr(FBinaryInst::OpID op, std::shared_ptr<Expression> lhs,
                 std::shared_ptr<Expression> rhs)
            : BinExpr(expr_type::e_fbin, lhs, rhs), _op(op) {}

        bool operator==(const FBinExpr &other) const {
            if (_op == FBinaryInst::fadd or _op == FBinaryInst::fmul)
                return _op == other._op && ((*get_lhs() == *other.get_lhs() &&
                                             *get_rhs() == *other.get_rhs()) ||
                                            (*get_lhs() == *other.get_rhs() &&
                                             *get_rhs() == *other.get_lhs()));
            return _op == other._op && *get_lhs() == *other.get_lhs() &&
                   *get_rhs() == *other.get_rhs();
        }

        FBinaryInst::OpID get_fbin_op() { return _op; }

        virtual std::string print() {
            std::string lhs_s = "[" + get_lhs()->print() + "]\n";
            std::string rhs_s = "[" + get_rhs()->print() + "]\n";
            return "FBin:{\n" + lhs_s + rhs_s + "}\n";
        }

      private:
        FBinaryInst::OpID _op;
    };

    class ICmpExpr final : public BinExpr {
      public:
        ICmpExpr(ICmpInst::OpID op, std::shared_ptr<Expression> lhs,
                 std::shared_ptr<Expression> rhs)
            : BinExpr(expr_type::e_icmp, lhs, rhs), _op(op) {}

        bool operator==(const ICmpExpr &other) const {
            return _op == other._op && *get_lhs() == *other.get_lhs() &&
                   *get_rhs() == *other.get_rhs();
        }

        ICmpInst::OpID get_icmp_op() { return _op; }

        virtual std::string print() {
            std::string lhs_s = "[" + get_lhs()->print() + "]\n";
            std::string rhs_s = "[" + get_rhs()->print() + "]\n";
            return "ICmp:{\n" + lhs_s + rhs_s + "}\n";
        }

      private:
        ICmpInst::OpID _op;
    };

    class FCmpExpr final : public BinExpr {
      public:
        FCmpExpr(FCmpInst::OpID op, std::shared_ptr<Expression> lhs,
                 std::shared_ptr<Expression> rhs)
            : BinExpr(expr_type::e_fcmp, lhs, rhs), _op(op) {}

        bool operator==(const FCmpExpr &other) const {
            return _op == other._op && *get_lhs() == *other.get_lhs() &&
                   *get_rhs() == *other.get_rhs();
        }

        FCmpInst::OpID get_fcmp_op() { return _op; }

        virtual std::string print() {
            std::string lhs_s = "[" + get_lhs()->print() + "]\n";
            std::string rhs_s = "[" + get_rhs()->print() + "]\n";
            return "FCmp:{\n" + lhs_s + rhs_s + "}\n";
        }

      private:
        FCmpInst::OpID _op;
    };

    class GepExpr final : public Expression {
      public:
        GepExpr(std::vector<std::shared_ptr<Expression>> &&idxs)
            : Expression(expr_type::e_gep), _idxs(idxs) {}

        bool operator==(const GepExpr &other) const {
            if (this->_idxs.size() != other._idxs.size())
                return false;
            for (unsigned i = 0; i < _idxs.size(); i++) {
                if (not(*_idxs[i] == *other._idxs[i]))
                    return false;
            }
            return true;
        }

        virtual std::string print() { return "Gep"; }

      private:
        std::vector<std::shared_ptr<Expression>> _idxs;
    };

    class PhiExpr final : public Expression {
      public:
        PhiExpr(BasicBlock *bb)
            : Expression(expr_type::e_phi), _ori_bb(bb),
              _pre_bbs({bb->get_pre_basic_blocks().begin(), bb->get_pre_basic_blocks().end()}), _vals{} {}
        PhiExpr(BasicBlock *bb,
                std::vector<std::shared_ptr<Expression>> &&vals)
            : Expression(expr_type::e_phi), _ori_bb(bb),
              _pre_bbs({bb->get_pre_basic_blocks().begin(), bb->get_pre_basic_blocks().end()}),
              _vals(vals) {}

        size_t size() const { return _vals.size(); }

        BasicBlock *get_ori_bb() { return _ori_bb; }

        std::shared_ptr<Expression> get_val(size_t i) { return _vals[i]; }

        BasicBlock *get_pre_bb(size_t i) { return _pre_bbs[i]; }

        void add_val(std::shared_ptr<Expression> ve) { _vals.push_back(ve); }

        bool operator==(const PhiExpr &other) const {
            if (_ori_bb != other._ori_bb)
                return false;
            for (unsigned i = 0; i < _vals.size(); i++) {
                assert(not(_vals[i] == nullptr || other._vals[i] == nullptr));
                if (not(*_vals[i] == *other._vals[i]))
                    return false;
            }
            return true;
        }

        virtual std::string print() {
            std::string val_s{};
            for (auto val : _vals) {
                val_s += "[" + val->print() + "]";
            }
            return "Phi:{\n" + val_s + "}\n";
        }

      private:
        BasicBlock *_ori_bb;
        std::vector<BasicBlock *> _pre_bbs;
        std::vector<std::shared_ptr<Expression>> _vals;
    };


    size_t next_value_number;
    struct CongruenceClass {
        unsigned index;
        Value *leader;
        std::shared_ptr<Expression> val_expr;
        std::shared_ptr<PhiExpr> phi_expr;
        std::set<Value *> members;
        bool operator<(const CongruenceClass &other) const {
            return this->index < other.index;
        }
        bool operator==(const CongruenceClass &other) const;

        CongruenceClass(size_t index)
            : index(index), leader{}, val_expr{}, phi_expr{}, members{} {}
        CongruenceClass(size_t index, Value *leader,
                        std::shared_ptr<Expression> val_expr,
                        std::shared_ptr<PhiExpr> phi_expr)
            : index(index), leader(leader), val_expr(val_expr),
              phi_expr(phi_expr), members{} {}
        CongruenceClass(size_t index, Value *leader,
                        std::shared_ptr<Expression> val_expr,
                        std::shared_ptr<PhiExpr> phi_expr, Value *member)
            : index(index), leader(leader), val_expr(val_expr),
              phi_expr(phi_expr), members{member} {}
    };
    template <typename... Args>
    static std::shared_ptr<CongruenceClass>
    create_cc(Args... args) { // only the number of TOP cc can be 0
        return std::make_shared<CongruenceClass>(std::forward<Args>(args)...);
    }
   
    struct less_part {
        bool
        operator()(const std::shared_ptr<GVN::CongruenceClass> &a,
                   const std::shared_ptr<GVN::CongruenceClass> &b) const {
            return *a < *b;
        }
    };
    using partitions = std::set<std::shared_ptr<CongruenceClass>, less_part>;
    partitions clone(const partitions &p);
    partitions clone(partitions &&p);

    // GVN functions
    void detect_equivalences(Function *func, const FuncInfo *func_info);
    partitions join(const partitions &p1, const partitions &p2);
    std::shared_ptr<CongruenceClass>
        intersect(std::shared_ptr<CongruenceClass>,
                  std::shared_ptr<CongruenceClass>);
    partitions transfer_function(Instruction *, partitions &, const FuncInfo *func_info);
    std::shared_ptr<Expression> valueExpr(Value *, partitions &, const FuncInfo *func_info);
    std::shared_ptr<PhiExpr> valuePhiFunc(std::shared_ptr<Expression>);
    std::shared_ptr<Expression> getVN(partitions &,
                                      std::shared_ptr<Expression>);


    void replace_cc_members();

   
    std::shared_ptr<Expression> get_ve(Value *, partitions &);
    template <typename Derived, typename Base>
    static bool is_a(std::shared_ptr<Base> base) {
        static_assert(std::is_base_of<Base, Derived>::value);
        return std::dynamic_pointer_cast<Derived>(base) != nullptr;
    }
    template <typename Derived, typename Base>
    static std::shared_ptr<Derived> as_a(std::shared_ptr<Base> base) {
        static_assert(std::is_base_of<Base, Derived>::value);
        std::shared_ptr<Derived> derived =
            std::dynamic_pointer_cast<Derived>(base);
        if (not derived) {
            throw std::logic_error{"bad asa"};
        }
        return derived;
    }
    template <typename ExprT, typename InstT,
              typename OP> 
    std::shared_ptr<ExprT> create_BinOperExpr(OP op, Value *val,
                                              partitions &pin, const FuncInfo *func_info) {
        std::shared_ptr<Expression> lhs, rhs;
        lhs = valueExpr(dynamic_cast<InstT*>(val)->get_operand(0), pin, func_info);
        rhs = valueExpr(dynamic_cast<InstT*>(val)->get_operand(1), pin, func_info);
        return create_expr<ExprT>(op, lhs, rhs);
    }

    void clear() {
        _val2expr.clear();
        non_copy_pout.clear();
    }

  private:
    partitions TOP{create_cc(0)};
    Function *_func;
    BasicBlock *_bb;
    //std::shared_ptr<FuncInfo> func_info;
    //逆后序遍历相关
    std::map<BasicBlock *, unsigned> bb_postorder_{};
    std::vector<BasicBlock*> bb_vector_{};
    std::map<BasicBlock *, bool> bb_vis_{};
    void DFS(BasicBlock* bb);
    
    std::map<BasicBlock *, partitions> _pin, _pout;

    std::unordered_map<Value *, std::shared_ptr<Expression>>
        _val2expr{}; 
    unsigned phi_construct_point;
    std::map<BasicBlock *, partitions> non_copy_pout;
};

