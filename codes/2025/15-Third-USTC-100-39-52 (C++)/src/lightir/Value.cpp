#include "Value.hpp"
#include "Type.hpp"
#include "User.hpp"
#include "Instruction.hpp"
#include "Constant.hpp"
#include <cassert>

bool Value::set_name(std::string name) {
    if (name_ == "") {
        name_ = name;
        return true;
    }
    return false;
}

void Value::add_use(User *user, unsigned arg_no) {
    use_list_.emplace_back(user, arg_no);
};

void Value::remove_use(User *user, unsigned arg_no) {
    auto target_use = Use(user, arg_no);
    use_list_.remove_if([&](const Use &use) { return use == target_use; });
}

void Value::replace_all_use_with(Value *new_val) {
    if (this == new_val)
        return;
    while (use_list_.size()) {
        auto use = use_list_.begin();
        use->val_->set_operand(use->arg_no_, new_val);
    }
}
void Value::replace_all_use_with_phi(Value *new_val) {
    if (this == new_val) {
        LOG(DEBUG) << "[replace_all_use_with_phi] New value is same as old. Skipping.";
        return;
    }

    LOG(DEBUG) << "[replace_all_use_with_phi] Replacing all uses of value: " << this->get_name()
               << " with new value: " << new_val->get_name();

    while (!use_list_.empty()) {
        auto use = use_list_.begin();
        LOG(DEBUG) << "[replace_all_use_with_phi] Replacing use in instruction: "
                   << use->val_->get_name()
                   << " at operand index: " << use->arg_no_;

        use->val_->set_operand(use->arg_no_, new_val);
        //auto val1=dynamic_cast<Value *>(use->val_);
        auto str1=dynamic_cast<Instruction *>(use->val_);
        // 如果是phi指令，处理 incoming
        if (auto *phi = dynamic_cast<PhiInst *>(str1)) {
            for (auto &incoming_pair : phi->incoming) {
                if (incoming_pair.first == this) {
                    LOG(DEBUG) << "[replace_all_use_with_phi] Updating phi incoming from "
                               << this->get_name() << " to " << new_val->get_name()
                               << " in block ";
                    incoming_pair.first = new_val;
                }
            }
        }
    }

    LOG(DEBUG) << "[replace_all_use_with_phi] Finished replacing all uses.";
}

void Value::replace_use_with_if(Value *new_val,
                                std::function<bool(Use)> should_replace) {
    if (this == new_val)
        return;
    for (auto iter = use_list_.begin(); iter != use_list_.end();) {
        auto use = *iter++;
        if (not should_replace(use))
            continue;
        use.val_->set_operand(use.arg_no_, new_val);
    }
}
