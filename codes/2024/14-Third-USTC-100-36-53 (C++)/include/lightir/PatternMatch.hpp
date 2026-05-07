#include "Constant.hpp"
#include "Instruction.hpp"
class Value;
namespace PatternMatch {

template <typename Val, typename Pattern>
bool match(Val *val, Pattern pattern) {
    return pattern.match(val);
}

template <typename Val, typename Pattern>
bool match(Val val, const Pattern pattern) {
    return pattern.match(val);
}

template <typename Class> struct class_match {
    template <typename ITy> bool match(ITy *V) {
        return dynamic_cast<Class *>(V) != nullptr;
    }
};
inline class_match<Value> m_value() { return class_match<Value>(); }
inline class_match<Constant> m_constant() { return class_match<Constant>(); }

template <typename Class> struct bind_ty {
    Class *&VR;

    bind_ty(Class *&V) : VR(V) {}

    template <typename ITy> bool match(ITy *V) {
        auto *CV = dynamic_cast<Class *>(V);
        if (CV != nullptr) {
            VR = CV;
            return true;
        }
        return false;
    }
};
inline bind_ty<Value> m_value(Value *&V) { return V; }
inline bind_ty<Instruction> m_instruction(Instruction *&I) { return I; }
inline bind_ty<Constant> m_constant(Constant *&C) { return C; }

struct op_bind_ty {
    Instruction::OpID &O;
    op_bind_ty(Instruction::OpID &op) : O(op) {}
    bool match(Instruction::OpID op) {
        O = op;
        return true;
    }
};

inline op_bind_ty m_op(Instruction::OpID &op) { return op; }

template <typename Op, typename LHS, typename RHS> struct binary_match {
    LHS &L;
    RHS &R;
    Op &O;

    binary_match(const Op &O, const LHS &lhs, const RHS &rhs)
        : O(O), L(lhs), R(rhs) {}

    template <typename ITy> bool match(ITy *V) {
        auto *CV = dynamic_cast<Instruction *>(V);
        if (CV != nullptr) {
            return match(CV->get_instr_type(), O) &&
                   match(CV->get_operand(0), L) && match(CV->get_operand(1), R);
        }
        return false;
    }
};

template <typename Op, typename LHS, typename RHS>
binary_match<Op, LHS, RHS> m_binary(const Op &O, const LHS &lhs, const RHS &rhs) {
    return binary_match<Op, LHS, RHS>(O, lhs, rhs);
}


template <typename Op, typename LHS, typename RHS> struct icmp_match {
    LHS L;
    RHS R;
    Op O;

    icmp_match(const Op &O, const LHS &lhs, const RHS &rhs)
        : L(lhs), R(rhs),O(O) {}

    template <typename ITy> bool match(ITy *V) {
        auto *CV = dynamic_cast<Instruction *>(V);
        if (CV != nullptr && CV->is_cmp()) {
            return O.match(CV->get_instr_type()) &&
                   L.match(CV->get_operand(0)) && R.match(CV->get_operand(1));
        }
        return false;
    }
};

template <typename Op, typename LHS, typename RHS>
icmp_match<Op, LHS, RHS> m_icmp(const Op &O, const LHS &lhs, const RHS &rhs) {
    return icmp_match<Op, LHS, RHS>(O, lhs, rhs);
}

template <typename Op, typename LHS, typename RHS> struct fcmp_match {
    LHS L;
    RHS R;
    Op O;

    fcmp_match(Op &O, const LHS &lhs, const RHS &rhs)
        : L(lhs), R(rhs),O(O) {}

    template <typename ITy> bool match(ITy *V) {
        auto *CV = dynamic_cast<Instruction *>(V);
        if (CV != nullptr && CV->is_fcmp()) {
            return O.match(CV->get_instr_type()) &&
                   L.match(CV->get_operand(0)) && R.match(CV->get_operand(1));
        }
        return false;
    }
};

template <typename Op, typename LHS, typename RHS>
fcmp_match<Op, LHS, RHS> m_fcmp(const Op &O, const LHS &lhs, const RHS &rhs) {
    return fcmp_match<Op, LHS, RHS>(O, lhs, rhs);
}

template <typename LHS, typename RHS, typename Op> struct binary_op_match {
    LHS L;
    RHS R;
    Op O;

    binary_op_match(const LHS &lhs, const RHS &rhs, const Op &op)
        : L(lhs), R(rhs), O(op) {}

    template <typename ITy> bool match(ITy *V) {
        auto *CV = dynamic_cast<Instruction *>(V);
        if (CV != nullptr && CV->get_instr_type() == O) {
            return L.match(CV->get_operand(0)) && R.match(CV->get_operand(1));
        }
        return false;
    }
};

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_add(const LHS &lhs,
                                                   const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::add);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_sub(const LHS &lhs,
                                                   const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::sub);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_mul(const LHS &lhs,
                                                   const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::mul);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_sdiv(const LHS &lhs,
                                                    const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(
        lhs, rhs, Instruction::OpID::sdiv);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_srem(const LHS &lhs,
                                                    const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(
        lhs, rhs, Instruction::OpID::srem);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_fadd(const LHS &lhs,
                                                    const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(
        lhs, rhs, Instruction::OpID::fadd);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_fsub(const LHS &lhs,
                                                    const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(
        lhs, rhs, Instruction::OpID::fsub);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_fmul(const LHS &lhs,
                                                    const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(
        lhs, rhs, Instruction::OpID::fmul);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_fdiv(const LHS &lhs,
                                                    const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(
        lhs, rhs, Instruction::OpID::fdiv);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_ge(const LHS &lhs,
                                                      const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::ge);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_gt(const LHS &lhs,
                                                      const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::gt);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_le(const LHS &lhs,
                                                      const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::le);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_lt(const LHS &lhs,
                                                      const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::lt);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_eq(const LHS &lhs,
                                                      const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::eq);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_ne(const LHS &lhs,
                                                      const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::ne);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_fge(const LHS &lhs,
                                                       const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::fge);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_fgt(const LHS &lhs,
                                                       const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::fgt);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_fle(const LHS &lhs,
                                                       const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::fle);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_flt(const LHS &lhs,
                                                       const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::flt);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_feq(const LHS &lhs,
                                                       const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::feq);
}

template <typename LHS, typename RHS>
binary_op_match<LHS, RHS, Instruction::OpID> m_cmp_fne(const LHS &lhs,
                                                       const RHS &rhs) {
    return binary_op_match<LHS, RHS, Instruction::OpID>(lhs, rhs,
                                                        Instruction::OpID::fne);
}

// unary match
template <typename Op, typename Ty> struct unary_match {
    Ty T;
    Op O;

    unary_match(Op &O, const Ty &ty) : O(O), T(ty) {}

    template <typename ITy> bool match(ITy *V) {
        auto *CV = dynamic_cast<Instruction *>(V);
        if (CV != nullptr) {
            return O.match(CV->get_instr_type()) &&
                   T.match(CV->get_operand(0));
        }
        return false;
    }
};

template <typename Op, typename Ty>
unary_match<Op, Ty> m_unary(const Op &O, const Ty &ty) {
    return unary_match<Op, Ty>(O, ty);
}

template <typename Op, typename Ty> struct unary_op_match {
    Op &O;
    Ty &T;

    unary_op_match(const Op &op, const Ty &ty) : O(op), T(ty) {}

    template <typename ITy> bool match(ITy *V) {
        auto *CV = dynamic_cast<Instruction *>(V);
        if (CV != nullptr && CV->get_instr_type() == O) {
            return T.match(CV->get_operand(0));
        }
        return false;
    }
};

template <typename Ty>
unary_op_match<Instruction::OpID, Ty> m_zext(const Ty &ty) {
    return unary_op_match<Instruction::OpID, Ty>(Instruction::OpID::zext, ty);
}

template <typename Ty>
unary_op_match<Instruction::OpID, Ty> m_fptosi(const Ty &ty) {
    return unary_op_match<Instruction::OpID, Ty>(Instruction::OpID::fptosi, ty);
}

template <typename Ty>
unary_op_match<Instruction::OpID, Ty> m_sitofp(const Ty &ty) {
    return unary_op_match<Instruction::OpID, Ty>(Instruction::OpID::sitofp, ty);
}

template <typename Ty>
unary_op_match<Instruction::OpID, Ty> m_bitcast(const Ty &ty) {
    return unary_op_match<Instruction::OpID, Ty>(Instruction::OpID::bitcast,
                                                 ty);
}

template <typename Ty> struct specific_constant_int_match {
    Ty T;

    specific_constant_int_match(const Ty &ty) : T(ty) {}

    template <typename ITy> bool match(ITy *V) {
        auto *CV = dynamic_cast<ConstantInt *>(V);
        if (CV != nullptr) {
            return CV->get_value() == T;
        }
        return false;
    }
};

template <typename Ty>
specific_constant_int_match<Ty> m_specific_constant_int(const Ty &ty) {
    return specific_constant_int_match<Ty>(ty);
}
template <typename Ty> specific_constant_int_match<Ty> m_zero(const Ty &ty) {
    return specific_constant_int_match<Ty>(0);
}

template <typename COND, typename TRUE, typename FALSE> struct cond_br_match {
    COND C;
    TRUE T;
    FALSE F;

    cond_br_match(const COND &cond, const TRUE &true_val,
                  const FALSE &false_val)
        : C(cond), T(true_val), F(false_val) {}

    template <typename ITy> bool match(ITy *V) {
        auto *CV = dynamic_cast<Instruction *>(V);
        if (CV != nullptr && CV->get_instr_type() == Instruction::OpID::br) {
            if (!dynamic_cast<BranchInst *>(CV)->is_cond_br())
                return false;
            return C.match(CV->get_operand(0)) &&
                   T.match(CV->get_operand(1)) && F.match(CV->get_operand(2));
        }
        return false;
    }
};

template <typename COND, typename TRUE, typename FALSE>
cond_br_match<COND, TRUE, FALSE>
m_cond_br(const COND &cond, const TRUE &true_val, const FALSE &false_val) {
    return cond_br_match<COND, TRUE, FALSE>(cond, true_val, false_val);
}

}; // namespace PatternMatch