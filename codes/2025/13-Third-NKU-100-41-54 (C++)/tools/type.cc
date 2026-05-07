#include "../include/type.h"
#include <iostream>
#include <vector>
//#include "../include/Instruction.h"
//#include "../include/basic_block.h"
//#include "../include/tree.h"
#include <functional>
#include <array>
#include <assert.h>
#include <map>
#include <utility>  // for std::pair
#include <string>

std::string OpType::GetOpTypeString(){
    switch (optype) {
        case OpType::Void:
            return "Void";
        case OpType::Add:
            return "+";
        case OpType::Sub:
            return "-";
        case OpType::Mul:
            return "*";
        case OpType::Div:
            return "/";
        case OpType::Mod:
            return "%";
        case OpType::And:
            return "&&";
        case OpType::Or:
            return "||";
        case OpType::Not:
            return "!";
        case OpType::Eq:
            return "==";
        case OpType::Neq:
            return "!=";
        case OpType::Lt:
            return "<";
        case OpType::Gt:
            return ">";
        case OpType::Le:
            return "<=";
        case OpType::Ge:
            return ">=";
        default:
            return "";
    }
}

std::string BuiltinType::getString(){
	std::string pointerTag = (this->isPointer) ? "(Ptr)" : "";
    std::string str="Builtin:";
        switch(builtinKind){
            case Int:
                return str+"Int"+pointerTag;
            case Float:
                return str+"Float"+pointerTag;
            case String:
                return str+"String"+pointerTag;
            case Bool:
                return str+"Bool"+pointerTag;
            case Void:
                return str+"Void";
			case IntPtr:
				return str+"IntPtr";
            case FloatPtr:
                return str+"FloatPtr";
            default:
                break;
        }
    return "";
}


int BuiltinType::getType(){
    return builtinKind;
}


std::string NodeAttribute::GetConstValueInfo() {
    if (!ConstTag) {
        return "";
    }
    switch(type->getType()){
        case 1:
            return "ConstValue: " + std::to_string(val.IntVal);
        case 2:
            return "ConstValue: " + std::to_string(val.FloatVal);
        case 3:
            return "ConstValue: " + StrVal;
        case 4:
            return "ConstValue: " + std::to_string(val.BoolVal);
        case 5:
            return "";
        default:
            assert(0);
    }
    return "";
}

std::string NodeAttribute::GetAttributeInfo() { 
    return type->getString() + "   " + GetConstValueInfo(); 
    return "";
}


//semant

/*实现思路如下：
SemantBinaryNode[6][6]（a,b,opcode）

---根据a,b的type-->调用SemantIntInt,SemantFloatFloat,SemantError
（其中SemantIntFloat,SemantBoolBool等函数会调用SemantIntInt或者SemantFloatFloat,函数内的处理主要是把节点转换成int类型或者float类型来传递。）

---根据操作数的类型指定op及resultType-->调用BinarySemantOperation<T,OP>(a,b,op,resultType)
（模板参数T表示参与运算的二元操作数的类型，要么都是int要么都是float;模板参数OP根据传入的参数op自动指定，用于根据opcode操作数类型来以不同的方式计算result.val(加还是减还是或还是别的？））
*/


extern std::vector<std::string> error_msgs;
extern int max_reg;
constexpr size_t TYPE_COUNT = 6;

NodeAttribute SemantErrorError(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    error_msgs.push_back("SemantErrorError in line " + std::to_string(a.line_number) + "\n");
    NodeAttribute result;
    return result;
}

//再通过操作数类型指定函数的resultType
template<typename T,typename OP>//OP会根据传入的参数自动推断
NodeAttribute BinarySemantOperation(NodeAttribute a, NodeAttribute b,OP op,BuiltinType::BuiltinKind resultType)
{
    NodeAttribute result;
    result.type=new BuiltinType(resultType);
    result.ConstTag=a.ConstTag&b.ConstTag;
    if(result.ConstTag)
    {// `if constexpr` 允许根据模板参数的类型在编译时选择执行的代码块。
    //`std::is_same<T1, T2>::value` 将返回一个 `bool` 值，表示类型 `T1` 是否与 `T2` 相同。
    //if的判断条件是，节点a,b使用的val类型;bool类型的节点会转换成int类型计算
    //但是还需要判断result用哪个val类型来赋值，这取决于resultType
        if constexpr(std::is_same<T,int>::value)
        {
            //std::cout<<"BinarySemantOperation:int\n";
            if(resultType==BuiltinType::BuiltinKind::Bool){result.val.BoolVal=op(a.val.IntVal,b.val.IntVal);}
            else{result.val.IntVal=op(a.val.IntVal,b.val.IntVal);}
        }
        else if constexpr(std::is_same<T,float>::value)
        {
            if(resultType==BuiltinType::BuiltinKind::Bool){result.val.BoolVal=op(a.val.FloatVal,b.val.FloatVal);}
            else{result.val.FloatVal=op(a.val.FloatVal,b.val.FloatVal);}
        }
        //需要考虑其他情况吗？--应该不用了
    }

    return result;
}
template<typename T>
NodeAttribute BinarySemantOperation_Div(NodeAttribute a, NodeAttribute b,BuiltinType::BuiltinKind resultType)
{
    //std::cout<<"BinarySemantOperation_Div is called\n";
    NodeAttribute result;
    result.type=new BuiltinType(resultType);
    result.ConstTag=a.ConstTag&b.ConstTag;
    if(b.ConstTag)
    {
        if constexpr(std::is_same<T,int>::value)
        {
            if(b.val.IntVal==0)
            {
                error_msgs.push_back("Divisor cannot be zero " + std::to_string(a.line_number) + "\n");
                return result;
            }
        }
        if constexpr(std::is_same<T,float>::value)
        {
            if(b.val.FloatVal==0)
            {
                error_msgs.push_back("Divisor cannot be zero " + std::to_string(a.line_number) + "\n");
                return result;
            }
           
        }
    }
    if(result.ConstTag)
    {
        if constexpr(std::is_same<T,int>::value)
        {
            if(resultType==BuiltinType::BuiltinKind::Bool){result.val.BoolVal=a.val.IntVal/b.val.IntVal;}
            else{result.val.IntVal=a.val.IntVal/b.val.IntVal;}
        }
        else if constexpr(std::is_same<T,float>::value)
        {
            if(resultType==BuiltinType::BuiltinKind::Bool){result.val.BoolVal=a.val.FloatVal/b.val.FloatVal;}
            else{result.val.FloatVal=a.val.FloatVal/b.val.FloatVal;}
        }
    }
    return result;
}
template<typename T>
NodeAttribute BinarySemantOperation_Mod(NodeAttribute a, NodeAttribute b,BuiltinType::BuiltinKind resultType)
{
    //std::cout<<"BinarySemantOperation_Mod is called\n";
    NodeAttribute result;
    result.type=new BuiltinType(resultType);
    result.ConstTag=a.ConstTag&b.ConstTag;
    if(b.ConstTag)
    {
        if constexpr(std::is_same<T,int>::value)
        {
            // std::cout<<"b.val.IntVal"<<b.val.IntVal<<"\n";
            if(b.val.IntVal==0)
            {
                error_msgs.push_back("Divisor cannot be zero " + std::to_string(a.line_number) + "\n");
                return result;
            }
        }
        
    }
    if(result.ConstTag)
    {
        if constexpr(std::is_same<T,int>::value)
        {
            if(resultType==BuiltinType::BuiltinKind::Bool){result.val.BoolVal=a.val.IntVal/b.val.IntVal;}
            else{result.val.IntVal=a.val.IntVal%b.val.IntVal;}
        }
    }
    return result;
}
//通过两个子节点exp的attribute，操作数类型跳转到这
NodeAttribute SemantIntInt(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    static std::function<NodeAttribute(NodeAttribute, NodeAttribute)> ops[] = {
        [](NodeAttribute a, NodeAttribute b){ return SemantErrorError(a, b,OpType::Void);},
        //+-*/%
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::plus<>(), BuiltinType::Int); },
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::minus<>(), BuiltinType::Int); },
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::multiplies<>(), BuiltinType::Int); },
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation_Div<int>(a, b, BuiltinType::Int); },
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation_Mod<int>(a, b,  BuiltinType::Int); },
        //and or not
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::logical_and<>(), BuiltinType::Bool); },
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::logical_or<>(), BuiltinType::Bool); },
        [](NodeAttribute a, NodeAttribute b){ return SemantErrorError(a, b,OpType::Not);},//单目没有not
        //eq,neq
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::equal_to<>(), BuiltinType::Bool); },
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::not_equal_to<>(), BuiltinType::Bool); },
        //< >
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::less<>(), BuiltinType::Bool); },
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::greater<>(), BuiltinType::Bool); },
        //<= >=
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::less_equal<>(), BuiltinType::Bool); },
        [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<int>(a, b, std::greater_equal<>(), BuiltinType::Bool); },
    };

    return ops[opcode](a, b);
}

NodeAttribute SemantIntBool(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    NodeAttribute tmp_b=b;
    tmp_b.type=new BuiltinType(BuiltinType::BuiltinKind::Int);
    tmp_b.val.IntVal=int(tmp_b.val.BoolVal);
    return SemantIntInt(a,tmp_b,opcode);
}

NodeAttribute SemantBoolInt(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    NodeAttribute tmp_a=a;
    tmp_a.type=new BuiltinType(BuiltinType::BuiltinKind::Int);
    tmp_a.val.IntVal=int(tmp_a.val.BoolVal);
    return SemantIntInt(tmp_a,b,opcode);
}


NodeAttribute SemantFloatFloat(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    static std::function<NodeAttribute(NodeAttribute, NodeAttribute)> ops[] = {
       //void
       [](NodeAttribute a, NodeAttribute b){ return SemantErrorError(a, b,OpType::Void);},
       //+-*/%
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::plus<>(), BuiltinType::Float); },
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::minus<>(), BuiltinType::Float); },
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::multiplies<>(), BuiltinType::Float); },
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation_Div<float>(a, b, BuiltinType::Float); },
       [](NodeAttribute a, NodeAttribute b){ return SemantErrorError(a,b,OpType::Mod); },//取余运算不能为float
       //and or not
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::logical_and<>(), BuiltinType::Bool); },
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::logical_or<>(), BuiltinType::Bool); },
       [](NodeAttribute a, NodeAttribute b){ return SemantErrorError(a, b,OpType::Not);},//单目没有not
       //eq,neq
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::equal_to<>(), BuiltinType::Bool); },
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::not_equal_to<>(), BuiltinType::Bool); },
       //< >
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::less<>(), BuiltinType::Bool); },
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::greater<>(), BuiltinType::Bool); },
       //<= >=
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::less_equal<>(), BuiltinType::Bool); },
       [](NodeAttribute a, NodeAttribute b){ return BinarySemantOperation<float>(a, b, std::greater_equal<>(), BuiltinType::Bool); },
    };

    return ops[opcode](a, b);
}

NodeAttribute SemantIntFloat(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    NodeAttribute tmp_a=a;
    tmp_a.type=new BuiltinType(BuiltinType::BuiltinKind::Float);
    tmp_a.val.FloatVal=float(tmp_a.val.IntVal);
    return SemantFloatFloat(tmp_a,b,opcode);
}
NodeAttribute SemantFloatInt(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    NodeAttribute tmp_b=b;
    tmp_b.type=new BuiltinType(BuiltinType::BuiltinKind::Float);
    tmp_b.val.FloatVal=float(tmp_b.val.IntVal);
    return SemantFloatFloat(a,tmp_b,opcode);
}
NodeAttribute SemantBoolFloat(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    NodeAttribute tmp_a=a;
    tmp_a.type=new BuiltinType(BuiltinType::BuiltinKind::Float);
    tmp_a.val.FloatVal=float(tmp_a.val.BoolVal);
    return SemantFloatFloat(tmp_a,b,opcode);
}
NodeAttribute SemantFloatBool(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    NodeAttribute tmp_b=b;
    tmp_b.type=new BuiltinType(BuiltinType::BuiltinKind::Float);
    tmp_b.val.FloatVal=float(tmp_b.val.BoolVal);
    return SemantFloatFloat(a,tmp_b,opcode);
}
NodeAttribute SemantBoolBool(NodeAttribute a, NodeAttribute b, OpType::Op opcode) {
    NodeAttribute tmp_a = a, tmp_b = b;
    tmp_a.type = new BuiltinType(BuiltinType::BuiltinKind::Int);
    tmp_b.type = new BuiltinType(BuiltinType::BuiltinKind::Int);
    tmp_a.val.IntVal = int(tmp_a.val.BoolVal);
    tmp_b.val.IntVal = int(tmp_b.val.BoolVal);
    return SemantIntInt(tmp_a,tmp_b,opcode);
}

std::map<std::pair<BuiltinType::BuiltinKind, BuiltinType::BuiltinKind>, NodeAttribute (*)(NodeAttribute, NodeAttribute, OpType::Op)> SemantBinaryNodeMap = {
    {{BuiltinType::BuiltinKind::Int, BuiltinType::BuiltinKind::Int}, SemantIntInt},
    {{BuiltinType::BuiltinKind::Int, BuiltinType::BuiltinKind::Float}, SemantIntFloat},
    {{BuiltinType::BuiltinKind::Int, BuiltinType::BuiltinKind::Bool}, SemantIntBool},
    {{BuiltinType::BuiltinKind::Float, BuiltinType::BuiltinKind::Int}, SemantFloatInt},
    {{BuiltinType::BuiltinKind::Float, BuiltinType::BuiltinKind::Float}, SemantFloatFloat},
    {{BuiltinType::BuiltinKind::Float, BuiltinType::BuiltinKind::Bool}, SemantFloatBool},
    {{BuiltinType::BuiltinKind::Bool, BuiltinType::BuiltinKind::Int}, SemantBoolInt},
    {{BuiltinType::BuiltinKind::Bool, BuiltinType::BuiltinKind::Float}, SemantBoolFloat},
    {{BuiltinType::BuiltinKind::Bool, BuiltinType::BuiltinKind::Bool}, SemantBoolBool},
};



//-------------下面是单操作数--------------
template<typename T>
NodeAttribute SingleOperation(NodeAttribute a,BuiltinType::BuiltinKind resultType)//不能区分add与sub,需要在调用后调整符号
{
    NodeAttribute result;
    result.type=new BuiltinType(resultType);
    result.ConstTag=a.ConstTag;
    if(result.ConstTag)
    {
        if constexpr(std::is_same<T,int>::value)//操作数是int类型
        {
            if(resultType==BuiltinType::BuiltinKind::Bool){result.val.BoolVal=!a.val.IntVal;}//说明是!非的运算，直接取反
            else{result.val.IntVal=a.val.IntVal;}
        }
        else if constexpr(std::is_same<T,float>::value)//操作数是float类型
        {
            if(resultType==BuiltinType::BuiltinKind::Bool){result.val.BoolVal=!a.val.FloatVal;}
            else{result.val.FloatVal=a.val.FloatVal;}
        }
    }

    return result;
}
NodeAttribute SemantInt(NodeAttribute a, OpType::Op opcode)
{
          static std::function<NodeAttribute(NodeAttribute)> ops[15] = {
            nullptr,//void
        [](NodeAttribute a){ return SingleOperation<int>(a,BuiltinType::Int); },//ADD
        [](NodeAttribute a)
        { 
            NodeAttribute result=SingleOperation<int>(a,BuiltinType::Int);
            result.val.IntVal= -result.val.IntVal;
            return result; 
        },//SUB,需要处理符号
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        [](NodeAttribute a){ return SingleOperation<int>(a,BuiltinType::Bool); },//！，NOT
        nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
    };
    

    return ops[opcode](a);
}

NodeAttribute SemantFloat(NodeAttribute a, OpType::Op opcode)
{
    static std::function<NodeAttribute(NodeAttribute)> ops[15] = {
            nullptr,//void
                [](NodeAttribute a){ return SingleOperation<float>(a,BuiltinType::Float);},//ADD
                [](NodeAttribute a)
                { 
                    NodeAttribute result=SingleOperation<float>(a,BuiltinType::Float);
                    result.val.FloatVal= -result.val.FloatVal;
                    return result; 
                },//SUB,需要处理符号
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                nullptr,
                [](NodeAttribute a){return SingleOperation<float>(a,BuiltinType::Bool);  },
                nullptr,nullptr,nullptr,nullptr,nullptr,nullptr,
            };
    
    return ops[opcode](a);
}

NodeAttribute SemantBool(NodeAttribute a, OpType::Op opcode) {
    NodeAttribute tmp_a = a;
    tmp_a.type = new BuiltinType(BuiltinType::BuiltinKind::Int);
    tmp_a.val.IntVal = int(a.val.BoolVal);
    return SemantInt(tmp_a,opcode);
}
NodeAttribute SemantError(NodeAttribute a, OpType::Op opcode) {
    error_msgs.push_back("SemantError invalid operators in line " + std::to_string(a.line_number) + "\n");
    NodeAttribute result;
    return result;
}


std::map<BuiltinType::BuiltinKind, NodeAttribute (*)(NodeAttribute, OpType::Op)> SemantSingleNodeMap = {
    {BuiltinType::BuiltinKind::Int, SemantInt},
    {BuiltinType::BuiltinKind::Float, SemantFloat},
    {BuiltinType::BuiltinKind::Bool, SemantBool}
    // {Type::PTR, SemantError},
    // {Type::VOID, SemantError}
};