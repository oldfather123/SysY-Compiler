#ifndef TYPE_H
#define TYPE_H

#include <iostream>
#include <vector>
class OpType{
public:
    enum Op{
        // not defined 
	Void,

    // arithmetic
    Add,        // +
    Sub,        // -
    Mul,        // *
    Div,        // /
    Mod,        // %

    // logic
    And,        // &&
    Or,         // ||
    Not,        // !

    // compare 
    Eq,         // ==
    Neq,        // !=
    Lt,         // <
    Gt,         // >
    Le,         // <=
    Ge          // >=
    }optype;

    OpType() { optype = Void; }
    OpType(Op op) { optype = op; }
    std::string GetOpTypeString();
};

class Type {
    public:
        enum TypeKind {
            Builtin=0,
            Pointer=6,
            Array=7
        }kind;
        virtual std::string getString()=0;
        virtual int getType()=0;
        virtual ~Type() = default;
};
    
class BuiltinType : public Type {
    public:
        enum BuiltinKind { Int =1, Float=2, String=3 , Bool=4, Void=5 , IntPtr=6, FloatPtr=7}builtinKind;
        bool isPointer;
        BuiltinType() { builtinKind = BuiltinKind::Void;kind=Type::Builtin;isPointer=false; }
        BuiltinType(BuiltinKind kind) : builtinKind(kind) {
            this->kind = Type::Builtin;
            isPointer=false;
        }
        std::string getString();
        int getType();
		bool checkPointer(){
			return (builtinKind == BuiltinKind::FloatPtr || builtinKind == BuiltinKind::IntPtr);
		}
};


class NodeAttribute {
    public:
        int line_number = -1;
        //Type* T;
        BuiltinType* type;
        bool ConstTag;

		// constValue
        union ConstVal {
            bool BoolVal;
            int IntVal;
            float FloatVal;
            double DoubleVal;
        } val;
		std::string StrVal;

		// array
		std::vector<int> dims{};    
		// 对于数组的初始化值，我们将高维数组看作一维后再存储 eg.([3 x [4 x i32]] => [12 x i32])
		std::vector<int> IntInitVals{}; 
		std::vector<float> FloatInitVals{};
		std::vector<int> RealInitvalPos;

		NodeAttribute() { 
			ConstTag = false; 
			val.IntVal=0; 
			StrVal="";
			type = new BuiltinType(BuiltinType::BuiltinKind::Void);
		}
		std::string GetAttributeInfo();
		std::string GetConstValueInfo();
};

class VarAttribute {
public:
    Type* type;
    bool ConstTag = 0;
    std::vector<int> dims{};    // 存储数组类型的相关信息
    // 对于数组的初始化值，我们将高维数组看作一维后再存储 eg.([3 x [4 x i32]] => [12 x i32])
    std::vector<int> IntInitVals{}; 
    std::vector<float> FloatInitVals{};

    // TODO():也许你需要添加更多变量
    VarAttribute() {
        type = new BuiltinType(BuiltinType::BuiltinKind::Void);
        ConstTag = false;
    }
	VarAttribute(std::vector<int> Dims, std::vector<int> InitVals) 
    : dims(Dims), IntInitVals(InitVals)
    {
        type = new BuiltinType(BuiltinType::BuiltinKind::Int);
        ConstTag = false;
    }
    VarAttribute(std::vector<int> Dims, std::vector<float> InitVals) 
    : dims(Dims), FloatInitVals(InitVals)
    {
        type = new BuiltinType(BuiltinType::BuiltinKind::Float);
        ConstTag = false;
    }
};

#endif