#include "Register.hpp"
#include <string>

// std::string Reg::print() const {
//     if (id == 0) {
//         return "zero";
//     }
//     if (id == 1) {
//         return "ra";
//     }
//     if (id == 2) {
//         return "tp";
//     }
//     if (id == 3) {
//         return "sp";
//     }
//     if (4 <= id and id <= 11) {
//         return "a" + std::to_string(id - 4);
//     }
//     if (12 <= id and id <= 20) {
//         return "t" + std::to_string(id - 12);
//     }
//     if (id == 22) {
//         return "fp";
//     }
//     assert(false);
// }
std::string Reg::print() const {
    switch (id) {
        case 0: return "zero";
        case 1: return "ra";
        case 2: return "sp";
        case 3: return "gp";
        case 4: return "tp";
        case 5: return "t0";
        case 6: return "t1";
        case 7: return "t2";
        case 8: return "fp";  // s0 == fp
        case 9: return "s1";
        case 10: return "a0";
        case 11: return "a1";
        case 12: return "a2";
        case 13: return "a3";
        case 14: return "a4";
        case 15: return "a5";
        case 16: return "a6";
        case 17: return "a7";
        case 18: return "s2";
        case 19: return "s3";
        case 20: return "s4";
        case 21: return "s5";
        case 22: return "s6";
        case 23: return "s7";
        case 24: return "s8";
        case 25: return "s9";
        case 26: return "s10";
        case 27: return "s11";
        case 28: return "t3";
        case 29: return "t4";
        case 30: return "t5";
        case 31: return "t6";
        default:
            assert(false && "Invalid register ID");
            return "";
    }
}

// std::string FReg::print() const {

//     if (0 <= id and id <= 7) {
//         return "fa" + std::to_string(id);
//     }
//     if (8 <= id and id <= 23) {
//         return "ft" + std::to_string(id - 8);
//     }
//     if (24 <= id and id <= 31) {
//         return "fs" + std::to_string(id - 24);
//     }
//     assert(false);
// }
std::string FReg::print() const {
    switch (id) {
        case 0: return "ft0";
        case 1: return "ft1";
        case 2: return "ft2";
        case 3: return "ft3";
        case 4: return "ft4";
        case 5: return "ft5";
        case 6: return "ft6";
        case 7: return "ft7";
        case 8: return "fs0";
        case 9: return "fs1";
        case 10: return "fa0";
        case 11: return "fa1";
        case 12: return "fa2";
        case 13: return "fa3";
        case 14: return "fa4";
        case 15: return "fa5";
        case 16: return "fa6";
        case 17: return "fa7";
        case 18: return "fs2";
        case 19: return "fs3";
        case 20: return "fs4";
        case 21: return "fs5";
        case 22: return "fs6";
        case 23: return "fs7";
        case 24: return "fs8";
        case 25: return "fs9";
        case 26: return "fs10";
        case 27: return "fs11";
        case 28: return "ft8";
        case 29: return "ft9";
        case 30: return "ft10";
        case 31: return "ft11";
        default:
            assert(false && "Invalid float register ID");
            return "";
    }
}
