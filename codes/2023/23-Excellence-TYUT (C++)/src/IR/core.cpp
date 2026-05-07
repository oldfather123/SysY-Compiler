//
// Created by 牛奕博 on 2023/4/17.
//
#include "core.h"

namespace anuc {
    void Module::insertBackToChild(Function *f) {
        childlist.insert_back(f);
    }

    void Module::insertFrontToChild(Function *f) {
        childlist.insert_front(f);
    }

    void Module::insertBackToChild(GlobalVar *v) {
        globalVarList.insert_back(v);
    }

    bool Module::insertFunc(string name, Function *f) {
        return funcLookUp.insert({name, f}).second;
    }

    bool Module::insertGlobal(string name, GlobalVar *g) {
        return globalLookUp.insert({name, g}).second;
    }

    map<Value *, bool>::iterator Module::insertIntoPool(Value *v, Value *rest...) {
        auto i = insertIntoPool(v);
        insertIntoPool(rest);
        return i;
    }

    map<Value *, bool>::iterator Module::insertIntoPool(Value *v) {
        auto i = valuePool.find(v);
        if (i == valuePool.end()) i = valuePool.insert({v, true}).first;
        return i;
    }

    map<Value *, bool>::iterator Module::lookUpValuePool(Value *v) {
        return valuePool.find(v);
    }

    bool Module::eraseFromValuePool(Value *v) {
        auto i = valuePool.find(v);
        if (i == valuePool.end()) return false;
        i->second = false;
        return true;
    }

    int Module::memoryClean() {
        unsigned num{0};
        for (auto i = valuePool.begin(); i != valuePool.end();) {
            if (!(*i).second) {
                delete (*i).first;
                valuePool.erase(i++);
                ++num;
                continue;
            }
            ++i;
        }
        return num;
    }

    void Module::print(string fileName) {
        freopen(fileName.c_str(), "w", stdout);
        cout << "; ModuleID = 'nyb'" << endl;
        for (auto i = globalVarList.begin(); i != globalVarList.end(); ++i) (*i).print();
        for (auto i = childlist.begin(); i != childlist.end(); ++i) (*i).print();
        cout << "declare i32 @getint(...) #1" << endl;
        cout <<"declare i32 @getch(...) #1" << endl;
        cout << "declare i32 @getarray(i32* noundef) #1" << endl;
        cout << "declare float @getfloat(...) #1" << endl;
        cout << "declare void @putint(i32 noundef) #1" << endl;
        cout << "declare void @putch(i32 noundef) #1" << endl;
        cout << "declare void @putfloat(float noundef) #1" << endl;
        cout << "declare void @putarray(i32 noundef, i32* noundef) #1" << endl;
        cout << "declare void @putfarray(i32 noundef, float* noundef) #1" << endl;
        fclose(stdout);
    }


    Module::~Module() {
        for (auto i = valuePool.begin(); i != valuePool.end(); ++i) delete (*i).first;
        for (auto i = funcLookUp.begin(); i != funcLookUp.end(); ++i) delete (*i).second;
        for (auto i = typePool.begin(); i != typePool.end(); ++i) delete *i;
    }

    void Function::insertBackToChild(BasicBlock *b) {
        childlist.insert_back(b);
    }

    void Function::eraseFromList(anuc::BasicBlock *bb) {
        childlist.erase_from_list(bb);
    }

    void Function::print() {
        cout << "define " << type->getRetType()->toString() << " @" << name
             << "(";
        for (int i = 0; i < argvs.size(); ++i) {
            cout << argvs[i].second->toString() << " %" << argvs[i].first;
            if (i != argvs.size() - 1) cout << ", ";
        }
        cout << ") " << "{" << endl;
        for (auto i = childlist.begin(); i != childlist.end(); ++i) (*i).print();
        cout << "}" << endl;
    }

    void BasicBlock::eraseFromList(Instruction *inst) {
        childlist.erase_from_list(inst);
    }

    void BasicBlock::eraseFromParent() {
        //将其中指令删除
        for(auto i = this->getBegin(); i != this->getEnd();) {
            Instruction *s = &*i;
            ++i;
            s->eraseFromParent();
        }

        //从前驱后继中删除
        for(auto &b: this->getPred()) {
            auto &predsSucc = b->getSucc();
            for(int idx = 0; idx < predsSucc.size(); ++idx) {
                if(predsSucc[idx] != this) continue;
                predsSucc[idx] = predsSucc.back();
                predsSucc.pop_back();
                idx--;
            }
        }
        //从后继前驱中删除
        for(auto &b: this->getSucc()) {
            auto &succsPred = b->getPred();
            for(int idx = 0; idx < succsPred.size(); ++idx) {
                if(succsPred[idx] != this) continue;
                succsPred[idx] = succsPred.back();
                succsPred.pop_back();
                idx--;
            }
        }

        //从链表上删除
        Function *fn = this->getParent();
        auto m = fn->getParent();
        auto i = m->lookUpValuePool(this);
        i->second = false;
        fn->eraseFromList(this);
    }

    void BasicBlock::insertBackToChild(Instruction *i) {
        childlist.insert_back(i);
    }

    void BasicBlock::insertFrontToChild(Instruction *i) {
        childlist.insert_front(i);
    }

    void BasicBlock::insertIntoChild(Instruction *inst, Instruction *insetPoint) {
        childlist.insert_into(inst, insetPoint);
    }

    void BasicBlock::insertIntoBackChild(Instruction *insertPoint, Instruction *inst) {
        childlist.insert_into_back(insertPoint, inst);
    }
    void BasicBlock::insertIntoBackChild(anuc::Instruction *insertPoint, vector<anuc::Instruction *> inst) {
        Instruction *p = insertPoint;
        for(auto &s: inst) {
            childlist.insert_into_back(p, s);
            p = s;
        }
    }

    void BasicBlock::print() {
        cout << name << ":" << endl;
        for (auto i = childlist.begin(); i != childlist.end(); ++i) (*i).print();
    }

    void PhiInst::accept(Visitor *V) { V->visit(this);}
    void GEPInst::accept(Visitor *V) { V->visit(this);}
    void AllocateInst::accept(Visitor *V) { V->visit(this);}
    void LoadInst::accept(Visitor *V) { V->visit(this);}
    void StoreInst::accept(Visitor *V) { V->visit(this);}
    void AddInst::accept(Visitor *V) { V->visit(this);}
    void SubInst::accept(Visitor *V) { V->visit(this);}
    void MulInst::accept(Visitor *V) { V->visit(this);}
    void DivInst::accept(Visitor *V) { V->visit(this);}
    void RemInst::accept(Visitor *V) { V->visit(this);}
    void FAddInst::accept(Visitor *V) { V->visit(this);}
    void FSubInst::accept(Visitor *V) { V->visit(this);}
    void FMulInst::accept(Visitor *V) { V->visit(this);}
    void FDivInst::accept(Visitor *V) { V->visit(this);}
    void ICmpEQ::accept(Visitor *V) { V->visit(this);}
    void ICmpNE::accept(Visitor *V) { V->visit(this);}
    void ICmpLT::accept(Visitor *V) { V->visit(this);}
    void ICmpLE::accept(Visitor *V) { V->visit(this);}
    void ICmpGT::accept(Visitor *V) { V->visit(this);}
    void ICmpGE::accept(Visitor *V) { V->visit(this);}
    void FCmpEQ::accept(Visitor *V) { V->visit(this);}
    void FCmpNE::accept(Visitor *V) { V->visit(this);}
    void FCmpLT::accept(Visitor *V) { V->visit(this);}
    void FCmpLE::accept(Visitor *V) { V->visit(this);}
    void FCmpGT::accept(Visitor *V) { V->visit(this);}
    void FCmpGE::accept(Visitor *V) { V->visit(this);}

    void CondBranchInst::accept(Visitor *V) { V->visit(this);}
    void BranchInst::accept(Visitor *V) { V->visit(this);}
    void IToFInst::accept(Visitor *V) { V->visit(this);}
    void FToIInst::accept(Visitor *V) { V->visit(this);}
    void XorInst::accept(Visitor *V) { V->visit(this);}
    void FNegInst::accept(Visitor *V) { V->visit(this);}
    void ZExtInst::accept(Visitor *V) { V->visit(this);}
    void RetInst::accept(Visitor *V) { V->visit(this);}
    void CallInst::accept(Visitor *V) { V->visit(this);}



}