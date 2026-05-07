#ifndef __TAIL_RECUR_ELI_H__
#define __TAIL_RECUR_ELI_H__

#include "opt.hpp"

class TailRecurEliminate: public Optimization {
private:
    void run(Function* func);
    void recover_ret(Function* func);
public:
	explicit TailRecurEliminate(Module* m): Optimization(m) {}
	void execute() override;
};

#endif // __SIMPLIFY_CFG_H__