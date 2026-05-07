#pragma once
#include "ir.h"
#include <iostream>
#include <ostream>

class ir_serializer : public Medium::ir_visitor//打印IR的类
{
    public:
    
    ir_serializer(ostream &_out) : out(_out) {}
    std::ostream& out;
   //在这些函数中打印东西就行了
    virtual void visit(Reg &node) override{
      out<<"R"<<(node.is_int?"i":"f")<<"["<<node.id<<"]";
    }
    virtual void visit(Variable &node)override{
      out<<node.name<<"("<<(node.global ? "gp" : "sp" )<<"+"<<node.offset<<","<<(node.size==INT_SIZE ? "var" : "addr" )<<","<<(node.is_int ? "int" : "float" )<<")";//变量的位置和类型
    }
    virtual void visit(MediumScope &node)override{
      out << "MemScope(" << node.name << "){\n";
      for (auto &x : node.objects) 
      {
        out << "\t";
        this->visit(*(x.get()));
        out<<"\n";
      }
      out  << "}\n";
    }
    virtual void visit(IrBlock &node)override{
      out << "BB(" << node.name << "){\n";
      for (auto &x : node.instrs) {
        out << "\t";
        x->accept(*this);
        out << "\n";
      }
      out << "}\n";
    }
    virtual void visit(IrFunction &node)override{
      
    }
    virtual void visit(LibFunction &node)override{
      out << "libFunc: " << node.name << "\n";
    }
    virtual void visit(UserFunction &node)override{
      out << "userFunc: " << node.name << "\n";
      out << "entry: " << node.entry->name << "\n";
      for (auto &bb : node.bbs)
      {
        bb->accept(*this);
      }
      out<< "\n";
    }
    virtual void visit(CompileUnit &node)override{
      node.scope.accept(*this);//全局变量
      // out<<"libfuns:"<<"\n";
      for(auto &f:node.lib_funcs)//函数
      {
        f.second->accept(*this);
      }
      // out<<"userfuns:"<<"\n";
      for (auto &f : node.funcs)
      {
        f.second->accept(*this);
      }
      out<<"\n";

    }
    virtual void visit(UnaryOp &node)override{
      out<<node.get_name();
    }
    virtual void visit(BinaryOp &node)override{
      out<<node.get_name();
    }
    virtual void visit(ArrayIndex &node)override{
      node.d1.accept(*this);
      out<<"=";
      node.s1.accept(*this);//数组基地址
      out<<"+";
      node.s2.accept(*this);//偏移地址，就是元素坐标乘以元素的大小
      out<<"*"<<node.size;
    }
     virtual void visit(LoadAddr &node)override{
      node.d1.accept(*this);
      out<<"=&";
      node.offset->accept(*this);
     }
    virtual void visit(LoadConst &node)override{
      node.d1.accept(*this);
      out<<"="<<node.value;
    }
    virtual void visit(LoadArg &node)override{
       node.d1.accept(*this);
       out<<"= arg("<<node.id<<")";
    }
    virtual void visit(UnaryOpInstr &node)override{
      node.d1.accept(*this);
      out<<"=";
      out<<" ";
      node.op.accept(*this);
      node.s1.accept(*this);
    }
    virtual void visit(BinaryOpInstr &node)override{
       node.d1.accept(*this);
      out<<"=";
      out<<" ";
      node.s1.accept(*this);
       out<<" ";
      node.op.accept(*this);
       out<<" ";
       node.s2.accept(*this);
      
    }
    virtual void visit(LoadInstr &node)override{
      node.d1.accept(*this);
      out<<"=";
      out<<" M[";
      node.addr.accept(*this);
      out<<"]";
    }
    virtual void visit(StoreInstr &node)override{
     
      out<<" M[";
      node.addr.accept(*this);
      out<<"]";
      out<<"=";
      node.s1.accept(*this);
      
    }
    virtual void visit(JumpInstr &node)override{
      out<<"goto"<<node.target->name;
    }
    virtual void visit(BranchInstr &node)override{
      node.cond.accept(*this);
      out<<"?"<<" goto "<<node.target1->name<<":"<<" goto "<<node.target0->name;
    }
    virtual void visit(ReturnInstr &node)override{
      out<<"return";
      node.s1.accept(*this);
    }
    virtual void visit(CallInstr &node)override{
      node.d1.accept(*this);
      out<<"="<<node.f->name<<"(";
      for (auto &[s,flag] : node.args) {
      s.accept(*this);
      out<<",";
      }
      out<<")";
    }
    virtual void visit(LocalVarDef &node)override{
      out<<"define"<<node.data->name;
    }
    virtual void visit(PhiInstr &node)override{
      node.d1.accept(*this);
      out<< " = phi";
      char c = '(';
      for (auto s : node.uses) {
        out << c << " " ;
         s.first.accept(*this);
         out<< ":"<<s.second->name;
        c = ',';
      }
      if (c == '(')out << c;
      out << " )";
    }
    virtual void visit(MemDef &node)override{
      node.d1.accept(*this);
      out<<"="<<"memdef"<<node.data->name;
    }
    virtual void visit(MemUse &node)override{
      node.s1.accept(*this);
      out<<"used";
    }
    virtual void visit(Convert &node)override{
      if (node.src.is_int == true && node.d1.is_int == false)
      {
        out<<"convert ";
        out<<" int32 ";
        node.src.accept(*this);
        out<<" to ";
        out<<" float32 ";
        node.d1.accept(*this);
      }
        
      else if (node.src.is_int ==false  && node.d1.is_int == true)
      {
         out<<"convert ";
        out<<" float32 ";
        node.src.accept(*this);
        out<<" to ";
        out<<" int32 ";
        node.d1.accept(*this);
      }
      else
       assert(false);
    }
    virtual void visit(MemEffect &node)override{
      node.d1.accept(*this);
      out<<"=";
      node.s1.accept(*this);
      out<<"update";
    }
    virtual void visit(MemRead &node)override{
      node.d1.accept(*this);
      out<<"=";
      node.mem.accept(*this);
      out<<"at";
      node.addr.accept(*this);
    }
    virtual void visit(MemWrite &node)override{
       node.d1.accept(*this);
      out<<"=";
      node.mem.accept(*this);
      out<<"at";
      node.addr.accept(*this);
      out<<"write";
      node.s1.accept(*this);
    }


};