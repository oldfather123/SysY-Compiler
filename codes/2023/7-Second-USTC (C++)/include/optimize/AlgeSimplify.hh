#pragma once

#include "DeadCodeEliWithBr.hh"
#include "DeadCodeEli.hh"
#include "DominateTree.hh"
#include "LocalComSubExprEli.hh"
#include "Module.hh"
#include "Pass.hh"
#include "Constant.hh"
#include "logging.hh"
#include "utils.hh"
#include "BasicBlock.hh"
#include "IRBuilder.hh"
#include <variant>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>

template <typename T>
using Ptr = std::shared_ptr<T>;
using InstIter = std::__cxx11::list<Instruction *>::iterator;

class AlgeSimplify : public Pass
{
private:
    Module *module_;
    const std::string name = "AlgeSimplify";

public:
    explicit AlgeSimplify(Module* module, bool print_ir=false): Pass(module, print_ir){module_ = module;}
    ~AlgeSimplify(){};
    void execute() final;
    const std::string get_name() const override {return name;}
    Module* get_module() {return module_;}
    
    struct TermInfo
    {
    public:
        int num;
        bool exp;
    };

    struct Term
    {
    public:
        Value* value;
        bool exp;
        Term(Value* value_, bool exp_) : value(value_), exp(exp_) {}
    };

    struct ExprNode
    {
    public:
        Module *module_;
        InstIter inst_iter;
        enum class AlgeOp { ADD, SUB, MUL, DIV, LEAF } op;
        enum class ExprType { INT, FLOAT } type;
        static std::string get_op_name(AlgeOp op)
        {
            switch(op) {
                case AlgeOp::ADD: return "ADD"; 
                case AlgeOp::SUB: return "SUB"; 
                case AlgeOp::MUL: return "MUL"; 
                case AlgeOp::DIV: return "DIV"; 
                case AlgeOp::LEAF: return "LEAF"; 
                default:
                    return "";
            }
        }

        bool isConst = false;
        bool needReplace = false;
        bool dealed = false;
        BasicBlock* parent;

        std::variant<int, float> constVal = 0;

        Value* val;
        Value* newVal;
        ExprNode* lChild;
        ExprNode* rChild;

        ExprNode(Value* value,
                 ExprNode* left,
                 ExprNode* right,
                 AlgeOp op_,
                 Module* module)
                 : lChild(left), rChild(right), op(op_), val(value), newVal(nullptr), module_(module)
        {
            if(val->get_type()->is_integer_type())
            {
                type = ExprType::INT;
            }
            else
            {
                type = ExprType::FLOAT;
            }
            set_constant();
        }

        ExprNode(Instruction* value,
                 ExprNode* left,
                 ExprNode* right,
                 Module* module)
                 : lChild(left), rChild(right), val(value), newVal(nullptr), module_(module)
        {
           if(val->get_type()->is_integer_type())
           {
                type = ExprType::INT;
           }
           else
           {
                type = ExprType::FLOAT;
           }
           set_constant();
           set_op(value->get_instr_type());
        }

        void set_constant()
        {
            if(val == nullptr)
            {
                isConst = false;
                return;
            }
            isConst = is_const(val);
            if(isConst)
            {
                if(val->get_type()->is_integer_type())
                {
                    set_const_val(dynamic_cast<ConstantInt*>(val)->get_value());
                }
                else
                {
                    set_const_val(dynamic_cast<ConstantFP*>(val)->get_value());
                }
            }
        }
        bool is_const(Value * v)
        {
            if(dynamic_cast<Constant *>(v))
                return true;
            else
                return false;
        }
        int get_const_int_val() { return std::get<int>(constVal); }
        float get_const_float_val() { return std::get<float>(constVal); }
        void set_const_val(int val) { constVal = val; }
        void set_const_val(float val) { constVal = val; }
        void set_op(Instruction::OpID op_)
        {
            switch(op_)
            {
                case Instruction::OpID::add : 
                case Instruction::OpID::fadd : 
                    op = AlgeOp::ADD;
                    break;
                case Instruction::OpID::sub : 
                case Instruction::OpID::fsub : 
                    op = AlgeOp::SUB;
                    break;
                case Instruction::OpID::mul : 
                case Instruction::OpID::fmul : 
                    op = AlgeOp::MUL;
                    break;
                case Instruction::OpID::sdiv : 
                case Instruction::OpID::fdiv : 
                    op = AlgeOp::DIV;
                    break;
            }
        }

        std::unordered_map<Term*, int > termList;//记录节点每个term及其项数
        // std::unordered_map<Value*, bool> termExp;//记录每个term的指数正负（即是乘项数还是除以项数）
        
        void add_term(Value* value, int n, bool exp)
        {
            // term为常量
            if(type == ExprType::INT)
            {
                auto v = Utils::get_const_int_val(value);
                if(v.has_value())
                {
                    if (get_const_int_val() != 0)
                        needReplace = true;
                    constVal = get_const_int_val() + v.value() * n ;
                    return;
                }
            }
            if(type == ExprType::FLOAT)
            {
                auto v = Utils::get_const_float_val(value);
                if(v.has_value())
                {
                    if (get_const_float_val() != 0)
                        needReplace = true;
                    constVal = get_const_float_val() + v.value() * n ;
                    return;
                }
            }

            // term为变量
            auto term = new Term(value, exp);
            Term* ptr = nullptr;
            for(auto iter = termList.begin(); iter != termList.end(); iter++)
            {
                if((*iter).first->exp == exp && (*iter).first->value == value)
                {
                    ptr = (*iter).first;
                    break;
                }
            }
            if(ptr != nullptr)
            {
                termList[ptr] += n;
                needReplace = true;
            }
            else
            {
                termList.insert({term, n});
                // std::cout<<"term: "<<term->value->print()<<" | "<<"Exp: "<<term->exp<<" | termNum: "<<n<<std::endl;
            }
        }
        void intake_node(ExprNode* node, int n, bool exp=true)
        {
            if(node->op == AlgeOp::LEAF)
            {
                add_term(node->val, n, exp);
            }
            else
            {
                //测试是否存在非整除情况
                for(auto term : node->termList)
                {
                    int divisor = term.second;
                    if(term.first->exp == false && (n % divisor != 0 && divisor % n != 0))
                    {
                        add_term(node->val, n, true);
                        return;
                    }
                }

                for(auto term : node->termList)
                {
                    int divisor = term.second;
                    if(divisor == 0)
                    {//若存在某项的项数为0,则跳过
                        continue;
                    }
                    int r = n % divisor;
                    if(term.first->exp == true)
                        add_term(term.first->value, term.second * n, true);
                    else if(n % divisor == 0)
                    {
                        if(n == divisor)
                        {
                            add_term(term.first->value, 1, true);
                        }
                        else if (n > divisor)
                        {
                            add_term(term.first->value, n / divisor, true);
                            std::cout<<"++++++++++++++++++++= n > divisor++++++++++++++++++++++++++"<<std::endl;
                            std::cout << term.first->value->print() <<std::endl;
                            std::cout <<n<<std::endl;
                            std::cout <<divisor<<std::endl;
                            std::cout <<n / divisor<<std::endl;
                        }
                    }
                    else if(divisor % n == 0)
                    {
                        add_term(term.first->value, divisor / n, false);
                    }
                    else
                    {//此处为错误情况
                        std::cout<<"Error"<<std::endl;
                        
                        // IRBuilder builder(parent, module_);
                        // auto div = builder.create_isdiv(term.first->value, ConstantInt::get(divisor, module_));
                        // // parent->get_instructions().pop_back();
                        // parent->add_instruction(inst_iter, div);
                        // add_term(div, n, true);
                    }
                }
                if(node->get_const_int_val() != 0)
                {
                    set_const_val(get_const_int_val() + node->get_const_int_val() * n);
                    needReplace = true;
                }
                // std::cout<<"=================================="<<std::endl;
                // std::cout<<termList.size()<<std::endl;
                // std::cout<<needReplace<<std::endl;
                // for(auto term : node->termList)
                // {

                //     std::cout<< term.first->value->print() << "|" << term.second << "|" << term.first->exp<<std::endl;
                // }
            }
        }
    };

    std::map<Value*, ExprNode* > value2Node;
    std::unordered_set<ExprNode*> rootSet;
    std::set<ExprNode*> noUseNodes;

    ExprNode* get_node(Value* val);
    void reassociation(ExprNode* node);
    void build_expr_tree(Instruction* inst, BasicBlock* bb, InstIter& inst_iter);
    void replace_instruction(Function* func);
    void add_node(ExprNode*);
    void sub_node(ExprNode*);
    void mul_node(ExprNode*);
    void div_node(ExprNode*);
};

