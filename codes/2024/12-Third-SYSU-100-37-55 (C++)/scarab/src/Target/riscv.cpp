
#include "riscv.h"

#include "register.h"
#include "remove_useless_moves.h"
#include "remove_redundant_instrs.h"
#include "use_analysis.h"
#include "algebraic_identity.h"
#include "remove_unused_instruction.h"
#include "peephole.h"


int labelNum=0;


Program_Asm *emit_vis(Module program_module) {
    auto program_asm = new Program_Asm;

    Initial_ir_asm_op_map();
    int i = 0;
    for (auto func : program_module.globalFunctions)
    {
        if(func->isLibraryFunction) continue;
        i++;
        auto func_asm = emit_func_vi(func, i, program_module.globalVariables, labelNum);
        program_asm->functions.push_back(func_asm);
    }

    return program_asm;
}

#define RUN(pass_name) pass_name(program_asm)
void backendOpt(Program_Asm *program_asm, bool opt) {

	RUN(remove_redundant_instrs);
    RUN(use_analysis);
    RUN(algebraic_identity);
    RUN(use_analysis);
    RUN(peephole);
    RUN(use_analysis);
    RUN(remove_unused_instruction);
    RUN(use_analysis);
    register_allocation(program_asm);
    RUN(remove_useless_moves);
    cerr << "ok\n";
}
#undef RUN

void build_program_asm(Asm_Buffer *s, Program_Asm *pro, vector<VariablePtr> &globalVariables) {
	s->print("    .option nopic\n");
    s->print("    .attribute arch, \"rv64i2p1_m2p0_a2p1_f2p2_d2p2_c2p0_zicsr2p0_zifencei2p0_zba1p0_zbb1p0\"\n");
    s->print("    .attribute unaligned_access, 0\n");
    s->print("    .attribute stack_align, 16\n");
    if(labelNum!=0){
        auto dir = get_current_file_directory();
        dir += "/runtime.s";
        s->read_from_file(dir.c_str());
    }
    

    build_globals(s, globalVariables);
	s->print("\n");

	s->print("    .text\n");
	s->print("    .align 1\n");

	s->print("    .globl main\n");
    s->print("    .type main, @function\n");
	for (int i = 0; i < pro->functions.size(); i++) {
		build_function_asm(s, pro->functions[i]);
        s->print("\n\n");
	}
}


// yyy: 把VI中用到的old_opr换成new_opr，相当于之后VI用到了之前的某个opr，但这个opr在优化中被换掉了，那么use要改
void replace_uses(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func) {
    vector<VOper *> oprs = get_uses_ptr(I);
    for(VOper *opr_ptr : oprs) {
        if (opr_ptr && *opr_ptr == *old_opr) {
            auto use = opr_ptr->get_use_head(func);
            while(use != nullptr) {
                if(use->I == I) {
                    use->rm_use(func);
                    auto new_use = new VI_Use(new_opr, use->user, I);
                    // VI_Store时user是nullptr
                    if(use->user != nullptr){
                        new_opr->add_use(new_use,func);
                        // replace_inst_operand(I,old_opr,new_opr,func);
                    }
                }
                use = use->next;
            }
            *opr_ptr = *new_opr;
        }
    }
}

void s_replace_uses(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func) {
    vector<VOper *> oprs = s_get_uses_ptr(I);
    for(VOper *opr_ptr : oprs) {
        if (opr_ptr && *opr_ptr == *old_opr) {
            auto use = opr_ptr->f_get_use_head(func);
            while(use != nullptr) {
                if(use->I == I) {
                    use->rm_use(func);
                    auto new_use = new VI_Use(new_opr, use->user, I);
                    if(use->user != nullptr)
                        use->user->s_add_use(new_use, func);
                }
                use = use->next;
            }
            *opr_ptr = *new_opr;
        }
    }
}

void replace_defs(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func) {
    vector<VOper *> oprs = get_defs_ptr(I);
    for(VOper *opr_ptr : oprs) {
        if (opr_ptr && *opr_ptr == *old_opr) {
            auto uses = get_uses_ptr(I);
            for(auto use_operand: uses) {
                rm_instruction_use(I, use_operand, func);
                auto new_use = new VI_Use(use_operand, new_opr, I);
                use_operand->add_use(new_use, func);
            }
            *opr_ptr = *new_opr;
        }
    }
}

void s_replace_defs(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func) {
    vector<VOper *> oprs = s_get_defs_ptr(I);
    for(VOper *opr_ptr : oprs) {
        if (opr_ptr && *opr_ptr == *old_opr) {
            auto uses = s_get_uses_ptr(I);
            for(auto use_operand: uses) {
                s_rm_instruction_use(I, use_operand, func);
                auto new_use = new VI_Use(use_operand, new_opr, I);
                use_operand->s_add_use(new_use, func);
            }
            *opr_ptr = *new_opr;
        }
    }
}

vector<VOper> get_defs(VI *I) {
    vector<VOper> oprs;
    switch(I->tag) {
        case VI_MOVE:   { oprs.push_back(((VI_Move *)I)->dst); } break;
        case VI_BINARY: { oprs.push_back(((VI_Binary *)I)->dst); } break;

        case VI_FCAST:{
            oprs.push_back(create_reg(a2));
        } break;

        case VI_FUNC_CALL: {
            oprs.push_back(create_reg(ra));
            for(uint8 r = x1; r < REG_COUNT; r++) {
                if (is_caller_save(r)) {
                    if (r == ra) cout << "ra appear in function call" << endl;
                    oprs.push_back(create_reg(r));
                }
            }
        } break;

        case VI_LOAD: { oprs.push_back(((VI_Load *)I)->reg); } break;

        case VI_SLT: { oprs.push_back(((VI_Slt *)I)->dst); } break;

        case VI_SEQZ:{ oprs.push_back(((VI_Seqz *)I)->dst); } break;

        case VI_SNEZ:{ oprs.push_back(((VI_Snez *)I)->dst); } break;

        case VI_VCVT: {
            auto fcvt = (VI_VCvt *)I;
            if(fcvt->to_type == S32) {
                oprs.push_back(fcvt->dst);
            }
        } break;

        case VI_FCMP_SET: {
            oprs.push_back(((VI_Fcmp_Set *)I)->dst);
        } break;

        case VI_COMPARE:
        case VI_PUSH:
        case VI_RETURN:
        case VI_POP:
        case VI_STORE:
        case VI_BRANCH: 
        //----float----
        case VI_FMOVE:
        case VI_VBINARY:
        case VI_VPUSH:
        case VI_VPOP:
        case VI_VLOAD:
        case VI_VSTORE:
        break;
        
        default: {
            printf("unknown instrID: %d\n", I->tag);
            assert(false && "meet unknown instr in get_defs");
        }
    }
    return oprs;
}

vector<VOper *> get_defs_ptr(VI *I) {
    vector<VOper *> oprs;
    switch(I->tag) {
        case VI_MOVE:   { oprs.push_back(&(((VI_Move *)I)->dst)); } break;
        case VI_BINARY: { oprs.push_back(&(((VI_Binary *)I)->dst)); } break;
        case VI_FCAST:
        case VI_FUNC_CALL: {} break;

        case VI_LOAD: { oprs.push_back(&(((VI_Load *)I)->reg)); } break;

        case VI_SLT: { oprs.push_back(&((VI_Slt *)I)->dst); } break;

        case VI_SNEZ:{oprs.push_back(&((VI_Snez*)I)->dst); } break;

        case VI_SEQZ:{oprs.push_back(&((VI_Seqz*)I)->dst); } break;

        case VI_VCVT: {
            auto fcvt = (VI_VCvt *)I;
            if(fcvt->to_type == S32) {
                oprs.push_back(&fcvt->dst);
            }
            else {
                oprs.push_back(nullptr);
            }
        } break;

        case VI_FCMP_SET: {
            oprs.push_back(&((VI_Fcmp_Set *)I)->dst);
        } break;
        
        case VI_COMPARE:
        case VI_PUSH:
        case VI_RETURN:
        case VI_POP:
        case VI_BRANCH: 
        case VI_STORE: {
            oprs.push_back(nullptr);
        } break;

        // yyy : float类型操作不算，之后其他函数处理
        case VI_FMOVE:
        case VI_VBINARY:
        case VI_VPUSH:
        case VI_VPOP:
        case VI_VLOAD:
        case VI_VSTORE:
        break;
        
        default: {
            printf("unknown instrID: %d\n", I->tag);
            assert(false && "meet unknown instr in get_defs");
        }
    }
    return oprs;
}

// yyy: 这里还有函数调用用到了哪些参数,然后立即数不影响寄存器分析，也不考虑，还有一些其他的差别
vector<VOper> get_uses(VI *I) {
    vector<VOper> oprs;

    switch(I->tag) {

        case VI_MOVE:   {
            if(((VI_Move *)I)->src.tag == REG || ((VI_Move *)I)->src.tag == VREG){
                oprs.push_back(((VI_Move *)I)->src);
            }
        } break;

        case VI_FMOVE: {
            if(((VI_FMove *)I)->src.tag == REG || ((VI_FMove *)I)->src.tag == VREG){
                oprs.push_back(((VI_FMove *)I)->src);
            }
        } break;

        case VI_BINARY: {
            oprs.push_back(((VI_Binary *)I)->lhs);
            if(((VI_Binary *)I)->rhs.tag != IMM)
                oprs.push_back(((VI_Binary *)I)->rhs);
        } break;

        case VI_COMPARE: {
            oprs.push_back(((VI_Compare *)I)->lhs);
            oprs.push_back(((VI_Compare *)I)->rhs);
        } break;

        case VI_SLT: {
            oprs.push_back(((VI_Slt *)I)->lhs);
            oprs.push_back(((VI_Slt *)I)->rhs);
        } break;

        case VI_SEQZ:{
            oprs.push_back(((VI_Seqz *)I)->src);
        } break;

        case VI_SNEZ:{
            oprs.push_back(((VI_Snez *)I)->src);
        } break;

        case VI_FCAST:{
            oprs.push_back(create_reg(a2));
        } break;

        case VI_FUNC_CALL: {
            auto call = (VI_Func_Call *)I;
            for(uint8 r = a0; r <= a3; r++) {
                if (r >= call->arg_count + a0) break;
                oprs.push_back(create_reg(r));
            }
        } break;

        case VI_STORE: { 
            auto store = (VI_Store *)I;
            oprs.push_back(store->reg);
            if(store->base.tag == VREG || store->base.tag == REG)
                oprs.push_back(store->base);
            // if(store->offset.tag == VREG || store->offset.tag == REG)
            //     oprs.push_back(store->offset);
        } break;

        // yyy: 这个load指令和那边get_uses_ptr区别实际上是这边IMM、global_adr也不算，因为不影响寄存器分配
        case VI_LOAD: { 
            auto load = (VI_Load *)I;
            if(load->base.tag == VREG || load->base.tag == REG)
                oprs.push_back(load->base);
            // if(load->offset.tag == VREG || load->offset.tag == REG)
            //     oprs.push_back(load->offset);
        } break;

        case VI_PUSH: {
            auto push = (VI_Push *) I;
            for (auto push_operand : push->operands) {
                oprs.push_back(push_operand);
            }
        } break;

        case VI_RETURN: {
            oprs.push_back(create_reg(ra));
                oprs.push_back(create_reg(a0));
        } break;

        case VI_VLOAD:{
            auto vldr = (VI_VLoad *)I;
            if(vldr->base.tag != ERRORTYPE)
                oprs.push_back(vldr->base);
            // if(vldr->offset.tag != ERRORTYPE)
            //     oprs.push_back(vldr->offset);
        } break;
        case VI_VSTORE:{
            auto vstr = (VI_VStore *)I;
            if(vstr->base.tag != ERRORTYPE)
                oprs.push_back(vstr->base);
            // if(vstr->offset.tag != ERRORTYPE)
            //     oprs.push_back(vstr->offset);
        } break;
        case VI_BRANCH: {
            auto br_I = (VI_Branch *)I;
            if (br_I->cond != NO_CONDITION) {
                oprs.push_back(br_I->lhs);
                oprs.push_back(br_I->rhs);
            }
        } break;
        case VI_VCVT: {
            auto fcvt = (VI_VCvt *)I;
            if(fcvt->from_type == S32) {
                oprs.push_back(fcvt->src);
            }
        } break;

        case VI_POP:
        case VI_FCMP_SET:
        case VI_VBINARY:
        case VI_VPUSH:
        case VI_VPOP:
        break;
        default: {
            printf("unknown instrID: %d\n", I->tag);
            assert(false && "meet unknown instr in get_uses");
        }
    }

    return oprs;
}

vector<VOper> s_get_defs(VI *I) {
    vector<VOper> oprs;
    switch(I->tag) {
        case VI_FMOVE:   { oprs.push_back(((VI_FMove *)I)->dst); } break;
        case VI_VBINARY: { oprs.push_back(((VI_VBinary *)I)->dst); } break;

        case VI_VLOAD:   { oprs.push_back(((VI_VLoad *)I)->reg); } break;

        case VI_FCMP_SET: { oprs.push_back(((VI_Fcmp_Set *)I)->dst); } break;

        case VI_VCVT: {
            auto fcvt = (VI_VCvt *)I;
            if(fcvt->to_type == F32) {
                oprs.push_back(fcvt->dst);
            }
        } break;
        
        case VI_FCAST:{} break;
        case VI_FUNC_CALL: {
            for(uint8 r = f0; r < FREG_COUNT; r++) {
                if (s_is_caller_save(r)) {
                    oprs.push_back(create_freg(r));
                }
            }
        } break;

        
        case VI_VPUSH:
        case VI_VPOP:
        case VI_VSTORE:
        case VI_MOVE:
        case VI_BINARY:
        case VI_CLZ:
        case VI_RETURN:
        case VI_BRANCH:
        case VI_COMPARE:
        case VI_PUSH:
        case VI_POP:
        case VI_LOAD:
        case VI_STORE:
        case VI_SLT:
        case VI_SEQZ:
        case VI_SNEZ:
        break;
        default: {
            printf("unknown instrID: %d\n", I->tag);
            assert(false && "meet unknown instr in s_get_defs");
        }
    }
    return oprs;
}

vector<VOper> s_get_uses(VI *I) {
    vector<VOper> oprs;

    switch(I->tag) {

        case VI_FMOVE:   {
            if (!(((VI_FMove *)I)->from_s32)) {
                oprs.push_back(((VI_FMove *)I)->src);
            }
        } break;

        case VI_VBINARY: {
            oprs.push_back(((VI_VBinary *)I)->lhs);
            if (((VI_VBinary *)I)->fneg == 0) {
                oprs.push_back(((VI_VBinary *)I)->rhs);
            }
        } break;

        case VI_FCMP_SET: {
            oprs.push_back(((VI_Fcmp_Set *)I)->lhs);
            oprs.push_back(((VI_Fcmp_Set *)I)->rhs);
        } break;

        case VI_VSTORE: { 
            oprs.push_back(((VI_VStore *)I)->reg);
        } break;

        case VI_VLOAD: {} break;

        case VI_VCVT: {
            auto fcvt = (VI_VCvt *)I;
            if(fcvt->from_type == F32) {
                oprs.push_back(fcvt->src);
            }
        } break;
        
        case VI_FCAST:{} break;
        case VI_FUNC_CALL: {
            for(uint8 r = f10; r < f14; r++) {
                oprs.push_back(create_freg(r));
            }
        } break;

        case VI_RETURN: {
            oprs.push_back(create_freg(f10));
        } break;

        
        case VI_SLT:
        case VI_SEQZ:
        case VI_SNEZ:
        case VI_VPOP:
        case VI_VPUSH:
        case VI_MOVE:
        case VI_BINARY:
        case VI_CLZ:
        case VI_BRANCH:
        case VI_COMPARE:
        case VI_PUSH:
        case VI_POP:
        case VI_LOAD:
        case VI_STORE:
        break;
        default: {
            printf("unknown instrID: %d\n", I->tag);
            assert(false && "meet unknown instr in s_get_uses");
        }

    }

    return oprs;
}



vector<VOper *> get_uses_ptr(VI *I) {
    vector<VOper *> oprs;

    switch(I->tag) {

        case VI_MOVE:   {
            if(((VI_Move *)I)->src.tag == REG || ((VI_Move *)I)->src.tag == VREG){
                oprs.push_back(&((VI_Move *)I)->src);
            }
        } break;

        case VI_FMOVE: {
            if(((VI_FMove *)I)->src.tag == REG || ((VI_FMove *)I)->src.tag == VREG){
                oprs.push_back(&((VI_FMove *)I)->src);
            }
        } break;

        case VI_SEQZ:  {
            oprs.push_back(&((VI_Seqz *)I)->src);
        } break;

        case VI_SNEZ:  {
            oprs.push_back(&((VI_Snez *)I)->src);
        } break;

        case VI_BINARY: {
            oprs.push_back(&((VI_Binary *)I)->lhs);
            oprs.push_back(&((VI_Binary *)I)->rhs);
        } break;

        case VI_COMPARE: {
            oprs.push_back(&((VI_Compare *)I)->lhs);
            oprs.push_back(&((VI_Compare *)I)->rhs);
        } break;

        case VI_SLT: {
            oprs.push_back(&((VI_Slt *)I)->lhs);
            oprs.push_back(&((VI_Slt *)I)->rhs);
        } break;

        case VI_FCAST:
        case VI_FUNC_CALL: {} break;

        case VI_STORE: { 
            auto store = (VI_Store *)I;
            oprs.push_back(&store->reg);
            if(store->base.tag != ERRORTYPE)
                oprs.push_back(&store->base);
            // if(store->offset.tag != ERRORTYPE)
            //     oprs.push_back(&store->offset);
        } break;

        case VI_LOAD: { 
            auto load = (VI_Load *)I;
            if(load->base.tag != ERRORTYPE)
                oprs.push_back(&load->base);
            // if(load->offset.tag != ERRORTYPE)
            //     oprs.push_back(&load->offset);
        } break;

        case VI_PUSH: {} break;

        // yyy: 为啥没有
        case VI_RETURN: {} break;

        case VI_VLOAD:{
            auto vldr = (VI_VLoad *)I;
            if(vldr->base.tag != ERRORTYPE)
                oprs.push_back(&vldr->base);
            if(vldr->offset.tag != ERRORTYPE)
                oprs.push_back(&vldr->offset);
        } break;
        case VI_VSTORE:{
            auto vstr = (VI_VStore *)I;
            if(vstr->base.tag != ERRORTYPE)
                oprs.push_back(&vstr->base);
            if(vstr->offset.tag != ERRORTYPE)
                oprs.push_back(&vstr->offset);
        } break;
        case VI_BRANCH: {
            auto br_I = (VI_Branch *)I;
            if (br_I->cond != NO_CONDITION) {
                oprs.push_back(&br_I->lhs);
                oprs.push_back(&br_I->rhs);
            }
        } break;
        case VI_VCVT: {
            auto fcvt = (VI_VCvt *)I;
            if(fcvt->from_type == S32) {
                oprs.push_back(&fcvt->src);
            }
        } break;

        case VI_POP:
        
        case VI_FCMP_SET:
        case VI_VBINARY:
        case VI_VPUSH:
        case VI_VPOP:
        break;
        default: {
            printf("unknown instrID: %d\n", I->tag);
            assert(false && "meet unknown instr in get_uses");
        }
    }

    return oprs;
}

vector<VOper *> s_get_defs_ptr(VI *I) {
    vector<VOper *> oprs;
    switch(I->tag) {
        case VI_FMOVE:   { oprs.push_back(&((VI_FMove *)I)->dst); } break;
        case VI_VBINARY: { oprs.push_back(&((VI_VBinary *)I)->dst); } break;

        case VI_VLOAD:   { oprs.push_back(&((VI_VLoad *)I)->reg); } break;

        case VI_FCMP_SET: { oprs.push_back(&((VI_Fcmp_Set *)I)->dst); } break;

        case VI_VCVT: {
            auto fcvt = (VI_VCvt *)I;
            if(fcvt->to_type == F32) {
                oprs.push_back(&fcvt->dst);
            }
            else {
                oprs.push_back(nullptr);
            }
        } break;

        case VI_FCAST:
        case VI_FUNC_CALL: {} break;

        case VI_VPUSH:
        case VI_VPOP:
        case VI_VSTORE: {
            oprs.push_back(nullptr);
        } break;

        case VI_MOVE:
        case VI_BINARY:
        case VI_CLZ:
        case VI_RETURN:
        case VI_BRANCH:
        case VI_COMPARE:
        case VI_PUSH:
        case VI_POP:
        case VI_LOAD:
        case VI_STORE:
        case VI_SLT:
        case VI_SEQZ:
        case VI_SNEZ:
        break;
        default: {
            printf("unknown instrID: %d\n", I->tag);
            assert(false && "meet unknown instr in s_get_defs");
        }
    }
    return oprs;
}

vector<VOper *> s_get_uses_ptr(VI *I) {
    vector<VOper *> oprs;

    switch(I->tag) {

        // yyy: 在get_uses_ptr中，只当tag是REG 或 src是VREG才添加
        case VI_FMOVE:   {
            if (!(((VI_FMove *)I)->from_s32)) {
                oprs.push_back(&((VI_FMove *)I)->src);
            }
        } break;

        case VI_VBINARY: {
            oprs.push_back(&((VI_VBinary *)I)->lhs);
            if (((VI_VBinary *)I)->fneg == 0) {
                oprs.push_back(&((VI_VBinary *)I)->rhs);
            }
        } break;

        case VI_FCMP_SET: {
            oprs.push_back(&((VI_Fcmp_Set *)I)->lhs);
            oprs.push_back(&((VI_Fcmp_Set *)I)->rhs);
        } break;

        case VI_VSTORE: { 
            oprs.push_back(&((VI_VStore *)I)->reg);
        } break;

        case VI_VLOAD: {} break;

        case VI_VCVT: {
            auto fcvt = (VI_VCvt *)I;
            if(fcvt->from_type == F32) {
                oprs.push_back(&fcvt->src);
            }
        } break;

        case VI_FCAST:
        case VI_FUNC_CALL: {} break;

        case VI_RETURN: {} break;

        case VI_SLT:
        case VI_SEQZ:
        case VI_SNEZ:
        case VI_VPOP:
        case VI_VPUSH:
        case VI_MOVE:
        case VI_BINARY:
        case VI_CLZ:
        case VI_BRANCH:
        case VI_COMPARE:
        case VI_PUSH:
        case VI_POP:
        case VI_LOAD:
        case VI_STORE:
        break;
        default: {
            printf("unknown instrID: %d\n", I->tag);
            assert(false && "meet unknown instr in s_get_uses");
        }

    }

    return oprs;
}

void delete_user(VOper *opr, Func_Asm *func) {
    auto I = opr->get_def_I(func);
    if(I != nullptr) {
        auto use_operands = get_uses(I);
        for(auto use_operand: use_operands) {
            auto use = use_operand.get_use_head(func);
            while(use != nullptr) {
                if(use->I == I) {
                    use->rm_use(func);
                }
                use = use->next;
            }
        }
    }
}

void replace_operand_by_operand(VOper *opr, VOper *new_opr, Func_Asm *func) {
    if(opr == nullptr || new_opr == nullptr)
        return;
    // yyy : 旧opr的use链表，逐个访问use了old_opr的指令
    auto use = opr->get_use_head(func);
    while(use != nullptr) {
        auto I = use->I;
        auto oprs = get_uses_ptr(I);
        for(auto old_opr : oprs){
            if(old_opr && *old_opr == *opr){
                auto be_used = old_opr->get_use_head(func);
                while(be_used){
                    if(be_used->I == I){
                        be_used->rm_use(func);
                        auto new_use = new VI_Use(new_opr, be_used->user, I);
                        if(be_used->user){
                            new_opr->add_use(new_use,func);
                        }
                    }
                    be_used = be_used->next;
                }
                *old_opr = *new_opr;
            }
        }
        use = use->next;
    }
    delete_user(opr, func);
}

std::string get_current_file_directory() {
    std::filesystem::path file_path = std::filesystem::absolute(__FILE__);
    std::filesystem::path directory = file_path.parent_path();
    return directory.string();
}
