#include <cassert>

#include "Value.hh"
#include "User.hh"

void Value::add_use(Value *val, unsigned arg_no) {
    use_list_.push_back(Use(val, arg_no));
}

void Value::replace_all_use_with(Value *new_val) {
    for (auto use : use_list_) {
        auto val = dynamic_cast<User *>(use.val_);
        assert(val && "exist a value(but not user) use old value");
        val->set_operand(use.arg_no_, new_val);
    }
    use_list_.clear();
}

void Value::replace_use_with_when(Value *new_val, std::function<bool(User *)> pred) {
    for (auto it = use_list_.begin(); it != use_list_.end();) {
        auto use = *it;
        auto val = dynamic_cast<User *>(use.val_);
        assert(val && "exist a value(but not user) use old value");
        if (not pred(val)) {
            ++it;
            continue;
        }
        val->set_operand(use.arg_no_, new_val);
        it = use_list_.erase(it);
    }
}

void Value::remove_use(Value *val) {
    auto is_val = [val](const Use &use) { return use.val_ == val; };
    use_list_.remove_if(is_val);
}
