//
// Created by 牛奕博 on 2023/4/17.
//

#ifndef ANUC_CORE_H
#define ANUC_CORE_H

#include <iostream>
#include <vector>
#include <map>
#include <set>
#include <stdio.h>
#include "alist.h"
#include "rtti.h"
#include "type.h"

using namespace std;
namespace anuc {
    class Module;

    class Function;

    class BasicBlock;

    class Instruction;

    class Visitor;

    class Module {
        alist<Function> childlist;
        alist<GlobalVar> globalVarList;
        //存储所有函数并查找
        map<string, Function *> funcLookUp;
        //存储所有全局变量并查找
        map<string, GlobalVar *> globalLookUp;
        //存储外部函数
        vector<Function*> externFunction;
        //内存管理池
        map<Value *, bool> valuePool;
        set<Type *> typePool;
    public:
        void insertBackToChild(Function *f);

        void insertFrontToChild(Function *f);

        void insertBackToChild(GlobalVar *v);

        bool insertFunc(string name, Function *f);

        bool insertGlobal(string name, GlobalVar *v);

        //返回外部函数
        vector<Function*> &getExternFuncs() {
            return externFunction;
        }

        //查找函数
        Function *lookUpFunc(string name) {
            auto p = funcLookUp.find(name);
            if (p == funcLookUp.end()) return nullptr;
            else return p->second;
        }

        //查找全局变量
        GlobalVar *lookUpGlobalVar(string name) {
            auto p = globalLookUp.find(name);
            if (p == globalLookUp.end()) return nullptr;
            else return p->second;
        }

        //进行内存管理，将new出来的东西插入内存池
        map<Value *, bool>::iterator insertIntoPool(Value *v);

        map<Value *, bool>::iterator insertIntoPool(Value *v, Value *rest...);

        //从内存池中查找
        map<Value *, bool>::iterator lookUpValuePool(Value *v);

        void insertIntoPool(Type *ty) {
            typePool.insert(ty);
            return;
        }

        //消除指定元素,实际上是将其bool设置为false，在memory中删除
        bool eraseFromValuePool(Value *v);

        //释放所有标记为0的内存空间，返回删除元素的个数
        int memoryClean();

        //打印整个模块
        void print(string fileName);

        alist<Function>::iterator getBegin() { return childlist.begin(); }

        alist<Function>::iterator getEnd() { return childlist.end(); }

        alist<GlobalVar>::iterator getGlobalBegin() { return globalVarList.begin(); }

        alist<GlobalVar>::iterator getGlobalEnd() { return globalVarList.end(); }

        ~Module();
    };

    class Function : public alist_node<Function> {
        friend class ASTVisitor;

    private:
        //记录函数栈上变量+大小
        map<Value*, int> frame;
        int frameSize{0};
        Module *parent;
        alist<BasicBlock> childlist;
        using argv = pair<string, Type *>;
        vector<argv> argvs;
        vector<Value *> argvals;
        FunctionType *type;
        string name;
        BasicBlock *exit;

    public:
        Function &operator=(const Function &) = delete;

        Function(const Function &) = delete;

        Function(Module *m, FunctionType *type, string name) :
                parent(m), name(name), type(type) {
            auto &paramTy = type->getArgvs();
            for (auto ty = paramTy.begin(); ty != paramTy.end(); ++ty) argvs.push_back({"", *ty});

        }

        //设置名称后value才会初始化（懒得改了）
        void setArgvName(vector<string> &names) {
            for (int i = 0; i < argvs.size(); ++i) {
                argvs[i].first = "argv" + names[i];
                Value *arg = new RegisterVar(argvs[i].second, argvs[i].first);
                argvals.push_back(arg);
                //可能是外部函数，没有父母
                if(parent) parent->insertIntoPool(arg);
            }
        }

        vector<Value *> &getArgVals() {
            return argvals;
        }

        argv getArgvsName(int i) { return argvs.at(i); }

        Module *getParent() { return parent; }

        void insertBackToChild(BasicBlock *b);

        void insertFrontToChild(BasicBlock *b);

        alist<BasicBlock>::iterator getBegin() { return childlist.begin(); }

        alist<BasicBlock>::iterator getEnd() { return childlist.end(); }

        BasicBlock *getEnrty() { return &*childlist.begin(); }
        BasicBlock *getExit() { return exit;}
        void setExit(BasicBlock *bb) { exit = bb;}


        string getName() { return name; }

        FunctionType *getFuncType() { return type; }

        map<Value*, int> &getFrame() { return frame; }
        string toString() {
            string s = name + "(";
            for (int i = 0; i < argvs.size(); ++i) {
                s +=  argvs[i].second->toString() + " " + argvs[i].first;
                if (i != argvs.size() - 1) s += ", ";
            }
            s += "):" ;
            return s;
        }
        void print();
        void setFrameSize(int n) {frameSize = n;}
        int getFrameSize() {return frameSize;}
        void eraseFromList(BasicBlock *bb);


    };

    //basicblock
    class BasicBlock : public Value, public alist_node_with_parent<BasicBlock, Function> {
        Function *parent;
        alist<Instruction> childlist;
        std::vector<BasicBlock *> pred;
        std::vector<BasicBlock *> succ;
        Instruction *terminate{nullptr};
        string name;
        Type *type;
    public:
        BasicBlock &operator=(const BasicBlock &) = delete;

        BasicBlock(const BasicBlock &) = delete;

        BasicBlock(string name, Type *ty) : Value(Value::VK_BasicBlock, ty), name(name), type(ty) {}

        static void insertBasicBlock(BasicBlock *p, BasicBlock *b, BasicBlock *s) {
            int index = 0;
            for(auto i = p->succBegin(); i != p->succEnd(); ++i) {
                if(*i != s) continue;
                vector<BasicBlock*> list = p->getSucc();
                list[index] = list.back();
                list.pop_back();
                ++index;
                break;
            }
            index = 0;
            for(auto i = s->predBegin(); i != s->predEnd(); ++i) {
                if(*i != p) continue;
                vector<BasicBlock*> list = s->getPred();
                list[index] = list.back();
                list.pop_back();
                ++index;
                break;
            }
            b->pushBackToPred(p);
            b->pushBackToSucc(s);
            s->pushBackToPred(b);
            p->pushBackToSucc(b);
        }

        bool static classof(Value *v) { return v->getKind() == VK_BasicBlock; }

        Type *getType() { return type; }

        void setParent(Function *func) { parent = func; }

        Function *getParent() { return parent; }

        void eraseFromList(Instruction *inst);

        void setTerminated(Instruction *inst) { terminate = inst; }
        Instruction *getTerminated() { return terminate; }
        bool isTerminated() { return terminate != nullptr; }

        void insertBackToChild(Instruction *i);

        void insertFrontToChild(Instruction *i);

        void insertIntoChild(Instruction *inst, Instruction *insertPoint);

        void insertIntoBackChild(Instruction *insertPoint, Instruction *inst);
        void insertIntoBackChild(Instruction *insertPoint, vector<Instruction *> inst);

        void pushBackToPred(BasicBlock *bb) { pred.push_back(bb); }

        void pushBackToPred(BasicBlock *bb, BasicBlock *rest ...) {
            pushBackToPred(bb);
            pushBackToPred(rest);
        }

        void pushBackToSucc(BasicBlock *bb) { succ.push_back(bb); }

        void pushBackToSucc(BasicBlock *bb, BasicBlock *rest ...) {
            pushBackToSucc(bb);
            pushBackToSucc(rest);
        }

        vector<BasicBlock *>::iterator predBegin() { return pred.begin(); }

        vector<BasicBlock *>::iterator predEnd() { return pred.end(); }

        vector<BasicBlock *>::iterator succBegin() { return succ.begin(); }

        vector<BasicBlock *>::iterator succEnd() { return succ.end(); }

        bool succEmpty() { return succ.empty(); }
        vector<BasicBlock *> &getSucc() {
            return  succ;
        }
        vector<BasicBlock *> &getPred() {
            return pred;
        }


        string getName() { return name; }

        void setName(string s) { name = s; }

        string toString() { return "%" + name; }

        alist<Instruction>::iterator getBegin() { return childlist.begin(); }

        alist<Instruction>::iterator getEnd() { return childlist.end(); }

        alist<Instruction>::iterator getFront() { return childlist.front(); }

        alist<Instruction>::iterator getBack() { return childlist.back(); }

        void eraseFromParent();
        void print();

    };

    class Instruction : public User, public alist_node_with_parent<Instruction, BasicBlock> {
        unsigned num{0};
    protected:
        BasicBlock *parent;
    public:
        Instruction &operator=(const Instruction &) = delete;

        Instruction(const Instruction &) = delete;

        Instruction(ValueKind kind, BasicBlock *parent) : User(kind), parent(parent) {}

        virtual void accept(Visitor *V) {}

        bool static classof(Value *v) {
            return v->getKind() >= Value::VK_Instruction
                   && v->getKind() < Value::VK_LastInstruction;
        }

        BasicBlock *getParent() { return parent; }

        virtual void print() {
            cout << "null inst print" << endl;
        }

        Instruction *getNext() { return static_cast<Instruction *>(next); }

        Instruction *getPrev() { return static_cast<Instruction *>(prev); }


        //删除，最后释放空间
        //清空use链条
        void eraseFromParent() {
            for (auto i = operands.begin(); i != operands.end(); ++i) {
                (*i)->value->eraseFromList((*i));
            }
            auto m = this->getParent()->getParent()->getParent();
            auto i = m->lookUpValuePool(this);
            i->second = false;
            this->getParent()->eraseFromList(this);
        }

        //只从链表中删除，不删除内存
        void removeFromParent() { this->erase(); }

        virtual Value *getResult() { return nullptr; }
        virtual void setResult(BaseReg *v) { return; }
    };

    //get element ptr
    class GEPInst : public Instruction {
        Type *type;
        vector<Value *> idx;
        RegisterVar *result; //创建出的结果
    public:
        GEPInst(BasicBlock *parent, Type *ty, Value *ptr, vector<Value *> idx, RegisterVar *result) :
                Instruction(VK_GEPInst, parent), type(ty), idx(idx), result(result) {
            Use *op0 = new Use(ptr, this);
            operands.push_back(op0);
            ptr->insertBackToUses(op0);
            for (int i = 0; i < idx.size(); ++i) {
                Use *op1 = new Use(idx[i], this);
                operands.push_back(op1);
                idx[i]->insertBackToUses(operands[i + 1]);
            }
            result->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_GEPInst; }

        Value *getPtr() { return operands[0]->value; }

        Value *getIdx(int i) { return idx[i]; }

        Value *getResult() { return result; }

        Type *getType() { return type; }

        void print() {
            cout << "  %" << result->getName() << " = getelementptr " + type->toString() +
                                                  ", " << operands[0]->value->getType()->toString()
                 << operands[0]->value->toString()
                 << ", ";
            string s;
            for (int i = 0; i < idx.size(); ++i) {
                s += operands[i + 1]->value->getType()->toString() + operands[i + 1]->value->toString();
                if (i + 1 != idx.size()) s += ", ";
            }
            cout << s << endl;
        }

        void accept(Visitor *V);
    };

    //allocate/store/load
    class AllocateInst : public Instruction {
        Type *type;
        RegisterVar *result;
        int align = 4;
    public:
        AllocateInst &operator=(const AllocateInst &) = delete;

        AllocateInst(BasicBlock *parent, Type *type, RegisterVar *result) : Instruction(Value::VK_AllocaInst, parent),
                                                                            type(type), result(result) {
            result->setInst(this);
            if (isa<PointerType>(type)) align = 8;
        }

        bool static classof(Value *v) { return v->getKind() == VK_AllocaInst; }

        Value *getResult() {
            return result;
        }

        Type *getType() {
            return type;
        }

        void print() {
            cout << "  %" << result->getName() << " = alloca "
                 << type->toString() << ", align " << align << endl;
        }

        void accept(Visitor *V);

    };

    class StoreInst : public Instruction {
        int align = 4;
    public:
        StoreInst(BasicBlock *parent, Value *val, Value *ptr) : Instruction(VK_StoreInst, parent) {
            Use *op0 = new Use(val, this);
            Use *op1 = new Use(ptr, this);
            operands.push_back(op0);
            operands.push_back(op1);
            val->insertBackToUses(operands[0]);
            ptr->insertBackToUses(operands[1]);
            if (isa<PointerType>(val->getType())) align = 8;
        }

        bool static classof(Value *v) { return v->getKind() == VK_StoreInst; }

        Value *getVal() { return operands[0]->value; }

        Value *getPtr() { return operands[1]->value; }

        void print() {
            cout << "  store " << operands[0]->value->getType()->toString() << operands[0]->value->toString()
                 << ", " << operands[1]->value->getType()->toString() << operands[1]->value->toString() << ", align "
                 << align << endl;
        }

        void accept(Visitor *V);
    };

    class LoadInst : public Instruction {
        Type *ty;
        RegisterVar *result;
        int align = 4;
    public:
        LoadInst(BasicBlock *parent, Type *ty, Value *ptr, RegisterVar *result) : Instruction(VK_LoadInst, parent),
                                                                                  ty(ty), result(result) {
            Use *op0 = new Use(ptr, this);
            operands.push_back(op0);
            ptr->insertBackToUses(operands[0]);
            result->setInst(this);
            if (isa<PointerType>(result->getType())) align = 8;

        }

        bool static classof(Value *v) { return v->getKind() == VK_LoadInst; }

        Type *getType() { return ty; }

        Value *getPtr() { return operands[0]->value; }

        Value *getResult() { return result; }

        void print() {
            cout << " " << result->toString() << " = load " << result->getType()->toString() << ", " <<
                 operands[0]->value->getType()->toString() << operands[0]->value->toString() << ", align "
                 << align << endl;
        }

        void accept(Visitor *V);


    };

    //phi指令
    class PhiInst : public Instruction {
        Type *ty;
        BaseReg *result;
        using phiValue = pair<Value *, BasicBlock *>;
        unsigned valueNum;
    public:
        PhiInst(BasicBlock *parent, Type *ty, BaseReg *result) : Instruction(VK_PhiInst, parent), ty(ty),
                                                                     result(result) {}

        bool static classof(Value *v) { return v->getKind() == VK_PhiInst; }

        PhiInst static *Get(Type *ty, string name, BasicBlock *insertFront) {
            auto rv = new RegisterVar(ty, name);
            auto pi = new PhiInst(insertFront, ty, rv);
            rv->setInst(pi);
            insertFront->insertFrontToChild(pi);
            return pi;
        }

        void print() {
            string printString;
            if (operands.empty()) printString = " [ ] ";
            for (int i = 0; i < operands.size(); i = i + 2) {
                printString += " [";
                printString += operands[i]->value->toString() + ", " + operands[i+1]->value->toString();
                printString += " ]";
                if (i != operands.size() - 2) printString += ",";
            }
            if(result->getType())
            cout << " " << result->toString() << " = phi " << result->getType()->toString() << printString << endl;
            else   cout << " " << result->toString() << " = phi "  << printString << endl;
        }

        void addIncoming(phiValue value) {
            Use *op0 = new Use(value.first, this);
            Use *op1 = new Use(value.second, this);
            operands.push_back(op0);
            operands.push_back(op1);
            value.first->insertBackToUses(op0);
            value.second->insertBackToUses(op1);
            ++valueNum;
        }

        void addIncoming(phiValue value, phiValue rest ...) {
            addIncoming(value);
            addIncoming(rest);
        }

        Value *getResult() {
            return result;
        }
        void setResult(BaseReg *v) {
            result = v;
        }
        void accept(Visitor *V);

    };

    //binary op
    //int加法
    class AddInst : public Instruction {
        RegisterVar *result;
    public:
        AddInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) : Instruction(VK_AddInst, parent),
                                                                               result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_AddInst; }

        void print() {
            cout << " " << result->toString() << " = add i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return result; }

        void accept(Visitor *V);
    };

    class SubInst : public Instruction {
        RegisterVar *result;
    public:
        SubInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) :
                Instruction(VK_SubInst, parent), result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_SubInst; }

        void print() {
            cout << " " << result->toString() << " = sub i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return result; }

        void accept(Visitor *V);
    };

    //int mul
    class MulInst : public Instruction {
        RegisterVar *result;
    public:
        MulInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) : Instruction(VK_MulInst, parent),
                                                                               result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_MulInst; }

        void print() {
            cout << " " << result->toString() << " = mul i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return result; }

        void accept(Visitor *V);

    };

    //int div
    class DivInst : public Instruction {
        RegisterVar *result;
    public:
        DivInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) : Instruction(VK_DivInst, parent),
                                                                               result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_DivInst; }

        void print() {
            cout << " " << result->toString() << " = sdiv i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return result; }

        void accept(Visitor *V);


    };

    class RemInst : public Instruction {
        RegisterVar *result;
    public:
        RemInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) : Instruction(VK_RemInst, parent),
                                                                               result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classop(Value *v) { return v->getKind() == VK_RemInst; }

        void print() {
            cout << " " << result->toString() << " = srem i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return result; }

        void accept(Visitor *V);


    };

    //Fadd
    class FAddInst : public Instruction {
        RegisterVar *result;
    public:
        FAddInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) : Instruction(VK_FAddInst, parent),
                                                                                result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FAddInst; }

        void print() {
            cout << " " << result->toString() << " = fadd float" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return result; }

        void accept(Visitor *V);
    };

    //fsub
    class FSubInst : public Instruction {
        RegisterVar *result;
    public:
        FSubInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) : Instruction(VK_FSubInst, parent),
                                                                                result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FSubInst; }

        void print() {
            cout << " " << result->toString() << " = fadd float" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return result; }

        void accept(Visitor *V);
    };

    //fmul
    class FMulInst : public Instruction {
        RegisterVar *result;
    public:
        FMulInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) : Instruction(VK_FMulInst, parent),
                                                                                result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FMulInst; }

        void print() {
            cout << " " << result->toString() << " = fmul float" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return result; }

        void accept(Visitor *V);
    };

    //FDiv
    class FDivInst : public Instruction {
        RegisterVar *result;
    public:
        FDivInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) : Instruction(VK_FDivInst, parent),
                                                                                result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FDivInst; }

        void print() {
            cout << " " << result->toString() << " = fdiv float" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return result; }

        void accept(Visitor *V);
    };

    //比较
    class ICmpEQ : public Instruction {
        RegisterVar *rv;
    public:
        ICmpEQ(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_ICmpEQ, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_ICmpEQ; }

        void print() {
            cout << " " << rv->toString() << " = icmp eq i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);

    };

    //IcmpNE
    class ICmpNE : public Instruction {
        RegisterVar *rv;
    public:
        ICmpNE(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_ICmpNE, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_ICmpNE; }

        void print() {
            cout << " " << rv->toString() << " = icmp ne i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);

    };

    //IcmpLT
    class ICmpLT : public Instruction {
        RegisterVar *rv;
    public:
        ICmpLT(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_ICmpLT, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_ICmpLT; }

        void print() {
            cout << " " << rv->toString() << " = icmp slt i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);

    };

    //IcmpLE
    class ICmpLE : public Instruction {
        RegisterVar *rv;
    public:
        ICmpLE(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_ICmpLE, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_ICmpLE; }

        void print() {
            cout << " " << rv->toString() << " = icmp sle i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);

    };

    //IcmpGT
    class ICmpGT : public Instruction {
        RegisterVar *rv;
    public:
        ICmpGT(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_ICmpGT, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_ICmpGT; }

        void print() {
            cout << " " << rv->toString() << " = icmp sgt i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);

    };

    //IcmpGE
    class ICmpGE : public Instruction {
        RegisterVar *rv;
    public:
        ICmpGE(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_ICmpGE, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_ICmpGE; }

        void print() {
            cout << " " << rv->toString() << " = icmp sge i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);

    };

    //FcmpEQ
    class FCmpEQ : public Instruction {
        RegisterVar *rv;
    public:
        FCmpEQ(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_FCmpEQ, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FCmpEQ; }

        void print() {
            cout << " " << rv->toString() << " = fcmp eq i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);


    };

    //FcmpNE
    class FCmpNE : public Instruction {
        RegisterVar *rv;
    public:
        FCmpNE(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_FCmpNE, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FCmpNE; }

        void print() {
            cout << " " << rv->toString() << " = fcmp ne i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);


    };

    //FcmpLT
    class FCmpLT : public Instruction {
        RegisterVar *rv;
    public:
        FCmpLT(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_FCmpLT, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FCmpLT; }

        void print() {
            cout << " " << rv->toString() << " = fcmp slt i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);


    };

    //FcmpLE
    class FCmpLE : public Instruction {
        RegisterVar *rv;
    public:
        FCmpLE(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_FCmpLE, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FCmpLE; }

        void print() {
            cout << " " << rv->toString() << " = fcmp sle i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);


    };

    //FcmpGT
    class FCmpGT : public Instruction {
        RegisterVar *rv;
    public:
        FCmpGT(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_FCmpGT, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(op0);
            R->insertBackToUses(op1);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FCmpGT; }

        void print() {
            cout << " " << rv->toString() << " = fcmp sge i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);


    };

    //FcmpGE
    class FCmpGE : public Instruction {
        RegisterVar *rv;
    public:
        FCmpGE(BasicBlock *parent, Value *L, Value *R, RegisterVar *rv) : Instruction(VK_FCmpGE, parent), rv(rv) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            rv->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FCmpGE; }

        void print() {
            cout << " " << rv->toString() << " = fcmp sle i32" <<
                 operands[0]->value->toString() << "," << operands[1]->value->toString()
                 << endl;
        }

        Value *getL() { return operands[0]->value; }

        Value *getR() { return operands[1]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);


    };


    //IToF
    class IToFInst : public Instruction {
        RegisterVar *rv;
        Type *destTy;
    public:
        IToFInst(BasicBlock *parent, Value *src, Type *destTy, RegisterVar *rv) :
                Instruction(VK_IToFInst, parent), destTy(destTy), rv(rv) {
            Use *op0 = new Use(src, this);
            operands.push_back(op0);
            rv->setInst(this);
            src->insertBackToUses(operands[0]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_IToFInst; }

        void print() {
            cout << " " << rv->toString() << " = sitofp " << operands[0]->value->getType()->toString()
                 << operands[0]->value->toString() << " to float" << endl;
        }

        Value *getSrc() { return operands[0]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);

    };

    //FToI
    class FToIInst : public Instruction {
        RegisterVar *rv;
        Type *destTy;
    public:
        FToIInst(BasicBlock *parent, Value *src, Type *destTy, RegisterVar *rv) : Instruction(VK_FToIInst, parent),
                                                                                  destTy(destTy), rv(rv) {
            Use *op0 = new Use(src, this);
            operands.push_back(op0);
            rv->setInst(this);
            src->insertBackToUses(operands[0]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FToIInst; }

        void print() {
            cout << " " << rv->toString() << " = fptosi " << operands[0]->value->getType()->toString()
                 << operands[0]->value->toString() << " to " << destTy->toString() << endl;
        }

        Value *getSrc() { return operands[0]->value; }

        Value *getResult() { return rv; }

        void accept(Visitor *V);

    };

    class FNegInst : public Instruction {
        RegisterVar *result;
    public:
        FNegInst(BasicBlock *parent, Value *v, RegisterVar *result) :
                Instruction(VK_FNegInst, parent), result(result) {
            Use *op0 = new Use(v, this);
            operands.push_back(op0);
            result->setInst(this);
            v->insertBackToUses(operands[0]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_FNegInst; }

        void print() {
            cout << " " << result->toString() << " = fneg float"
                 << operands[0]->value->toString() << endl;
        }

        Value *getResult() { return result; }

        void accept(Visitor *V);


    };

    class XorInst : public Instruction {
        RegisterVar *result;
    public:
        XorInst(BasicBlock *parent, Value *L, Value *R, RegisterVar *result) :
                Instruction(VK_XorInst, parent), result(result) {
            Use *op0 = new Use(L, this);
            Use *op1 = new Use(R, this);
            operands.push_back(op0);
            operands.push_back(op1);
            result->setInst(this);
            L->insertBackToUses(operands[0]);
            R->insertBackToUses(operands[1]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_XorInst; }

        void print() {
            cout << " " << result->toString() << " = xor i1"
                 << operands[0]->value->toString() << ","
                 << operands[1]->value->toString() << endl;
        }

        Value *getResult() { return result; }

        void accept(Visitor *V);
    };

    class ZExtInst : public Instruction {
        RegisterVar *result;
        Type *destTy;
    public:
        ZExtInst(BasicBlock *parent, Value *v, Type *DestTy, RegisterVar *result) :
                Instruction(VK_ZExtInst, parent), result(result), destTy(DestTy) {
            Use *op0 = new Use(v, this);
            operands.push_back(op0);
            result->setInst(this);
            v->insertBackToUses(operands[0]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_ZExtInst; }

        void print() {
            cout << " " << result->toString() << " = zext i1" <<
                 operands[0]->value->toString() << " to " << destTy->toString()
                 << endl;
        }

        Value *getResult() { return result; }
        Value *getSrc() {return operands[0]->value;}
        void accept(Visitor *V);
    };

    //块操作
    class BranchInst : public Instruction {
    public:
        BranchInst(BasicBlock *parent, BasicBlock *dest) : Instruction(VK_BrInst, parent) {
            Use *op0 = new Use(dest, this);
            operands.push_back(op0);
            dest->insertBackToUses(operands[0]);
        }

        Value *getDest() { return operands[0]->value; }

        void print() { cout << "  br label " << operands[0]->value->toString() << endl; }

        bool static classof(Value *v) { return v->getKind() == VK_BrInst; }

        void accept(Visitor *V);

    };

    class CondBranchInst : public Instruction {
    public:
        CondBranchInst(BasicBlock *parent, Value *cond, BasicBlock *trueBB, BasicBlock *falseBB) :
                Instruction(VK_CondBrInst, parent) {
            Use *op0 = new Use(cond, this);
            operands.push_back(op0);
            cond->insertBackToUses(operands[0]);

            Use *op1 = new Use(trueBB, this);
            operands.push_back(op1);
            trueBB->insertBackToUses(operands[1]);

            Use *op2 = new Use(falseBB, this);
            operands.push_back(op2);
            falseBB->insertBackToUses(operands[2]);

        }

        Value *getCond() { return operands[0]->value; }

        Value *getTrueBB() { return operands[1]->value; }

        Value *getFalseBB() { return operands[2]->value; }

        void print() {
            cout << "  br " << operands[0]->value->getType()->toString()
                 << operands[0]->value->toString() << ", label " << operands[1]->value->toString()
                 << ", label " << operands[2]->value->toString() << endl;
        }

        void accept(Visitor *V);

        bool static classof(Value *v) { return v->getKind() == VK_CondBrInst; }

    };

    //返回
    class RetInst : public Instruction {
        Value *ret;
    public:
        RetInst(BasicBlock *parent, Value *ret) : Instruction(VK_BrInst, parent), ret(ret) {
            if (!ret) return;
            Use *op0 = new Use(ret, this);
            operands.push_back(op0);
            ret->insertBackToUses(operands[0]);
        }

        bool static classof(Value *v) { return v->getKind() == VK_RetInst; }

        void print() {
            if (ret)
                cout << "  ret " << operands[0]->value->getType()->toString()
                     << operands[0]->value->toString() << endl;
            else
                cout << "  ret " << "void" << endl;
        }
        virtual void accept(Visitor *V);
        Value *getRet() {
            if(!ret) return nullptr;
            return operands[0]->value;
        }

    };

    class CallInst : public Instruction {
        Function *fn;
        BaseReg *ret;
    public:
        CallInst(BasicBlock *parent, Function *fn, vector<Value *> args, RegisterVar *ret) :
                Instruction(VK_CallInst, parent), fn(fn), ret(ret) {
            for (int i = 0; i < args.size(); ++i) {
                Value *v = args[i];
                Use *op0 = new Use(v, this);
                operands.push_back(op0);
                v->insertBackToUses(op0);
            }
            if (!ret) return;
            ret->setInst(this);
        }

        bool static classof(Value *v) { return v->getKind() == VK_CallInst; }

        void print() {
            if (ret) {
                cout << " " << ret->toString() << " =";
            } else cout << " ";
            cout << " call " << fn->getFuncType()->getRetType()->toString()
                 << " @" << fn->getName() << "(";
            for (int i = 0; i < operands.size(); ++i) {
                if(operands[i]->value->getType())cout << operands[i]->value->getType()->toString();
                 cout << operands[i]->value->toString();
                if (i != operands.size() - 1) cout << ", ";
            }
            cout << ")" << endl;
        }

        virtual void accept(Visitor *V);
        Function *getFunc() {return fn;}
        Value *getResult() { return ret; }
        void setResult(BaseReg *v) {
            ret = v;
        }



    };


    class Visitor {
    public:
        virtual bool visit(PhiInst *inst) { return false; }

        virtual bool visit(AllocateInst *inst) { return false; }

        virtual bool visit(LoadInst *inst) { return false; }

        virtual bool visit(StoreInst *inst) { return false; }

        virtual bool visit(GEPInst *inst) { return false; }

        virtual bool visit(AddInst *inst) { return false; }

        virtual bool visit(SubInst *inst) { return false; }

        virtual bool visit(MulInst *inst) { return false; }

        virtual bool visit(DivInst *inst) { return false; }

        virtual bool visit(RemInst *inst) { return false; }

        virtual bool visit(FAddInst *inst) { return false; }

        virtual bool visit(FSubInst *inst) { return false; }

        virtual bool visit(FMulInst *inst) { return false; }

        virtual bool visit(FDivInst *inst) { return false; }

        virtual bool visit(ICmpEQ *inst) { return false; }

        virtual bool visit(ICmpNE *inst) { return false; }

        virtual bool visit(ICmpGE *inst) { return false; }

        virtual bool visit(ICmpGT *inst) { return false; }

        virtual bool visit(ICmpLE *inst) { return false; }

        virtual bool visit(ICmpLT *inst) { return false; }

        virtual bool visit(FCmpEQ *inst) { return false; }

        virtual bool visit(FCmpNE *inst) { return false; }

        virtual bool visit(FCmpGE *inst) { return false; }

        virtual bool visit(FCmpGT *inst) { return false; }

        virtual bool visit(FCmpLE *inst) { return false; }

        virtual bool visit(FCmpLT *inst) { return false; }

        virtual bool visit(CondBranchInst *inst) { return false; }

        virtual bool visit(BranchInst *inst) { return false; }

        virtual bool visit(IToFInst *inst) { return false; }

        virtual bool visit(FToIInst *inst) { return false; }

        virtual bool visit(XorInst *inst) { return false; }

        virtual bool visit(FNegInst *inst) { return false; }

        virtual bool visit(ZExtInst *inst) { return false; }
        virtual bool visit(RetInst *inst) { return false; }
        virtual bool visit(CallInst *inst) { return false; }



    };

}
#endif //ANUC_CORE_H
