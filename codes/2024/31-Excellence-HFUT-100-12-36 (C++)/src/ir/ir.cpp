#include "ir.hpp"
#include <algorithm>
#include <bitset>
#include <cassert>
#include <cstdlib>
#include <iomanip>
#include <iterator>
#include <memory>
#include <string>
#include <unordered_map>
#include <set>
#include "ir.hpp"
#include "../riscv/arch.hpp"
#include "../riscv/coloring_allocator.hpp"
#include "../riscv/register_allocator.hpp"
// #include "SyntaxTree.hpp"
#include <functional>

extern bool using_rookie;

void ir::ir_reg::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}
void ir::ir_reg::print(std::ostream & out ) { 
    out << "%r" << this->id << '\t';
}

vartype ir::ir_reg::get_type() {
    return this->type;
}

string ir::ir_reg::get_val() {
    abort();
    return "";
}

string ir::ir_reg::get_name() {
    return "g" + std::to_string(this->id);
}

ptr<ir::ir_instr> ir::ir_reg::get_def_loc() {
    auto lock = this->def_at.lock();
    assert((pointed && lock) || (!pointed && lock == nullptr));
    return lock;
}

void ir::ir_reg::clone_attribute(ptr<ir::ir_reg> other) {
    this->is_global = other->check_global();
    this->is_const = other->check_const();
    this->is_arr = other->check_is_arr();
    this->is_addr = other->check_is_addr();
    this->is_param = other->check_is_param();
    this->is_local = other->check_local();
    this->size = other->get_size();
}

void ir::ir_memobj::accept(ir_visitor &visitor) {

}
void ir::ir_memobj::print(std::ostream & out ) {
    out << this->name << " ";
    out << this->size << std::endl;
}
void ir::ir_scope::accept(ir_visitor &visitor) {

}
void ir::ir_scope::print(std::ostream & out )
{
}

ptr<ir::ir_userfunc> ir::ir_instr::get_fun() {
    auto lock = this->map_to_fun.lock();
    assert(lock);
    return lock;
}

ptr<ir::ir_basicblock> ir::ir_instr::get_block() {
    auto lock = this->map_to_block.lock();
    assert(lock);
    return lock;
}

void ir::ir_basicblock::push_back(ptr<ir_instr> inst)
{
    auto def_val = inst->def_reg();
    for(auto def : def_val) {
        if(def)
            def->mark_def_loc(inst);
    }
    assert(this->get_block());
    assert(this->get_fun());
    inst->mark_block(this->get_block());
    inst->mark_fun(this->get_fun());
    this->instructions.push_back(inst);
}

void ir::ir_basicblock::push_front(ptr<ir_instr> inst) {
    auto def_val = inst->def_reg();
    for(auto def : def_val) {
        if(def)
            def->mark_def_loc(inst);
    }
    assert(this->get_block());
    assert(this->get_fun());
    inst->mark_block(this->get_block());
    inst->mark_fun(this->get_fun());
    this->instructions.push_front(inst);
}

void ir::ir_basicblock::insert_spill(std::list<ptr<ir::ir_instr>>::iterator it, ptr<ir::ir_instr> inst, bool set_rank) {
    auto def_val = inst->def_reg();
    for(auto def : def_val) {
        if(def)
            def->mark_def_loc(inst);
    }
    assert(this->get_block());
    assert(this->get_fun());
    inst->mark_block(this->get_block());
    inst->mark_fun(this->get_fun());
    if(set_rank) {
        inst->set_rank((*it)->get_rank());
    }
    this->instructions.insert(it, inst);
}

void ir::ir_basicblock::insert_phi_spill(ptr<ir_instr> inst, int rank) {
    auto it = this->instructions.end();
    auto begin = this->instructions.begin();
    while(it != begin) {
        auto prev_it = std::prev(it);
        auto is_jump = std::dynamic_pointer_cast<ir::jump>(*prev_it);
        auto is_br = std::dynamic_pointer_cast<ir::br>(*prev_it);
        auto is_bc = std::dynamic_pointer_cast<ir::break_or_continue>(*prev_it);
        auto is_while_loop = std::dynamic_pointer_cast<ir::while_loop>(*prev_it);
        if(is_jump == nullptr && is_br == nullptr && is_bc == nullptr && is_while_loop == nullptr && (*prev_it)->get_rank() <= rank) {
            break;
        }
        else {
            it = prev_it;
        }
    }
    inst->set_rank(rank);
    this->instructions.insert(it, inst);
}

void ir::ir_basicblock::insert_after_phi(ptr<ir_instr> inst) {
    auto it = this->instructions.begin();
    for(; it != this->instructions.end(); it++) {
        auto is_phi = std::dynamic_pointer_cast<ir::phi>(*it);
        if(is_phi == nullptr) break;
    }
    this->instructions.insert(it, inst);
}

ptr<ir::ir_instr> ir::ir_basicblock::pop_back() {
    auto back = this->instructions.back();
    this->instructions.pop_back();
    return back;
}

void ir::ir_basicblock::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::ir_basicblock::print(std::ostream & out )
{
}

std::shared_ptr<ir::ir_instr> ir::ir_basicblock::getLastInst() {
  return this->instructions.back();
}
std::shared_ptr<ir::ir_instr> ir::ir_basicblock::getFirstInst() {
  return this->instructions.front();
}
std::string ir::ir_basicblock::getName() { return this->name; }

void ir::ir_basicblock::for_each(std::function<void(std::shared_ptr<ir::ir_instr>)> f, bool isReverse) {
    if(!isReverse){
        for(auto & inst : this->instructions){
            f(inst);
        }
    }else{
        auto reverseInstruction(this->instructions);
        std::reverse(this->instructions.begin(),this->instructions.end());
        for(auto & inst : reverseInstruction){
            f(inst);
        }
        std::reverse(this->instructions.begin(),this->instructions.end());
    }
}

void ir::ir_basicblock::del_ins(ptr<ir::ir_instr> ins) {
    this->instructions.remove(ins);
}

void ir::ir_basicblock::del_ins_by_vec(ptr_list<ir::ir_instr> del_ins) {
    for(auto del_item : del_ins) {
        this->instructions.remove(del_item);
    }
}

ptr<ir::ir_userfunc> ir::ir_basicblock::get_fun() {
    auto lock = this->cur_func.lock();
    assert(lock);
    return lock;
}

ptr<ir::ir_basicblock> ir::ir_basicblock::get_block() {
    auto lock = this->cur_block_ptr.lock();
    assert(lock);
    return lock;
}

std::list<ptr<ir::ir_instr>>::iterator ir::ir_basicblock::search(ptr<ir::ir_instr> ins) {
    auto it = std::find(this->instructions.begin(), this->instructions.end(), ins);
    return it;
}

ptr<ir::ir_userfunc> ir::ir_module::new_func(std::string name, std::vector<vartype> arg_types) {
  auto pfunc = std::make_shared<ir_userfunc>(name, this->global_var_cnt, arg_types);
  pfunc->mark_fun(pfunc);
  usrfuncs.push_back({name, pfunc});
  return pfunc;
}

void ir::ir_module::add_lib_func(std::string name, ptr<ir_libfunc> fun) {
  libfuncs.push_back({name, fun});
  return;
}

ptr<ir::global_def> ir::ir_module::new_global(std::string name, vartype type) {
    auto reg = std::make_shared<ir_reg>(this->global_var.size(), type, 4, true);
    auto obj = std::make_shared<ir_memobj>(name, reg, 4);
    auto var = std::make_shared<global_def>(name, obj);
    global_var.push_back({name, var});
//   usrfuncs.push_back({name, pfunc});
    return var;
}

void ir::ir_module::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::ir_module::print(std::ostream & out)
{
}

void ir::ir_module::reg_allocate(int base_reg, ptr_list<global_def> global_var) {
    if(this->init_block) {
        if(using_rookie) {
            LoongArch::RookieAllocator rookie_allocator(this->global_init_func, base_reg, global_var);
            this->global_init_func->reg_allocate(rookie_allocator);
        }
        else {
            LoongArch::ColoringAllocator coloring_allocator(this->global_init_func, base_reg, global_var);
            this->global_init_func->reg_allocate(coloring_allocator);
        }
        // auto ret = allocator.run();

        // LoongArch::ColoringAllocator all(this->global_init_func, base_reg, global_var);
        // all.run(LoongArch::INT);
    }
    for(auto & [name, func] : this->usrfuncs){
        LoongArch::RegisterAllocator *allocator;
        if(using_rookie) {
            LoongArch::RookieAllocator rookie_allocator(func, base_reg, global_var);     // 我修改了allocator的构造函数
            allocator = &rookie_allocator;
            func->reg_allocate(rookie_allocator);
        }
        else{
            LoongArch::ColoringAllocator coloring_allocator(func, base_reg, global_var);
            allocator = &coloring_allocator;
            func->reg_allocate(coloring_allocator);
        }
        // auto ret = allocator.run();                                 // 我也修改了run方法的返回值

        // LoongArch::ColoringAllocator all(func, base_reg, global_var);
        // all.run(LoongArch::INT);
    }
}

ir::ir_userfunc::ir_userfunc(std::string name, int reg_cnt, std::vector<vartype> arg_types) : ir_func(name, arg_types), max_reg(reg_cnt) {
    this->scope = std::make_unique<ir::ir_scope>();
}

ptr<ir::ir_memobj> ir::ir_userfunc::new_obj(std::string name, vartype var_type) {
    std::unordered_map<vartype, vartype> var_reg_trans = {{vartype::INT, vartype::INTADDR}, {vartype::FLOAT, vartype::FLOATADDR}, {vartype::BOOL, vartype::BOOLADDR}, /*{vartype::FBOOL, vartype::FBOOLADDR},*/
    {vartype::INTADDR, vartype::INTADDR}, {vartype::FLOATADDR, vartype::FLOATADDR}, {vartype::BOOLADDR, vartype::BOOLADDR}/*, {vartype::FBOOLADDR, vartype::FBOOLADDR}*/};
    auto addr = this->new_reg(var_reg_trans[var_type]);
    addr->mark_local();
    addr->mark_addr();
    auto obj = std::make_shared<ir_memobj>(name, addr, i32_size);
    this->scope->ir_objs.push_back(obj);
    return obj;
}

ptr<ir::ir_memobj> ir::ir_userfunc::new_spill_obj(std::string name, vartype var_type) {
    auto obj = new_obj(name, var_type);
    if(var_type == vartype::INTADDR || var_type == vartype::FLOATADDR || var_type == vartype::BOOLADDR/* || var_type == vartype::FBOOLADDR*/) {
        obj->set_size(8);
        obj->get_addr()->set_size(8);
    }
    obj->get_addr()->mark_unspillable();
    this->alloc_list.push_back(std::make_shared<ir::alloc>(obj));
    return obj;
}

ptr<ir::ir_reg> ir::ir_userfunc::new_reg(vartype type)
{
    int reg_size = 4;
    return std::make_shared<ir_reg>(max_reg++,type,reg_size, false);
}

ptr<ir::ir_reg> ir::ir_userfunc::new_spill_reg(ptr<ir::ir_reg> reg, ptr<ir::ir_memobj> obj) {
    auto res = new_reg(reg->get_type());
    res->clone_attribute(reg);
    res->mark_unspillable();
    if(reg->check_is_addr()) {
        obj->attach_spill(res);
    }
    return res;
}

ptr<ir::ir_basicblock> ir::ir_userfunc::new_block()
{
    auto bb = std::make_shared<ir_basicblock>(max_bb++);
    if(dealing_while) {
        bb->mark_while(dealing_while);
    }
    bb->mark_block(bb);
    assert(this->get_fun());
    bb->mark_fun(this->get_fun());
    this->bbs.push_back(bb);
    return bb;
}

void ir::ir_userfunc::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::ir_userfunc::print(std::ostream & out)
{
}

void ir::ir_libfunc::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::ir_libfunc::print(std::ostream & out)
{
}

std::unordered_map<ptr<ir::ir_value>,Pass::LiveInterval> ir::ir_userfunc::GetLiveInterval() 
{
    return std::unordered_map<ptr<ir::ir_value>,Pass::LiveInterval>();
}

std::vector<ptr<ir::ir_basicblock>> ir::ir_userfunc::GetLinerSequence() 
{
    return std::vector<ptr<ir::ir_basicblock>>();
}

void ir::ir_userfunc::reg_allocate(LoongArch::RegisterAllocator &allocator) {

    auto ret_int = allocator.run(LoongArch::Rtype::INT);
    for(auto res : ret_int.mapping_to_reg) {
        regAllocateOut.insert(res);
    }
    for(auto res : ret_int.mapping_to_spill) {
        regSpill.push_back(res);
    }
    auto ret_float = allocator.run(LoongArch::Rtype::FLOAT);
    for(auto res : ret_float.mapping_to_reg) {
        regAllocateOut.insert(res);
    }
    for(auto res : ret_float.mapping_to_spill) {
        regSpill.push_back(res);
    }
    // auto ret_fbool = allocator.run(LoongArch::Rtype::FBOOL);
    // for(auto res : ret_fbool.mapping_to_reg) {
    //     regAllocateOut.insert(res);
    // }
    // for(auto res : ret_fbool.mapping_to_spill) {
    //     regSpill.push_back(res);
    // }
    for(auto alloc : this->alloc_list) {
        this->arrobj.push_back(alloc->get_var());
    }
    // regAllocateOut = map;       // 成功分配的寄存器映射
    // regSpill = spill;           // 无法分配，被流放到内存中的寄存器
    // this->arrobj = arrobj;
}

void ir::ir_userfunc::save_current_globl(std::list<std::pair<std::string, ptr<ir::global_def>>> current_globl) {
    this->current_globl = current_globl;
}

ptr_list<ir::ir_basicblock> ir::ir_userfunc::check_predecessor(ptr<ir::ir_basicblock> tar) {
    // auto it = this->successor.find(tar);
    // if(it == this->successor.end()) {
    //     it = this->s_back_trace.find(tar);
    // }
    // return it->second;
    // auto suc = this->predecessor[tar];
    // auto bak = this->s_back_trace[tar];
    // std::sort(suc.begin(), suc.end());
    // std::sort(bak.begin(), bak.end());
    // ptr_list<ir_basicblock> ret;
    // std::set_union(
    //     suc.begin(), suc.end(),
    //     bak.begin(), bak.end(),
    //     std::back_inserter(ret)
    // );
    // return ret;
    return this->predecessor[tar];
}

ptr_list<ir::ir_basicblock> ir::ir_userfunc::check_nxt(ptr<ir::ir_basicblock> tar) {
    // auto it = this->nxt.find(tar);
    // if(it == this->nxt.end()) {
    //     it = this->n_back_trace.find(tar);
    // }
    // return it->second;
    // auto nxt = this->nxt[tar];
    // auto bak = this->n_back_trace[tar];
    // std::sort(nxt.begin(), nxt.end());
    // std::sort(bak.begin(), bak.end());
    // ptr_list<ir_basicblock> ret;
    // std::set_union(
    //     nxt.begin(), nxt.end(),
    //     bak.begin(), bak.end(),
    //     std::back_inserter(ret)
    // );
    // return ret;
    return this->nxt[tar];
}

void ir::ir_userfunc::del_alloc(ptr_list<ir::alloc> del_items) {
    for(auto del_item : del_items) {
        auto it = std::find(this->alloc_list.begin(), this->alloc_list.end(), del_item);
        if(it != this->alloc_list.end()) {
            this->alloc_list.erase(it);
        }
    }
}

ptr<ir::ir_userfunc> ir::ir_userfunc::get_fun() {
    auto lock = this->cur_fun_ptr.lock();
    assert(lock);
    return lock;
}

void ir::ir_userfunc::del_ret_block(ptr<ir::ir_basicblock> block) {
    if(block->is_ret()) {
        auto it = std::find(this->bbs.begin(), this->bbs.end(), block);
        if(it != bbs.end()) {
            this->bbs.erase(it);
        }
    }
}

bool ir::ir_func::set_retype(vartype rettype)
{
    this->rettype = rettype; 
    return true;
}

void ir::store::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::store::print(std::ostream & out )
{
    out << "store" << '\t';
    out << "def:" << '\t';
    auto defs = this->def_reg();
    for(auto & i : defs){
        i->print();
    }
    out << "use:" << '\t';
    auto uses = this->use_reg();
    for(auto & i : uses){
        i->print();
    }
    out << '\n';
}

std::vector<ptr<ir::ir_reg>> ir::store::use_reg() {
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->value);
    if(is_reg) return {this->addr, is_reg};
    else return {this->addr};
}

std::vector<ptr<ir::ir_reg>> ir::store::def_reg() {
    return {};
}

void ir::store::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    auto it = replace_map.find(this->value);
    if(it != replace_map.end()) {
        auto first = it->first;
        this->value = it->second;
    }
    it = replace_map.find(this->addr);
    if(it != replace_map.end()) {
        auto replace_reg = std::dynamic_pointer_cast<ir::ir_reg>(it->second);
        assert(replace_reg);
        this->addr = replace_reg;
    }
}

void ir::jump::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::jump::print(std::ostream & out )
{
    out << "br-jump" << '\t';
    out << "def:" << '\t';
    auto defs = this->def_reg();
    for(auto & i : defs){
        i->print();
    }
    out << "use:" << '\t';
    auto uses = this->use_reg();
    for(auto & i : uses){
        i->print();
    }
    out << '\n';
}

std::vector<ptr<ir::ir_reg>> ir::jump::use_reg() {
  return {};
}

std::vector<ptr<ir::ir_reg>> ir::jump::def_reg() {
  return {};
}

std::shared_ptr<ir::ir_basicblock> ir::jump::get_target() {
    auto lock = this->target.lock();
    assert(lock);
    return lock;
}

void ir::jump::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {

}

void ir::br::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::br::print(std::ostream & out )
{
    out << "br" << '\t';
    out << "def:" << '\t';
    auto defs = this->def_reg();
    for(auto & i : defs){
        i->print();
    }
    out << "use:" << '\t';
    auto uses = this->use_reg();
    for(auto & i : uses){
        i->print();
    }
        out << '\n';
}

std::vector<ptr<ir::ir_reg>> ir::br::use_reg() {
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->cond);
    if(is_reg) return {is_reg};
    else return {};
}

std::vector<ptr<ir::ir_reg>> ir::br::def_reg() { return std::vector<ptr<ir::ir_reg>>(); }

void ir::br::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {        // sysy没有bool，br使用的都是icmp比较出来的条件，所以不用换？
    auto it = replace_map.find(this->cond);
    if(it != replace_map.end()) {
        this->cond = it->second;
    }
}

void ir::ret::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::ret::print(std::ostream & out)
{
    out << "ret" << '\t';
    out << "def:" << '\t';
    auto defs = this->def_reg();
    for(auto & i : defs){
        i->print();
    }
    out << "use:" << '\t';
    auto uses = this->use_reg();
    for(auto & i : uses){
        i->print();
    }
        out << '\n';
}

std::vector<ptr<ir::ir_reg>> ir::ret::use_reg() {
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->value);
    if(is_reg) return {is_reg};
    else return {};
}

std::vector<ptr<ir::ir_reg>> ir::ret::def_reg() { return std::vector<ptr<ir::ir_reg>>(); }

void ir::ret::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    auto it = replace_map.find(this->value);
    if(it != replace_map.end()) {
        this->value = it->second;
    }
}

void ir::load::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::load::print(std::ostream & out )
{
    out << "load" << '\t';
    out << "def:" << '\t';
    auto defs = this->def_reg();
    for(auto & i : defs){
        i->print();
    }
    out << "use:" << '\t';
    auto uses = this->use_reg();
    for(auto & i : uses){
        i->print();
    }
    out << '\n';
}

std::vector<ptr<ir::ir_reg>> ir::load::use_reg() {
    return {addr};
}

std::vector<ptr<ir::ir_reg>> ir::load::def_reg() {
    return {dst};
}

void ir::load::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    auto it = replace_map.find(this->addr);
    if(it != replace_map.end()) {
        auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(it->second);
        if(is_reg)
            this->addr = is_reg;
    }
}

void ir::alloc::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::alloc::print(std::ostream & out )
{
    out << "alloc" << '\t';
    out << "def:" << '\t';
    auto defs = this->def_reg();
    for(auto & i : defs){
        i->print();
    }
    out << "use:" << '\t';
    auto uses = this->use_reg();
    for(auto & i : uses){
        i->print();
    }
    out << '\n';
}

std::vector<ptr<ir::ir_reg>> ir::alloc::use_reg() {
  return std::vector<ptr<ir::ir_reg>>();
}

std::vector<ptr<ir::ir_reg>> ir::alloc::def_reg() {
  return {var->get_addr()};
}

void ir::alloc::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {

}

void ir::phi::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::phi::print(std::ostream & out )
{
    out << "phi" << '\t';
    out << "def:" << '\t';
    auto defs = this->def_reg();
    for(auto & i : defs){
        i->print();
    }
    out << "use:" << '\t';
    auto uses = this->use_reg();
    for(auto & i : uses){
        i->print();
    
    }
        out << '\n';
}

std::vector<ptr<ir::ir_reg>> ir::phi::use_reg() {
    std::vector<ptr<ir::ir_reg>> use_regs;
    std::vector<ptr<ir::ir_value>> use_values;
    for(auto & [value, bb] : this->uses) {
        if(auto reg = std::dynamic_pointer_cast<ir_reg>(value)){
            use_regs.push_back(reg);
        }else if(auto constant_value = std::dynamic_pointer_cast<ir_constant>(value)){
            use_values.push_back(value);
        }
    }
    return use_regs;
}

std::vector<ptr<ir::ir_reg>> ir::phi::def_reg() { 
    return {this->dst}; 
}

void ir::phi::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    for(auto &use : this->uses) {
        auto it = replace_map.find(use.first);
        if(it != replace_map.end()) {
            use.first = it->second;
        }
    }
}

void ir::unary_op_ins::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::unary_op_ins::print(std::ostream & out )
{
    
}

std::vector<ptr<ir::ir_reg>> ir::unary_op_ins::use_reg() {
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->src);
    if(is_reg) return {is_reg};
    else return {};
}

std::vector<ptr<ir::ir_reg>> ir::unary_op_ins::def_reg() {
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->dst);
    if(is_reg) return {is_reg};
    else return {};
};

void ir::unary_op_ins::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    auto it = replace_map.find(this->src);
    if(it != replace_map.end()) {
        this->src = it->second;
    }
}

void ir::binary_op_ins::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::binary_op_ins::print(std::ostream & out )
{
    out << "binary" << '\t';
    out << "def:" << '\t';
    auto defs = this->def_reg();
    for(auto & i : defs){
        i->print();
    }
    out << "use:" << '\t';
    auto uses = this->use_reg();
    for(auto & i : uses){
        i->print();
    }
}
std::vector<ptr<ir::ir_reg>> ir::binary_op_ins::use_reg() {
    std::vector<ptr<ir::ir_reg>> ret;
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->src1);
    if(is_reg) ret.push_back(is_reg);
    is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->src2);
    if(is_reg) ret.push_back(is_reg);
    return ret;
}
std::vector<ptr<ir::ir_reg>> ir::binary_op_ins::def_reg() {
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->dst);
    if(is_reg) return {is_reg};
    else return {};
}

void ir::binary_op_ins::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    auto it = replace_map.find(this->src1);
    if(it != replace_map.end()) {
        this->src1 = it->second;
    }
    it = replace_map.find(this->src2);
    if(it != replace_map.end()) {
        this->src2 = it->second;
    }
}

void ir::ir_constant::accept(ir_visitor &visitor) {
     visitor.visit(*this);
}

void ir::ir_constant::print(std::ostream &out)
{
}

vartype ir::ir_constant::get_type() {
    return this->type;
}

string ir::ir_constant::get_val() {
    auto value = this->init_val.value();
    if(std::holds_alternative<int>(value)){
        return std::to_string(std::get<int>(value));
    }else{
        float num = std::get<float>(value);
        // double double_num = static_cast<double>(num);
        std::bitset<32> bits(*reinterpret_cast<unsigned int*>(&num));
        std::stringstream ss;
        ss << std::hex << std::setw(8) << std::setfill('0') << bits.to_ullong();
        return "0x" + ss.str();
    }
}

void ir::jumpList::accept(ir_visitor &visitor)
{
}

void ir::jumpList::print(std::ostream &out)
{
}

string ir::jumpList::get_val() {
    abort();
    return "";
}

void ir::cmp_ins::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::cmp_ins::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::cmp_ins::use_reg() {
    std::vector<ptr<ir::ir_reg>> ret;
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->src1);
    if(is_reg) ret.push_back(is_reg);
    is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->src2);
    if(is_reg) ret.push_back(is_reg);
    return ret;
}

std::vector<ptr<ir::ir_reg>> ir::cmp_ins::def_reg() {
  return {this->dst};
}

void ir::cmp_ins::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    auto it = replace_map.find(this->src1);
    if(it != replace_map.end()) {
        this->src1 = it->second;
    }
    it = replace_map.find(this->src2);
    if(it != replace_map.end()) {
        this->src2 = it->second;
    }
}

void ir::logic_ins::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::logic_ins::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::logic_ins::use_reg() {
    std::vector<ptr<ir::ir_reg>> ret;
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->src1);
    if(is_reg) ret.push_back(is_reg);
    is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->src2);
    if(is_reg) ret.push_back(is_reg);
    return ret;
}

std::vector<ptr<ir::ir_reg>> ir::logic_ins::def_reg() {
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->dst);
    if(is_reg) return {is_reg};
    else return {};
}

void ir::logic_ins::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    auto it = replace_map.find(this->src1);
    if(it != replace_map.end()) {
        this->src1 = it->second;
    }
    it = replace_map.find(this->src2);
    if(it != replace_map.end()) {
        this->src2 = it->second;
    }
}

std::vector<ptr<ir::ir_reg>> ir::reg_write_ins::use_reg() {
  return std::vector<ptr<ir::ir_reg>>();
}

std::vector<ptr<ir::ir_reg>> ir::reg_write_ins::def_reg() {
  return std::vector<ptr<ir::ir_reg>>();
}

void ir::reg_write_ins::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {}    // 抽象类，不用换

std::vector<ptr<ir::ir_reg>> ir::control_ins::use_reg() {
  return std::vector<ptr<ir::ir_reg>>();
}

std::vector<ptr<ir::ir_reg>> ir::control_ins::def_reg() {
  return std::vector<ptr<ir::ir_reg>>();
}

void ir::control_ins::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {}      // 抽象类，不用换

void ir::get_element_ptr::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::get_element_ptr::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::get_element_ptr::use_reg() {
    ptr_list<ir::ir_reg> ret;
    for(auto offset : this->obj_offset) {
        auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(offset);
        if(is_reg) ret.push_back(is_reg);
    }
    ret.push_back(this->base_reg);
    return ret;
}

std::vector<ptr<ir::ir_reg>> ir::get_element_ptr::def_reg() {
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->dst);
    if(is_reg) return {is_reg};
    else return {};
}

void ir::get_element_ptr::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    for(auto &offset : this->obj_offset) {
        auto it = replace_map.find(offset);
        if(it != replace_map.end()) {
            offset = it->second;
        }
    }
    auto it = replace_map.find(this->base_reg);
    if(it != replace_map.end()) {
        auto reg = std::dynamic_pointer_cast<ir::ir_reg>(it->second);
        if(reg) {
            this->base_reg = reg;
        }
    }
}

void ir::while_loop::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::while_loop::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::while_loop::use_reg() {
  return {};
}

std::vector<ptr<ir::ir_reg>> ir::while_loop::def_reg() {
  return {};
}

ptr<ir::ir_basicblock> ir::while_loop::get_out_block() {
    auto lock = this->out_block.lock();
    assert(lock);
    return lock;
}

ptr<ir::ir_basicblock> ir::while_loop::get_cond_from() {
    auto lock = this->cond_from.lock();
    assert(lock);
    return lock;
}

void ir::while_loop::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {}       // while回边，不用换

void ir::break_or_continue::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::break_or_continue::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::break_or_continue::use_reg() {
  return {};
}

std::vector<ptr<ir::ir_reg>> ir::break_or_continue::def_reg() {
  return {};
}

ptr<ir::ir_basicblock> ir::break_or_continue::get_target() {
    auto lock = this->target.lock();
    assert(lock);
    return lock;
}

void ir::break_or_continue::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {}    // while中的返回，不用换

void ir::func_call::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::func_call::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::func_call::use_reg() {
    ptr_list<ir::ir_reg> ret;
    for(auto offset : this->params) {
        auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(offset);
        if(is_reg) ret.push_back(is_reg);
    }
    return ret;
//   return {};
}

std::vector<ptr<ir::ir_reg>> ir::func_call::def_reg() {
    if(this->ret_reg)
        return {this->ret_reg};
    else
        return {};
}

void ir::func_call::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    for(auto it = params.begin(); it != params.end(); it++) {
        auto map_it = replace_map.find(*it);
        if(map_it != replace_map.end()) {
            *it = map_it->second;
        }
    }
}

ptr<ir::ir_func> ir::func_call::get_callee() {
    auto lock = this->callee.lock();
    assert(lock);
    return lock;
}

void ir::func_call::set_live_in(std::unordered_set<ptr<ir::ir_reg>> live_in, LoongArch::Rtype dealing)  {
    if(dealing == LoongArch::INT) {
        this->int_live_in = live_in;
    }
    else if(dealing == LoongArch::FLOAT) {
        this->float_live_in = live_in;
    }
}

std::unordered_set<ptr<ir::ir_reg>> ir::func_call::get_live_in()  {
    std::unordered_set<ptr<ir::ir_reg>> res;
    res.insert(int_live_in.begin(), int_live_in.end());
    res.insert(float_live_in.begin(), float_live_in.end());
    return res;
}

void ir::tail_call::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::tail_call::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::tail_call::use_reg() {
    return this->call_ins->use_reg();
//   return {};
}

std::vector<ptr<ir::ir_reg>> ir::tail_call::def_reg() {
    return this->call_ins->def_reg();
}

void ir::tail_call::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    this->call_ins->replace_reg(replace_map);
}

void ir::global_def::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::global_def::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::global_def::use_reg() {
    ptr_list<ir::ir_reg> ret;
    for(auto offset : this->init_val) {
        auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(offset);
        if(is_reg) ret.push_back(is_reg);
    }
    return ret;
}

std::vector<ptr<ir::ir_reg>> ir::global_def::def_reg() {
  return {obj->get_addr()};
}

ptr<ir::ir_memobj> ir::global_def::get_obj() {
    return obj;
}

void ir::global_def::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {}       // mem2reg不处理全局变量，不用换？coloring_allocator也不需要

void ir::trans::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::trans::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::trans::use_reg() {
    auto is_reg = std::dynamic_pointer_cast<ir::ir_reg>(this->src);
    if(is_reg) return {is_reg};
    else return {};
}

std::vector<ptr<ir::ir_reg>> ir::trans::def_reg() {
  return {dst};
}

void ir::trans::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {
    auto it = replace_map.find(this->src);
    if(it != replace_map.end()) {
        this->src = it->second;
    }
}

void ir::memset::accept(ir_visitor &visitor)
{
    visitor.visit(*this);
}

void ir::memset::print(std::ostream &out)
{
}

std::vector<ptr<ir::ir_reg>> ir::memset::use_reg() {
  return {base};
}

std::vector<ptr<ir::ir_reg>> ir::memset::def_reg() {
  return {};
}

void ir::memset::replace_reg(std::unordered_map<ptr<ir::ir_value>, ptr<ir::ir_value>> replace_map) {        // mem2reg不用换，register_allocator需要
    auto it = replace_map.find(base);
    if(it != replace_map.end()) {
        auto reg = std::dynamic_pointer_cast<ir::ir_reg>(it->second);
        assert(reg);
        base = reg;
    }
}