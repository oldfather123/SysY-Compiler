#pragma once

#include <string>
#include <list>

#include "Module.hh"
#include "logging.hh"

class Pass {
public:
    explicit Pass(Module *m, bool print_ir) : m_(m), print_ir(print_ir) {}
    virtual void execute() = 0;
    virtual const std::string get_name() const = 0;
public:
    bool print_ir;
protected:
    Module *m_;
};

class PassMgr {
public:
    explicit PassMgr(Module *m) :m_(m) { pass_list_ = std::list<Pass*> (); }
    
    template <typename PassTy> 
    void add_pass(bool print_ir=false) { pass_list_.push_back(new PassTy(m_, print_ir)); }

    void execute(bool print_ir = false);
private:
    Module* m_;
    std::list<Pass*> pass_list_;
};
