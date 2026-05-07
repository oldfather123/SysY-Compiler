//
// Created by 牛奕博 on 2023/4/22.
//

#ifndef ANUC_BLOCKDFSCALCULATOR_H
#define ANUC_BLOCKDFSCALCULATOR_H

#include "core.h"
#include <vector>
#include <set>

using namespace std;
namespace anuc {
    class blockDFSCalulator {
    public:
        static vector<BasicBlock *> &calculateBBPostOrder(vector<BasicBlock *> &postOrder, BasicBlock *entry) {
            vector<BasicBlock *> workList;
            set<BasicBlock *> visited;
            workList.push_back(entry);
            visited.insert(entry);
            while (!workList.empty()) {
                auto b = workList.back();
                bool changed = false;
                for (auto i = b->succBegin(); i != b->succEnd(); ++i) {
                    if (!visited.insert(*i).second) continue;
                    changed = true;
                    workList.push_back(*i);
                    break;
                }
                if (changed) continue;
                b = workList.back();
                postOrder.push_back(b);
                workList.pop_back();
            }
            return postOrder;
        }
    };


}


#endif //ANUC_BLOCKDFSCALCULATOR_H
