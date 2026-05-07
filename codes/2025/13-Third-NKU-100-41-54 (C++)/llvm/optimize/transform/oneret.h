#ifndef ONERET_H
#define ONERET_H
#include "../../include/ir.h"
#include "../pass.h"

class OneRetPass : public IRPass { 
private:
	std::unordered_map<FuncDefInstruction,std::vector<std::pair<Operand, Operand>>> ret_phi_list;//block_id,operand

public:
    OneRetPass(LLVMIR *IR) : IRPass(IR) {}
    void Execute();
};

#endif