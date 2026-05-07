#include "instruction.h"
#include "basicblock.h"
#include "recycle.h"

namespace IR
{
    std::string Instruction::getOpStr()
    {
        switch (getOpcode())
        {
        case Add:
            return "add";
        case Sub:
            return "sub";
        case Mul:
            return "mul";
        case Div:
            return "sdiv";
        case Rem:
            return "srem";
        case FAdd:
            return "fadd";
        case FSub:
            return "fsub";
        case FMul:
            return "fmul";
        case FDiv:
            return "fdiv";
        case FRem:
            return "frem";
        case And:
            return "and";
        case Or:
            return "or";
        case Xor:
            return "xor";
        case Neg:
            return "neg";
        case FNeg:
            return "fneg";
        case Not:
            return "not";
        case Alloca:
            return "alloca";
        case Load:
            return "load";
        case Store:
            return "store";
        case GEP:
            return "getElementPtr";
        case FPtoSI:
            return "fptosi";
        case SItoFP:
            return "sitofp";
        case ICmp:
            return "icmp";
        case FCmp:
            return "fcmp";
        case Call:
            return "call";
        case Phi:
            return "phi";
        case Select:
            return "select";
        case BR:
            return "br";
        case Ne:
            return "ne";
        case Lt:
            return "lt";
        case Le:
            return "le";
        case Gt:
            return "gt";
        case Ge:
            return "ge";
        case Eq:
            return "eq";
        case FNe:
            return "fne";
        case FLt:
            return "flt";
        case FLe:
            return "fle";
        case FGt:
            return "fgt";
        case FGe:
            return "fge";
        case FEq:
            return "feq";
        case Return:
            return "return";
        default:
            return "unknown";
        }
    }

    void Instruction::removeNode() {
        // 移除形成的前驱后继关系, (已不需要)
        // 从BB中的instructionList中移除
        if (prev != nullptr)
        {
            prev->next = next;
        }
        if (next != nullptr)
        {
            next->prev = prev;
        }
    }

    void Instruction::waste()
    {
        // 从BB中的instructionList中移除
        assert(parent != nullptr); // 保证parent不为空
        parent->getInstruction().remove(this);

        // 移除所有被使用的位置
        for (ListNode* i = useList.begin(); i != useList.end(); i = i->nextNode())
        {
            Use* use = static_cast<Use*>(i);
            use->val->useList.remove(use);
            User* user = use->user;
            user->removeUseFromVector(use);
        }

        // 移除所有的使用, 将其从对应value的useList中删除
        for (auto &use : uses)
        {
            use->val->useList.remove(use);
        }

        utils::Recycle::free(this, [](void *ptr) {
            delete static_cast<Instruction *>(ptr);
        });
    }

    bool Instruction::isUseless()
    {
        switch (getOpcode())
        {
        case Store:
            return false;
        case Call:
            return false;
        case Return:
            return false;
        case BR:
            return false;
        case InderectBr:
            return false;
        default:
            return useList.empty();
        }
    }
}