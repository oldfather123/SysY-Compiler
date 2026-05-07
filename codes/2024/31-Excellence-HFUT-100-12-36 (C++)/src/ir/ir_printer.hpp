#ifndef IR_PRINTER_HPP
#define IR_PRINTER_HPP

#include "ir.hpp"
// #include "SyntaxTree.hpp"

namespace ir {

class IrPrinter : public ir_visitor {
public:
   std::unordered_map<std::variant<vartype,binop,relop, unaryop>,std::string> mapping = {
        // {vartype::FLOAT, "float"},
        {vartype::FLOAT, "float"},
        {vartype::INT,  "i32"},
        {vartype::VOID , "void"},
        {vartype::BOOL, "i1"},
        // {vartype::FBOOL, "i1"},
        {vartype::BOOLADDR, "i1*"},
        // {vartype::FLOATADDR, "float*"},
        {vartype::FLOATADDR, "float*"},
        {vartype::INTADDR, "i32*"},
        {binop::plus, "add"},
        {binop::minus,  "sub"},
        {binop::multiply , "mul"},
        {binop::divide, "sdiv"},
        {binop::modulo, "srem"},
        {relop::greater, "sgt"},
        {relop::less,  "slt"},
        {relop::greater_equal , "sge"},
        {relop::less_equal, "sle"},
        {relop::equal, "eq"},
        {relop::non_equal, "ne"},
        {relop::op_and, "and"},
        {relop::op_or, "or"},
        {unaryop::minus, "mul"},
        {unaryop::plus, "mul"},
        {unaryop::op_not, "xor"}
    };
    std::unordered_map<std::variant<vartype,binop,relop, unaryop>,std::string> fmapping = {
        {binop::plus, "fadd"},
        {binop::minus,  "fsub"},
        {binop::multiply , "fmul"},
        {binop::divide, "fdiv"},
        {binop::modulo, "frem"},
        {relop::greater, "ogt"},
        {relop::less,  "olt"},
        {relop::greater_equal , "oge"},
        {relop::less_equal, "ole"},
        {relop::equal, "oeq"},
        {relop::non_equal, "one"},
        {relop::op_and, "and"},
        {relop::op_or, "or"},
        {unaryop::minus, "fmul"},
        {unaryop::plus, "fmul"},
        {unaryop::op_not, "xor"}
    };
   std::unordered_map<vartype, std::string> base_type = {
    //    {vartype::FLOATADDR, "float"},
       {vartype::FLOATADDR, "float"},
       {vartype::INTADDR, "i32"},
       {vartype::BOOLADDR, "i1"},
    //    {vartype::FBOOLADDR, "i1"}
   };
   std::ostream &out;

   IrPrinter(std::ostream &out = std::cout) : out(out){};
   virtual void visit(ir_reg &node) override final;
   virtual void visit(ir_constant &node) override final;
   virtual void visit(ir_basicblock &node) override final;
   virtual void visit(ir_module &node) override;
   virtual void visit(ir_userfunc &node) override final;
   virtual void visit(ir_libfunc &node) override final;
   virtual void visit(store &node) override final;
   virtual void visit(jump &node) override final;
   virtual void visit(br &node) override final;
   virtual void visit(ret &node) override final;
   virtual void visit(load &node) override final;
   virtual void visit(alloc &node) override final;
   virtual void visit(phi &node) override final;
   virtual void visit(unary_op_ins &node) override final;
   virtual void visit(binary_op_ins &node) override final;
   virtual void visit(cmp_ins &node) override final;
   virtual void visit(logic_ins &node) override final;
   virtual void visit(get_element_ptr &node) override final;
   virtual void visit(while_loop &node) override final;
   virtual void visit(break_or_continue &node) override final;
   virtual void visit(func_call &node) override final;
   virtual void visit(tail_call &node) override final;
   virtual void visit(global_def &node) override final;
   virtual void visit(trans &node) override final;
   virtual void visit(ir::memset &node) override final;
   std::string get_value(const ptr<ir::ir_value> &val);
   std::string get_reg_name(ptr<ir_reg> &node);
   void llvm(ptr<int> pointer, ptr_list<ir::ir_value> init_val, ptr_list<ast::expr_syntax> dimensions, string init_type, vartype type);
};

} // namespace ir



#endif