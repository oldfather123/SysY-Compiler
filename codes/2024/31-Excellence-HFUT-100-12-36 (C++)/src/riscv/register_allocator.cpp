#include "register_allocator.hpp"
#include "../ir/ir.hpp"
#include "arch.hpp"
#include "../parser/SyntaxTree.hpp"
#include <algorithm>
// #include <cstdlib>
#include <iterator>
#include <memory>
// #include <pstl/glue_algorithm_defs.h>
#include <unordered_map>
#include <vector>

void LoongArch::RookieAllocator::SimplifyGraph() {
  auto conf_graph = conflictGraph;
  bool try_again = true;
  // color_count = (this->dealing == Rtype::INT ? i_color.size() : (this->dealing == Rtype::FLOAT ? f_color.size() : fb_color.size()));
  color_count = this->dealing == Rtype::INT ? i_color.size() : f_color.size();
  int param_num = 0;
  if(dealing == INT)
    for(auto reg : i_color) {
      if(reg.id >= 5 && reg.id <=11) {
        param_num++;
      }
    }
  while(try_again) {
    bool changed = true;
    while(changed) {
      changed = false;
      if(log_status)
        std::clog << "当前冲突图中节点个数：" << conf_graph.size() << std::endl;
      std::vector<std::shared_ptr<ir::ir_reg>> del_list;
      for(auto [reg, vec] : conf_graph) {
        int cnt = color_count - (reg->is_param ? param_num : 0);
        if(vec.size() < cnt) {
          s.push_back(reg);
          changed = true;
          del_list.push_back(reg);
        }
      }
      while(!del_list.empty()) {
        auto del_item = del_list.back();
        del_list.pop_back();
        for(auto tar : conf_graph[del_item]) {
          conf_graph[tar].erase(std::remove(conf_graph[tar].begin(), conf_graph[tar].end(), del_item), conf_graph[tar].end());
        }
        auto it = conf_graph.find(del_item);
        if(it != conf_graph.end()) {
          conf_graph.erase(it);
        }
      }
    }
    if(conf_graph.size()) {
      // need split
      Spill(conf_graph);
      try_again = true;
    } else {
      // have solution
      try_again = false;
      // ！！！需要将不和任何其他虚拟寄存器冲突的虚拟寄存器放进来！！！
      for(auto reg : non_conf_regs) {
        s.push_back(reg);
      }
    }
  }
}

void LoongArch::RookieAllocator::Spill(std::unordered_map<std::shared_ptr<ir::ir_reg> ,std::vector<std::shared_ptr<ir::ir_reg>>> &conf_graph) {
  // if(dealing == Rtype::FBOOL) abort(); // 暂未在龙芯文档上查找到如何直接存储fcc寄存器到内存中
  auto it = conf_graph.begin();
  auto del_item = it->first;
  auto vec = it->second;
  mappingToSpill.push_back(del_item);
  if(log_status)
    std::clog << del_item->id << "被放置到内存了" << std::endl;

  // conf_graph.erase(it);
  for(auto tar : vec) {
    conf_graph[tar].erase(std::remove(conf_graph[tar].begin(), conf_graph[tar].end(), del_item), conf_graph[tar].end());
  }
  conf_graph.erase(it);
}

void LoongArch::RookieAllocator::BuildConflictGraph() {
  for(auto global : func->current_globl) {
    if(is_target(global.second->obj->addr))
      allregs.push_back(global.second->obj->addr);
  }
  for(auto funcf : func->func_args) {
    if(is_target(funcf->addr))
      allregs.push_back(funcf->addr);
  }
  for(auto alloc : func->alloc_list) {
    if(is_target(alloc->var->addr))
      allregs.push_back(alloc->var->addr);
  }
  for(auto block : func->bbs) {
    for(auto ins : block->instructions) {
      // auto is_alloc = std::dynamic_pointer_cast<ir::alloc>(ins);
      // if(is_alloc) {
      //   // if(is_alloc->var->dim) {
      //     arrobj.push_back(is_alloc->var);
      //     // continue;
      //   // }
      // }

      auto is_call = std::dynamic_pointer_cast<ir::func_call>(ins);
      if(is_call) {
        auto b = is_call->func_name;
      }

      for(auto raw_reg : ins->def_reg()) {
        auto reg = std::dynamic_pointer_cast<ir::ir_reg>(raw_reg);
        if(reg != nullptr && is_target(reg)) {
          allregs.push_back(reg);
        }
      }
    }
  }
  // log_status = allregs.size() > log_limit;
  if(log_status)
    std::ios_base::sync_with_stdio(false);
  for(int i = 0; i < allregs.size(); i++) {
    auto reg = allregs[i];
    for(int j = i + 1; j < allregs.size(); j++) {
      auto examine = allregs[j];
      if(examine != reg && conflict(reg, examine)) {
        conflictGraph[reg].push_back(examine);
        conflictGraph[examine].push_back(reg);
      }
      if(log_status)
        std::clog << "正在为寄存器\t" << reg->id << "\t分析与\t" << examine->id << "\t的冲突情况" << std::endl;
    }
    auto it = conflictGraph.find(reg);
    if(it == conflictGraph.end()) {
      non_conf_regs.push_back(reg);
    }
  }
  if(log_status)
    std::clog << "寄存器冲突图分析完毕" <<std::endl;
}

bool LoongArch::RookieAllocator::conflict(std::shared_ptr<ir::ir_reg> r1, std::shared_ptr<ir::ir_reg> r2) {
  Pass::LiveInterval r1_i;
  Pass::LiveInterval r2_i;
  if(mappingToInterval[r1].l <= mappingToInterval[r2].l) {
    r1_i = mappingToInterval[r1];
    r2_i = mappingToInterval[r2];
  }
  else {
    r1_i = mappingToInterval[r2];
    r2_i = mappingToInterval[r1];
  }
  if(r1_i.r < r2_i.l)
    return false;
  else
    return true;
}

// bool differ(ptr<ir::ir_reg> src, vartype dst) {
//   if(src->get_type() == vartype::FLOAT || src->get_type() == vartype::FLOATADDR) {
//     return dst != vartype::FLOAT && dst != vartype::FLOATADDR;
//   }
//   else {
//     return dst != vartype::INT && dst != vartype::INTADDR && dst != vartype::BOOL && dst != vartype::BOOLADDR && dst != vartype::VOID;
//   }
// }

LoongArch::alloc_res LoongArch::RookieAllocator::getAllocate() {
  if(log_status)
    std::clog << "简化冲突图中" << std::endl;
  SimplifyGraph();
  if(log_status)
    std::clog << "简化冲突图完毕" << std::endl;
  auto stk = s;
  std::vector<std::shared_ptr<ir::ir_reg>> colored;
  std::unordered_map<std::shared_ptr<ir::ir_reg>, Reg> color_map;
  // vector<Reg> using_color = (this->dealing == Rtype::INT ? i_color : (this->dealing == Rtype::FLOAT ? f_color : fb_color));
  vector<Reg> using_color = this->dealing == Rtype::INT ? i_color : f_color;
  auto conflit_graph = conflictGraph;
  for(auto spilled : mappingToSpill) {
    auto vec = conflit_graph[spilled];
    for(auto tar : vec) {
      conflit_graph[tar].erase(std::remove(conflit_graph[tar].begin(), conflit_graph[tar].end(), spilled), conflit_graph[tar].end());
    }
    conflit_graph.erase(spilled);
  }
  // std::vector<int> total_color;
  // for(int i = 0; i < color_count; i++) total_color.push_back(i);
  while(stk.size()) {
    if(log_status)
      std::clog << "正在为寄存器进行染色，剩余" << stk.size() << std::endl;
    // auto available_i = i_color;
    // auto available_f = f_color;
    auto reg = stk.back();
    stk.pop_back();
    // if(available_color.empty()) {
    //   mappingToSpill.push_back(reg);
    //   continue;
    // }
    vector<Reg> available_color;
    if(reg->is_param && dealing == INT) {
      for(auto color : using_color) {
        if(!(color.id >= 5 && color.id <=11)) {
          available_color.push_back(color);
        }
      }
    }
    else {
      available_color = using_color;
    }
    for(auto [c, color] : color_map) {
      auto it = std::find(conflit_graph[reg].begin(), conflit_graph[reg].end(), c);
      if(it != conflit_graph[reg].end()) {
        available_color.erase(std::remove(available_color.begin(), available_color.end(), color), available_color.end());
      }
    }
    if(available_color.size()) {
      // mappingToReg[reg] = available_color[0] + base_reg;
      mappingToReg[reg] = available_color.front();
      color_map[reg] = available_color.front();
    }
    else {
      abort();
    }
  }
  if(log_status)
    std::clog << "寄存器分配完毕" << std::endl;
  return alloc_res(this->mappingToReg, this->mappingToSpill, this->arrobj);
}


LoongArch::RookieAllocator::RookieAllocator(std::shared_ptr<ir::ir_userfunc> _func, int _base, ptr_list<ir::global_def> global_var) : func(_func), base_reg(_base), global_var(global_var)
{
  auto blocks = func->bbs;
  // for(auto block : blocks) {
  //   in[block] = {};
  //   out[block] = {};
  // }
  // 分析live_use、live_def以及确定基本块之间的前驱、后继关系
  for(auto block : blocks) {
    for(auto instruction : block->instructions) {
      // auto def = instruction->def_reg();
      // auto use = instruction->use_reg();
      // for(auto u : use) {
      //   auto it = find(live_def[block].begin(), live_def[block].end(), u);
      //   if(it == live_def[block].end()) {
      //     live_use[block].push_back(u);
      //   }
      // }
      // for(auto d : def) {
      //   auto it = find(live_def[block].begin(), live_def[block].end(), d);
      //   if(it == live_def[block].end()) {
      //     live_use[block].push_back(d);
      //   }
      //   live_def[block].push_back(d);
      // }
      auto jump_ins = std::dynamic_pointer_cast<ir::jump>(instruction);
      auto br_ins = std::dynamic_pointer_cast<ir::br>(instruction);
      auto while_ins = std::dynamic_pointer_cast<ir::while_loop>(instruction);
      if(jump_ins != nullptr) {
        auto tar = jump_ins->get_target();
        predecessor[tar].push_back(block);
        nxt[block].push_back(tar);
      }
      if(br_ins != nullptr) {
        auto tar = br_ins->get_target_true();
        predecessor[tar].push_back(block);
        nxt[block].push_back(tar);
        tar = br_ins->get_target_false();
        predecessor[tar].push_back(block);
        nxt[block].push_back(tar);
      }
      if(while_ins != nullptr) {
        auto tar = while_ins->get_out_block();
        predecessor[tar].push_back(block);
        nxt[block].push_back(tar);
      }
    }
  }



  // 计算in、out
  // bool changed = true;
  // while(changed) {
  //   changed = false;
  //   for(auto block : blocks) {
  //     for(auto s : successor[block]) {
  //       std::vector<std::shared_ptr<ir::ir_value>> bakup = out[block];
  //       // out[block] = out[block] + in[s];
  //       std::sort(out[block].begin(), out[block].end());
  //       std::sort(in[s].begin(), in[s].end());
  //       std::vector<std::shared_ptr<ir::ir_value>> res;
  //       auto it = std::set_union(
  //         out[block].begin(), out[block].end(), 
  //         in[s].begin(), in[s].end(), std::back_inserter(res)
  //       );
  //       // out[block].resize(it - out[block].begin());
  //       out[block] = res;

  //       if(bakup != out[block])
  //         changed = true;
  //     }
  //     std::vector<std::shared_ptr<ir::ir_value>> bakup = in[block];
  //     // in[block] = live_use[block] + (out[block] - live_def[block]);
  //     std::vector<std::shared_ptr<ir::ir_value>> sub;
  //     std::sort(out[block].begin(), out[block].end());
  //     std::sort(live_def[block].begin(), live_def[block].end());
  //     std::set_difference(
  //       out[block].begin(), out[block].end(), 
  //       live_def[block].begin(), live_def[block].end(), std::back_inserter(sub)
  //     );
  //     std::sort(live_use[block].begin(), live_use[block].end());
  //     std::sort(sub.begin(), sub.end());
  //     std::vector<std::shared_ptr<ir::ir_value>> res;
  //     auto it = std::set_union(
  //       live_use[block].begin(), live_use[block].end(), 
  //       sub.begin(), sub.end(), std::back_inserter(res)
  //     );
  //     // in[block].resize(it - in[block].begin());
  //     in[block] = res;

  //     // if(bakup != in[block])
  //     //   changed = true;
  //   }
  // }


  // 计算基本块的伪线性序
  auto tmp_b = blocks;
  auto tmp_s = predecessor;
  auto tmp_n = nxt;
  std::vector<std::shared_ptr<ir::ir_basicblock>> lines;

  // 保证只从bb0起始，且去除了空块
  vector<std::shared_ptr<ir::ir_basicblock>> rm_vec;
  for(auto block : tmp_b) {
    if(block->name != "bb0" && tmp_s[block].size() == 0) {
      rm_vec.push_back(block);
    }
  }
  for(auto rm : rm_vec) {
      auto new_end = std::remove(tmp_b.begin(), tmp_b.end(), rm);
      tmp_b.erase(new_end, tmp_b.end());
      for(auto child : tmp_n[rm]) {
        // tmp_s[child].remove(block);
        auto new_end = std::remove(tmp_s[child].begin(), tmp_s[child].end(), rm);
        tmp_s[child].erase(new_end, tmp_s[child].end());
      }
  }
  auto erased_block = tmp_b;

  while(tmp_b.size()) {
    for(auto block : tmp_b) {
      if(tmp_s[block].size() == 0) {
        for(auto child : tmp_n[block]) {
          // tmp_s[child].remove(block);
          auto new_end = std::remove(tmp_s[child].begin(), tmp_s[child].end(), block);
          tmp_s[child].erase(new_end, tmp_s[child].end());
        }

        lines.push_back(block);
        auto new_end = std::remove(tmp_b.begin(), tmp_b.end(), block);
        tmp_b.erase(new_end, tmp_b.end());
        break;
      }
    }
  }


  // 计算liveIntervals
  // 先计算行号
  int line = 0;
  for(auto block : lines) {                     // 使用lines则为伪线性序行号，erased_block则为中间代码行号
                                                                           // 虽说使用哪种行号无所谓，只要能算出生命周期即可
                                                                           // 但是使用中间代码行号（erased_block)，则会出现return处理
                                                                           // （中间代码中return处理固定在bb1？）时，生命周期倒乱的情况
    block_line_start[block] = line;
    for(auto instruction : block->instructions) {
      line++;
    }
    block_line_end[block] = line;
  }
  for(auto block_it = lines.rbegin(); block_it != lines.rend(); block_it++) {
    auto block = *block_it;
    int block_start = block_line_start[block];
    int block_end = block_line_end[block];
    int cur_line = block_line_end[block];
    int ultimate_block_start = block_start;
    int ultimate_block_end = block_end;
    if(block->loop_depth) {
      ptr_list<ir::ir_basicblock> block_set = {block};
      ptr_list<ir::ir_basicblock> ins_del_list = {};
      std::unordered_map<ptr<ir::ir_basicblock>, bool> visit;
      while(!block_set.empty()) {
        for(auto s_block : block_set) {
          for(auto check_block : predecessor[s_block]) {
            if(check_block->loop_depth) {
              ultimate_block_start = std::min(ultimate_block_start, block_line_start[check_block]);
              ultimate_block_end = std::max(ultimate_block_end, block_line_end[check_block]);
              if(!visit[check_block])
                ins_del_list.push_back(check_block);
              visit[check_block] = true;
            }
          }
        }
        block_set = ins_del_list;
        ins_del_list.clear();
      }

      block_set = {block};
      ins_del_list = {};
      visit.clear();
      while(!block_set.empty()) {
        for(auto s_block : block_set) {
          for(auto check_block : nxt[s_block]) {
            if(check_block->loop_depth) {
              ultimate_block_start = std::min(ultimate_block_start, block_line_start[check_block]);
              ultimate_block_end = std::max(ultimate_block_end, block_line_end[check_block]);
              if(!visit[check_block])
                ins_del_list.push_back(check_block);
              visit[check_block] = true;
            }
          }
        }
        block_set = ins_del_list;
        ins_del_list.clear();
      }
    }
    ptr_list<ir::ir_reg> judge_list;
    // for(auto instruction : block->instructions) {
    for(auto ins_it = block->instructions.rbegin(); ins_it != block->instructions.rend(); ins_it++) {
      auto instruction = *ins_it;
      auto x = instruction->def_reg();
      auto phi = std::dynamic_pointer_cast<ir::phi>(instruction);
      auto get = std::dynamic_pointer_cast<ir::get_element_ptr>(instruction);
      auto is_call = std::dynamic_pointer_cast<ir::func_call>(instruction);
      if(get) {
        auto ss = get->base_reg->id;
      }
      if(block->loop_depth) {
        for(auto r : x) {
          auto reg = std::dynamic_pointer_cast<ir::ir_reg>(r);
          if(reg != nullptr) {
            mappingToInterval[reg].l = ultimate_block_start;
            mappingToInterval[reg].r = std::max(mappingToInterval[reg].r, ultimate_block_end);
            // if(is_call)
            //   judge_list.push_back(reg);
          }
        }
      }
      else if(phi != nullptr) {
        for(auto r : x) {
          auto reg = std::dynamic_pointer_cast<ir::ir_reg>(r);
          if(reg != nullptr)
            mappingToInterval[reg].l = block_start;
        }
      }
      else {
        for(auto r : x) {
          auto reg = std::dynamic_pointer_cast<ir::ir_reg>(r);
          if(reg != nullptr) {
            if(get) {
              mappingToInterval[reg].l = cur_line;
            }
            else {
              mappingToInterval[reg].l = cur_line + 1;
            }
            if(is_call)
              judge_list.push_back(reg);
          }
        }
      }
      auto y = instruction->use_reg();
      if(block->loop_depth) {
        for(auto r : y) {
          auto reg = std::dynamic_pointer_cast<ir::ir_reg>(r);
          if(reg != nullptr)
            mappingToInterval[reg].r = std::max(mappingToInterval[reg].r, ultimate_block_end);
        }
      }
      if(phi != nullptr) {
        for(auto r : y) {
          auto reg = std::dynamic_pointer_cast<ir::ir_reg>(r);
          if(reg != nullptr)
            mappingToInterval[reg].r = std::max(mappingToInterval[reg].r, cur_line);
        }
      }
      else {
        for(auto r : y) {
          auto reg = std::dynamic_pointer_cast<ir::ir_reg>(r);
          if(reg != nullptr)
            mappingToInterval[reg].r = std::max(mappingToInterval[reg].r, cur_line);
        }
      }
      cur_line--;
    }
    for(auto judge_tar : judge_list) {
      if(mappingToInterval[judge_tar].r == 0) {       // 情况：存在返回值的函数但没有使用该返回值（激进点应该可以直接把它从allregs中删掉了，有待测试）
        mappingToInterval[judge_tar].r = block_end;
      }
    }
  }

}

LoongArch::alloc_res LoongArch::RookieAllocator::run(LoongArch::Rtype target) {
  switch (target) {
    case Rtype::INT: {
      is_target = [](ptr<ir::ir_reg> ireg) {
        return ireg->type != vartype::FLOAT/* && ireg->type != vartype::FBOOL*/;
      };
      break;
    }
    case Rtype::FLOAT: {
      is_target = [](ptr<ir::ir_reg> ireg) {
        return ireg->type == vartype::FLOAT;
      };
      break;
    }
    // case Rtype::FBOOL: {
    //   is_target = [](ptr<ir::ir_reg> ireg) {
    //     return ireg->type == vartype::FBOOL;
    //   };
    //   break;
    // }
  }
  this->dealing = target;
  allregs.clear();
  conflictGraph.clear();
  non_conf_regs.clear();
  arrobj.clear();
  s.clear();
  mappingToSpill.clear();
  mappingToReg.clear();
  this->BuildConflictGraph();
  return this->getAllocate();
}
