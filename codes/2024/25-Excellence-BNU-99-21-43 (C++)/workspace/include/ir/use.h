#pragma once
#include "error.h"
#include "listnode.h"

namespace IR
{

    struct Value;
    struct User;
    struct Use : public ListNode
    {
        Value *val;
        User *user;

        Use(Value *val, User *user) : ListNode(1), val(val), user(user) {}
        
        static Use *create(Value *val, User *user);

        void setValue(Value *val);
    };

}
