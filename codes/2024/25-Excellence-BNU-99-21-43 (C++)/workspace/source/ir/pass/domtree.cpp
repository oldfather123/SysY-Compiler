#include "domtree.h"

namespace IR
{
    void DominatorTree::build(Function *func)
    {
        std::map<BasicBlock *, std::set<BasicBlock *>> cfg = func->cfg();
        root = func->getEntryBlock();
        assert(root != nullptr);
        dom.clear();
        idom.clear();
        domFrontier.clear();
        domChildren.clear();
        std::map<BasicBlock *, std::set<BasicBlock *>> prev;
        for (auto &[v, G] : cfg)
        {
            for (auto &u : G)
            {
                prev[u].insert(v);
            }
        }

        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *block = static_cast<BasicBlock *>(i);
            dom[block].insert(block);
            if (block != root)
            {
                for (ListNode *j = func->blocks().begin(); j != func->blocks().end(); j = j->nextNode())
                {
                    BasicBlock *b = static_cast<BasicBlock *>(j);
                    dom[block].insert(b);
                }
            }
        }

        // 计算支配集
        bool changed = true;
        while (changed)
        {
            changed = false;
            for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
            {
                BasicBlock *block = static_cast<BasicBlock *>(i);
                if (block == func->getEntryBlock())
                    continue;
                std::map<BasicBlock *, int> cnt;
                int prevNum = prev[block].size();
                for (auto &p : prev[block])
                {
                    for (auto &v : dom[p])
                    {
                        cnt[v]++;
                    }
                }
                std::set<BasicBlock *> newDom;
                newDom.insert(block);
                for (auto &[b, c] : cnt)
                {
                    if (c == prevNum)
                    {
                        newDom.insert(b);
                    }
                }
                if (newDom != dom[block])
                {
                    changed = true;
                    dom[block] = newDom;
                }
            }
        }
        // std::cerr << "Dom: " << std::endl;
        // for (auto &[bb, doms] : dom)
        // {
        //     std::cerr << "BasicBlock: " << bb->getIRName() << std::endl;
        //     for (auto &dom : doms)
        //     {
        //         std::cerr << dom->getIRName() << std::endl;
        //     }
        // }
        // std::cerr << "==================" << std::endl;
        // 计算直接支配节点, 构建支配树
        for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        {
            BasicBlock *v = static_cast<BasicBlock *>(i);
            if (v == func->getEntryBlock())
                continue;
            // std::cerr << "calc idom: " << v->getIRName() << std::endl;
            for (auto domi : dom[v])
            {
                if (domi == v)
                    continue;
                bool flag = true; // 如果domi被dom[v]中的所有节点支配，则domi是v的直接支配点
                // std::cerr << "\tcalc idom: " << domi->getIRName() << std::endl;
                for (auto domj : dom[v])
                {
                    // std::cerr << "\t\t" << domj->getIRName() << std::endl;
                    if (domj != v && dom[domi].find(domj) == dom[domi].end())
                    {
                        flag = false;
                        break;
                    }
                }
                // std::cerr << "\tflag: " << flag << ' ' << "BasicBlock: " << domi->getIRName() << std::endl;
                if (flag)
                {
                    idom[v] = domi;
                    domChildren[domi].insert(v);
                    break;
                }
            }
        }

        // 计算支配边界
        // for (ListNode *i = func->blocks().begin(); i != func->blocks().end(); i = i->nextNode())
        // {
        //     BasicBlock *block = static_cast<BasicBlock *>(i);
        //     if (block == func->getEntryBlock())
        //         continue;
        //     for (auto &pred : prev[block])
        //     {
        //         auto runner = pred;
        //         while (runner != idom[block])
        //         {
        //             domFrontier[runner].insert(block);
        //             runner = idom[runner];
        //         }
        //     }
        // }
        return;
    }
}
