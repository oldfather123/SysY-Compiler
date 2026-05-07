//
// Created by 牛奕博 on 2023/6/11.
//
#ifndef ANUC_RVVALUE_H
#define ANUC_RVVALUE_H

#include <map>
#include <memory>
#include "string"
#include "type.h"

using namespace std;
namespace anuc {

    class RvRegister : public BaseReg {
        //分配权重，权重越大越先使用
        int weight = 0;
    public:
        enum RgKind {
            sp, zero, ra,
            s0, s1, s2, s3, s4, s5, s6, s7, s8, s9, s10, s11,
            a0, a1, a2, a3, a4, a5, a6, a7,
            t0, t1, t2, t3, t4, t5, t6,
            fa0, fa1, fa2, fa3, fa4, fa5, fa6, fa7
        };
    private:
        RgKind rgKind;
    public:
        explicit RvRegister(RgKind rgKind, int weight) : BaseReg(VK_RvRegister, nullptr),
        rgKind(rgKind), weight(weight) {

        }
        bool static classof(Value *v) { return v->getKind() == VK_RvRegister; }

        string toString() {
            string s;
            switch (rgKind) {
#define REG_TOSTRING_CASE(KIND) case RgKind::KIND: s = #KIND; break;
                REG_TOSTRING_CASE(sp)
                REG_TOSTRING_CASE(zero)
                REG_TOSTRING_CASE(ra)

                REG_TOSTRING_CASE(s0)
                REG_TOSTRING_CASE(s1)
                REG_TOSTRING_CASE(s2)
                REG_TOSTRING_CASE(s3)
                REG_TOSTRING_CASE(s4)
                REG_TOSTRING_CASE(s5)
                REG_TOSTRING_CASE(s6)
                REG_TOSTRING_CASE(s7)
                REG_TOSTRING_CASE(s8)
                REG_TOSTRING_CASE(s9)
                REG_TOSTRING_CASE(s10)
                REG_TOSTRING_CASE(s11)
                REG_TOSTRING_CASE(a0)
                REG_TOSTRING_CASE(a1)
                REG_TOSTRING_CASE(a2)
                REG_TOSTRING_CASE(a3)
                REG_TOSTRING_CASE(a4)
                REG_TOSTRING_CASE(a5)
                REG_TOSTRING_CASE(a6)
                REG_TOSTRING_CASE(a7)
                REG_TOSTRING_CASE(t0)
                REG_TOSTRING_CASE(t1)
                REG_TOSTRING_CASE(t2)
                REG_TOSTRING_CASE(t3)
                REG_TOSTRING_CASE(t4)
                REG_TOSTRING_CASE(t5)
                REG_TOSTRING_CASE(t6)
                REG_TOSTRING_CASE(fa0)
                REG_TOSTRING_CASE(fa1)
                REG_TOSTRING_CASE(fa2)
                REG_TOSTRING_CASE(fa3)
                REG_TOSTRING_CASE(fa4)
                REG_TOSTRING_CASE(fa5)
                REG_TOSTRING_CASE(fa6)
                REG_TOSTRING_CASE(fa7)

            }
            return ' ' + s;
        }
        int getWeight() { return weight;}

        static bool isSReg(RvRegister *reg) {
            return reg->rgKind >= s0 && reg->rgKind <= s11;
        }
        static bool isTReg(RvRegister *reg) {
            return reg->rgKind >= t0 && reg->rgKind <= t6;
        }
        static bool isAReg(RvRegister *reg) {
            return reg->rgKind >= a0 && reg->rgKind <= a7;
        }

        static bool isFReg(RvRegister *reg) {
            return reg->rgKind >= fa0 && reg->rgKind <= fa7;
        }
    };


    class RegTable {
        vector<RvRegister*> integerRegs{};
        unique_ptr<RvRegister> sp;
        unique_ptr<RvRegister> zero;
        unique_ptr<RvRegister> ra;

        unique_ptr<RvRegister> s0;
        unique_ptr<RvRegister> s1;
        unique_ptr<RvRegister> s2;
        unique_ptr<RvRegister> s3;
        unique_ptr<RvRegister> s4;
        unique_ptr<RvRegister> s5;
        unique_ptr<RvRegister> s6;
        unique_ptr<RvRegister> s7;
        unique_ptr<RvRegister> s8;
        unique_ptr<RvRegister> s9;
        unique_ptr<RvRegister> s10;
        unique_ptr<RvRegister> s11;
        unique_ptr<RvRegister> a0;
        unique_ptr<RvRegister> a1;
        unique_ptr<RvRegister> a2;
        unique_ptr<RvRegister> a3;
        unique_ptr<RvRegister> a4;
        unique_ptr<RvRegister> a5;
        unique_ptr<RvRegister> a6;
        unique_ptr<RvRegister> a7;
        unique_ptr<RvRegister> t0;
        unique_ptr<RvRegister> t1;
        unique_ptr<RvRegister> t2;
        unique_ptr<RvRegister> t3;
        unique_ptr<RvRegister> t4;
        unique_ptr<RvRegister> t5;
        unique_ptr<RvRegister> t6;
        unique_ptr<RvRegister> fa0;
        unique_ptr<RvRegister> fa1;
        unique_ptr<RvRegister> fa2;
        unique_ptr<RvRegister> fa3;
        unique_ptr<RvRegister> fa4;
        unique_ptr<RvRegister> fa5;
        unique_ptr<RvRegister> fa6;
        unique_ptr<RvRegister> fa7;

        map<Value *, RvRegister *> valueToReg;
    public:
        RegTable() {
            sp = make_unique<RvRegister>(RvRegister::sp, 0);
            zero = make_unique<RvRegister>(RvRegister::zero, 0);
            ra = make_unique<RvRegister>(RvRegister::ra, 0);
            s0 = make_unique<RvRegister>(RvRegister::s0, 0);
            s1 = make_unique<RvRegister>(RvRegister::s1, 0);

            s2 = make_unique<RvRegister>(RvRegister::s2, 1);
            s3 = make_unique<RvRegister>(RvRegister::s3, 1);
            s4 = make_unique<RvRegister>(RvRegister::s4, 1);
            s5 = make_unique<RvRegister>(RvRegister::s5, 1);
            s6 = make_unique<RvRegister>(RvRegister::s6, 1);
            s7 = make_unique<RvRegister>(RvRegister::s7, 1);
            s8 = make_unique<RvRegister>(RvRegister::s8, 1);
            s9 = make_unique<RvRegister>(RvRegister::s9, 1);
            s10 = make_unique<RvRegister>(RvRegister::s10, 1);
            s11 = make_unique<RvRegister>(RvRegister::s11, 1);
            a0 = make_unique<RvRegister>(RvRegister::a0, 2);
            a1 = make_unique<RvRegister>(RvRegister::a1, 2);
            a2 = make_unique<RvRegister>(RvRegister::a2, 2);
            a3 = make_unique<RvRegister>(RvRegister::a3, 2);
            a4 = make_unique<RvRegister>(RvRegister::a4, 2);
            a5 = make_unique<RvRegister>(RvRegister::a5, 2);
            a6 = make_unique<RvRegister>(RvRegister::a6, 2);
            a7 = make_unique<RvRegister>(RvRegister::a7, 2);
            t0 = make_unique<RvRegister>(RvRegister::t0, 3);
            t1 = make_unique<RvRegister>(RvRegister::t1, 3);
            t2 = make_unique<RvRegister>(RvRegister::t2, 3);
            t3 = make_unique<RvRegister>(RvRegister::t3, 3);
            t4 = make_unique<RvRegister>(RvRegister::t4, 3);
            t5 = make_unique<RvRegister>(RvRegister::t5, 3);
            t6 = make_unique<RvRegister>(RvRegister::t6, 3);
            fa0 = make_unique<RvRegister>(RvRegister::fa0, 2);
            fa1 = make_unique<RvRegister>(RvRegister::fa1, 2);
            fa2 = make_unique<RvRegister>(RvRegister::fa2, 2);
            fa3 = make_unique<RvRegister>(RvRegister::fa3, 2);
            fa4 = make_unique<RvRegister>(RvRegister::fa4, 2);
            fa5 = make_unique<RvRegister>(RvRegister::fa5, 2);
            fa6 = make_unique<RvRegister>(RvRegister::fa6, 2);
            fa7 = make_unique<RvRegister>(RvRegister::fa7, 2);

        }

        RvRegister *find(Value *v) {
            auto it = valueToReg.find(v);
            if (it == valueToReg.end()) return nullptr;
            return it->second;
        }

        RvRegister *getReg(RvRegister::RgKind kind) {
            switch (kind) {
#define REG_GET_CASE(KIND) case RvRegister::KIND : return KIND.get();
                REG_GET_CASE(sp)
                REG_GET_CASE(zero)
                REG_GET_CASE(ra)

                REG_GET_CASE(s0)
                REG_GET_CASE(s1)
                REG_GET_CASE(s2)
                REG_GET_CASE(s3)
                REG_GET_CASE(s4)
                REG_GET_CASE(s5)
                REG_GET_CASE(s6)
                REG_GET_CASE(s7)
                REG_GET_CASE(s8)
                REG_GET_CASE(s9)
                REG_GET_CASE(s10)
                REG_GET_CASE(s11)
                REG_GET_CASE(a0)
                REG_GET_CASE(a1)
                REG_GET_CASE(a2)
                REG_GET_CASE(a3)
                REG_GET_CASE(a4)
                REG_GET_CASE(a5)
                REG_GET_CASE(a6)
                REG_GET_CASE(a7)
                REG_GET_CASE(t0)
                REG_GET_CASE(t1)
                REG_GET_CASE(t2)
                REG_GET_CASE(t3)
                REG_GET_CASE(t4)
                REG_GET_CASE(t5)
                REG_GET_CASE(t6)
                REG_GET_CASE(fa0)
                REG_GET_CASE(fa1)
                REG_GET_CASE(fa2)
                REG_GET_CASE(fa3)
                REG_GET_CASE(fa4)
                REG_GET_CASE(fa5)
                REG_GET_CASE(fa6)
                REG_GET_CASE(fa7)
                default:
                    return nullptr;
            }
        }
        //返回所有整型变量
        vector<RvRegister *>& getAllIntegerRegs() {
            if(integerRegs.empty()) {
                integerRegs.push_back(this->getReg(RvRegister::a0));
                integerRegs.push_back(this->getReg(RvRegister::a1));
                integerRegs.push_back(this->getReg(RvRegister::a2));
                integerRegs.push_back(this->getReg(RvRegister::a3));
                integerRegs.push_back(this->getReg(RvRegister::a4));
                integerRegs.push_back(this->getReg(RvRegister::a5));
                integerRegs.push_back(this->getReg(RvRegister::a6));
                integerRegs.push_back(this->getReg(RvRegister::a7));
                //integerRegs.push_back(this->getReg(RvRegister::s0));
                //integerRegs.push_back(this->getReg(RvRegister::s1));
                integerRegs.push_back(this->getReg(RvRegister::s2));
                integerRegs.push_back(this->getReg(RvRegister::s3));
                integerRegs.push_back(this->getReg(RvRegister::s4));
                integerRegs.push_back(this->getReg(RvRegister::s5));
                integerRegs.push_back(this->getReg(RvRegister::s6));
                integerRegs.push_back(this->getReg(RvRegister::s7));
                integerRegs.push_back(this->getReg(RvRegister::s8));
                integerRegs.push_back(this->getReg(RvRegister::s9));
                integerRegs.push_back(this->getReg(RvRegister::s10));
                integerRegs.push_back(this->getReg(RvRegister::t0));
                integerRegs.push_back(this->getReg(RvRegister::t1));
                integerRegs.push_back(this->getReg(RvRegister::t2));
                integerRegs.push_back(this->getReg(RvRegister::t3));
                integerRegs.push_back(this->getReg(RvRegister::t4));
                integerRegs.push_back(this->getReg(RvRegister::t5));
                integerRegs.push_back(this->getReg(RvRegister::t6));
            }
            return integerRegs;
        }
    };

}

#endif //ANUC_RVVALUE_H
