#pragma once

#include "Common.h"
#include "IR.h"

namespace ir {
    struct DFAContext {

        struct NodeInfo {
            friend class ir::DFAContext;

        private:
            // 到达定值
            std::bitset<BITSET_SIZE> rdGenBitset;
            std::bitset<BITSET_SIZE> rdKillBitset;
            std::bitset<BITSET_SIZE> rdInBitset;
            std::bitset<BITSET_SIZE> rdOutBitset;

            // 活跃变量
            std::bitset<BITSET_SIZE> lvUseBitset;
            std::bitset<BITSET_SIZE> lvDefBitset;
            std::bitset<BITSET_SIZE> lvInBitset;
            std::bitset<BITSET_SIZE> lvOutBitset;

            // 可用表达式
            std::bitset<BITSET_SIZE> aeGenBitset;
            std::bitset<BITSET_SIZE> aeKillBitset;
            std::bitset<BITSET_SIZE> aeInBitset;
            std::bitset<BITSET_SIZE> aeOutBitset;

        public:
            // 到达定值
            Set<ir::Definition> rdGenSet;
            Set<ir::Definition> rdKillSet;
            Set<ir::Definition> rdInSet;
            Set<ir::Definition> rdOutSet;

            // 活跃变量
            Set<ir::Variable> lvUseSet;
            Set<ir::Variable> lvDefSet;
            Set<ir::Variable> lvInSet;
            Set<ir::Variable> lvOutSet;

            // 可用表达式
            Set<Ptr<ir::DefInst>> aeGenSet;
            Set<Ptr<ir::DefInst>> aeKillSet;
            Set<ir::Expr> aeInSet;
            Set<ir::Expr> aeOutSet;

            NodeInfo() { }
        };

    private:
        int analysisFlags;
        Map<ir::BBPtr, NodeInfo> bbInfoMap;

        // 到达定值分析
        void rd();

        // 活跃变量分析
        void lv();

        // 可用表达式分析
        void ae();

    public:
        ir::FuncPtr func;

        enum AnalysisFlag {
            NONE = 0,
            RD = 1 << 0,
            LV = 1 << 1,
            AE = 1 << 2,
            ALL = RD | LV | AE,
        };

        bool assertAvailable(int flag) const {
            if (!(flag & analysisFlags)) {
                dbgassert(false, "Required analysis is not available for this access");
                return false;
            }
            return true;
        }

        const Set<ir::Variable> &getLVInSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::LV);
            return bbInfoMap.at(bb).lvInSet;
        }
        const Set<ir::Variable> &getLVOutSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::LV);
            return bbInfoMap.at(bb).lvOutSet;
        }
        const Set<ir::Definition> &getRDInSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::RD);
            return bbInfoMap.at(bb).rdInSet;
        }
        const Set<ir::Definition> &getRDOutSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::RD);
            return bbInfoMap.at(bb).rdOutSet;
        }
        const Set<ir::Expr> &getAEInSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::AE);
            return bbInfoMap.at(bb).aeInSet;
        }
        const Set<ir::Expr> &getAEOutSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::AE);
            return bbInfoMap.at(bb).aeOutSet;
        }

        const Set<ir::Variable> &getLVUseSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::LV);
            return bbInfoMap.at(bb).lvUseSet;
        }
        const Set<ir::Variable> &getLVDefSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::LV);
            return bbInfoMap.at(bb).lvDefSet;
        }
        const Set<ir::Definition> &getRDGenSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::RD);
            return bbInfoMap.at(bb).rdGenSet;
        }
        const Set<ir::Definition> &getRDKillSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::RD);
            return bbInfoMap.at(bb).rdKillSet;
        }
        const Set<Ptr<ir::DefInst>> &getAEGenSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::AE);
            return bbInfoMap.at(bb).aeGenSet;
        }
        const Set<Ptr<ir::DefInst>> &getAEKillSet(ir::BBPtr bb) const {
            assertAvailable(AnalysisFlag::AE);
            return bbInfoMap.at(bb).aeKillSet;
        }

        DFAContext(ir::FuncPtr func, int analysisFlags = AnalysisFlag::NONE) : func(func), analysisFlags(analysisFlags) {
            if (analysisFlags & RD) {
                rd();
            }
            if (analysisFlags & LV) {
                lv();
            }
            if (analysisFlags & AE) {
                ae();
            }
            dbgout << (String) "├── DFAContext" + (analysisFlags & RD ? " RD," : "") + (analysisFlags & LV ? " LV," : "") + (analysisFlags & AE ? " AE," : "") + " constructed for function " << func->name() << "." << std::endl;
        }
        DFAContext(const DFAContext &) = delete;

        void printDebug() {
            dbgout << "DFA result for function " << func->name() << ":\n";
            for (auto bb : func->bfsBBList()) {
                dbgout << "  BB " << bb->label() << ":\n";
                dbgout << "    RD IN:";
                for (auto def : bbInfoMap.at(bb).rdInSet) {
                    dbgout << "\n        \"" << def.toString() + "\"";
                }
                dbgout << std::endl;
                dbgout << "    RD OUT:";
                for (auto def : bbInfoMap.at(bb).rdOutSet) {
                    dbgout << "\n        \"" << def.toString() + "\"";
                }
                dbgout << std::endl;
                dbgout << "    LV IN:";
                for (auto var : bbInfoMap.at(bb).lvInSet) {
                    dbgout << " \"" << var.toString() << "\"";
                }
                dbgout << std::endl;
                dbgout << "    LV OUT:";
                for (auto var : bbInfoMap.at(bb).lvOutSet) {
                    dbgout << " \"" << var.toString() << "\"";
                }
                dbgout << std::endl;
                dbgout << "    AE IN:";
                for (auto expr : bbInfoMap.at(bb).aeInSet) {
                    dbgout << "\n        \"" << expr.toString() + "\"";
                }
                dbgout << std::endl;
                dbgout << "    AE OUT:";
                for (auto expr : bbInfoMap.at(bb).aeOutSet) {
                    dbgout << "\n        \"" << expr.toString() + "\"";
                }
                dbgout << std::endl;
            }
            dbgout << std::endl;
        }
    };
}
