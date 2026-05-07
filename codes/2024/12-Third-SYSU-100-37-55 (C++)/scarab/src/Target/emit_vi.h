#ifndef EMIT_VI_H
#define EMIT_VI_H

#include <string.h>
#include <string>
#include <sstream>
#include <stdint.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <utility>
#include <map>
#include <set>
#include <queue>
#include <deque>
#include<iostream>

#include <cstring>

#include "vi_inst.h"
#include "Module.h"
#define LAST_GEP_LIMIT 4

using namespace std;

extern float2int f2i;

// yyy: 将value映射到voper
static Map<ValuePtr, VOper> val_vopr_map; 
// yyy: 存储已经计算过的数组指针地址，当call函数调用数组时会将其地址存储到这里
static Map<ValuePtr, VOper> array_ptr_map;
static Map<VOper, VOper> imm_map; 
// yyy: ptr_val_map实际上计算的是基址的offset而不是其基址，比如arr[0][0]记录为0
static Map<ValuePtr, int> ptr_val_map; 
// yyy: 把moperand(vreg等)映射为其值，只用于常量
static map<VOper, int> int_val_map;
static map<string, Binary_Op_Type> Binary_ir2asm;
static map<string, Branch_Condition> string_to_branchCondition;
static map<pair<VOper, VOper>, int32> vopr_offset_map;

class Gep_address_map {
public:
    Gep_address_map() : max_size(LAST_GEP_LIMIT) {}

    // 插入元素
    void insert(std::pair<GetElementPtrInstruction*, VOper> pair) {
        if (mydeque.size() == max_size) {
            mydeque.pop_front(); // 移除最早的元素
        }
        mydeque.push_back(pair); // 插入新元素
    }

    std::vector<std::pair<GetElementPtrInstruction*, VOper>> get() const {
        std::vector<std::pair<GetElementPtrInstruction*, VOper>> result;
        result.reserve(mydeque.size()); // 预留空间提高效率
        for (auto it = mydeque.rbegin(); it != mydeque.rend(); ++it) {
            result.push_back(*it);
        }
        return result;
    }

    void clear(){
        mydeque.clear();
    }

    std::deque<std::pair<GetElementPtrInstruction*, VOper>> mydeque;
    size_t max_size;
};

VOper create_imm(int32 constant);
VOper create_reg(uint8 reg);
VOper create_vreg(int32 vreg_index);
VOper create_freg(uint8 sreg);
VOper create_vfreg(int32 vsreg_index);
VOper create_legal_imm(int32 constant, MachineBlock *mb);

void insert(VI *vi, VI *before);
void push(VI *vi, MachineBlock *mb);
template<typename T>
void push_end(T* vi, MachineBlock* mb);
VI_Load *generate_li(VOper vreg, int32 constant, MachineBlock *mb, VI *I=NULL);

Func_Asm* emit_func_vi(FunctionPtr func, int idx, vector<VariablePtr> &globalValues, int& labelNum);

Branch_Condition invert_branch_cond(Branch_Condition cond);
template<typename T>
bool conver_intReg_to_floatReg(T *bi_I, VOper& intReg, MachineBlock *mb, bool left=true, bool s32tof32=true);
void replace_inst_operand(VI *I, VOper *old_opr, VOper *new_opr, Func_Asm *func);
Func_Asm* initialize_func_vi(FunctionPtr func, int idx, vector<VariablePtr> &globalValues);
VOper cal_new_gep_base(VOper old_base, VOper old_offset, MachineBlock* mb);
VOper move_f32_to_s32(ValuePtr v, MachineBlock *mb);
template<typename T>
VOper deal_with_gep(T* ptr_I, MachineBlock* mb);
template<typename T>
void combineGep(T* ptr_I, MachineBlock* mb);
bool compare_gep(GetElementPtrInstruction* ptr1, GetElementPtrInstruction* ptr2, int& offset);
bool find_in_value_offset(VOper v1, VOper v2, int& result);
void Initial_ir_asm_op_map();
std::vector<int> extract_array_sizes(const std::string& type_str);
bool deal_with_memset(CallInstruction* func_I, MachineBlock * mb);
int calculateOffset(const std::vector<int>& dimensions, const std::vector<int>& pos1, const std::vector<int>& pos2);
void put_vi_binary_into_offset(VOper& dst, VOper& lhs, VOper& rhs, char op);
void loadOrStoreTranslator(ValuePtr from, ValuePtr to, MachineBlock* mb, vector<VariablePtr> &globalValues, Func_Asm* func_asm, bool isFloat, bool isLoad);
#endif
