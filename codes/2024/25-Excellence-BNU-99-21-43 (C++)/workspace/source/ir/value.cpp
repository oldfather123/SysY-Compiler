#include "value.h"
#include "user.h"
#include <algorithm>
#include "recycle.h"

namespace IR
{
    void Value::waste()
    {
        // 当 Value 被删除时，好像没啥要做的，此时其useList应该为空
        assert(useList.empty());
        utils::Recycle::free(this, [](void *ptr)
                             { delete static_cast<Value *>(ptr); });
    }

    std::vector<Use *> Value::getVectorUses()
    {
        std::vector<Use *> uses;
        for (ListNode *i = useList.begin(); i != useList.end(); i = i->nextNode())
        {
            uses.push_back(static_cast<Use *>(i));
        }
        return uses;
    }

    void Value::emitUse(std::ostream &os)
    {
        os << getIRName() << " used by:" << std::endl;
        for (ListNode *i = useList.begin(); i != useList.end(); i = i->nextNode())
        {
            Use *use = static_cast<Use *>(i);
            os << '\t' << use->user->getIRName() << std::endl;
        }
    }

    void Value::addUsage(Use *use)
    {
        useList.pushBack(use);
    }

    std::vector<User *> Value::getUsers()
    {
        std::vector<User *> users;
        for (ListNode *i = useList.begin(); i != useList.end(); i = i->nextNode())
        {
            Use *use = static_cast<Use *>(i);
            users.push_back(use->user);
        }
        return users;
    }

    Value *Value::createTemp(BasicType *type)
    {
        return new Value(type, ValueTy::TemporaryVal);
    }

    void Value::replaceAllUsageTo(Value *newUser)
    {
        // 首先把所有使用了 this 的 user 的 uses 和 operands 更正。
        std::vector<Use *> thisUses;
        for (ListNode *i = useList.begin(); i != useList.end(); i = i->nextNode())
        {
            assert(dynamic_cast<Use *>(i));
            Use *use = dynamic_cast<Use *>(i);
            assert(dynamic_cast<User *>(use->user));
            auto &op = use->user->operands;
            auto it = std::find(op.begin(), op.end(), this);
            assert(it != op.end());
            *it = newUser;
            use->val = newUser;
            thisUses.push_back(use);
        }
        // 然后清空 this 的 useList 并将所有 use 添加到 newUser 的 useList 中
        for (auto use : thisUses)
        {
            use->val->useList.remove(use);
            newUser->addUsage(use);
        }
    }
}