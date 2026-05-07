#pragma once

class Module;

//之前调用passmanager的api一直出错，一气之下直接写了个类似的，结果只是单纯写错了。看到这个直接不用看了删了都行。

class Pass {
public:
    explicit Pass(Module *m) : m_(m) {}
    virtual ~Pass() = default;
    
    
    virtual void run() = 0;
    
protected:
    Module *m_;  
};