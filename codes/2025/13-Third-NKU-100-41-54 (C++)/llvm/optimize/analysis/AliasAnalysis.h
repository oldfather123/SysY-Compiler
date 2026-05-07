#ifndef ALIASANALYSIS_H
#define ALIASANALYSIS_H
#include "../../include/ir.h"
#include "../pass.h"
#include "../lib_function_names.h"
#include <cmath>

/*
   README：
1. 此Pass依赖于Build_CFG时建立的def_map
2. 基础SysY中仅数组会用到Pointer，因此此AliasAnalysis仅考虑了Array的别名分析；若后续添加LoopParallel，会产生新的Pointer，需要进一步修缮
3. 调用方式：
    - 首先确保你查询的指针非nullptr
    - 查看两个ptr（RegOperand/GlobalOperand）是否别名冲突：
        if( QueryAlias(op1,op2, cfg) == MustAlias )...
    - 查看某指令(store/load/call)是否Mod/Ref某ptr：
        if( QueryInstModRef(inst,op,cfg) == Ref )...
4. 对于GEP指令Index为RegOperand的情况，我们无法直接判定为是否相同，统一认为存在别名冲突的风险，即MustAlias
5. 若查询的指针是数组基址，默认代表整个数组，即某inst是否访问了该数组
6. 对于call函数名称为lib_function_names中的指令，也可以识别和分析
*/
enum AliasStatus{ 
    NoAlias=0, 
    MustAlias=1 
};
enum ModRefStatus{ 
    NoModRef=0, 
    Ref=1, 
    Mod=2, 
    ModRef=3 
};

struct PtrInfo{//根类型的PtrInfo的AliasOps一定为空； 非根类型的PtrInfo一定要有root
    enum sources{ Undef=0, Gep=1, Phi=2 }source;
    enum types{ Undefed=0, Global=1, Param=2, Local=3 }type;
    Operand root;//祖先指针，Global/Param/Local定义的数组基址
    GetElementptrInstruction* gep; //记录初次索引的GEP指令，初次！！
    double se_gep_offset;//若经过了二代GEP指令，记录相对初次GEP的总偏移量
    std::unordered_set<Operand> AliasOps;//别名集，仅记录本身及其祖先

    PtrInfo(){ source=sources::Undef; type=types::Undefed; se_gep_offset=0; root=nullptr;gep=nullptr;}
    PtrInfo(types ty){source=sources::Undef;  type=ty; se_gep_offset=0; se_gep_offset=0;root=nullptr;gep=nullptr;}//用于根信息的创建
    PtrInfo(types ty, Operand op){ source=sources::Undef;  type=ty;  AliasOps.emplace(op); se_gep_offset=0;root=nullptr;gep=nullptr; }
    PtrInfo(sources so,types ty,  Operand op){ source=so;  type=ty;  AliasOps.emplace(op); se_gep_offset=0;root=nullptr;gep=nullptr;}
    void AddAliass(std::unordered_set<Operand> newOps){ AliasOps.insert(newOps.begin(),newOps.end());gep=nullptr;}

    void PrintDebugInfo();
};

struct RWInfo{
    std::unordered_set<Operand> ReadRoots;
    std::unordered_set<Operand> WriteRoots;
    bool has_lib_func_call = false;

    RWInfo(){has_lib_func_call = false;}
    void AddRead(Operand root){ ReadRoots.insert(root); }
    void AddWrite(Operand root){ WriteRoots.insert(root); }
};

struct CallInfo{
    std::unordered_map<Operand,int> param_order;//parameters ~ order in function_def_inst/call_inst
    std::unordered_map<CFG*,CallInstruction> callers;//caller ~ the call instruction
    CallInfo(){}
};

struct GlobalValInfo{//每个函数读/写的global val（不含global array)
    std::unordered_set<std::string> ref_ops;
    std::unordered_set<std::string> mod_ops;
    GlobalValInfo(){}
    void AddInfo(GlobalValInfo* info){
        ref_ops.insert(info->ref_ops.begin(),info->ref_ops.end());
        mod_ops.insert(info->mod_ops.begin(),info->mod_ops.end());
    }
};

class AliasAnalysisPass : public IRPass { 
private:
    std::unordered_map<CFG*,std::unordered_map<int,PtrInfo*>>ptrmap;//regno-->PtrInfo (only for RegOperand)
    std::unordered_map<CFG*,RWInfo>rwmap;//ref_operands,mod_operands
    std::unordered_map<std::string,CallInfo>ReCallGraph;
    std::unordered_set<std::string>LeafFuncs;

    void FindPhi();
    void DefineLibFuncStatus();

    Operand CalleeParamToCallerArgu(Operand op, CFG* callee_cfg, CallInstruction* CallI);
    void RWInfoAnalysis();
    void GatherRWInfos(std::string func_name);

    void GatherPtrInfos(CFG* cfg, int regno);
    void PtrPropagationAnalysis();
    bool IsSameArraySameConstIndex(GetElementptrInstruction* inst1,GetElementptrInstruction* inst2 ,double offset1, double offset2);
    
public:
    std::unordered_map<CFG*,GlobalValInfo*> globalmap;

    AliasAnalysisPass(LLVMIR *IR) : IRPass(IR) {DefineLibFuncStatus();}
    PtrInfo* GetPtrInfo(Operand op, CFG* cfg);
    AliasStatus QueryAlias(Operand op1,Operand op2, CFG* cfg);
    ModRefStatus QueryInstModRef(Instruction inst, Operand op, CFG* cfg);
    void Execute();

    std::unordered_map<CFG*,std::unordered_map<int,PtrInfo*>>& GetPtrMap(){return ptrmap;}
	std::unordered_map<CFG*,RWInfo>& GetRWMap() { return rwmap; }
    std::unordered_map<std::string,CallInfo>& GetReCallGraph() { return ReCallGraph; }
    ModRefStatus QueryCallGlobalModRef(CallInstruction* callI, std::string global_name);//基于globalmap，查询call指令是否mod/ref某全局非Array变量

    // 判断函数是否有副作用（side effect）
    bool HasSideEffect(CFG* cfg);
    bool HasSideEffect(const std::string& func_name);

    void Test();
    void PrintAAResult();

};

#endif
