#include "GVN.hpp"

#include "BasicBlock.hpp"
#include "Constant.hpp"
#include "DeadCode.hpp"
#include "FuncInfo.hpp"
#include "Function.hpp"
#include "Instruction.hpp"
#include "logging.hpp"
#include "ilist.hpp"

#include <algorithm>
#include <fstream>
#include <memory>
#include <sstream>
#include <utility>
#include <unordered_map>

using namespace GVNExpression;
using std::string_literals::operator""s;
using std::shared_ptr;

static auto get_const_int_value = [](Value *v) {
    return dynamic_cast<ConstantInt *>(v)->get_value();
};
static auto get_const_fp_value = [](Value *v) {
    return dynamic_cast<ConstantFP *>(v)->get_value();
};

// Constant Propagation helper, folders are done for you
Constant *ConstFolder::compute(Instruction *instr, Constant *value1, Constant *value2) {
    auto op = instr->get_instr_type();
    switch (op) {
    case Instruction::add:
        return ConstantInt::get(get_const_int_value(value1) + get_const_int_value(value2), module_);
    case Instruction::sub:
        return ConstantInt::get(get_const_int_value(value1) - get_const_int_value(value2), module_);
    case Instruction::mul:
        return ConstantInt::get(get_const_int_value(value1) * get_const_int_value(value2), module_);
    case Instruction::sdiv:
        return ConstantInt::get(get_const_int_value(value1) / get_const_int_value(value2), module_);
    case Instruction::fadd:
        return ConstantFP::get(get_const_fp_value(value1) + get_const_fp_value(value2), module_);
    case Instruction::fsub:
        return ConstantFP::get(get_const_fp_value(value1) - get_const_fp_value(value2), module_);
    case Instruction::fmul:
        return ConstantFP::get(get_const_fp_value(value1) * get_const_fp_value(value2), module_);
    case Instruction::fdiv:
        return ConstantFP::get(get_const_fp_value(value1) / get_const_fp_value(value2), module_);
    case Instruction::eq:
        return ConstantInt::get(get_const_int_value(value1) == get_const_int_value(value2),
                                module_);
    case Instruction::ne:
        return ConstantInt::get(get_const_int_value(value1) != get_const_int_value(value2),
                                module_);
    case Instruction::gt:
        return ConstantInt::get(get_const_int_value(value1) > get_const_int_value(value2), module_);
    case Instruction::ge:
        return ConstantInt::get(get_const_int_value(value1) >= get_const_int_value(value2),
                                module_);
    case Instruction::lt:
        return ConstantInt::get(get_const_int_value(value1) < get_const_int_value(value2), module_);
    case Instruction::le:
        return ConstantInt::get(get_const_int_value(value1) <= get_const_int_value(value2),
                                module_);
    case Instruction::feq:
        return ConstantInt::get(get_const_fp_value(value1) == get_const_fp_value(value2), module_);
    case Instruction::fne:
        return ConstantInt::get(get_const_fp_value(value1) != get_const_fp_value(value2), module_);
    case Instruction::fgt:
        return ConstantInt::get(get_const_fp_value(value1) > get_const_fp_value(value2), module_);
    case Instruction::fge:
        return ConstantInt::get(get_const_fp_value(value1) >= get_const_fp_value(value2), module_);
    case Instruction::flt:
        return ConstantInt::get(get_const_fp_value(value1) < get_const_fp_value(value2), module_);
    case Instruction::fle:
        return ConstantInt::get(get_const_fp_value(value1) <= get_const_fp_value(value2), module_);
    default:
        return nullptr;
    }
}

Constant *ConstFolder::compute(Instruction *instr, Constant *value1) {
    auto op = instr->get_instr_type();
    switch (op) {
    case Instruction::sitofp:
        return ConstantFP::get(static_cast<float>(get_const_int_value(value1)), module_);
    case Instruction::fptosi:
        return ConstantInt::get(static_cast<int>(get_const_fp_value(value1)), module_);
    case Instruction::zext:
        return ConstantInt::get(static_cast<int>(get_const_int_value(value1)), module_);
    default:
        return nullptr;
    }
}

namespace utils {
    static std::string print_congruence_class(const CongruenceClass &cc) {
        std::stringstream ss;
        if (cc.index_ == 0) {
            ss << "top class\n";
            return ss.str();
        }
        ss << "\nindex: " << cc.index_ << "\nleader: " << cc.leader_->print()
           << "\nvalue phi: " << (cc.value_phi_ ? cc.value_phi_->print() : "nullptr"s)
           << "\nvalue expr: " << (cc.value_expr_ ? cc.value_expr_->print() : "nullptr"s)
           << "\nmembers: {";
        for (const auto &member : cc.members_) {
            ss << member->print() << "; ";
        }
        ss << "}\n";
        return ss.str();
    }

    static std::string dump_cc_json(const CongruenceClass &cc) {
        std::string json;
        json += "[";
        for (auto *member : cc.members_) {
            if (dynamic_cast<Constant *>(member) != nullptr) {
                json += member->print() + ", ";
            } else {
                json += "\"%" + member->get_name() + "\", ";
            }
        }
        json += "]";
        return json;
    }

    static std::string dump_partition_json(const GVN::partitions &p) {
        std::string json;
        json += "[";
        for (const auto &cc : p) {
            json += dump_cc_json(*cc) + ", ";
        }
        json += "]";
        return json;
    }

    static std::string dump_bb2partition(const std::map<BasicBlock *, GVN::partitions> &map) {
        std::string json;
        json += "{";
        for (auto [bb, p] : map) {
            json += "\"" + bb->get_name() + "\": " + dump_partition_json(p) + ",";
        }
        json += "}";
        return json;
    }

    // logging utility for you
    static void print_partitions(const GVN::partitions &p) {
        if (p.empty()) {
            LOG_DEBUG << "empty partitions\n";
            return;
        }
        std::string log;
        for (const auto &cc : p) {
            log += print_congruence_class(*cc);
        }
        LOG_DEBUG << log; // please don't use std::cout
    }
} // namespace utils

GVN::partitions GVN::join(const partitions &P1, const partitions &P2, BasicBlock *lbb, BasicBlock *rbb) {
    partitions out;
    for (auto &Ci : P1) {
        for (auto &Cj : P2) {
            if (GVNExpression::operator==(Ci->value_expr_, Cj->value_expr_)) {
                auto cc = intersect(Ci, Cj, lbb, rbb);
                if (cc && !cc->members_.empty()) {
                    out.insert(cc);
                }
            }
        }
    }
    return out;
}

std::shared_ptr<CongruenceClass> GVN::intersect(const shared_ptr<CongruenceClass> &Ci,
                                                const shared_ptr<CongruenceClass> &Cj,
                                                BasicBlock *lbb,
                                                BasicBlock *rbb) {
    auto cc = createCongruenceClass(next_value_number_++);
    // 1) 交集成员
    for (auto *v : Ci->members_) {
        if (Cj->members_.count(v)) {
        cc->members_.insert(v);
        }
    }
    if (cc->members_.empty())
        return nullptr;

    // 2) 表达式相同时继承
    if (Ci->value_expr_ && Cj->value_expr_
        && *Ci->value_expr_ == *Cj->value_expr_) {
        cc->value_expr_ = Ci->value_expr_;
    }

    // 3) Leader 继承：优先常量，其次相同 leader，其次空
    // 3.1 如果表达式本身就是常量表达式
    if (auto ce = std::dynamic_pointer_cast<ConstantExpression>(cc->value_expr_)) {
        cc->leader_ = ce->get_constant();
    }
    // 3.2 否则，如果 Ci 和 Cj 的 leader 指向同一个非空值，也继承
    else if (Ci->leader_ && Ci->leader_ == Cj->leader_) {
        cc->leader_ = Ci->leader_;
    }
    // 否则保持 leader_ = nullptr，后面 transferFunction 会给成员重新选 leader
    if (!cc->leader_) {
    // 比如，优先选取交集里第一个常量成员：
        for (auto *m : cc->members_) {
            if (auto c = dynamic_cast<Constant*>(m)) {
            cc->leader_ = c;
            break;
            }
        }
        // 再不行，就随便用 members_.begin()：
        if (!cc->leader_ && !cc->members_.empty()) {
            cc->leader_ = *cc->members_.begin();
        }
    }

    return cc;
}

void GVN::detectEquivalences(ilist<GlobalVariable> *global_list) {
    auto top = std::set<shared_ptr<CongruenceClass>>();
    top.insert(createCongruenceClass(0)); // 初始化top等价类
    bool changed = false;

    // 所有块的 pout 初始为 top
    for (auto &bb : func_->get_basic_blocks()) {
        pout_[&bb] = top;
    }

    // Entry块 pin 设为 ∅
    BasicBlock *entry = func_->get_entry_block();
    pin_[entry] = {};  // 空集，表示没有前驱

    // 💡 TODO-1: 设置 entry 之后其他 block 的 pin 也是空集，这一步你也可以删掉

    do {
        changed = false;

        for (auto &bb : func_->get_basic_blocks()) {
            GVN::partitions p;
            LOG_INFO << "[BB=" << bb.get_name() << "] partitionOut:";
            for (auto &cc : pout_[&bb]) {
            LOG_INFO << utils::dump_cc_json(*cc);
            }

            // ✅ TODO-2: 计算当前 bb 的 pin，通过 join 所有前驱的 pout 得到
            if (&bb != entry) {
                p = top;
                auto preds=bb.get_pre_basic_blocks();
                for (auto *pred : preds) {
                    p = join(p, pout_[pred], pred, &bb);
                }
                pin_[&bb] = p;      // ← 更新 pin_
            } else {
                pin_[&bb] = {};     // entry 的 pin 还是空集
            }
            pin_[&bb] = p;

            // 遍历 block 内指令，对每条应用 transferFunction
            for (auto &instr : bb.get_instructions()) {
                p = transferFunction(&instr, &instr, p);
            }

            // 判断是否收敛
            if (p != pout_[&bb]) {
                changed = true;
            }
            pout_[&bb] = std::move(p);
        }
    } while (changed);
}

//oi！这里做的是很粗的，正常有九种instr->isBinary() || instr->is_si2fp() || instr->is_fp2si() || instr->is_zext() ||
        //    instr->is_cmp() || instr->is_fcmp()||instr->is_load()
        //    instr->is_store() || instr->is_call() || instr->is_phi() || instr->is_alloca()|| instr->is_gep()
shared_ptr<Expression> GVN::valueExpr(const partitions &pout, Value *v) {
    return valueExprHelper(pout, v, std::set<Value*>());
}

shared_ptr<Expression> GVN::valueExprHelper(const partitions &pout, Value *v, std::set<Value*> visited, int depth) {
    // 防止循环引用导致的无限递归
    if (visited.count(v) > 0) {
        LOG_DEBUG << "Circular reference detected for value: " << v->get_name() 
                  << ", treating as variable expression";
        return GVNExpression::VariableExpression::create(v);
    }
    
    // 限制表达式深度，避免过长的表达式
    if (depth > MAX_EXPRESSION_DEPTH) {
        LOG_DEBUG << "Expression depth limit reached for value: " << v->get_name() 
                  << ", treating as variable expression";
        return GVNExpression::VariableExpression::create(v);
    }
    
    // 添加当前值到访问集合
    visited.insert(v);
    
    if (auto c = dynamic_cast<Constant *>(v)) {
        return GVNExpression::ConstantExpression::create(c);
    }
    auto instr = dynamic_cast<Instruction *>(v);
    if (!instr || instr->is_void()) {
        if (!dynamic_cast<Instruction *>(v)) { // 非指令、非常量，视为变量
            return GVNExpression::VariableExpression::create(v);
        }
        return nullptr;
    }
    if (auto callInst = dynamic_cast<CallInst *>(instr)) {
        // 简单起见，把call指令当作变量表达式处理
        return GVNExpression::VariableExpression::create(callInst);
    }
    auto op = instr->get_instr_type();

    if (instr->isBinary()) {
        auto lhs = instr->get_operand(0);
        auto rhs = instr->get_operand(1);
        auto lhs_expr = valueExprHelper(pout, lhs, visited, depth + 1);
        auto rhs_expr = valueExprHelper(pout, rhs, visited, depth + 1);
        
        if (!lhs_expr || !rhs_expr) return nullptr;
        
        // 规范化：确保操作数顺序一致（如加法交换律）
        if (op == Instruction::add || op == Instruction::mul) {
            if (lhs_expr->get_expr_type() == Expression::e_const) {
                std::swap(lhs_expr, rhs_expr);
            }
        }
        
        auto expr = GVNExpression::BinaryExpression::create(op, lhs_expr, rhs_expr);
        return expr;
    }

    if (instr->is_cmp()) {
        auto lhs = instr->get_operand(0);
        auto rhs = instr->get_operand(1);
        auto lhs_expr = valueExprHelper(pout, lhs, visited, depth + 1);
        auto rhs_expr = valueExprHelper(pout, rhs, visited, depth + 1);
        if (!lhs_expr || !rhs_expr) return nullptr;
        
        auto expr = GVNExpression::CmpExpression::create(op, lhs_expr, rhs_expr);
        return expr;
    }

    if (op == Instruction::sitofp || op == Instruction::fptosi || op == Instruction::zext) {
        auto operand = instr->get_operand(0);
        auto operand_expr = valueExprHelper(pout, operand, visited, depth + 1);
        if (!operand_expr) return nullptr;
        
        auto expr = GVNExpression::UnaryExpression::create(op, operand_expr);
        return expr;
    }

    if (instr->is_gep()) {
        // 注释掉GEP指令处理
        /*
        // 处理GEP指令 (getelementptr) - 优化版本
        std::vector<std::shared_ptr<GVNExpression::Expression>> indices;
        
        // 获取基础指针，并进行规范化处理
        auto base_ptr = instr->get_operand(0);
        
        // 重要优化：如果基础指针是全局变量，直接使用全局变量本身
        // 这样可以确保所有指向同一全局变量的GEP指令有相同的基础指针
        if (auto global_var = dynamic_cast<GlobalVariable*>(base_ptr)) {
            base_ptr = global_var;
        }
        
        // 新增：安全检查 - 如果这个GEP指令在函数调用之后，标记为不可合并
        bool after_call = isAfterFunctionCall(instr);
        
        // 处理所有索引操作数
        for (unsigned i = 1; i < instr->get_num_operand(); i++) {
            auto operand = instr->get_operand(i);
            auto index_expr = valueExprHelper(pout, operand, visited, depth + 1);
            if (!index_expr) return nullptr;
            
            // 尝试常量折叠：如果索引是常量表达式，直接使用常量值
            if (dynamic_cast<GVNExpression::ConstantExpression*>(index_expr.get())) {
                indices.push_back(index_expr);
            } else if (auto bin_expr = dynamic_cast<GVNExpression::BinaryExpression*>(index_expr.get())) {
                // 尝试常量折叠二元表达式
                auto folded = tryConstantFold(bin_expr);
                if (folded) {
                    indices.push_back(folded);
                } else {
                    indices.push_back(index_expr);
                }
            } else {
                indices.push_back(index_expr);
            }
        }
        
        // 创建GEP表达式，如果指令在函数调用之后，添加特殊标记
        auto gep_expr = std::make_shared<GVNExpression::GEPExpression>(
            base_ptr, indices, after_call);
        
        return gep_expr;
        */
        return nullptr;
    } else if (instr->is_load()) {
        // 注释掉load指令处理
        /*
        // 获取load指令的指针操作数
        auto ptr_operand = instr->get_operand(0);
        
        // 新增：Load-Store转发优化
        // 如果这个load紧跟在store之后，且指向同一内存位置，直接使用store的值
        // 移除阈值限制，确保常量表达式优化不受影响
        if (instr->get_parent()) {
            auto bb = instr->get_parent();
            bool found_store_forward = false;
            
            // 检查：遍历基本块中的指令，寻找Load-Store转发机会
            for (auto& bb_instr : bb->get_instructions()) {
                if (&bb_instr == instr) {
                    break;  // 到达当前load指令，停止检查
                }
                if (bb_instr.is_store()) {
                    auto store_addr = bb_instr.get_operand(1);
                    if (store_addr == ptr_operand) {
                        // 发现Load-Store转发机会！
                        // 直接返回store的值，而不是创建LoadExpression
                        auto store_value = bb_instr.get_operand(0);
                        LOG_DEBUG << "Load-Store forwarding: " << instr->print() 
                                  << " -> " << store_value->get_name();
                        return valueExprHelper(pout, store_value, visited, depth + 1);
                    }
                }
            }
        }
        
        // 确保ptr_expr被创建，用于LoadExpression
        auto ptr_expr = valueExprHelper(pout, ptr_operand, visited, depth + 1);
        if (!ptr_expr) return nullptr;
        
        // 简化策略：如果这个load指令在store之后，分配唯一ID
        // 这样可以确保store后的load不会被错误合并
        // 移除阈值限制，确保常量表达式优化不受影响
        if (instr->get_parent()) {
            auto bb = instr->get_parent();
            bool has_store_before = false;
            
            // 检查：如果基本块中有store指令，且load在store之后，则分配唯一ID
            for (auto& bb_instr : bb->get_instructions()) {
                if (bb_instr.is_store()) {
                    has_store_before = true;
                    break;
                }
                if (&bb_instr == instr) {
                    break;  // 到达当前load指令，停止检查
                }
            }
            
            if (has_store_before) {
                load_id = ++load_counter_;
            }
        }
        
        // 创建LoadExpression，表示从某个地址加载的值
        // 使用唯一的load_id来区分不同的load指令
        size_t load_id = 0;  // 默认使用0
        
        auto expr = GVNExpression::LoadExpression::create(ptr_expr, load_id);
        LOG_DEBUG << "Created LoadExpression with ID " << load_id
                  << " for pointer: " << ptr_expr->print() 
                  << " in instruction: " << instr->print();
        return expr;
        */
        return nullptr;
    }

    if (instr->is_phi()) {
        std::vector<std::shared_ptr<GVNExpression::Expression>> ops;
        for (unsigned i = 0; i < instr->get_num_operand(); i++) {
            auto op_expr = valueExprHelper(pout, instr->get_operand(i), visited, depth + 1);
            if (!op_expr) return nullptr;
            ops.push_back(op_expr);
        }
        
        auto expr = GVNExpression::PhiExpression::create(ops);
        return expr;
    }

    LOG_DEBUG << "Unhandled instruction in valueExpr: " << instr->print();
    return GVNExpression::VariableExpression::create(instr);
}


GVN::partitions GVN::transferFunction(Instruction *x, Value *e, const partitions &pin) {
    partitions pout = clone(pin);

    if (x->is_void()) {
        return pout;
    }
    
    // 新增：处理store指令，标记内存修改
    /*
    if (x->is_store()) {
        auto store_addr = x->get_operand(1); // store指令的第二个操作数是地址
        markMemoryModified(store_addr);
        LOG_DEBUG << "Detected store instruction, marked memory as modified: " << store_addr->get_name();
    }
    */
    
    // 新增：处理GEP指令，添加调试信息
    /*
    if (x->is_gep()) {
        LOG_DEBUG << "Processing GEP instruction: " << x->print();
        // GEP指令的优化将在valueExpr中处理
    }
    */
    
    // 新增：处理load指令，添加调试信息
    /*
    if (x->is_load()) {
        LOG_DEBUG << "Processing load instruction: " << x->print();
    }
    */

    // 移除 x 在 pout 中已有的等价类
    for (auto &cc : pout) {
        cc->members_.erase(x);
    }

    auto ve  = valueExpr(pout, dynamic_cast<Instruction*>(e));
    auto vpf = valuePhiFunc(ve, pin);
    
    // 新增：尝试重用现有表达式
    if (ve) {
        auto reused_ve = tryReuseExpression(ve, pin, pout);
        if (reused_ve) {
            ve = reused_ve;
        }
    }

    // 1. 如果 ve/vpf 在已有同余类里，直接加入并更新 leader（如果是常量）
    // 优化：使用更高效的查找策略
    bool found_existing = false;
    for (auto &Ci : pout) {
        // 添加空指针检查，避免解引用空指针导致段错误
        if ((Ci->value_expr_ && ve && *Ci->value_expr_ == *ve) || 
            (Ci->value_phi_ && vpf && *Ci->value_phi_ == *vpf)) {
            
            // 新增：调试信息，显示表达式比较
            if (Ci->value_expr_ && ve) {
                LOG_DEBUG << "Expression comparison: existing=" << Ci->value_expr_->print() 
                          << " vs new=" << ve->print() 
                          << " result: " << (*Ci->value_expr_ == *ve);
            }
            
            // 新增：检查是否应该阻止合并（内存状态相关）
            /*
            if (shouldPreventMerge(x, ve, Ci)) {
                LOG_DEBUG << "Preventing merge due to memory state: " << x->print();
                continue; // 跳过这个等价类，继续查找
            }
            */
            
            LOG_DEBUG << "Found existing congruence class for instruction: " << x->print() 
                      << " with ve: " << (ve ? ve->print() : "nullptr");
            Ci->members_.insert(x);
            // 如果 ve 本身是 ConstantExpression，记得把 leader 也指向这个常量
            if (auto ce = std::dynamic_pointer_cast<ConstantExpression>(ve)) {
                Ci->leader_     = ce->get_constant();
                Ci->value_expr_ = ve;
            }
            found_existing = true;
            break;
        }
    }
    
    if (found_existing) {
        return pout;
    }

    // 2. 新建同余类前，尝试常量折叠
    if (auto be = std::dynamic_pointer_cast<BinaryExpression>(ve)) {
        // 用公有 getter 拿左右 expr，再 dynamic_pointer_cast
        auto lhs_ce = std::dynamic_pointer_cast<ConstantExpression>(be->get_lhs());
        auto rhs_ce = std::dynamic_pointer_cast<ConstantExpression>(be->get_rhs());
        if (lhs_ce && rhs_ce) {
            if (auto folded = folder_->compute(x,
                lhs_ce->get_constant(),
                rhs_ce->get_constant())) {
                ve = ConstantExpression::create(folded);
            }
        }
    }
    if (auto ue = std::dynamic_pointer_cast<UnaryExpression>(ve)) {
        auto op_ce = std::dynamic_pointer_cast<ConstantExpression>(ue->get_operand());
        if (op_ce) {
            if (auto folded = folder_->compute(x, op_ce->get_constant())) {
                ve = ConstantExpression::create(folded);
            }
        }
    }

    // 3. 构造并插入新同余类
    auto cc = createCongruenceClass(next_value_number_++);
    cc->members_    = {x};
    if (!ve) {
        LOG_ERROR << "valueExpr returned nullptr for binary instr: " << x->print();
    } else {
        LOG_INFO  << "Creating CC with ve=" << ve->print();
    }

    cc->value_phi_  = vpf;
    // 如果折叠之后 ve 是常量，就把 leader 设为那个常量
    if (auto ce = std::dynamic_pointer_cast<ConstantExpression>(ve)) {
        cc->leader_ = ce->get_constant();
    }
    // 在插入之前，给非 ConstantExpression 的 ve 也挑一个代表：
    if (!cc->leader_) {
        cc->leader_ = x;  // 例如直接把当前指令当作初始 leader
    }

    cc->value_expr_ = ve;
    pout.insert(cc);
    return pout;
}

shared_ptr<Expression> GVN::valuePhiFunc(
    const shared_ptr<Expression> &ve,
    const partitions &P) 
{
    // 尝试把 ve 转成二元表达式
    auto bin = std::dynamic_pointer_cast<BinaryExpression>(ve);
    if (bin && bin->both_phi()) {
        // 将左右操作数都转成 PhiExpression
        auto left_phi  = std::dynamic_pointer_cast<PhiExpression>(bin->get_lhs());
        auto right_phi = std::dynamic_pointer_cast<PhiExpression>(bin->get_rhs());
        assert(left_phi && right_phi &&
               left_phi->get_operands().size() == right_phi->get_operands().size());

        std::vector<std::shared_ptr<Expression>> new_operands;
        auto &lops = left_phi->get_operands();
        auto &rops = right_phi->get_operands();
        // 对每一个分支 i，都生成一个 (op lops[i] rops[i]) 的子表达式
        for (size_t i = 0; i < lops.size(); ++i) {
            new_operands.push_back(
                BinaryExpression::create(
                    bin->get_op(),
                    lops[i],
                    rops[i]
                )
            );
        }
        // 最终返回一个 PhiExpression，其每个分支都是上面新组的二元表达式
        return PhiExpression::create(new_operands);
    }
    // 其他情况暂不做 Phi 推导
    return nullptr;
}


shared_ptr<Expression> GVN::getVN(const partitions &pout, shared_ptr<Expression> ve) {
    for (const auto &cc : pout) {
        if (cc->value_expr_ && *(cc->value_expr_) == *ve) {
            return cc->value_expr_;
        }
    }
    return ve;
}


void GVN::initPerFunction() {
    next_value_number_ = 1;
    pin_.clear();
    pout_.clear();
    
    // 清理表达式缓存，避免跨函数污染
    clearExpressionCache();
    
    // 新增：清理内存状态，避免跨函数污染
    /*
    clearMemoryState();
    */
}

void GVN::replace_cc_members() {
    for (auto &[_bb, part] : pout_) {
        auto *bb = _bb; // workaround: structured bindings can't be captured in C++17
        for (const auto &cc : part) {
            if (cc->index_ == 0) {
                continue;
            }
            // if you are planning to do constant propagation, leaders should be set to constant at
            // some point
            for (const auto &member : cc->members_) {
                bool member_is_phi = dynamic_cast<PhiInst *>(member) != nullptr;
                bool value_phi     = cc->value_phi_ != nullptr;
                if (member != cc->leader_ and (value_phi or !member_is_phi)) {
                    // only replace the members if users are in the same block as bb
                    member->replace_use_with_if(cc->leader_, [bb](Use use) {
                        auto *user = use.val_;
                        if (auto *instr = dynamic_cast<Instruction *>(user)) {
                            auto *parent = instr->get_parent();
                            auto &bb_pre = parent->get_pre_basic_blocks();
                            if (instr->is_phi()) { // as copy stmt, the phi belongs to this block
                                return std::find(bb_pre.begin(), bb_pre.end(), bb) != bb_pre.end();
                            }
                            return parent == bb;
                        }
                        return false;
                    });
                }
            }
        }
    }
}

// top-level function, done for you
void GVN::run() {
   // m_->set_print_name();
    std::ofstream gvn_json;
    if (dump_json_) {
        gvn_json.open("gvn.json", std::ios::out);
        gvn_json << "[";
    }

    folder_    = std::make_unique<ConstFolder>(m_);
    func_info_ = std::make_unique<FuncInfo>(m_);
    func_info_->run();
    dce_ = std::make_unique<DeadCode>(m_);
    dce_->run(); // let dce take care of some dead phis with undef

    for (auto &f : m_->get_functions()) {
        if (f.get_basic_blocks().empty()) {
            continue;
        }
        func_ = &f;
        initPerFunction();
        LOG_INFO << "Processing " << f.get_name();
        detectEquivalences(&m_->get_global_variable());
        LOG_INFO << "===============pin=========================\n";
        for (auto &[bb, part] : pin_) {
            LOG_INFO << "\n===============bb: " << bb->get_name()
                     << "=========================\npartitionIn: ";
            for (const auto &cc : part) {
                LOG_DEBUG << f.get_name();
                LOG_INFO << utils::print_congruence_class(*cc);
            }
        }
        LOG_INFO << "\n===============pout=========================\n";
        for (auto &[bb, part] : pout_) {
            LOG_INFO << "\n=====bb: " << bb->get_name() << "=====\npartitionOut: ";
            for (const auto &cc : part) {
                LOG_DEBUG << f.get_name();
                LOG_INFO << utils::print_congruence_class(*cc);
            }
        }
        if (dump_json_) {
            gvn_json << "{\n\"function\": ";
            gvn_json << "\"" << f.get_name() << "\", ";
            gvn_json << "\n\"pout\": " << utils::dump_bb2partition(pout_);
            gvn_json << "},";
        }
        replace_cc_members(); // don't delete instructions, just replace them
    }
    dce_->run(); // let dce do that for us
    if (dump_json_) {
        gvn_json << "]";
    }
}

template<typename T>
static bool equiv_as(const Expression &lhs, const Expression &rhs) {
    // we use static_cast because we are very sure that both operands are actually T, not other
    // types.
    return static_cast<const T *>(&lhs)->equiv(static_cast<const T *>(&rhs));
}

bool GVNExpression::operator==(const Expression &lhs, const Expression &rhs) {
    // 添加有效性检查，避免访问无效的表达式对象
    try {
        if (lhs.get_expr_type() != rhs.get_expr_type()) {
            return false;
        }

        switch (lhs.get_expr_type()) {
        case Expression::e_bin:
            return equiv_as<BinaryExpression>(lhs, rhs);

        case Expression::e_const:
            return equiv_as<ConstantExpression>(lhs, rhs);

        case Expression::e_phi:
            return equiv_as<PhiExpression>(lhs, rhs);

        case Expression::e_var:
            return equiv_as<VariableExpression>(lhs, rhs);

        case Expression::e_gep:
            // 注释掉GEP表达式比较
            /*
            return equiv_as<GEPExpression>(lhs, rhs);
            */
            return false;

        case Expression::e_cmp:
            return equiv_as<CmpExpression>(lhs, rhs);

        case Expression::e_unary:
            return equiv_as<UnaryExpression>(lhs, rhs);

        case Expression::e_load:
            // 注释掉LoadExpression比较
            /*
            return equiv_as<LoadExpression>(lhs, rhs);
            */
            return false;

        default:
            return false;
        }
    } catch (...) {
        // 如果访问表达式时出现异常，说明表达式对象无效，返回false
        return false;
    }
}



bool GVNExpression::operator==(const shared_ptr<Expression> &lhs,
                               const shared_ptr<Expression> &rhs) {
    return lhs and rhs and *lhs == *rhs;
}

GVN::partitions GVN::clone(const partitions &p) {
    partitions data;
    for (const auto &cc : p) {
        data.insert(std::make_shared<CongruenceClass>(*cc));
    }
    return data;
}

bool operator==(const GVN::partitions &p1, const GVN::partitions &p2) {
    if (p1.size() != p2.size()) {
        return false;
    }
    for (auto it1 = p1.begin(), it2 = p2.begin(); it1 != p1.end(); ++it1, ++it2) {
        if (!(**it1 == **it2)) {
            return false;
        }
    }
    return true;
}

bool CongruenceClass::operator==(const CongruenceClass &other) const {
    if (leader_ != other.leader_) {
        return false;
    }
    if (members_.size() != other.members_.size()) {
        return false;
    }
    for (auto &m : members_) {
        if (other.members_.count(m) == 0) {
            return false;
        }
    }
    return true;
}

// 尝试常量折叠二元表达式
std::shared_ptr<GVNExpression::Expression> GVN::tryConstantFold(
    const GVNExpression::BinaryExpression* bin_expr) {
    
    // 检查两个操作数是否都是常量
    auto left_const = std::dynamic_pointer_cast<GVNExpression::ConstantExpression>(bin_expr->get_lhs());
    auto right_const = std::dynamic_pointer_cast<GVNExpression::ConstantExpression>(bin_expr->get_rhs());
    
    if (!left_const || !right_const) {
        return nullptr; // 不是两个常量，无法折叠
    }
    
    // 获取常量值
    auto left_const_int = dynamic_cast<ConstantInt*>(left_const->get_constant());
    auto right_const_int = dynamic_cast<ConstantInt*>(right_const->get_constant());
    
    if (!left_const_int || !right_const_int) {
        return nullptr; // 不是整数常量，无法折叠
    }
    
    int left_val = left_const_int->get_value();
    int right_val = right_const_int->get_value();
    int result = 0;
    
    // 根据操作符进行常量折叠
    switch (bin_expr->get_op()) {
        case Instruction::add:
            result = left_val + right_val;
            break;
        case Instruction::sub:
            result = left_val - right_val;
            break;
        case Instruction::mul:
            result = left_val * right_val;
            break;
        case Instruction::sdiv:
            if (right_val == 0) return nullptr; // 除零错误
            result = left_val / right_val;
            break;
        case Instruction::srem:
            if (right_val == 0) return nullptr; // 除零错误
            result = left_val % right_val;
            break;
        default:
            return nullptr; // 不支持的操作符
    }
    
    // 创建新的常量表达式
    auto const_val = ConstantInt::get(result, m_);
    return GVNExpression::ConstantExpression::create(const_val);
}

// 新增：尝试重用现有表达式
std::shared_ptr<GVNExpression::Expression> GVN::tryReuseExpression(
    std::shared_ptr<GVNExpression::Expression> ve,
    const partitions& pin,
    const partitions& pout) {
    
    if (!ve) return nullptr;
    
    // 特殊优化：对于GEP表达式，优先查找完全相同的表达式
    /*
    if (ve->get_expr_type() == Expression::e_gep) {
        auto gep_expr = std::dynamic_pointer_cast<GVNExpression::GEPExpression>(ve);
        if (gep_expr) {
            // 在当前基本块的输出分区中查找语义相同的GEP表达式
            for (const auto& cc : pout) {
                if (cc->value_expr_ && cc->value_expr_->get_expr_type() == Expression::e_gep) {
                    auto existing_gep = std::dynamic_pointer_cast<GVNExpression::GEPExpression>(cc->value_expr_);
                    if (existing_gep && ve->equiv(cc->value_expr_.get())) {
                        // 新增：额外的安全检查 - 确保两个GEP指令可以安全合并
                        if (gep_expr->is_after_call() == existing_gep->is_after_call()) {
                            LOG_DEBUG << "Reusing existing GEP expression: " << ve->print() << " -> " << cc->value_expr_->print();
                            return cc->value_expr_;
                        } else {
                            LOG_DEBUG << "GEP expressions have different timing, cannot reuse: " 
                                     << ve->print() << " vs " << cc->value_expr_->print();
                        }
                    }
                }
            }
            
            // 在输入分区中查找
            for (const auto& cc : pin) {
                if (cc->value_expr_ && cc->value_expr_->get_expr_type() == Expression::e_gep) {
                    auto existing_gep = std::dynamic_pointer_cast<GVNExpression::GEPExpression>(cc->value_expr_);
                    if (existing_gep && ve->equiv(cc->value_expr_.get())) {
                        // 新增：额外的安全检查
                        if (gep_expr->is_after_call() == existing_gep->is_after_call()) {
                            LOG_DEBUG << "Reusing GEP expression from input partition: " << ve->print() << " -> " << cc->value_expr_->print();
                            return cc->value_expr_;
                        }
                    }
                }
            }
        }
    }
    */
    
    // 对于其他类型的表达式，使用原有的重用逻辑
    for (const auto& cc : pout) {
        if (cc->value_expr_ && ve->equiv(cc->value_expr_.get())) {
            return cc->value_expr_;
        }
    }
    
    for (const auto& cc : pin) {
        if (cc->value_expr_ && ve->equiv(cc->value_expr_.get())) {
            return cc->value_expr_;
        }
    }
    
    return nullptr;
}

// 新增：表达式缓存和规范化相关函数实现

// 获取或创建表达式（缓存机制）
std::shared_ptr<GVNExpression::Expression> GVN::getOrCreateExpression(std::shared_ptr<GVNExpression::Expression> expr) {
    if (!expr) return nullptr;
    
    // 规范化表达式
    auto canonical_expr = canonicalizeExpression(expr);
    
    return canonical_expr;
}

// 规范化表达式
std::shared_ptr<GVNExpression::Expression> GVN::canonicalizeExpression(std::shared_ptr<GVNExpression::Expression> expr) {
    if (!expr) return nullptr;
    
    switch (expr->get_expr_type()) {
        case Expression::e_bin: {
            auto bin_expr = std::dynamic_pointer_cast<BinaryExpression>(expr);
            if (!bin_expr) return expr;
            
            auto lhs = bin_expr->get_lhs();
            auto rhs = bin_expr->get_rhs();
            auto op = bin_expr->get_op();
            
            // 对于加法和乘法，确保常量在右边
            if (op == Instruction::add || op == Instruction::mul) {
                if (lhs->get_expr_type() == Expression::e_const && 
                    rhs->get_expr_type() != Expression::e_const) {
                    std::swap(lhs, rhs);
                }
            }
            
            // 递归规范化子表达式
            auto canonical_lhs = canonicalizeExpression(lhs);
            auto canonical_rhs = canonicalizeExpression(rhs);
            
            // 如果子表达式没有变化，返回原表达式
            if (canonical_lhs == lhs && canonical_rhs == rhs) {
                return expr;
            }
            
            return BinaryExpression::create(op, canonical_lhs, canonical_rhs);
        }
        
        case Expression::e_gep: {
            auto gep_expr = std::dynamic_pointer_cast<GEPExpression>(expr);
            if (!gep_expr) return expr;
            
            // 注释掉GEP表达式规范化
            /*
            // 对于GEP表达式，进行深度规范化
            auto base_ptr = gep_expr->get_base_ptr();
            auto indices = gep_expr->get_indices();
            
            // 递归规范化索引表达式
            std::vector<std::shared_ptr<Expression>> canonical_indices;
            bool indices_changed = false;
            
            for (auto& index : indices) {
                auto canonical_index = canonicalizeExpression(index);
                canonical_indices.push_back(canonical_index);
                if (canonical_index != index) {
                    indices_changed = true;
                }
            }
            
            // 如果索引没有变化，返回原表达式
            if (!indices_changed) {
                return expr;
            }
            
            // 创建新的规范化GEP表达式
            return GEPExpression::create(base_ptr, canonical_indices);
            */
            return expr;
        }
        
        case Expression::e_load: {
            auto load_expr = std::dynamic_pointer_cast<LoadExpression>(expr);
            if (!load_expr) return expr;
            
            auto ptr_expr = load_expr->get_ptr_expr();
            auto canonical_ptr = canonicalizeExpression(ptr_expr);
            
            // 注释掉LoadExpression规范化
            /*
            if (canonical_ptr == ptr_expr) {
                return expr;
            }
            
            // 修复：保持原来的load_id，确保唯一性不丢失
            return LoadExpression::create(canonical_ptr, load_expr->get_load_id());
            */
            return expr;
        }
        
        default:
            return expr;
    }
}

// 新增：GEP表达式规范化函数
/*
std::shared_ptr<GVNExpression::Expression> GVN::canonicalizeGEPExpression(
    std::shared_ptr<GVNExpression::GEPExpression> gep_expr) {
    
    if (!gep_expr) return nullptr;
    
    auto base_ptr = gep_expr->get_base_ptr();
    auto indices = gep_expr->get_indices();
    
    // 优化1：如果所有索引都是常量，尝试预计算
    bool all_constants = true;
    std::vector<int> const_values;
    
    for (const auto& index : indices) {
        if (auto const_expr = std::dynamic_pointer_cast<GVNExpression::ConstantExpression>(index)) {
            if (auto const_int = dynamic_cast<ConstantInt*>(const_expr->get_constant())) {
                const_values.push_back(const_int->get_value());
            } else {
                all_constants = false;
                break;
            }
        } else {
            all_constants = false;
            break;
        }
    }
    
    // 优化2：如果所有索引都是常量，尝试常量折叠
    if (all_constants && indices.size() >= 2) {
        // 对于数组访问 a[i][j]，如果i和j都是常量，可以预计算偏移
        // 这里我们保持原有的GEP结构，但确保索引被规范化
        return gep_expr;
    }
    
    // 优化3：规范化索引表达式
    std::vector<std::shared_ptr<GVNExpression::Expression>> canonical_indices;
    bool changed = false;
    
    for (const auto& index : indices) {
        auto canonical_index = canonicalizeExpression(index);
        canonical_indices.push_back(canonical_index);
        if (canonical_index != index) {
            changed = true;
        }
    }
    
    if (changed) {
        return GVNExpression::GEPExpression::create(base_ptr, canonical_indices);
    }
    
    return gep_expr;
}
*/

// 清理表达式缓存
void GVN::clearExpressionCache() {
    // 目前没有缓存需要清理
}

// 新增：内存状态跟踪相关函数实现

// 标记内存位置为已修改
/*
void GVN::markMemoryModified(Value* memory_location) {
    if (!memory_location) return;
    
    modified_memory_locations_.insert(memory_location);
    
    // 如果是全局变量，也标记为已修改
    if (auto global_var = dynamic_cast<GlobalVariable*>(memory_location)) {
        modified_global_vars_.insert(global_var);
    }
    
    LOG_DEBUG << "Marked memory location as modified: " << memory_location->get_name();
}
*/

// 检查内存位置是否已被修改
/*
bool GVN::isMemoryModified(Value* memory_location) {
    if (!memory_location) return false;
    
    // 检查具体的内存位置
    if (modified_memory_locations_.count(memory_location) > 0) {
        return true;
    }
    
    // 检查全局变量
    if (auto global_var = dynamic_cast<GlobalVariable*>(memory_location)) {
        return modified_global_vars_.count(global_var) > 0;
    }
    
    return false;
}
*/

// 清理内存状态
/*
void GVN::clearMemoryState() {
    modified_memory_locations_.clear();
    modified_global_vars_.clear();
    LOG_DEBUG << "Cleared memory state";
}
*/

// 新增：检查是否应该阻止合并
/*
bool GVN::shouldPreventMerge(Instruction* instr, std::shared_ptr<GVNExpression::Expression> ve, 
                             std::shared_ptr<CongruenceClass> cc) {
    if (!instr || !ve || !cc) return false;
    
    // 如果当前指令是load，检查是否应该阻止合并
    if (instr->is_load()) {
        auto ptr_operand = instr->get_operand(0);
        
        // 检查内存是否已被修改
        if (isMemoryModified(ptr_operand)) {
            LOG_DEBUG << "Preventing load merge due to modified memory: " << ptr_operand->get_name();
            return true;
        }
        
        // 检查等价类中是否已经有其他load指令
        for (const auto& member : cc->members_) {
            if (auto member_instr = dynamic_cast<Instruction*>(member)) {
                if (member_instr->is_load()) {
                    auto member_ptr = member_instr->get_operand(0);
                    // 如果两个load指向相同的内存位置，且内存被修改过，阻止合并
                    if (ptr_operand == member_ptr && isMemoryModified(ptr_operand)) {
                        LOG_DEBUG << "Preventing load merge due to modified memory in same location";
                        return true;
                    }
                }
            }
        }
    }
    
    return false;
}
*/

// 新增：检查指令是否在函数调用之后
/*
bool GVN::isAfterFunctionCall(Instruction* instr) {
    if (!instr || !instr->get_parent()) return false;
    
    auto bb = instr->get_parent();
    auto instrs = bb->get_instructions();
    
    // 使用简单的循环来查找指令位置
    size_t instr_pos = 0;
    bool found = false;
    
    for (const auto& inst : instrs) {
        if (&inst == instr) {
            found = true;
            break;
        }
        instr_pos++;
    }
    
    if (!found) return false;
    
    // 向前查找，看是否有函数调用
    size_t i = 0;
    for (const auto& inst : instrs) {
        if (i >= instr_pos) break;
        if (inst.is_call()) {
            return true;
        }
        i++;
    }
    
    return false;
}
*/

// 新增：检查两个GEP指令是否可以安全合并
/*
bool GVN::canSafelyMergeGEP(Instruction* gep1, Instruction* gep2) {
    if (!gep1 || !gep2) return false;
    
    // 如果两个GEP指令在不同的基本块中，不能合并
    if (gep1->get_parent() != gep2->get_parent()) return false;
    
    auto bb = gep1->get_parent();
    auto instrs = bb->get_instructions();
    
    // 使用简单的循环来查找指令位置
    size_t pos1 = 0, pos2 = 0;
    bool found1 = false, found2 = false;
    
    size_t i = 0;
    for (const auto& inst : instrs) {
        if (&inst == gep1) {
            pos1 = i;
            found1 = true;
        }
        if (&inst == gep2) {
            pos2 = i;
            found2 = true;
        }
        if (found1 && found2) break;
        i++;
    }
    
    if (!found1 || !found2) return false;
    
    // 检查两个GEP指令之间是否有函数调用
    size_t start = std::min(pos1, pos2);
    size_t end = std::max(pos1, pos2);
    
    i = 0;
    for (const auto& inst : instrs) {
        if (i >= start && i <= end) {
            if (inst.is_call()) {
                // 有函数调用，不能安全合并
                return false;
            }
        }
        i++;
    }
    
    return true;
}
*/

