//
// Created by 牛奕博 on 2023/7/15.
//

#ifndef ANUC_INSTEMIT_H
#define ANUC_INSTEMIT_H

#include "core.h"
#include "lowInst.h"
#include "callGraph.h"
#include <cstdio>
#include <string>
#include <queue>
#include <algorithm>

using namespace std;

//指令发射
namespace anuc {
    class InstEmit {
        Module *m;
        IRBuilder *Builder;
        string fileName;
        //构造data段
        FILE *file;
        //被调用者保存
        map<Function *, set<RvRegister *>> &calledSaved;
        map<Function *, CallNode *> &callGraph;

        void dataEmit() {
            fprintf(file, ".data\n");
            for (auto g = m->getGlobalBegin(); g != m->getGlobalEnd(); ++g) {
                GlobalVar *x = &*g;
                Type *ty = cast<PointerType>(x->getType())->getElementType();
                //数组
                if (ty->isArrayType()) {
                    globalConstantArrayEmit(x);
                } else {
                    fprintf(file, "%s:\n", x->getName().c_str());
                    if (isa<Int32Type>(ty)) {
                        int val = cast<ConstantInt>(x->getInit())->getValue();
                        fprintf(file, "  .word %d\n", val);
                    }

                }
            }
        }

        void textEmit() {
            //构造text段
            fprintf(file, ".text\n");
            for (auto f = m->getBegin(); f != m->getEnd(); ++f) {
                Function *func = &*f;
                functionEmit(func);
            }
        }

        //处理function
        void functionEmit(Function *func) {
            string funcName = func->getName();
            fprintf(file, "  .globl %s\n", funcName.c_str());
            fprintf(file, "  .type %s, @function\n", funcName.c_str());

            fprintf(file, "%s:\n", funcName.c_str());
            auto &frame = func->getFrame();
            vector<RvRegister *> save;
            for (auto &s: calledSaved[func]) save.emplace_back(s);
            int size = save.size() * 8 + 16;
            //和16对齐
            if (size % 16 != 0) size += (16 - size % 16);
            //如果函数为最外层函数，不再被其他函数调用，不用保存s
            if (callGraph[func]->calledBy.empty()) size = 16;

            //s和ra的空间
            fprintf(file, "  addi sp, sp, %d\n", -size);
            //保存ra
            fprintf(file, "  sd ra, 0 ( sp ) \n");

            //保存s寄存器
            int offset = 8;
            if (size > 16) {
                for (auto r: save) {
                    string regName = r->toString();
                    fprintf(file, "  sd %s, %d ( sp ) \n", regName.c_str(), offset);
                    offset += 8;
                }
            }

            //构造栈上局部变量
            int localFrameSize = func->getFrameSize();
            if (localFrameSize % 16 != 0)localFrameSize += (16 - localFrameSize % 16);
            if (localFrameSize) {
                if (localFrameSize < 2048)
                    fprintf(file, "  addi sp, sp, %d\n", -localFrameSize);
                else {
                    fprintf(file, "  li s0, %d\n", -localFrameSize);
                    fprintf(file, "  add sp, sp, s0\n");
                }
            }

            BasicBlock *entry = func->getEnrty();
            BasicBlock *exit = func->getExit();
            //按照bfs顺序打印
            set<BasicBlock *> visited;
            queue<BasicBlock *> workQueue;
            workQueue.push(entry);
            visited.insert(entry);
            visited.insert(exit);
            while (!workQueue.empty()) {
                BasicBlock *b = workQueue.front();
                workQueue.pop();
                fprintf(file, "%s: \n", b->getName().c_str());
                for (auto i = b->getBegin(); i != b->getEnd(); ++i) {
                    fprintf(file, "%s\n", (&*i)->toString().c_str());
                }
                for (auto succ = b->succBegin(); succ != b->succEnd(); ++succ) {
                    if (!visited.insert(*succ).second) continue;
                    workQueue.push(*succ);
                }
            }

            //打印exit块
            if (exit != entry) {
                fprintf(file, "%s: \n", exit->getName().c_str());
                for (auto i = exit->getBegin(); i != exit->getEnd(); ++i)
                    fprintf(file, "%s\n", (&*i)->toString().c_str());
            }

            //恢复栈上局部变量空间
            if (localFrameSize) {
                if (localFrameSize < 2048)
                    fprintf(file, "  addi sp, sp, %d\n", localFrameSize);
                else {
                    fprintf(file, "  li s0, %d\n", localFrameSize);
                    fprintf(file, "  add sp, sp, s0\n");
                }
            }

            //恢复ra
            fprintf(file, "  ld ra, 0 ( sp ) \n");
            //恢复s寄存器
            offset = 8;
            if (size > 8) {
                for (auto r: save) {
                    string regName = r->toString();
                    fprintf(file, "  ld %s, %d ( sp ) \n", regName.c_str(), offset);
                    offset += 8;
                }
            }
            fprintf(file, "  addi sp, sp, %d\n", size);

            //释放函数栈空间
            fprintf(file, "  ret\n");

        }

        void globalConstantArrayEmit(GlobalVar *g) {
            fprintf(file, "%s:\n", g->getName().c_str());
            auto list = g->getInitList();
            PointerType *pty = cast<PointerType>(g->getType());
            ArrayType *aty = cast<ArrayType>(pty->getElementType());
            int byteSize = aty->getByteSize();
            vector<int> indexes;
            vector<int> values;
            if(list)
            for (auto &i: list->initInfos) {
                ConstantInt *x = cast<ConstantInt>(i.second);
                int size{0};
                Type *ty = aty;
                for (auto &idx: i.first) {
                    ty = cast<ArrayType>(ty)->getArrayType();
                    size += ty->getByteSize() * idx;
                }
                indexes.push_back(size);
                values.push_back(x->getValue());
            }
            int currentIndex = 0;
            //if(!indexes.empty()) fprintf(file, "  .zero %d\n", indexes[0]);
            for(int i = 0; i < indexes.size(); ++i) {
                int zeroNums = indexes[i] - currentIndex;
                if(zeroNums) fprintf(file, "  .zero %d\n", zeroNums);
                fprintf(file, "  .word %d\n", values[i]);
                currentIndex = indexes[i] + 4;
            }
            int zeroNums = byteSize - currentIndex;
            if(zeroNums) fprintf(file, "  .zero %d\n", zeroNums);
        }

    public:
        InstEmit(Module *m, IRBuilder *Builder, string fileName,
                 map<Function *, set<RvRegister *>> &calledSaved, map<Function *, CallNode *> &callGraph) :
                m(m), Builder(Builder), fileName(fileName),
                calledSaved(calledSaved), callGraph(callGraph) {
            const char *name = fileName.c_str();
            file = fopen(name, "w");
            if (file == NULL) {
                printf("无法创建文件 %s\n", name);
                return;
            }
        }

        void run() {
            dataEmit();
            textEmit();
        }
    };
}


#endif //ANUC_INSTEMIT_H
