#include "backend_utils.h"

void report_error(const char *format, ...) {
    va_list args;
    va_start(args, format);

    vprintf(format, args);
    printf("\n");

    va_end(args);
    assert(false);
}

void Asm_Buffer::print(const char* s, ...) {
    char buf[8192];
    va_list args;

    va_start(args, s);
    size_t len = vsprintf(buf, s, args);
    va_end(args);

    if (len >= sizeof(buf)) {
        throw std::runtime_error("Buffer overflow in Asm_Buffer::print");
    }

    buffer.reserve(buffer.size() + len);

    buffer.insert(buffer.end(), buf, buf + len);
}

void Asm_Buffer::read_from_file(const char* filename) {
    // std::cout << "Current working directory: " << std::filesystem::current_path() << std::endl;
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Failed to open file");
    }

    // 获取文件大小
    file.seekg(0, std::ios::end);
    size_t file_size = file.tellg();
    file.seekg(0, std::ios::beg);

    // 预留足够的空间
    buffer.reserve(buffer.size() + file_size);

    // 读取文件内容到缓冲区
    buffer.insert(buffer.end(), std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}



void VOper::set_use_head(VI_Use *use, Func_Asm *func) {
    func->use_head_map[*this] = use;
}

void VOper::s_set_use_head(VI_Use *use, Func_Asm *func) {
    func->s_use_head_map[*this] = use;
}

VI_Use *VOper::get_use_head(Func_Asm *func) {
    if(tag != VREG && tag != REG) return nullptr;
    return func->use_head_map[*this];
}

VI_Use *VOper::f_get_use_head(Func_Asm *func) {
    if(tag != VFREG && tag != FREG) return nullptr;
    return func->s_use_head_map[*this];
}

// yyy: 头插法
void VOper::add_use(VI_Use *u, Func_Asm *func) {
    u->prev = nullptr;
    auto use_head = get_use_head(func);
    if(use_head){
        use_head->prev = u;
    }
    u->next = use_head;
    set_use_head(u, func);
}

void VOper::s_add_use(VI_Use *u, Func_Asm *func) {
    u->prev = nullptr;
    auto s_use_head = f_get_use_head(func);
    if(s_use_head){
        s_use_head->prev = u;
    }
    u->next = s_use_head;
    s_set_use_head(u, func);
}

VI *VOper::get_def_I(Func_Asm *func) {
    return func->def_I_map[*this];
}

// yyy: 设置这个def与定义其的指令添加到def_I_map中
void VOper::set_def_I(VI *I, Func_Asm *func) {
    func->def_I_map[*this] = I;
}

void VI_Use::rm_use(Func_Asm *func) {
    if (prev == nullptr && next == nullptr) {
        val->set_use_head(nullptr, func);
    } 
    else if (prev == nullptr && next != nullptr) {
        val->set_use_head(next, func);
        next->prev = nullptr;
    } 
    else if (prev != nullptr && next == nullptr) {
        prev->next = nullptr;
    } 
    else if (prev != nullptr && next != nullptr) {
        prev->next = next;
        next->prev = prev;
    }
}

void Func_Asm::clear_use_head_map() {
    use_head_map.clear();
    s_use_head_map.clear();
}

void Func_Asm::clear_def_I_map() {
    def_I_map.clear();
}

bool rm_instruction_use(VI* I, VOper *v, Func_Asm *func) {
    auto use = v->get_use_head(func);
    while(use != nullptr){
        if(use->I == I){
            use->rm_use(func);
            return true;
        }
        use = use->next;
    }
    return false;
}

bool s_rm_instruction_use(VI* I, VOper *v, Func_Asm *func) {
    auto use = v->f_get_use_head(func);
    while(use != nullptr){
        if(use->I == I){
            use->rm_use(func);
            return true;
        }
        use = use->next;
    }
    return false;
}

void VI::erase_from_parent() {
    if(mb != NULL) {
        if(mb->inst == this) {
            mb->inst = next;
        }
        if(mb->last_inst == this) {
            mb->last_inst = prev;
        }
        if(mb->control_transfer_inst == this) {
            mb->control_transfer_inst = NULL;
        }
        mb = NULL;
    }
    if(prev) {
        prev->next = next;
    }
    if(next) { 
        next->prev = prev;
    }
    next = prev = NULL;
}

void MachineBlock::erase_marked_values(){
    vector<VI *> unmarked_values(0);

    // inst指mb第一条指令
    for(VI* I  = inst ; I ; I = I->next){
        if(!I->marked)unmarked_values.push_back(I);
    }
    if(unmarked_values.empty()){
        inst = last_inst = control_transfer_inst = nullptr;
        succs = {};
    }else{
        inst = unmarked_values[0];
        last_inst = control_transfer_inst = unmarked_values.back();
        inst->prev = nullptr;
        last_inst->next = nullptr;
        unmarked_values.push_back(nullptr);
        for(int i = 0;i<unmarked_values.size()-1;i++){
            auto pre = unmarked_values[i];
            auto nex = unmarked_values[i+1];
            pre->next = nex;
            if(nex) nex->prev = pre;
        }
    }
}

bool is_callee_save(uint8 reg) {
    return reg == x9 || (reg >= x18 && reg <= x27) || reg == fp || reg == sp || reg == ra;
}

bool is_caller_save(uint8 reg) {
    return (reg >= x5 && reg <= x7) || (reg >= x10 && reg <= x17) || (reg >= x28 && reg <= x31);
}

bool s_is_callee_save(uint8 reg) {
    return (reg >= f8 && reg <= f9) || (reg >= f18 && reg <= f27);
}

bool s_is_caller_save(uint8 sreg) {
    return (sreg >= f0 && sreg <= f7) || (sreg >= f10 && sreg <= f17) || (sreg >= f28 && sreg <= f31);
}

bool can_be_imm12(int32 x) {
    return (x >= -2048) && (x <= 2047);
}

bool is_fcmp_cond(Branch_Condition cond) {
    return cond == LESS_THAN || cond == LESS_THAN_OR_EQUAL || cond == EQUAL;
}

bool is_float_moperand(VOper opr) {
    return opr.tag == FREG || opr.tag == VFREG;
}

bool is_float_reg(VOper opr) {
    return opr.tag == FREG || opr.tag == VFREG;
}