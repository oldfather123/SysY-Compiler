#include "ir_printer.hpp"
#include "ir.hpp"
// #include "SyntaxTree.hpp"
#include <bitset>
#include <cassert>
#include <iomanip>
#include <ios>
#include <memory>
#include <string>
#include <variant>

std::string ir::IrPrinter::get_reg_name(ptr<ir::ir_reg> &node) {
    string ans = "";
    if(node->is_global) {
        ans = "@g" + std::to_string(node->id);
    }
    else {
        ans = "%r" + std::to_string(node->id);
    }
    return ans;
}

void ir::IrPrinter::visit(ir_reg &node)
{
    out << this->mapping[node.type] << " ";
    if(node.is_global) {
        out << "@g" + std::to_string(node.id);
    }
    else {
        out << "%" << "r" <<node.id;
    }
}

std::string float_to_hex_string(float num) {
    double double_num = static_cast<double>(num);
    std::bitset<64> bits(*reinterpret_cast<unsigned long long*>(&double_num));
    std::stringstream ss;
    ss << std::hex << std::setw(16) << std::setfill('0') << bits.to_ullong();
    return "0x" + ss.str();
    // std::ostringstream oss;
    // oss << std::hex << std::showbase << std::setw(16) << std::setfill('0') << *(reinterpret_cast<unsigned int*>(&num));
    // return oss.str();
}


void ir::IrPrinter::visit(ir_constant &node)
{
    out << mapping[node.type] << " ";
    auto value = node.init_val.value();
    if(std::holds_alternative<int>(value)){
        out << std::get<int>(value);
    }else{
        // out << std::fixed << std::setprecision(1) << std::hexfloat << std::get<float>(value);
        out << float_to_hex_string(std::get<float>(value));
    }
}

void ir::IrPrinter::visit(ir_basicblock &node)
{
    if(!node.instructions.size()) return;
    out << node.name << ":" << std::endl;
    for(auto & inst : node.instructions)
        inst->accept(*this);
}
void ir::IrPrinter::visit(ir_module& node){
    for(auto & [name, fun] : node.libfuncs) {
        fun->accept(*this);
    }
    if(node.enable_mem_set) {
        out << "declare void @llvm.memset.p0.i64(ptr nocapture writeonly, i8, i64, i1 immarg)" << std::endl;
    }
    for(auto & [name, var] : node.global_var) {
        var->accept(*this);
    }
    if(node.init_block) {
        node.global_init_func->accept(*this);
    }
    for(auto & [name,func] : node.usrfuncs){
        func->accept(*this);
    }
}

void ir::IrPrinter::visit(ir_userfunc &node)
{    
    out << "define" << " " << "dso_local" << " " << mapping[node.rettype]
    <<  " " << "@" << node.name <<"(";
    for(auto a : node.func_args) {
        if(a != node.func_args.front()) {
            out << ", ";
        }
        out << mapping[a->addr->type] << " " << get_reg_name(a->addr);
    }
    out << ")"<< " " <<"{" << std::endl;
    for(auto alloc : node.alloc_list) {
        alloc->accept(*this);
    }
    for(auto [param, obj] : node.spilled_args) {
        ir::store store_ins(obj->get_addr(), param);
        store_ins.accept(*this);
    }
    ir::jump to_first_bb(node.entry);
    to_first_bb.accept(*this);
    for(auto & bb : node.bbs)
        bb->accept(*this);
    out << "}" << std::endl;
}

void ir::IrPrinter::visit(ir_libfunc &node)
{    
    out << "declare" << " " << mapping[node.rettype]
    <<  " " << "@" << node.name <<"(";
    out << "...";
    // for(auto a : node.func_args) {
    //     if(a != node.func_args.front()) {
    //         out << ", ";
    //     }
    //     out << mapping[a->addr->type] << " " << get_reg_name(a->addr);
    // }
    out << ")" << std::endl;
}

void ir::IrPrinter::visit(store &node)
{
    out << "\t" << "store ";
    node.value->accept(*this);
    out << "," << " ";
    node.addr->accept(*this);
    out << std::endl;
}

void ir::IrPrinter::visit(jump &node)
{
    out<<"\t"<<"br "<<"label "<<"%"<<node.get_target()->name<<std::endl;
}

void ir::IrPrinter::visit(br &node)
{
    out << "\t" << "br" << " ";
    out << "i1 ";
    out << this->get_value(node.cond);
    out << ",label ";
    out << "%" <<  node.target_true->name;
    out << ",label ";
    out << "%" <<  node.target_false->name;
    out << std::endl;
}

void ir::IrPrinter::visit(ret &node)
{
    if(!node.has_return_value)
        out << '\t' << "ret" << " "<< "void" << std::endl;
    else{
        out << "\t" << "ret" << " ";
        node.value->accept(*this);
        out << std::endl;
    }
}

void ir::IrPrinter::visit(load &node)
{
    out << "\t" << get_reg_name(node.dst) <<" = "<< "load " << mapping[node.dst->type] << "," << " ";
    node.addr->accept(*this);
    out << std::endl; 
}

void ir::IrPrinter::visit(alloc &node)
{
    out<<"\t";
    out << get_reg_name(node.var->addr);
    out<<" = "<<"alloca ";

    if(node.var->dim && node.var->dim->has_first_dim) {
        for(auto a : node.var->dim->dimensions) {
            out << "[" << a->calc_res() << " x ";
            if(a == node.var->dim->dimensions.back()) {
                out<<base_type[node.var->addr->type];
            }
            // out << "]";
        }
        for(auto a : node.var->dim->dimensions) {
            out << "]";
        }
    }
    else {
        out<<base_type[node.var->addr->type];
    }
    if(node.var->addr->size == 8)
        out << "*";

    out << " , align " << node.var->addr->size;
    out<<std::endl;
}

void ir::IrPrinter::visit(phi &node)
{
    out << "\t";
    out << get_reg_name(node.dst) << " = " <<  "phi" << " " << mapping[node.dst->type] << " ";
    for(int i = 0 ; i < node.uses.size(); ++i){
        auto & [a,b] = node.uses[i];
        out << "[" << " ";
        out << this->get_value(a);
        out << "," ;
        out << "%" << b.lock()->name << " " << "]" ;
        if(i != node.uses.size() - 1) out << ", ";
    }
    out << std::endl;
}

void ir::IrPrinter::visit(unary_op_ins &node)
{
    out << "\t";
    auto dst_r = std::dynamic_pointer_cast<ir::ir_reg>(node.dst);
    out << get_reg_name(dst_r) << " = ";
    if(node.src->get_type() == vartype::FLOAT) {
        out << fmapping[node.op] << " ";
    }
    else {
        out << mapping[node.op] << " ";
    }
    node.src->accept(*this);
    out << ", ";
    if(node.op == unaryop::minus) {
        if(node.src->get_type() == vartype::FLOAT) {
            // out << "-1.0";
            out << float_to_hex_string(-1.0);
        }
        else {
            out << "-1";
        }
    }
    else if(node.op == unaryop::plus) {
        if(node.src->get_type() == vartype::FLOAT) {
            // out << "1.0";
            out << float_to_hex_string(1.0);
        }
        else {
            out << "1";
        }
    }
    else if(node.op == unaryop::op_not) {
        out << "true";
    }
    out << std::endl;
}

void ir::IrPrinter::visit(binary_op_ins &node)
{
    out<<"\t";
    auto dst_r = std::dynamic_pointer_cast<ir::ir_reg>(node.dst);
    out<<get_reg_name(dst_r)<<" = ";
    if(node.src1->get_type() == vartype::FLOAT || node.src2->get_type() == vartype::FLOAT) {
        out<< fmapping[node.op] << " ";
    }
    else {
        out << mapping[node.op] << " ";
        if(node.op != binop::divide && node.op != binop::modulo){
            out<<"nsw ";
        }                      
    }
    // if(node.op != binop::divide){
    //     out<<"nsw ";
    // }
    node.src1->accept(*this);
    out<<", ";
    out << this->get_value(node.src2);
    out<<std::endl;
}

void ir::IrPrinter::visit(cmp_ins &node)
{
    out << "\t" << get_reg_name(node.dst);
    auto src1_r = std::dynamic_pointer_cast<ir::ir_constant>(node.src1);
    if(node.src1->get_type() == vartype::FLOAT || node.src2->get_type() == vartype::FLOAT) {
        out << " = fcmp "<< fmapping[node.op] << " ";
    }
    else {
        out << " = icmp "<< mapping[node.op] << " ";
    }
    if(src1_r){
        // if(src1_r->type == vartype::INT) {
        //     out << " = icmp "<< mapping[node.op] << " ";
        // }
        // else {
        //     out << " = fcmp "<< fmapping[node.op] << " ";
        // }
        out << mapping[src1_r->type] << " ";
        auto value = src1_r->init_val.value();
        if(std::holds_alternative<int>(value)){
            out << std::get<int>(value);
        }else{
            // out << std::fixed << std::setprecision(1) << std::get<float>(value);
            out << float_to_hex_string(std::get<float>(value));
        }
    }else{
        auto aa = std::dynamic_pointer_cast<ir::ir_reg>(node.src1);
        // if(aa->type == vartype::INT) {
        //     out << " = icmp "<< mapping[node.op] << " ";
        // }
        // else {
        //     out << " = fcmp "<< fmapping[node.op] << " ";
        // }
        out << mapping[aa->type] << " "<< get_reg_name(aa);
    }
    out << ",";
    out << this->get_value(node.src2);
    out<<std::endl;
}

void ir::IrPrinter::visit(logic_ins &node)
{
    auto dst = std::dynamic_pointer_cast<ir::ir_reg>(node.dst);
    out << "\t" << get_reg_name(dst) << " = " << mapping[node.op] << " ";
    out << "i1 ";
    // out << "%r" << get_value(node.src1) << ", %r" << get_value(node.src2);
    out << get_value(node.src1) << ", " << get_value(node.src2);
    out << "\n";
}

void ir::IrPrinter::visit(get_element_ptr &node) {
    for(auto [value, obj] : node.spilled_obj) {
        auto reg = std::dynamic_pointer_cast<ir::ir_reg>(value);
        assert(reg != nullptr);
        ir::load load_ins(reg, obj->addr);
        load_ins.accept(*this);
    }
    auto dst = std::dynamic_pointer_cast<ir::ir_reg>(node.dst);
    out << "\t" << get_reg_name(node.dst) << " = getelementptr ";
    // for(auto a : node.base->dim->dimensions) {
    for(auto a : node.base_dimension->dimensions) {
        out << "[" << a->calc_res() << " x ";
        if(a == node.base_dimension->dimensions.back()) {
            out<<base_type[node.base_reg->type];
        }
        // out << "]";
    }
    for(auto a : node.base_dimension->dimensions) {
        out << "]";
    }
    if(node.base_dimension->dimensions.empty()) {
        out << base_type[node.base_reg->type];
    }
    // }
    out << ", ";
    // for(auto a : node.base->dim->dimensions) {
    for(auto a : node.base_dimension->dimensions) {
        out << "[" << a->calc_res() << " x ";
        if(a == node.base_dimension->dimensions.back()) {
            out<<base_type[node.base_reg->type];
        }
        // out << "]";
    }
    for(auto a : node.base_dimension->dimensions) {
        out << "]";
        out << "*";
    }
    if(node.base_dimension->dimensions.empty()) {
        out << "ptr";
    }
    // }
    // out << "* ";
    out << " ";
    out << get_value(node.base_reg);
    if(node.base_dimension->has_first_dim) {
        out << ", i32 0";
    }
    for(auto offset : node.obj_offset) {
        out << ", i32 " << get_value(offset)/* << offset*/;
    }
    out << "\n";
}

void ir::IrPrinter::visit(ir::while_loop &node) {
    out<<"\t"<<"br "<<"label "<<"%"<<node.get_cond_from()->name<<std::endl;
}

void ir::IrPrinter::visit(ir::break_or_continue &node) {
    out<<"\t"<<"br "<<"label "<<"%"<<node.get_target()->name<<std::endl;
}

void ir::IrPrinter::visit(ir::func_call &node) {
    for(auto [value, obj] : node.spilled_obj) {
        auto reg = std::dynamic_pointer_cast<ir::ir_reg>(value);
        assert(reg != nullptr);
        ir::load load_ins(reg, obj->addr);
        load_ins.accept(*this);
    }
    out << "\t";
    if(node.ret_reg) {
        out << get_reg_name(node.ret_reg) << " = ";
    }
    out << "call " << mapping[node.ret_type] << " @" << node.func_name;
    out << "(";
    for(auto par = node.params.begin(); par != node.params.end(); par++) {
        (*par)->accept(*this);
        if(par != node.params.end() - 1) {
            out << ", ";
        }
    }
    out << ")";
    out << std::endl;
}

void ir::IrPrinter::visit(ir::tail_call &node) {
    auto call_ins = node.get_call_ins();
    call_ins->accept(*this);
    ir::ret r(call_ins->get_ret_reg(), true);
    r.accept(*this);
}

void ir::IrPrinter::llvm(ptr<int> pointer, ptr_list<ir::ir_value> init_val, ptr_list<ast::expr_syntax> dimensions, string init_type, vartype type) {
    for(auto a : dimensions) {
        // if(dimensions.size() > 1)
            out << "[" << a->calc_res() << " x ";
        if(a == dimensions.back()) {
            // if(dimensions.size() == 1)
                out<<base_type[type];
            init_type = "[" + std::to_string(a->calc_res()) + " x " + base_type[type] + "]";
        }
        // out << "]";
    }
    for(auto a : dimensions) {
        // if(dimensions.size() > 1)
            out << "]";
    }
    out << " [";
    for(int i = 0; i < dimensions.front()->calc_res(); i++) {
        if(dimensions.size() > 1) {
            ptr_list<ast::expr_syntax> nxt_dim(dimensions.begin() + 1, dimensions.end());
            llvm(pointer, init_val, nxt_dim, init_type, type);
            if(i != dimensions.front()->calc_res() - 1) {
                out << ", ";
            }
        }
        else {
            init_val[*pointer]->accept(*this);
            (*pointer)++;
            if(i != dimensions.front()->calc_res() - 1) {
                out << ", ";
            }
        }
    }
    out << "]";
}

void ir::IrPrinter::visit(ir::global_def &node) {
    out << "@g" << node.obj->addr->id << " = dso_local global ";
    string init_type = "";
    if(node.obj->dim) {
        for(auto a : node.obj->dim->dimensions) {
            // if(node.obj->dim->dimensions.size() > 1)
                // out << "[" << a->calc_res() << " x ";
            if(a == node.obj->dim->dimensions.back()) {
                // if(node.obj->dim->dimensions.size() > 1)
                    // out<<base_type[node.obj->addr->type];
                init_type = "[" + std::to_string(a->calc_res()) + " x " + base_type[node.obj->addr->type] + "]";
            }
            // out << "]";
        }
        for(auto a : node.obj->dim->dimensions) {
            // if(node.obj->dim->dimensions.size() > 1)
                // out << "]";
        }
        // out << " ";
    }
    else {
        // if(node.init_val.empty())
        //     out << base_type[node.obj->addr->type] << " ";
        init_type = base_type[node.obj->addr->type];
    }
    if(!node.init_val.empty()) {
        if(node.obj->dim) {
            // if(node.obj->dim->dimensions.size() > 1)
            //     out << "[";
            // // for(auto a : node.init_val) {
            // //     out << init_type << " ";
            // //     a->accept(*this);
            // // }
            // int back = node.obj->dim->dimensions.back()->calc_res();
            
            // for(int i = 0; i < node.init_val.size();) {
            //     if(i % back == 0) {
            //         out << init_type << " [";
            //     }
            //     node.init_val[i++]->accept(*this);
            //     if(i % back == 0) {
            //         out << "]";
            //         if(i / back != node.init_val.size() / back) {
            //             out << ", ";
            //         }
            //     }
            //     else {
            //         out << ", ";
            //     }
            // }
            // if(node.obj->dim->dimensions.size() > 1)
            //     out << "]";
            llvm(std::make_shared<int>(0), node.init_val, node.obj->dim->dimensions, init_type, node.obj->addr->type);
        }
        else {
            node.init_val.front()->accept(*this);
        }
    }
    else {
        // // if(node.obj->dim->dimensions.size() == 1) {
        //     out << init_type << " ";
        // // }
        if(node.obj->dim && node.obj->dim->dimensions.size() != 1) {
                for(auto a : node.obj->dim->dimensions) {
                    if(node.obj->dim->dimensions.size() > 1)
                        out << "[" << a->calc_res() << " x ";
                    if(a == node.obj->dim->dimensions.back()) {
                        if(node.obj->dim->dimensions.size() > 1)
                            out<<base_type[node.obj->addr->type];
                    }
                }
                for(auto a : node.obj->dim->dimensions) {
                    if(node.obj->dim->dimensions.size() > 1)
                        out << "]";
                }
        }
        else {
            out << init_type;
        }
        out << " zeroinitializer";
    }
    out << ", align " << node.obj->addr->size;
    out << std::endl;
}

void ir::IrPrinter::visit(ir::trans &node) {
    out << "\t";
    out << get_reg_name(node.dst) << " = ";
    if((node.src->get_type() == vartype::BOOL/* || node.src->get_type() == vartype::FBOOL*/) && node.target == vartype::INT) {
        out << "zext ";
        node.src->accept(*this);
        out << " to i32";
    }
    else {
        if(node.target == vartype::FLOAT) {
            out << "sitofp i32";
        }
        else {
            out << "fptosi float";
        }
        out << " ";
        out << get_value(node.src);
        out << " ";
        if(node.target == vartype::FLOAT) {
            out << "to float";
        }
        else {
            out << "to i32";
        }
    }
    out << std::endl;
}

void ir::IrPrinter::visit(ir::memset &node) {
    out << "\t";
    out << "call void @llvm.memset.p0.i64(";
    node.base->accept(*this);
    out << ", i8 " << node.val;
    out << ", i64 " << node.cnt * node.base->size;
    out << ", i1 " << (node.is_volatile ? "true" : "false");
    out << ")";
    out << std::endl;
}

std::string ir::IrPrinter::get_value(const ptr<ir::ir_value> &val)
{
    std::string ans;
    auto aa = std::dynamic_pointer_cast<ir::ir_constant>(val);
    if(aa){
        auto value = aa->init_val.value();
        if(std::holds_alternative<int>(value)){   
           ans += std::to_string(std::get<int>(value));
        }else{
            ans += float_to_hex_string(std::get<float>(value));
        }
    }else{
        auto aa = std::dynamic_pointer_cast<ir::ir_reg>(val);
        if(aa->is_global) {
            ans += ("@g" + std::to_string(aa->id));
        }
        else {
            ans += ("%r" + std::to_string(aa->id));
        }
    }
    return ans;
}
