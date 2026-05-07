// #ifndef MACHINE_CSE
// #define MACHINE_CSE
// #include "MachinePass.h"
// class RiscV64CSE  {
// private:
//     MachineUnit *unit;
//     MachineFunction *current_func;
//     MachineBlock *cur_block;
//     void CSEInCurrFunc();

//     // std::set<MachineBaseInstruction *> CSESet;
//     // std::map<int, int> regreplace_map;
//     // bool is_changed;

// public:
//     RiscV64CSE(MachineUnit *u) : unit(u),is_changed(true) {}
//     void dfs(int bbid);
//     void Execute();
// };
// #endif