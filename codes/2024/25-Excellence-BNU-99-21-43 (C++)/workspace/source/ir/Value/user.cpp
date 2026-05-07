#include "user.h"

namespace IR
{
    int User::totalUsers = 0;

    void User::addUse(Value *val)
    {
        assert(val != nullptr);
        operands.push_back(val);
        Use::create(val, this);
    }

    void User::removeUseFromVector(Use *use)
    {
        // 删除operands中的val
        auto valit = std::find(operands.begin(), operands.end(), use->val);
        if (valit != operands.end())
            operands.erase(valit);

        // 删除使用了Value的use
        auto it = std::find(uses.begin(), uses.end(), use);
        if (it != uses.end())
        {
            (*it)->val->useList.remove(*it);
            uses.erase(it);
        }
    }
}