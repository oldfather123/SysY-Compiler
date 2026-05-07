
#include "asm_passes.h"
#include "../arm.h"
#include <iostream>

// Optimization passes
#include "stack_ra.h"
#include "register_allocation.h"
#include "simplify_asm.h"
#include "remove_identical_moves.h"
#include "block_placement.h"
#include "remove_redundant_ldrs.h"
#include "condify.h"

inline void run_on_every_function(Program_Asm *program_asm, Asm_Func_Pass pass) {
    for(auto func_asm : program_asm->functions) {
        pass(func_asm);
    }
}

#define RUN(pass_name) pass_name(program_asm)
void bless(Program_Asm *program_asm, bool opt) {
	RUN(remove_redundant_ldrs);
    register_allocation(program_asm, opt);
    sregister_allocation(program_asm, opt);
    RUN(remove_identical_moves);
    printf("ok\n");
    // RUN(stack_ra);
    if (opt) {
        RUN(simplify_asm);
        RUN(remove_redundant_ldrs);
        RUN(block_placement);
        RUN(condify);
    }
}
#undef RUN

// helper functions

void replace_uses(MI *I, MOperand old_opr, MOperand new_opr) {
    Array<MOperand *> oprs;

    switch(I->tag) {

        case MI_MOVE:   {
            oprs.push(&((MI_Move *)I)->src);
        } break;

        case MI_CLZ:   {
            oprs.push(&((MI_Clz *)I)->operand);
        } break;

        case MI_BINARY: {
            oprs.push(&((MI_Binary *)I)->lhs);
            oprs.push(&((MI_Binary *)I)->rhs);
        } break;

        case MI_COMPARE: {
            oprs.push(&((MI_Compare *)I)->lhs);
            oprs.push(&((MI_Compare *)I)->rhs);
        } break;

        // no need to handle function call
        case MI_FUNC_CALL: {

        } break;

        case MI_STORE: { 
            oprs.push(&((MI_Store *)I)->reg);
            oprs.push(&((MI_Store *)I)->base);
            oprs.push(&((MI_Store *)I)->offset);
        } break;

        case MI_LOAD: { 
            oprs.push(&((MI_Load *)I)->base);
            oprs.push(&((MI_Load *)I)->offset);
        } break;

        case MI_VSTORE: {
            oprs.push(&((MI_VStore *)I)->reg);
            oprs.push(&((MI_VStore *)I)->base);
            oprs.push(&((MI_VStore *)I)->offset);
        } break;
        
        case MI_VLOAD: {
            oprs.push(&((MI_VLoad *)I)->base);
            oprs.push(&((MI_VLoad *)I)->offset);
        } break;
        /*
        case MI_PUSH: {
            auto push = (MI_Push *) I;
            for(int i = 0; i < push->operands.len; i++) {
                oprs.push(&push->operands[i]);
            }
        } break;
        */

        // also no need for return
        case MI_RETURN:
        case MI_POP:
        case MI_PUSH:
        case MI_BRANCH: 
        //----float----
        case MI_VBINARY:
        case MI_VCOMPARE:
        case MI_VPUSH:
        case MI_VPOP:
        case MI_VCVT:   // convert
        break;
        default: exit(60); assert(false);

    }

    for(MOperand *opr_ptr : oprs) {
        if (opr_ptr && *opr_ptr == old_opr) {
            // @NOTE! remember to keep lsl info!!!
            opr_ptr->tag   = new_opr.tag;
            opr_ptr->value = new_opr.value;
            opr_ptr->adr   = new_opr.adr;
            //*opr_ptr = new_opr;
        }
    }

}

void replace_defs(MI *I, MOperand old_opr, MOperand new_opr) {
    Array<MOperand *> oprs;
    switch(I->tag) {
        case MI_MOVE:   { oprs.push(&((MI_Move *)I)->dst); } break;
        case MI_BINARY: { oprs.push(&((MI_Binary *)I)->dst); } break;

        // no need to do function call
        case MI_FUNC_CALL: {
        } break;

        // bx lr
        case MI_RETURN: {} break;

        case MI_LOAD: { oprs.push(&((MI_Load *)I)->reg); } break;

        case MI_CLZ: { oprs.push(&((MI_Clz *)I)->dst); } break;

        /*
        case MI_POP:  {
            auto pop = (MI_Pop *) I;
            for(int i = 0; i < pop->operands.len; i++) {
                oprs.push(&pop->operands[i]);
            }
        } break;
        */

        case MI_COMPARE:
        case MI_PUSH:
        case MI_POP:
        case MI_STORE: // @TODO: store/load may use self increment thing?
        case MI_BRANCH: 
        //----float----
        case MI_VBINARY:
        case MI_VCOMPARE:
        case MI_VPUSH:
        case MI_VPOP:
        case MI_VLOAD: // vldr
        case MI_VSTORE:  // vstr
        case MI_VCVT:   // convert
        break;
        
        default: exit(61); assert(false);
    }

    for(MOperand *opr_ptr : oprs) {
        if (opr_ptr && *opr_ptr == old_opr) {
            *opr_ptr = new_opr;
        }
    }

}

Array<MOperand> get_defs(MI *I) {
    Array<MOperand> oprs;
    switch(I->tag) {
        case MI_MOVE:   { oprs.push(((MI_Move *)I)->dst); } break;
        case MI_VMOVE: {
            if(((MI_VMove *)I)->dst.tag == REG || ((MI_VMove *)I)->dst.tag == VREG ){
                oprs.push(((MI_VMove *)I)->dst);
            }
        } break;
        case MI_BINARY: { oprs.push(((MI_Binary *)I)->dst); } break;
        case MI_FUNC_CALL: {
            oprs.push(make_reg(lr));
            for(uint8 r = r0; r < REG_COUNT; r++) {
                if (is_caller_save(r)) {
                    oprs.push(make_reg(r));
                }
            }
        } break;

        case MI_LOAD: { oprs.push(((MI_Load *)I)->reg); } break;

        case MI_CLZ: { oprs.push(((MI_Clz *)I)->dst); } break;

        /*
        case MI_POP:  {
            auto pop = (MI_Pop *) I;
            for (auto pop_operand : pop->operands) {
                oprs.push(pop_operand);
            }
        } break;
        */

        case MI_COMPARE:
        case MI_PUSH:
        case MI_RETURN:
        case MI_POP:
        case MI_STORE: // @TODO: store/load may use self increment thing?
        case MI_BRANCH: 
        //----float----
        case MI_VBINARY:
        case MI_VCOMPARE:
        case MI_VPUSH:
        case MI_VPOP:
        case MI_VLOAD: // vldr
        case MI_VSTORE:  // vstr
        case MI_VCVT:   // convert
        break;
        
        default: exit(62); assert(false);
    }
    return oprs;
}

Array<MOperand> get_uses(MI *I, bool func_has_return_value) {
    Array<MOperand> oprs;

    switch(I->tag) {

        case MI_MOVE:   {
            oprs.push(((MI_Move *)I)->src);
        } break;

        case MI_VMOVE: {
            if(((MI_VMove *)I)->src.tag == REG || ((MI_VMove *)I)->src.tag == VREG ){
                oprs.push(((MI_VMove *)I)->src);
            }
        } break;
        case MI_CLZ:   {
            oprs.push(((MI_Clz *)I)->operand);
        } break;

        case MI_BINARY: {
            oprs.push(((MI_Binary *)I)->lhs);
            oprs.push(((MI_Binary *)I)->rhs);
        } break;

        case MI_COMPARE: {
            oprs.push(((MI_Compare *)I)->lhs);
            oprs.push(((MI_Compare *)I)->rhs);
        } break;

        case MI_FUNC_CALL: {
            auto call = (MI_Func_Call *)I;
            // @TODO: get uses based on arg count
            for(uint8 r = r0; r <= r3; r++) {
                if (r >= call->arg_count) break;
                oprs.push(make_reg(r));
            }
        } break;

        case MI_STORE: { 
            oprs.push(((MI_Store *)I)->reg);
            oprs.push(((MI_Store *)I)->base);
            oprs.push(((MI_Store *)I)->offset);
        } break;

        case MI_LOAD: { 
            oprs.push(((MI_Load *)I)->base);
            oprs.push(((MI_Load *)I)->offset);
        } break;

        /*
        case MI_PUSH: {
            auto push = (MI_Push *) I;
            for (auto push_operand : push->operands) {
                oprs.push(push_operand);
            }
        } break;
        */

        case MI_RETURN: {
            oprs.push(make_reg(lr));
            if (func_has_return_value) {
                oprs.push(make_reg(r0));
            }
        } break;

        case MI_VLOAD:{
            auto vldr = (MI_VLoad *)I;
            if(vldr->base.tag == REG || vldr->base.tag == VREG){
                oprs.push(vldr->base);
            }
            if(vldr->offset.tag == REG || vldr->offset.tag == VREG){
                oprs.push(vldr->offset);
            }
        } break;
        case MI_VSTORE:{
            auto vstr = (MI_VStore *)I;
            if(vstr->base.tag == REG || vstr->base.tag == VREG){
                oprs.push(vstr->base);
            }
            if(vstr->offset.tag == REG || vstr->offset.tag == VREG){
                oprs.push(vstr->offset);
            }
        } break;

        case MI_POP:
        case MI_PUSH:
        case MI_BRANCH: 
        //----float----
        case MI_VBINARY:
        case MI_VCOMPARE:
        case MI_VPUSH:
        case MI_VPOP:
        case MI_VCVT:   // convert
        break;
        default: exit(63); assert(false);

    }

    return oprs;
}

Array<MOperand> s_get_defs(MI *I) {
    Array<MOperand> oprs;
    // cout<<"I->tag is"<<I->tag<<endl;
    switch(I->tag) {
        case MI_VMOVE:   { 
            auto myobj = ((MI_VMove *)I)->dst;
            oprs.push(((MI_VMove *)I)->dst); 
        } break;
        case MI_VBINARY: { oprs.push(((MI_VBinary *)I)->dst); } break;

        case MI_VLOAD: { oprs.push(((MI_VLoad *)I)->reg); } break;

        case MI_FUNC_CALL: {
            for(uint8 r = s0; r < SREG_COUNT; r++) {
                if (s_is_caller_save(r)) {
                    oprs.push(make_sreg(r));
                }
            }
        } break;

        

        case MI_VCVT:
        case MI_VCOMPARE:
        case MI_VPUSH:
        case MI_VPOP:
        case MI_VSTORE: // @TODO: store/load may use self increment thing?
        case MI_MOVE:
        case MI_BINARY:
        case MI_CLZ:
        case MI_RETURN:
        case MI_BRANCH:
        case MI_COMPARE:
        case MI_PUSH:
        case MI_POP:
        case MI_LOAD: // ldr
        case MI_STORE: // str
        break;
        default: exit(62); assert(false);
    }
    return oprs;
}

Array<MOperand> s_get_uses(MI *I, bool func_has_return_value) {
    Array<MOperand> oprs;
    // cout<<"I->tag is"<<I->tag<<endl;

    switch(I->tag) {
        case MI_VMOVE:   {
            auto myobj = ((MI_VMove *)I)->src;
            if(myobj.tag == VSREG || myobj.tag == SREG)
            oprs.push(((MI_VMove *)I)->src);
        } break;

        case MI_VBINARY: {
            oprs.push(((MI_VBinary *)I)->lhs);
            if(((MI_VBinary *)I)->fneg != 1)
                oprs.push(((MI_VBinary *)I)->rhs);
        } break;

        case MI_VCOMPARE: {
            oprs.push(((MI_VCompare *)I)->lhs);
            oprs.push(((MI_VCompare *)I)->rhs);
        } break;

        case MI_VSTORE: { 
            oprs.push(((MI_VStore *)I)->reg);
            // oprs.push(((MI_VStore *)I)->base);
            // oprs.push(((MI_VStore *)I)->offset);
        } break;

        case MI_VLOAD: { 
            // oprs.push(((MI_VLoad *)I)->base);
            // oprs.push(((MI_VLoad *)I)->offset);
        } break;

        case MI_VCVT:
        {
            
            oprs.push(((MI_VCvt *)I)->dst);
            if(!(((MI_VCvt *)I)->dst == ((MI_VCvt *)I)->src))
                oprs.push(((MI_VCvt *)I)->src);
        }break;

        case MI_FUNC_CALL: {
            auto call = (MI_Func_Call *)I;
            // @TODO: get uses based on arg count
            for(uint8 r = s0; r <= s3; r++) {
                if (r >= call->arg_count) break;
                oprs.push(make_sreg(r));
            }
        } break;

        case MI_RETURN: {
            if (func_has_return_value) {
                oprs.push(make_sreg(s0));
            }
        } break;
        case MI_VPOP:
        case MI_VPUSH:
        case MI_MOVE:
        case MI_BINARY:
        case MI_CLZ:
        case MI_BRANCH:
        case MI_COMPARE:
        case MI_PUSH:
        case MI_POP:
        case MI_LOAD: // ldr
        case MI_STORE: // str
        break;
        default: exit(63); assert(false);

    }

    return oprs;
}