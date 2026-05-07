#include <algorithm>
#include <unordered_map>
#include "ir.hpp"
#include "ir_type.hpp"
#include "ir_value.hpp"
#include "ir_instruction.hpp"
#include "ir_basic_block.hpp"
#include "ir_function.hpp"
#include "ir_module.hpp"
#include "ir_builder.hpp"
#include "ir_scope.hpp"
#include "ir_generator.hpp"

/// 本文件用于实现 IR 的打印函数，提交时删除本文件
//! 特别注意：ConstFloat::print(ostream& out) 在后端扫描浮点常量时会用到，故不在此处列出

unordered_map<IRInstID, string> iid_to_str = {
    {IRInstID::Add, "add"}, {IRInstID::Sub, "sub"}, {IRInstID::Mul, "mul"}, {IRInstID::Div, "div"}, {IRInstID::Rem, "rem"},
    {IRInstID::FAdd, "fadd"}, {IRInstID::FSub, "fsub"}, {IRInstID::FMul, "fmul"}, {IRInstID::FDiv, "fdiv"},
    {IRInstID::Shl, "shl"}, {IRInstID::AShr, "ashr"}, {IRInstID::LShr, "lshr"},
    {IRInstID::ICmp, "icmp"}, {IRInstID::FCmp, "fcmp"},
    {IRInstID::Alloca, "alloca"}, {IRInstID::GetElementPtr, "getelementptr"}, {IRInstID::Load, "load"}, {IRInstID::Store, "store"},
    {IRInstID::ZExt, "zext"}, {IRInstID::FpToSi, "fptosi"}, {IRInstID::SiToFp, "sitofp"}, {IRInstID::BitCast, "bitcast"},
    {IRInstID::Call, "call"}, {IRInstID::Br, "br"}, {IRInstID::Ret, "ret"},
    {IRInstID::Phi, "phi"}
};

unordered_map<ICmpOp, string> icmp_op_to_str = {
    {ICmpOp::EQ, "eq"}, {ICmpOp::NE, "ne"}, {ICmpOp::GT, "gt"},
    {ICmpOp::GE, "ge"}, {ICmpOp::LT, "lt"}, {ICmpOp::LE, "le"}
};

unordered_map<FCmpOp, string> fcmp_op_to_str = {
    {FCmpOp::FEQ, "eq"}, {FCmpOp::FNE, "ne"}, {FCmpOp::FGT, "gt"},
    {FCmpOp::FGE, "ge"}, {FCmpOp::FLT, "lt"}, {FCmpOp::FLE, "le"}
};

void print_as_op(Value* val, ostream& out, bool print_type = false) {
    if(print_type) {
        val->type->print(out);
        out << " ";
    }
    if(dynamic_cast<GlobalVariable*>(val)) {
        out << "@" << val->name;
    } else if(dynamic_cast<Function*>(val)) {
        out << "@" << val->name;
    } else if(dynamic_cast<Const*>(val)) {
        val->print(out);
    } else { // instruction ...
        out << "%" << val->name;
    }
}

void Type::print(ostream& out) {
    switch(tid) {
        case TypeID::VoidTy:
            out << "void";
            break;
        case TypeID::IntegerTy:
            out << "i" + to_string(static_cast<IntegerType*>(this)->num_bits);
            break;
        case TypeID::FloatTy:
            out << "float";
            break;
        case TypeID::ArrayTy: {
            auto array_type = static_cast<ArrayType*>(this);
            out << "[" + to_string(array_type->num_elements) + " x ";
            array_type->element_type->print(out);
            out << "]";
            break;
        }
        case TypeID::PointerTy:
            static_cast<PointerType*>(this)->pointed_type->print(out);
            out << "*";
            break;
        case TypeID::LabelTy:
            out << "label";
            break;
        case TypeID::FunctionTy: {
            auto func_type = static_cast<FunctionType*>(this);
            func_type->ret_type->print(out);
            out << " (";
            if(!func_type->param_types.empty()) {
                func_type->param_types[0]->print(out);
                for(int i = 1; i < func_type->param_types.size(); ++i) {
                    out << ", ";
                    func_type->param_types[i]->print(out);
                }
            }
            out << ")";
            break;
        }
        default: {
            cerr << "[ERROR] Type::print: unknown type ID!" << endl;
            exit(1);
        }
    }
}

void ValUndef::print(ostream& out) {
    out << "undef";
}

void ConstInt::print(ostream& out) {
    if(auto int_type = dynamic_cast<IntegerType*>(type)) {
        if(int_type->num_bits == 1) {
            out << (val ? "1" : "0");
        } else if(int_type->num_bits == 32) {
            out << to_string(val);
        }
    }
}

void ConstArray::print(ostream& out) {
    auto array_type = static_cast<ArrayType*>(type);
    out << "[";
    array_type->element_type->print(out);
    out << " ";
    if(!const_array.empty()) {
        const_array[0]->print(out);
        for(int i = 1; i < const_array.size(); ++i) {
            out << ", ";
            array_type->element_type->print(out);
            out << " ";
            const_array[i]->print(out);
        }
    }
    out << "]";
}

void ConstZero::print(ostream& out) {
    out << "zeroinitializer";
}

void GlobalVariable::print(ostream& out) {
    print_as_op(this, out);
    out << " = ";
    out << (is_const ? "constant ": "global ");
    static_cast<PointerType*>(type)->pointed_type->print(out);
    out << " ";
    init_val->print(out);
}

void BinaryInst::print(ostream& out) {
    print_as_op(this, out);
    out << " = ";
    out << iid_to_str[iid] << " ";
    get_op(0)->type->print(out);
    out << " ";
    print_as_op(get_op(0), out);
    out << ", ";
    print_as_op(get_op(1), out);
}

void ICmpInst::print(ostream& out) {
    print_as_op(this, out);
    out << " = ";
    out << iid_to_str[iid] + " " + icmp_op_to_str[icmp_op] + " ";
    get_op(0)->type->print(out);
    out << " ";
    print_as_op(get_op(0), out);
    out << ", ";
    if(get_op(0)->type->tid == get_op(1)->type->tid) {
        print_as_op(get_op(1), out);
    } else {
        get_op(1)->type->print(out);
        out << " ";
        print_as_op(get_op(1), out);
    }
}

void FCmpInst::print(ostream& out) {
    print_as_op(this, out);
    out << " = ";
    out << iid_to_str[iid] + " " + fcmp_op_to_str[fcmp_op] + " ";
    get_op(0)->type->print(out);
    out << " ";
    print_as_op(get_op(0), out);
    out << ", ";
    if(get_op(0)->type->tid == get_op(1)->type->tid) {
        print_as_op(get_op(1), out);
    } else {
        get_op(1)->type->print(out);
        out << " ";
        print_as_op(get_op(1), out);
    }
}

void AllocaInst::print(ostream& out) {
    print_as_op(this, out);
    out << " = ";
    out << iid_to_str[iid] << " ";
    alloca_type->print(out);
}

void GetElementPtrInst::print(ostream& out) {
    print_as_op(this, out);
    out << " = ";
    out << iid_to_str[iid] + " ";
    static_cast<PointerType*>(get_op(0)->type)->pointed_type->print(out);
    out << ", ";
    get_op(0)->type->print(out);
    out << " ";
    print_as_op(get_op(0), out);
    for(int i = 1; i < operands.size(); ++i) {
        out << ", ";
        get_op(i)->type->print(out);
        out << " ";
        print_as_op(get_op(i), out);
    }
}

void LoadInst::print(ostream& out) {
    print_as_op(this, out);
    out << " = ";
    out << iid_to_str[iid] + " ";
    static_cast<PointerType*>(get_op(0)->type)->pointed_type->print(out);
    out << ", ";
    get_op(0)->type->print(out);
    out << " ";
    print_as_op(get_op(0), out);
}

void StoreInst::print(ostream& out) {
    out << iid_to_str[iid] + " ";
    get_op(0)->type->print(out);
    out << " ";
    print_as_op(get_op(0), out);
    out << ", ";
    get_op(1)->type->print(out);
    out << " ";
    print_as_op(get_op(1), out);
}

void UnaryInst::print(ostream& out) {
    print_as_op(this, out);
    out << " = ";
    out << iid_to_str[iid] + " ";
    get_op(0)->type->print(out);
    out << " ";
    print_as_op(get_op(0), out);
    out << " to ";
    target_type->print(out);
}

void CallInst::print(ostream& out) {
    if(type->tid != TypeID::VoidTy) {
        print_as_op(this, out);
        out << " = ";
    }
    out << iid_to_str[iid] + " ";
    static_cast<FunctionType*>(get_op(operands.size() - 1)->type)->ret_type->print(out);
    out << " ";
    //__aeabi_memclr4 -> llvm_memset
    if(dynamic_cast<Function*>(get_op(operands.size() - 1))->name == "__aeabi_memclr4") {
        out << "@llvm.memset.p0.i32(";
        // i32* 目的内存地址
        get_op(0)->type->print(out);
        out << " ";
        print_as_op(get_op(0), out);
        // i8 0
        out << ", i8 0, ";
        // i32 修改总字节数
        get_op(1)->type->print(out);
        out << " ";
        print_as_op(get_op(1), out);
        // i1 false
        out << ", i1 false)";
    } else {
        print_as_op(get_op(operands.size() - 1), out);
        out << "(";
        if(operands.size() > 1) {
            get_op(0)->type->print(out);
            out << " ";
            print_as_op(get_op(0), out);
            for(int i = 1; i < operands.size() - 1; ++i) {
                out << ", ";
                get_op(i)->type->print(out);
                out << " ";
                print_as_op(get_op(i), out);
            }
        }
        out << ")";
    }
}

void BranchInst::print(ostream& out) {
    out << iid_to_str[iid] + " ";
    print_as_op(get_op(0), out, true);
    if(operands.size() == 3) {
        out << ", ";
        print_as_op(get_op(1), out, true);
        out << ", ";
        print_as_op(get_op(2), out, true);
    }
}

void ReturnInst::print(ostream& out) {
    out << iid_to_str[iid] + " ";
    if(operands.size() == 1) {
        get_op(0)->type->print(out);
        out << " ";
        print_as_op(get_op(0), out);
    } else {
        out << "void";
    }
}

void PhiInst::print(ostream& out) {
    print_as_op(this, out);
    out << " = ";
    out << iid_to_str[iid] + " ";
    if(operands.size() >= 2) {
        out << "[ ";
        print_as_op(get_op(0), out);
        out << ", ";
        print_as_op(get_op(1), out);
        out << " ]"; // 第一个操作数是类型，第二个操作数是第一个值
        for(int i = 2; i < operands.size(); i += 2) {
            out << ", [ ";
            print_as_op(get_op(i), out);
            out << ", ";
            print_as_op(get_op(i + 1), out);
            out << " ]"; // 每两个操作数一组
        }
    }
    if(operands.size() / 2 < parent->pre_bbs.size()) {
        for(auto pre_bb: parent->pre_bbs) {
            if(find(operands.begin(), operands.end(), static_cast<Value*>(pre_bb)) == operands.end()) {
                out << ", [ missing_val, ";
                print_as_op(pre_bb, out);
                out << " ]"; // 如果没有对应的操作数对，则添加一个 missing_val
            }
        }
    }
}

void BasicBlock::print(ostream& out) {
    out << name + ":";
    if(!pre_bbs.empty()) {
        out << "                                                ; preds =";
        for(auto pre_bb: pre_bbs) {
            out << " ";
            print_as_op(pre_bb, out);
        }
    }
    out << "\n";
    for(auto inst: inst_list) {
        out << "  " ;
        inst->print(out);
        out << "\n";
    }
}

void Argument::print(ostream& out) {
    print_as_op(this, out, true);
}

void Function::print(ostream& out) {
    if(name == "llvm.memset.p0.i32") {
        out << "declare void @llvm.memset.p0.i32(i32*, i8, i32, i1)";
        return;
    }
    string result;
    if(bb_list.empty()) { // 区分函数声明和定义
        out << "declare ";
    } else {
        out << "define ";
    }
    get_ret_type()->print(out);
    out << " ";
    print_as_op(this, out); // 获取函数名
    // 打印参数
    out << "(";
    if(bb_list.empty()) {
        auto func_type = static_cast<FunctionType*>(type);
        if(!func_type->param_types.empty()) {
            func_type->param_types[0]->print(out);
            for(int i = 1; i < func_type->param_types.size(); i++) {
                out << ", ";
                func_type->param_types[i]->print(out);
            }
        }
    } else {
        if(!args.empty()) {
            static_cast<Argument*>(args[0])->print(out);
            for(int i = 1; i < args.size(); i++) {
                out << ", ";
                static_cast<Argument*>(args[i])->print(out);
            }
        }
    }
    out << ")";
    // 打印基本块
    if(!bb_list.empty()) {
        out << " {\n";
        for(auto bb: bb_list) {
            bb->print(out);
        }
        out << "}";
    }
}

void Module::print(ostream& out) {
    for(auto global_var: global_var_list) {
        global_var->print(out);
        out << "\n";
    }
    for(auto func: func_list) {
        func->print(out);
        out << "\n";
    }
}