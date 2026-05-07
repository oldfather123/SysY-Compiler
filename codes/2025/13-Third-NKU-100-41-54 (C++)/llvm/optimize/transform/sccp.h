#ifndef SCCP_H
#define SCCP_H
#include "../../include/ir.h"
#include "../pass.h"
#include <deque>
#include <memory>
#include "simplify_cfg.h"

class SCCPPass : public IRPass { 
private:
    enum LatticeStatus {UNDEF=0,CONST=1,OVERDEF=2};
    struct Lattice {
        LatticeStatus status;
        union {
            int intVal; 
            float floatVal; 
        }val;
        enum ValType{VOID,INT,FLOAT}valtype;
        
        Lattice() : status(LatticeStatus::UNDEF) {val.intVal=0;valtype=VOID;}
        void setLattice(LatticeStatus s,int v)  { status = s; val.intVal=v;valtype=INT;}
        void setLattice(LatticeStatus s,float v) {status = s; val.floatVal=v;valtype=FLOAT;}
        bool NE(const Lattice& lhs) {
            if (lhs.status != status) {
                return true;
            }
            if(lhs.status == LatticeStatus::CONST) {
                if (lhs.val.intVal != val.intVal) {
                    return true;
                }
            }
            return false;
        }
    };
    std::unordered_map<int, std::unique_ptr<Lattice>> ValueLattice;//Value的reg的regno -> Lattice . 防止内存泄漏
    //std::unordered_map<int, Lattice*> ValueLattice; 
    LatticeStatus Intersect(LatticeStatus a, LatticeStatus b){
            if(a == b) return a;
            if(a == LatticeStatus::OVERDEF || b == LatticeStatus::OVERDEF) return LatticeStatus::OVERDEF;
            if(a == LatticeStatus::UNDEF || b == LatticeStatus::UNDEF) return LatticeStatus::UNDEF;
            return LatticeStatus::CONST;
        }
    std::deque<BasicBlock*> CFGWorkList;
    std::deque<int> SSAWorkList;
    std::unordered_map<int,int> edges_to_remove;//【label_block ~ br_block】（br_cond-->br_uncond，删除了一条边，边上的数据流传递也该删除（phi））

    void buildSSAGraph(CFG* cfg);
    void SCCP();
    void visit_phi(CFG* cfg, PhiInstruction* phi , int block_id);
    void visit_other_operations(CFG* cfg,Instruction inst , int block_id);

    void ConstGlobalReplace();

public:
    SCCPPass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

#endif