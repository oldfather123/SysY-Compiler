#include "LocalComSubExprEli.hh"
#include "DominateTree.hh"

void LocalComSubExprEli::execute(){
    auto dom_tree = DominateTree(module_);
    dom_tree.execute();
    get_eliminatable_call();
    for(auto func: module_->get_functions()){
      func_ = func;
      visited.clear();
      auto entry_bb = func_->get_entry_block();
      AEB.clear();
      AEB_riconst.clear();
      AEB_liconst.clear();
      AEB_rfconst.clear();
      AEB_lfconst.clear();
      gep_2op.clear();
      gep_2op_const.clear();
      gep_3op.clear();
      gep_3op_const.clear();
      load_val.clear();
      load_off_val_const.clear();
      load_off_val.clear();
      visited.clear();
      for (auto bb : func->get_basic_blocks()){
        if(visited.find(bb)==visited.end()){
          delete_expr_local(bb);
        }
      }
    }
}

void LocalComSubExprEli::get_eliminatable_call(){
    for(auto func: module_->get_functions()){
        auto can_eli = true;
        if(func->get_basic_blocks().size()==0){
            can_eli = false;
        }
        for(auto bb: func->get_basic_blocks()){
            for(auto instr: bb->get_instructions()){
                if(instr->is_store()){
                    can_eli = false;
                }
                if(instr->is_call()){
                  auto call_func = dynamic_cast<Function*>(instr->get_operand(0));
                  if(eliminatable_call.find(call_func->get_name())== eliminatable_call.end()&&call_func!=func){
                    can_eli = false;
                  }
                }
            }
        }
        if(can_eli){
            eliminatable_call.insert(func->get_name());
        }
    }
}

bool LocalComSubExprEli::is_call_eliminatable(std::string func_id) {
    if (eliminatable_call.find(func_id) != eliminatable_call.end()) {
        return true;
    } 
    return false;
}

Arglist *LocalComSubExprEli::get_call_args(CallInst *instr) {
  auto args = new Arglist();
  for (int i = 1; i < instr->get_num_operands(); i++) {
    args->args_.push_back(instr->get_operand(i));
  }
  return args;
}

void LocalComSubExprEli::delete_expr_local(BasicBlock* bb){
    //get pre_info
    if(bb!=func_->get_entry_block()){
      auto idom_bb = bb->get_idom();
      if(visited.find(idom_bb)==visited.end()){
        delete_expr_local(idom_bb);
      }

      AEB[bb] = AEB[idom_bb];
      AEB_riconst[bb] = AEB_riconst[idom_bb];
      AEB_liconst[bb] = AEB_liconst[idom_bb];
      AEB_rfconst[bb] = AEB_rfconst[idom_bb];
      AEB_lfconst[bb] = AEB_lfconst[idom_bb];
      gep_2op[bb] = gep_2op[idom_bb];
      gep_2op_const[bb] = gep_2op_const[idom_bb];
      gep_3op[bb] = gep_3op[idom_bb];
      gep_3op_const[bb] = gep_3op_const[idom_bb];
      // load_val[bb] = load_val[idom_bb];
      // load_off_val_const[bb] = load_off_val_const[idom_bb];
      // load_off_val[bb] = load_off_val[idom_bb];
      // call_val[bb] = call_val[idom_bb];
    }


    std::vector<Instruction *> delete_list;
    int pos = 0;
    for(auto instr: bb->get_instructions()){
        if(instr->is_int_binary()){
            auto lhs = instr->get_operand(0);
            auto rhs = instr->get_operand(1);
            auto lhs_const = dynamic_cast<ConstantInt *>(lhs);
            auto rhs_const = dynamic_cast<ConstantInt *>(rhs);

            if(rhs_const){
                if(AEB_riconst[bb].find({lhs, rhs_const->get_value(), instr->get_instr_type()})!=AEB_riconst[bb].end()){
                    auto iter = AEB_riconst[bb].find({lhs, rhs_const->get_value(), instr->get_instr_type()});
                    instr->replace_all_use_with(iter->second);
                    delete_list.push_back(instr);
                }
                else{
                    AEB_riconst[bb].insert({{lhs, rhs_const->get_value(), instr->get_instr_type()}, instr});
                }
            }
            else if(lhs_const){
                if(AEB_liconst[bb].find({lhs_const->get_value(), rhs, instr->get_instr_type()})!=AEB_liconst[bb].end()){
                    auto iter = AEB_liconst[bb].find({lhs_const->get_value(), rhs, instr->get_instr_type()});
                    instr->replace_all_use_with(iter->second);
                    delete_list.push_back(instr);
                }
                else{
                    AEB_liconst[bb].insert({{lhs_const->get_value(), rhs, instr->get_instr_type()}, instr});
                }
            }
            else{
                if(AEB[bb].find({lhs, rhs, instr->get_instr_type()})!=AEB[bb].end()){
                    auto iter = AEB[bb].find({lhs, rhs, instr->get_instr_type()});
                    instr->replace_all_use_with(iter->second);
                    delete_list.push_back(instr);
                }
                else{
                    AEB[bb].insert({{lhs, rhs, instr->get_instr_type()}, instr});
                }
            }
        }
        else if(instr->is_float_binary()){
            auto lhs = instr->get_operand(0);
            auto rhs = instr->get_operand(1);
            auto lhs_const = dynamic_cast<ConstantFP *>(lhs);
            auto rhs_const = dynamic_cast<ConstantFP *>(rhs);

            if(rhs_const){
                if(AEB_rfconst[bb].find({lhs, rhs_const->get_value(), instr->get_instr_type()})!=AEB_rfconst[bb].end()){
                    auto iter = AEB_rfconst[bb].find({lhs, rhs_const->get_value(), instr->get_instr_type()});
                    instr->replace_all_use_with(iter->second);
                    delete_list.push_back(instr);
                }
                else{
                    AEB_rfconst[bb].insert({{lhs, rhs_const->get_value(), instr->get_instr_type()}, instr});
                }
            }
            else if(lhs_const){
                if(AEB_lfconst[bb].find({lhs_const->get_value(), rhs, instr->get_instr_type()})!=AEB_lfconst[bb].end()){
                    auto iter = AEB_lfconst[bb].find({lhs_const->get_value(), rhs, instr->get_instr_type()});
                    instr->replace_all_use_with(iter->second);
                    delete_list.push_back(instr);
                }
                else{
                    AEB_lfconst[bb].insert({{lhs_const->get_value(), rhs, instr->get_instr_type()}, instr});
                }
            }
            else{
                if(AEB[bb].find({lhs, rhs, instr->get_instr_type()})!=AEB[bb].end()){
                    auto iter = AEB[bb].find({lhs, rhs, instr->get_instr_type()});
                    instr->replace_all_use_with(iter->second);
                    delete_list.push_back(instr);
                }
                else{
                    AEB[bb].insert({{lhs, rhs, instr->get_instr_type()}, instr});
                }
            }
        }
        else if(instr->is_gep()){
          auto ptr = instr->get_operand(0);
          if(instr->get_num_operands()==2){
            auto offset = instr->get_operand(1);
            auto offset_const = dynamic_cast<ConstantInt *>(offset);
            if(offset_const){
              if(gep_2op_const[bb].find({ptr,offset_const->get_value()})!=gep_2op_const[bb].end()){
                auto iter = gep_2op_const[bb].find({ptr,offset_const->get_value()});
                instr->replace_all_use_with(iter->second);
                delete_list.push_back(instr);
              }
              else{
                gep_2op_const[bb].insert({{ptr,offset_const->get_value()}, instr});
              }
            }
            else{
              if(gep_2op[bb].find({ptr,offset})!=gep_2op[bb].end()){
                auto iter = gep_2op[bb].find({ptr,offset});
                instr->replace_all_use_with(iter->second);
                delete_list.push_back(instr);
              }
              else{
                gep_2op[bb].insert({{ptr,offset}, instr});
              }
            }
          }
          else{
            auto offset = instr->get_operand(2);
            auto offset_const = dynamic_cast<ConstantInt *>(offset);
            if(offset_const){
              if(gep_3op_const[bb].find({ptr,offset_const->get_value()})!=gep_3op_const[bb].end()){
                auto iter = gep_3op_const[bb].find({ptr,offset_const->get_value()});
                instr->replace_all_use_with(iter->second);
                delete_list.push_back(instr);
              }
              else{
                gep_3op_const[bb].insert({{ptr,offset_const->get_value()}, instr});
              }
            }
            else{
              if(gep_3op[bb].find({ptr,offset})!=gep_3op[bb].end()){
                auto iter = gep_3op[bb].find({ptr,offset});
                instr->replace_all_use_with(iter->second);
                delete_list.push_back(instr);
              }
              else{
                gep_3op[bb].insert({{ptr,offset}, instr});
              }
            }
          }
        }
        else if(instr->is_loadoffset()){
            auto ptr = instr->get_operand(0);
            auto offset = instr->get_operand(1);
            auto const_offset = dynamic_cast<ConstantInt *>(offset);
            if(const_offset){
                if (load_off_val_const[bb].find({ptr, const_offset->get_value(), instr->get_instr_type()}) != load_off_val_const[bb].end()) {
                    auto iter = load_off_val_const[bb].find({ptr, const_offset->get_value(), instr->get_instr_type()});
                    instr->replace_all_use_with(iter->second);
                    delete_list.push_back(instr);
                } else {
                    load_off_val_const[bb].insert({{ptr, const_offset->get_value(), instr->get_instr_type()}, instr});
                }
            }
            else{
                if (load_off_val[bb].find({ptr, offset, instr->get_instr_type()}) != load_off_val[bb].end()) {
                    auto iter = load_off_val[bb].find({ptr, offset, instr->get_instr_type()});
                    instr->replace_all_use_with(iter->second);
                    delete_list.push_back(instr);
                } else {
                    load_off_val[bb].insert({{ptr, offset, instr->get_instr_type()}, instr});
                }
            }
        }
        else if(instr->is_load()){
            auto ptr = instr->get_operand(0);
            if(load_val[bb].find(ptr)!=load_val[bb].end()){
                auto iter = load_val[bb].find(ptr);
                instr->replace_all_use_with(iter->second);
                delete_list.push_back(instr);
            }
            else{
                load_val[bb].insert({ptr, instr});
            }
        }
        else if(instr->is_store()||instr->is_storeoffset()){
            load_val[bb].clear();
            load_off_val[bb].clear();
            load_off_val_const[bb].clear();
        }    
        else if(instr->is_call()){
            auto func_id = static_cast<Function *>(instr->get_operand(0))->get_name();
            auto args = get_call_args(dynamic_cast<CallInst *>(instr));
            if(!is_call_eliminatable(func_id)){
              load_val[bb].clear();
              load_off_val[bb].clear();
              load_off_val_const[bb].clear();
            }
            if(is_call_eliminatable(func_id) && !instr->is_void()){
                if (call_val[bb].find({func_id, *args}) != call_val[bb].end()) {
                    auto iter = call_val[bb].find({func_id, *args});
                    instr->replace_all_use_with(iter->second);
                    delete_list.push_back(instr);
                } 
                else {
                    call_val[bb].insert({{func_id, *args}, instr});
                }
            }
        }
        
        pos++;
    }

    for(auto instr: delete_list){
        bb->delete_instr(instr);
    }
    visited.insert(bb);
}

const bool Arglist::operator<(const Arglist &a) const {
  if (this->args_.size() < a.args_.size()) {
    return true;
  } else if (this->args_.size() == a.args_.size()) {
    for (int i = 0; i < this->args_.size(); i++) {
      auto c1 = dynamic_cast<ConstantInt *>(this->args_[i]);
      auto c2 = dynamic_cast<ConstantInt *>(a.args_[i]);
      if (c1 && c2) {
        if (c1->get_value() == c2->get_value()) {
          continue;
        } else {
          return c1->get_value() < c2->get_value();
        }
      } else {
        if (this->args_[i] == a.args_[i]) {
          continue;
        } else {
          return this->args_[i] < a.args_[i];
        }
      }
    }
  }
  return false;
}

const bool Arglist::operator>(const Arglist &a) const {
  if (this->args_.size() > a.args_.size()) {
    return true;
  } else if (this->args_.size() == a.args_.size()) {
    for (int i = 0; i < this->args_.size(); i++) {
      auto c1 = dynamic_cast<ConstantInt *>(this->args_[i]);
      auto c2 = dynamic_cast<ConstantInt *>(a.args_[i]);
      if (c1 && c2) {
        if (c1->get_value() == c2->get_value()) {
          continue;
        } else {
          return c1->get_value() < c2->get_value();
        }
      } else {
        if (this->args_[i] == a.args_[i]) {
          continue;
        } else {
          return this->args_[i] < a.args_[i];
        }
      }
    }
  } else {
    return false;
  }
  return false;
}

const bool Arglist::operator==(const Arglist &a) const {
  if (this->args_.size() == a.args_.size()) {
    for (int i = 0; i < this->args_.size(); i++) {
      if (i < a.args_.size()) {
        auto c1 = dynamic_cast<ConstantInt *>(this->args_[i]);
        auto c2 = dynamic_cast<ConstantInt *>(a.args_[i]);
        if (c1 && c2) {
          if (c1->get_value() == c2->get_value()) {
            continue;
          } else {
            return false;
          }
        } else {
          if (this->args_[i] == a.args_[i]) {
            continue;
          } else {
            return false;
          }
        }
      }
    }
  } else {
    return false;
  }
  return true;
}


