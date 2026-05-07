#include "allocate_register.hh"
#include "cgen.hh"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <unordered_map>

using std::unordered_map;

// see https://starfivetech.com/uploads/u74_core_complex_manual_21G1.pdf for
// instruction delays
int getDelay(MInstruction *ins) {
  switch (ins->getInsTag()) {
  //  Three-cycle latency, assuming cache hit
  case MInstruction::LW:
  case MInstruction::LD:
  case MInstruction::MUL:
  case MInstruction::MULW: {
    return 3;
  }
  // Between six-cycle to 68-cycle latency, depending on operand  values
  //  Latency = 2 cycles + log2(dividend) - log2(divisor)
  //  + 1 cycle if the input is negative + 1 cycle if the output is negative
  case MInstruction::DIVW:
  case MInstruction::REMW: {
    return 20;
  }
  case MInstruction::FADD_S:
  case MInstruction::FSUB_S: {
    return 5;
  }

  // 9–36
  case MInstruction::FMUL_S:
  case MInstruction::FDIV_S: {
    return 20;
  }

  case MInstruction::FLD:
  case MInstruction::FLW: {
    return 2;
  }
  case MInstruction::FSD:
  case MInstruction::FSW: {
    return 4;
  }
  case MInstruction::FCVTS_W: {
    return 2;
  }
  case MInstruction::FCVTW_S: {
    return 4;
  }
  case MInstruction::FEQ_S:
  case MInstruction::FLT_S:
  case MInstruction::FLE_S: {
    return 4;
  }
  case MInstruction::FMV_S_X:
  case MInstruction::FMV_S: {
    return 2;
  }
    // The pipeline has a peak execution rate of two instructions per clock
    // cycle, and is fully bypassed so that most instructions have a one-cycle
    // result latency
  default:
    return 1;
  }
}

// the longest latency-weighted
uint assign_priority(
    MInstruction *ins,
    unordered_map<MInstruction *, set<MInstruction *>> &successor,
    unordered_map<MInstruction *, uint> &priority) {
  if (priority.count(ins) != 0) {
    return priority[ins];
  }
  int max_pri = 0;
  // std::cout << *ins << endl;
  for (auto succ : successor[ins]) {
    // std::cout << "  " << *succ << endl;
    auto succ_pri = assign_priority(succ, successor, priority);
    max_pri = max_pri > succ_pri ? max_pri : succ_pri;
  }
  priority[ins] = max_pri + getDelay(ins);
  return priority[ins];
}

struct ScheduleEle {
  MInstruction *ins;
  uint priority;
};

void buildDependenceGraph(
    std::vector<MInstruction *> &instrs,
    unordered_map<MInstruction *, set<MInstruction *>> &successor,
    unordered_map<MInstruction *, set<MInstruction *>> &previous,
    set<MInstruction *> &leaves) {
  // todo!!! add leaves
  // node: instr // edge: def->use
  // 1. true dependence — RAW i2 reads a value written by i1 (read after write
  // or RAW),
  // 2. antidependence — WAR i2 writes a value read by i1 (write after read or
  // WAR),
  // 3. antidependence — WRW i2 writes a value written by i1 (write after write
  // or WAW).
  // we don't use pointer analysis here, so all RAW/WAR/WRW to pointers's orders
  // are preserverd.
  unordered_map<Register *, MInstruction *> writes = {};
  unordered_map<Register *, vector<MInstruction *>> reads = {};
  MInstruction *mem_write = nullptr;
  vector<MInstruction *> mem_reads = {};

  static set<Register *> call_read_registers = {Register::reg_sp,
                                                Register::reg_s1};

  static set<Register *> call_write_registers = {
      Register::reg_t0,   Register::reg_t1,   Register::reg_t2,
      Register::reg_a0,   Register::reg_a1,   Register::reg_a2,
      Register::reg_a3,   Register::reg_a4,   Register::reg_a5,
      Register::reg_a6,   Register::reg_a7,   Register::reg_t3,
      Register::reg_t4,   Register::reg_t5,   Register::reg_t6,
      Register::reg_ft0,  Register::reg_ft1,  Register::reg_ft2,
      Register::reg_ft3,  Register::reg_ft4,  Register::reg_ft5,
      Register::reg_ft6,  Register::reg_ft7,  Register::reg_fa0,
      Register::reg_fa1,  Register::reg_fa2,  Register::reg_fa3,
      Register::reg_fa4,  Register::reg_fa5,  Register::reg_fa6,
      Register::reg_fa7,  Register::reg_ft8,  Register::reg_ft9,
      Register::reg_ft10, Register::reg_ft11,
  };
  for (auto ins : instrs) {
    successor[ins] = {};
    previous[ins] = {};

    if (ins->getInsTag() == MInstruction::CALL) {
      // call is equvilent to memory write first
      if (mem_write != nullptr) { // RAW
        successor[mem_write].insert(ins);
        previous[ins].insert(mem_write);
      }
      mem_reads.clear();
      mem_write = ins;

      for (auto usedr : call_read_registers) {
        if (usedr) {
          if (writes.count(usedr) > 0) { // RAW
            successor[writes[usedr]].insert(ins);
            previous[ins].insert(writes[usedr]);
          }
          reads[usedr].push_back(ins);
        }
      }

      for (auto write_reg : call_write_registers) {
        if (writes.count(write_reg) > 0) { // WAW
          successor[writes[write_reg]].insert(ins);
          previous[ins].insert(writes[write_reg]);
        }
        for (auto reader : reads[write_reg]) { // WAR
          successor[reader].insert(ins);
          previous[ins].insert(reader);
        }
        reads[write_reg].clear();
        writes[write_reg] = ins;
      }

      if (previous[ins].size() == 0) {
        leaves.insert(ins);
      }
      continue;
    }

    if (is_mem_write(ins)) {
      if (mem_write != nullptr) { // WAW
        successor[mem_write].insert(ins);
        previous[ins].insert(mem_write);
      }
      for (auto reader : mem_reads) { // WAR
        successor[reader].insert(ins);
        previous[ins].insert(reader);
      }
      mem_reads.clear();
      mem_write = ins;
    } else if (is_mem_read(ins)) {
      if (mem_write != nullptr) { // RAW
        successor[mem_write].insert(ins);
        previous[ins].insert(mem_write);
      }
      mem_reads.push_back(ins);
    }

    auto defd = ins->getTarget();
    if (defd) {
      if (writes.count(defd) > 0) { // WAW
        successor[writes[defd]].insert(ins);
        previous[ins].insert(writes[defd]);
      }
      for (auto reader : reads[defd]) { // WAR
        successor[reader].insert(ins);
        previous[ins].insert(reader);
      }
      reads[defd].clear();
    }

    for (int i = 0; i < ins->getRegNum(); i++) {
      auto used = ins->getReg(i);
      if (writes.count(used) > 0) { // RAW
        successor[writes[used]].insert(ins);
        previous[ins].insert(writes[used]);
      }
      reads[used].push_back(ins);
    }

    // to avoid interference with self
    if (defd) {
      writes[defd] = ins;
    }

    if (previous[ins].size() == 0) {
      leaves.insert(ins);
    }
  }
}

struct ScheduleInsPriorityCompare {
  bool operator()(ScheduleEle &a, ScheduleEle &b) {
    return a.priority > b.priority;
  }
};

bool can_issue_together(MInstruction *i1, MInstruction *i2) {
  // • At most one instruction accesses data memory.
  if (is_mem_op(i1) && is_mem_op(i2)) {
    return false;
  }
  // • At most one instruction is a branch or jump. (we only issue non-control
  // ops here)
  // • At most one instruction is a floating-point arithmetic
  // operation.
  if (is_float_op(i1) && is_float_op(i2)) {
    return false;
  }
  // • At most one instruction is an integer multiplication or division
  // operation.

  if (is_mul_div(i1) && is_mul_div(i2)) {
    return false;
  }
  // • Neither instruction explicitly accesses a CSR. (No access to CSR in
  // user-level program)
  return true;
}

vector<MInstruction *>
issue_instrs(std::priority_queue<ScheduleEle, std::vector<ScheduleEle>,
                                 ScheduleInsPriorityCompare> &ready) {
  vector<MInstruction *> res = {};
  MInstruction *ins = ready.top().ins;
  ready.pop();
  res.push_back(ins);
  // try to issue another instr
  vector<ScheduleEle> can_not_issue = {};
  while (ready.size() != 0) {
    auto ins2 = ready.top();
    ready.pop();
    if (can_issue_together(ins, ins2.ins)) {
      res.push_back(ins2.ins);
      break;
    }
    can_not_issue.push_back(ins2);
  }
  for (auto ins : can_not_issue) {
    ready.push(ins);
  }
  return res;
}

vector<MInstruction *>
scheduleInstrs(std::priority_queue<ScheduleEle, std::vector<ScheduleEle>,
                                   ScheduleInsPriorityCompare> &ready,
               unordered_map<MInstruction *, uint> &priority,
               unordered_map<MInstruction *, set<MInstruction *>> &successor,
               unordered_map<MInstruction *, set<MInstruction *>> &previous) {

  uint cycle = 1;
  unordered_map<MInstruction *, uint> start;
  set<MInstruction *> active = {};
  while (ready.size() != 0 || active.size() != 0) {

    vector<MInstruction *> wl;
    for (auto ins : active) {
      wl.push_back(ins);
    }
    for (auto ins : wl) {
      if (auto currentCycle = start[ins] + getDelay(ins);
          currentCycle < cycle) {

        auto it = active.find(ins);
        if (it != active.end()) {
          active.erase(it);
        }

        // std::cout << "Check successor of " << *ins << endl;
        for (auto succ : successor[ins]) {
          // std::cout << "  Check " << *succ << endl;
          bool succ_ready = true;
          // std::cout << previous[succ].size() << endl;
          for (auto prev : previous[succ]) {
            // std::cout << "     Check prev " << *prev << endl;
            if (!start.count(prev)) {
              succ_ready = false;
              break;
            }
            auto prevCycle = start[prev] + getDelay(prev);
            if (prevCycle >= cycle) {
              succ_ready = false;
              break;
            }
          }

          if (succ_ready) {
            // std::cout << "  Ready " << endl;
            ready.push(ScheduleEle{succ, priority[succ]});
          }
        }
      }
    }

    if (ready.size() != 0) {

      vector<MInstruction *> inss = issue_instrs(ready);
      for (auto ins : inss) {
        start[ins] = cycle;
        active.insert(ins);
        // std::cout << "Issue " << *ins << endl;
      }
    }
    cycle++;
  }

  std::vector<MInstruction *> sortedInstructions;
  for (auto &pair : start) {
    sortedInstructions.push_back(pair.first);
  }
  std::sort(sortedInstructions.begin(), sortedInstructions.end(),
            [&start](MInstruction *a, MInstruction *b) {
              return start[a] < start[b];
            });
  return sortedInstructions;
}

void printVector(const std::vector<MInstruction *> &vec) {
  std::cout << "[";
  for (size_t i = 0; i < vec.size(); ++i) {
    std::cout << "\"" << *vec[i] << "\"";
    if (i < vec.size() - 1)
      std::cout << ", ";
  }
  std::cout << "]" << std::endl;
}

#include <string>

string printDependenceGraph(
    const MBasicBlock *bb,
    const std::unordered_map<MInstruction *, std::set<MInstruction *>> &graph,
    const vector<MInstruction *> &instrs) {
  std::stringstream ss;
  auto name = bb->getName();
  std::replace(name.begin(), name.end(), '.', '_'); // 替换操作
  ss << "digraph " << name << " {\n";
  for (const auto instr : instrs) {
    ss << "    n" << std::hex << instr << " [ label=\"" << *instr << "\"]"
       << ";\n";
  }
  for (const auto &pair : graph) {
    for (MInstruction *succ : pair.second) {
      ss << "    n" << std::hex << pair.first << " -> n" << std::hex << succ
         << ";\n";
    }
  }

  ss << "}"; // 结束定义
  return ss.str();
}

void schedule(MBasicBlock *bb) {

  // std::cout << endl << endl << "Before Schedule!!" << endl;
  // std::cout << *bb;
  vector<MInstruction *> instrs = bb->removeAllInstructions();
  uint instr_sz = instrs.size();
  set<MInstruction *> leaves = {};

  // step1. Build dependence graphE
  unordered_map<MInstruction *, set<MInstruction *>> successor;
  unordered_map<MInstruction *, set<MInstruction *>> previous;
  buildDependenceGraph(instrs, successor, previous, leaves);

  // auto s = printDependenceGraph(bb, successor, instrs);
  // std::ofstream out_s;
  // out_s.open(bb->getName() + "_dg.dot", std::ios::app);
  // out_s << s;
  // std::cout << s;

  // auto s2 = printDependenceGraph(bb, previous, instrs);
  // std::ofstream out_s2;
  // out_s2.open(bb->getName() + "_dg.rev.dot", std::ios::app);
  // out_s2 << s2;
  // std::cout << s2;

  // for (auto leaf : leaves) {
  //   std::cout << " LEAF: " << *leaf << endl;
  // }

  // step2. Assign priorities to each operation
  unordered_map<MInstruction *, uint> priority;
  for (auto leaf : leaves) {
    assign_priority(leaf, successor, priority);
  }
  std::priority_queue<ScheduleEle, std::vector<ScheduleEle>,
                      ScheduleInsPriorityCompare>
      ready;
  for (auto leaf : leaves) {
    ready.push(ScheduleEle{leaf, priority[leaf]});
  }

  // step3. Iteratively select an operation and schedule it
  auto sortedInstructions =
      scheduleInstrs(ready, priority, successor, previous);

  assert(bb->getInstructions().size() == 0);
  bb->pushInstrs(sortedInstructions);

  // std::cout << endl << "After Schedule!!" << endl;
  // std::cout << *bb;

  assert(sortedInstructions.size() == instrs.size());
}

void schedule_instruction(MModule *mod) {
  for (auto func : mod->getFunctions()) {
    for (auto bb : func->getBasicBlocks()) {
      schedule(bb);
    }
  }
}