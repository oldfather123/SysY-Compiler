#include "../../include/ast.h"
#include "IRgen.h"
#include "../include/ir.h"
#include "semant.h"
int max_reg = -1;
int max_label = -1;

extern SemantTable semant_table;    // 也许你会需要一些语义分析的信息

IRgenTable irgen_table;    // 中间代码生成的辅助变量
LLVMIR llvmIR;             // 我们需要在这个变量中生成中间代码

std::map<Symbol*,Operand> ptrmap;
Operand currentptr;
Operand currentop;
int currentint;
float currentfloat;
Symbol* currentname;
FunctionDefineInstruction* funcdefI;

enum operand_type { I32 = 1, FLOAT32 = 2, BOOL = 3, I32_PTR = 4, FLOAT32_PTR = 5};
std::map<int,operand_type> optype_map;

// 判断左值还是右值，左值的上层为AssignmentLval的左部，其余都为右值

bool isLeftlval;
bool isRightlval;

// 判断是否为函数参数

bool isParam;

// 处理break continue

std::deque<LLVMBlock> continuebbque;
// 存储需要跳转到entrybb (continue) 的基本块
std::deque<LLVMBlock> breakbbque;
// 存储需要跳转到exitbb (break) 的基本块

// 处理短路求值

std::deque<LLVMBlock> truebbque;
std::deque<LLVMBlock> falsebbque;
BasicBlock* genbb;

/*************************************************************************************
 *					                数组初值处理 	
 *************************************************************************************
 * 	ISO_IEC_9899  p117 example9  section 6.7.9
 *	
 * 	除了最外围的{}以外，每次遇到嵌套的{}都会进入一个子数组(subArray)
 * 	subArray.size() 取决于括号所处的位置，
 * 	如q[3][5][2], As subArray begin at pos
 * 	if(pos % (2 * 5) == 0) 开辟 q[]     这样的数组, subArray.size() = 10;
 * 	if(pos % (2) == 0)     开辟 q[][]   这样的数组, subArray.size() = 2;
 * 	else                   开辟 q[][][] 这样的数组, subArray.size() = 1; (线性填充)
 * 	
 * 	如果开辟的数组大小没有{}内的元素多，就会顺序填充，例如 e 数组
 * 	
 * 
 * 	我们想要实现如下的转换，即将{}嵌套的数组初始化，转换为等效的std::vector<int> initValint
 * 	c[3][5][2] = {{1}, 2, {3}, 4, 5, {3}, 4, 5, {6}, {7}};
 * 	<=> c[3][5][2] = {1,0,0,0,0,0,0,0,0,0,2,3,4,5,3,0,4,5,6,0,7,0,0,0,0,0,0,0,0,0}
 * 	
 * 	d[3][5][2] = {{1}, 2, {3, 5}, 4};
 * 	<=> d[3][5][2] = {1,0,0,0,0,0,0,0,0,0,2,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}
 * 	
 * 	e[3][5][2] = {{1}, {2}, {3, 5, 6, 7}};
 * 	<=> e[3][5][2] = {1,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,3,5,6,7,0,0,0,0,0,0}
 * 	
 * 	f[3][5][2] = {{1}, {2, 3, {4}, 5}, 6}
 * 	<=> e[3][5][2] = {1,0,0,0,0,0,0,0,0,0,2,3,4,0,5,0,0,0,0,0,6,0,0,0,0,0,0,0,0,0} 
 * 
 * 	具体数据维护可以看 handouts 中的数组初始化文档
 *  */

bool dimcount; 
// dimconut为true时表示目前的intcount对应数组定义的下标，不需要生成立即数相加的指令
std::set<int> paramptr; 
// 判断一个寄存器是否是函数参数和指针
// 函数参数的指针可以直接当数组指针作为getelementptr中的ptr使用
// 函数参数里的其他非指针变量需要先在entrybb里alloca，并把变量store进alloca的新指针
std::vector<int> initdim;
// 用于描述数组定义中各维度的数值
std::vector<int> initarrayint;
// 用于描述数组定义中的初值
std::vector<float> initarrayfloat;
// 用于描述数组定义中的初值

int head;
// 数组当前初始化的起始位置
bool isGobal;
bool isTopVarInitVal;
// 1 - 表示可以分配最大为数组去掉第 1 维的大小
// 2 - 表示可以分配最大为数组去掉第 2 维的大小
// ......
int subArraydim;
Operand initarrayReg;

// 根据数组当前初始化起始位置和维度信息，确定当前初始化的末尾 tail
int getTail(){
	int allocaSpace = 1, product = 1;

	// for(int i = initdim.size() - 1; i >= 1; i--) {
	// 	product *= initdim[i];
	// 	if(head % product == 0) {
	// 		allocaSpace = product;
	// 	}
	// 	else break;
	// }
	// std::cerr << "subArraydim : " << subArraydim << std::endl; 

	int subArraydim_copy = subArraydim;
	for(int i = initdim.size() - 1; i >= subArraydim_copy; i--) {
		product *= initdim[i];
		if(head % product == 0) {
			allocaSpace = product;
			subArraydim = i + 1;
		}
		else break;
	}

	return head + allocaSpace - 1;
}


// 线性偏移转换为数组各个维度的索引
std::vector<Operand> offset_to_indexs(int offset, const std::vector<int>& initdim) {
    int dims = initdim.size();
    std::vector<Operand> indexs(dims + 1, 0); 

	indexs[0] = new ImmI32Operand(0);
    for (int i = dims; i >= 1; --i) {
        indexs[i] = new ImmI32Operand(offset % initdim[i - 1]);
        offset /= initdim[i - 1];
    }
	
    return indexs;
}

void AddLibFunctionDeclare();

// 在基本块B末尾生成一条新指令
void IRgenArithmeticI32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg);
void IRgenArithmeticF32(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int reg1, int reg2, int result_reg);
void IRgenArithmeticI32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int reg2, int result_reg);
void IRgenArithmeticF32ImmLeft(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, int reg2,
                               int result_reg);
void IRgenArithmeticI32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, int val1, int val2, int result_reg);
void IRgenArithmeticF32ImmAll(LLVMBlock B, BasicInstruction::LLVMIROpcode opcode, float val1, float val2,
                              int result_reg);

void IRgenIcmp(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int reg2, int result_reg);
void IRgenFcmp(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, int reg2, int result_reg);
void IRgenIcmpImmRight(LLVMBlock B, BasicInstruction::IcmpCond cmp_op, int reg1, int val2, int result_reg);
void IRgenFcmpImmRight(LLVMBlock B, BasicInstruction::FcmpCond cmp_op, int reg1, float val2, int result_reg);

void IRgenFptosi(LLVMBlock B, int src, int dst);
void IRgenSitofp(LLVMBlock B, int src, int dst);
void IRgenZextI1toI32(LLVMBlock B, int src, int dst);

void IRgenGetElementptrIndexI32(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                        std::vector<int> dims, std::vector<Operand> indexs);

void IRgenGetElementptrIndexI64(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr,
                        std::vector<int> dims, std::vector<Operand> indexs);

void IRgenLoad(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, Operand ptr);
void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, int value_reg, Operand ptr);
void IRgenStore(LLVMBlock B, BasicInstruction::LLVMType type, Operand value, Operand ptr);

void IRgenCall(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg,
               std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name);
void IRgenCallVoid(LLVMBlock B, BasicInstruction::LLVMType type,
                   std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args, std::string name);

void IRgenCallNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, int result_reg, std::string name);
void IRgenCallVoidNoArgs(LLVMBlock B, BasicInstruction::LLVMType type, std::string name);

void IRgenRetReg(LLVMBlock B, BasicInstruction::LLVMType type, int reg);
void IRgenRetImmInt(LLVMBlock B, BasicInstruction::LLVMType type, int val);
void IRgenRetImmFloat(LLVMBlock B, BasicInstruction::LLVMType type, float val);
void IRgenRetVoid(LLVMBlock B);

void IRgenBRUnCond(LLVMBlock B, int dst_label);
void IRgenBrCond(LLVMBlock B, int cond_reg, int true_label, int false_label);

void IRgenAlloca(LLVMBlock B, BasicInstruction::LLVMType type, int reg);
void IRgenAllocaArray(LLVMBlock B, BasicInstruction::LLVMType type, int reg, std::vector<int> dims);

void IRgenTypeConverse(LLVMBlock B, BuiltinType::BuiltinKind type_src, BuiltinType::BuiltinKind type_dst, int src);
void IRgenGlobalVarDefineArray(std::string name, BasicInstruction::LLVMType type, VarAttribute v,bool is_const) {
    auto newI = new GlobalVarDefineInstruction(name, type, v,is_const);
    llvmIR.global_def.insert(newI);
    
}
void IRgenGlobalVarDefine(std::string name, BasicInstruction::LLVMType type, Operand init_val,bool is_const,bool has_init) {
    auto newI = new GlobalVarDefineInstruction(name, type, init_val,is_const,has_init);
    llvmIR.global_def.insert(newI);
}
RegOperand *GetNewRegOperand(int RegNo);

// generate TypeConverse Instructions from type_src to type_dst
// eg. you can use fptosi instruction to converse float to int
// eg. you can use zext instruction to converse bool to int
// LLVMType to_type, Operand result_receiver, LLVMType from_type, Operand value_for_cast
void IRgenTypeConverse(LLVMBlock B, int type_src, int type_dst, int src, int dst) {
    auto newresultop = GetNewRegOperand(dst);
    if(type_src == BuiltinType::BuiltinKind::Bool && type_dst == BuiltinType::BuiltinKind::Int){
        B->InsertInstruction(1, new ZextInstruction(BasicInstruction::LLVMType::I32, GetNewRegOperand(src), BasicInstruction::LLVMType::I1, newresultop));
        optype_map[dst] = operand_type::I32;
        currentop = newresultop;
    }else if(type_src == BuiltinType::BuiltinKind::Int && type_dst == BuiltinType::BuiltinKind::Bool){
        B->InsertInstruction(1, new ZextInstruction(BasicInstruction::LLVMType::I1, GetNewRegOperand(src), BasicInstruction::LLVMType::I32, newresultop));
        optype_map[dst] = operand_type::BOOL;
        currentop = newresultop;
    }else if(type_src == BuiltinType::BuiltinKind::Int && type_dst == BuiltinType::BuiltinKind::Float){
        B->InsertInstruction(1, new SitofpInstruction(GetNewRegOperand(src), newresultop));
        optype_map[dst] = operand_type::FLOAT32;
        currentop = newresultop;
    }else if(type_src == BuiltinType::BuiltinKind::Float && type_dst == BuiltinType::BuiltinKind::Int){
        B->InsertInstruction(1, new FptosiInstruction(GetNewRegOperand(src), newresultop));
        optype_map[dst] = operand_type::I32;
        currentop = newresultop;
    }
    
    // DONE("IRgenTypeConverse. Implement it if you need it");
}

void BasicBlock::InsertInstruction(int pos, Instruction Ins) {
    assert(pos == 0 || pos == 1);
    if (pos == 0) {
        Instruction_list.push_front(Ins);
    } else if (pos == 1) {
        Instruction_list.push_back(Ins);
    }
}

LLVMBlock& GetCurrentBlock(){
    return genbb == nullptr ? llvmIR.function_block_map[funcdefI][max_label] : genbb;
}

// 二元运算符需要保证两个的寄存器都非Bool，否则需要转换
void BinaryBoolCheckandConverse(LLVMBlock B, Operand &conversereg){
    if(B->Instruction_list.back()->GetOpcode() == BasicInstruction::ICMP || B->Instruction_list.back()->GetOpcode() == BasicInstruction::FCMP){
        auto newresultreg_zext = GetNewRegOperand(++max_reg);
        IRgenTypeConverse(B,
                BuiltinType::BuiltinKind::Bool,
                BuiltinType::BuiltinKind::Int,
                max_reg,
                ((RegOperand*)conversereg)->GetRegNo());
        optype_map[max_reg] = operand_type::I32;
        conversereg = newresultreg_zext;
    }else{
        conversereg = currentop;
    }
}

void BinaryFloattoIntConverse(LLVMBlock B, Operand &conversereg){
    auto newresultreg_zext = GetNewRegOperand(++max_reg);
    
    IRgenTypeConverse(B,
            BuiltinType::BuiltinKind::Float,
            BuiltinType::BuiltinKind::Int,
            max_reg,
            ((RegOperand*)conversereg)->GetRegNo());
    optype_map[max_reg] = operand_type::I32;
    conversereg = newresultreg_zext;
}

void BinaryInttoFloatConverse(LLVMBlock B, Operand &conversereg){
    auto newresultreg_zext = GetNewRegOperand(++max_reg);
    IRgenTypeConverse(B,
            BuiltinType::BuiltinKind::Int,
            BuiltinType::BuiltinKind::Float,
            max_reg,
            ((RegOperand*)conversereg)->GetRegNo());
    optype_map[max_reg] = operand_type::FLOAT32;
    conversereg = newresultreg_zext;
}



void GenArithmeticBinaryIR(NodeAttribute attribute, OpType::Op opKind, ExprBase lhs, ExprBase rhs) {
    currentop = nullptr;
    currentint = 0;
    currentfloat = 0;

    if (dimcount && attribute.ConstTag) {
        currentint = attribute.val.IntVal;
        currentfloat = attribute.val.FloatVal;
        return;
    }

    lhs->codeIR();
    auto leftop = currentop;
    BinaryBoolCheckandConverse(GetCurrentBlock(), leftop);

    rhs->codeIR();
    auto rightop = currentop;
    BinaryBoolCheckandConverse(GetCurrentBlock(), rightop);

    auto &entrybb = GetCurrentBlock();
    auto leftregno = ((RegOperand*)leftop)->GetRegNo();
    auto rightregno = ((RegOperand*)rightop)->GetRegNo();
    auto leftop_type = optype_map[leftregno];
    auto rightop_type = optype_map[rightregno];

    auto promoteToFloat = [&]() {
        if (leftop_type == operand_type::I32) {
            BinaryInttoFloatConverse(entrybb, leftop);
            leftregno = ((RegOperand*)leftop)->GetRegNo();
			leftop_type = optype_map[leftregno];
        }
        if (rightop_type == operand_type::I32) {
            BinaryInttoFloatConverse(entrybb, rightop);
            rightregno = ((RegOperand*)rightop)->GetRegNo();
			rightop_type = optype_map[rightregno];
        }
    };

    if (leftop_type == operand_type::I32 && rightop_type == operand_type::I32) {
        auto newreg = GetNewRegOperand(++max_reg);
        IRgenArithmeticI32(entrybb, mapToLLVMOpcodeInt(opKind), leftregno, rightregno, max_reg);
        optype_map[max_reg] = operand_type::I32;
        currentop = newreg;
    } else {
        promoteToFloat();
        auto newreg = GetNewRegOperand(++max_reg);
        IRgenArithmeticF32(entrybb, mapToLLVMOpcodeFloat(opKind), leftregno, rightregno, max_reg);
        optype_map[max_reg] = operand_type::FLOAT32;
        currentop = newreg;
    }
}

void GenArithmeticModIR(NodeAttribute attribute, ExprBase lhs, ExprBase rhs){
    currentop = nullptr;

    if(dimcount && attribute.ConstTag){
        currentint = attribute.val.IntVal;
        currentfloat = attribute.val.FloatVal;
        return;
    }
	
    lhs->codeIR();
    auto leftop = currentop;
    BinaryBoolCheckandConverse(GetCurrentBlock(), leftop);

    rhs->codeIR();
    auto rightop = currentop;
    BinaryBoolCheckandConverse( GetCurrentBlock(),rightop);
    auto &entrybb = GetCurrentBlock();

    auto leftregno = ((RegOperand*)leftop)->GetRegNo();
    auto rightregno = ((RegOperand*)rightop)->GetRegNo();
    auto leftop_type = optype_map[leftregno];
    auto rightop_type = optype_map[rightregno];

	// 不支持浮点数运算以及对应的类型转换
	assert(leftop_type != operand_type::I32 || rightop_type == operand_type::I32);

    auto newresultreg = GetNewRegOperand(++max_reg);
    IRgenArithmeticI32(entrybb,
        BasicInstruction::LLVMIROpcode::MOD, 
        ((RegOperand*)leftop)->GetRegNo(),
        ((RegOperand*)rightop)->GetRegNo(),max_reg);
    optype_map[max_reg] = operand_type::I32;
    currentop = newresultreg;
}

void GenCompareBinaryIR(OpType::Op opKind, ExprBase lhs, ExprBase rhs) {
    currentop = nullptr;
    currentint = 0;
    currentfloat = 0;

    lhs->codeIR();
    auto leftop = currentop;
    BinaryBoolCheckandConverse(GetCurrentBlock(), leftop);

    rhs->codeIR();
    auto rightop = currentop;
    BinaryBoolCheckandConverse(GetCurrentBlock(), rightop);

    auto &entrybb = GetCurrentBlock();
    auto leftregno = ((RegOperand*)leftop)->GetRegNo();
    auto rightregno = ((RegOperand*)rightop)->GetRegNo();
    auto leftop_type = optype_map[leftregno];
    auto rightop_type = optype_map[rightregno];

    if (leftop_type == operand_type::I32 && rightop_type == operand_type::FLOAT32) {
        BinaryInttoFloatConverse(entrybb, leftop);
        leftregno = ((RegOperand*)leftop)->GetRegNo();
        leftop_type = optype_map[leftregno];
    } else if (leftop_type == operand_type::FLOAT32 && rightop_type == operand_type::I32) {
        BinaryInttoFloatConverse(entrybb, rightop);
        rightregno = ((RegOperand*)rightop)->GetRegNo();
        rightop_type = optype_map[rightregno];
    }

    auto newreg = GetNewRegOperand(++max_reg);

    if (leftop_type == operand_type::FLOAT32 || rightop_type == operand_type::FLOAT32) {
        IRgenFcmp(entrybb, mapToFcmpCond(opKind), leftregno, rightregno, max_reg);
    } else {
        IRgenIcmp(entrybb, mapToIcmpCond(opKind), leftregno, rightregno, max_reg);
    }

    optype_map[max_reg] = operand_type::BOOL;
    currentop = newreg;
}


void GenUnaryAddSubIR(OpType::Op kind, NodeAttribute attribute, ExprBase unaryExp){
	auto instTypeInt = (kind == OpType::Op::Add) ? BasicInstruction::LLVMIROpcode::ADD : BasicInstruction::LLVMIROpcode::SUB;
	auto instTypeFloat = (kind == OpType::Op::Add) ? BasicInstruction::LLVMIROpcode::FADD : BasicInstruction::LLVMIROpcode::FSUB;

	currentop = nullptr;
    unaryExp->codeIR();
    auto &entrybb = GetCurrentBlock();
	auto unaryExpType = unaryExp->attribute.type->getType();
	if(currentop == nullptr) return;
    if(unaryExpType == BuiltinType::BuiltinKind::Int || unaryExpType == BuiltinType::BuiltinKind::Bool){
        
        if(optype_map[((RegOperand*)currentop)->GetRegNo()] == operand_type::I32){
            
        }else if(optype_map[((RegOperand*)currentop)->GetRegNo()] == operand_type::BOOL){
            BinaryBoolCheckandConverse(entrybb, currentop);
        }else{
            BinaryFloattoIntConverse(entrybb, currentop);
        }
        auto newresultreg = GetNewRegOperand(++max_reg);
        IRgenArithmeticI32ImmLeft(entrybb,
            instTypeInt, 
            0,
            ((RegOperand*)currentop)->GetRegNo(),max_reg);
        optype_map[max_reg] = operand_type::I32;
        currentop = newresultreg;
    }else if(unaryExpType == BuiltinType::BuiltinKind::Float){
        if(optype_map[((RegOperand*)currentop)->GetRegNo()] == operand_type::FLOAT32){
            
        }else if(optype_map[((RegOperand*)currentop)->GetRegNo()] == operand_type::BOOL){
            BinaryBoolCheckandConverse(entrybb, currentop);
            BinaryInttoFloatConverse(entrybb, currentop);
        }else{
            BinaryInttoFloatConverse(entrybb, currentop);
        }
        auto newresultreg = GetNewRegOperand(++max_reg);
        IRgenArithmeticF32ImmLeft(entrybb,
            instTypeFloat, 
            0,
            ((RegOperand*)currentop)->GetRegNo(),max_reg);
        optype_map[max_reg] = operand_type::FLOAT32;
        currentop = newresultreg;
    }
}

void GenUnaryNotIR(NodeAttribute attribute, ExprBase unaryExp){
   	currentop = nullptr;
    auto unaryExpType = unaryExp->attribute.type->getType();

    if(!unaryExp->attribute.ConstTag){
        unaryExp->codeIR();
        auto &entrybb = GetCurrentBlock();
        auto addop = currentop;
        auto newresultreg = GetNewRegOperand(++max_reg);
        IRgenIcmpImmRight(entrybb,
                    BasicInstruction::IcmpCond::eq, 
                    ((RegOperand*)addop)->GetRegNo(),
                    0,max_reg);
        optype_map[max_reg] = operand_type::BOOL;
        auto newresultreg_zext = GetNewRegOperand(++max_reg);
        IRgenTypeConverse(entrybb,
                BuiltinType::BuiltinKind::Bool,
                BuiltinType::BuiltinKind::Int,
                max_reg,
                max_reg-1);
        optype_map[max_reg] = operand_type::I32;
        currentop = newresultreg_zext;
    }else{
        auto newresultreg = GetNewRegOperand(++max_reg);
        auto &entrybb = GetCurrentBlock();
        if(unaryExpType == BuiltinType::BuiltinKind::Float){
            BinaryInttoFloatConverse(entrybb, currentop);
        }
        IRgenArithmeticI32ImmAll(entrybb,
                BasicInstruction::LLVMIROpcode::ADD, 
                0,
                attribute.val.IntVal,max_reg);
        optype_map[max_reg] = operand_type::I32;
        currentop = newresultreg;
    }
}


void Exp::codeIR() { addExp->codeIR(); }

void ConstExp::codeIR() { addExp->codeIR();}

void AddExp::codeIR() { GenArithmeticBinaryIR(attribute, op.optype, addExp, mulExp);}

void MulExp::codeIR() { 
	if(op.optype != OpType::Op::Mod){
		GenArithmeticBinaryIR(attribute, op.optype, mulExp, unaryExp);	
	} else { // mod 操作只支持整数
		GenArithmeticModIR(attribute, mulExp, unaryExp);
	}
}

void RelExp::codeIR() { GenCompareBinaryIR(op.optype, relExp, addExp);}

void EqExp::codeIR()  { GenCompareBinaryIR(op.optype, eqExp, relExp); }

void LAndExp::codeIR() {
    currentop = nullptr;
    max_label++;
    auto newtruebb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    truebbque.push_back(newtruebb);
    lAndExp->codeIR();

    
    auto &entrybb = GetCurrentBlock();
    if(entrybb->Instruction_list.back()->GetOpcode() != BasicInstruction::ICMP && entrybb->Instruction_list.back()->GetOpcode() != BasicInstruction::FCMP){
        auto currentopno = ((RegOperand*)currentop)->GetRegNo();
        auto currentoptype = optype_map[currentopno];
        auto newreg = GetNewRegOperand(++max_reg);
        if(currentoptype == operand_type::I32){
            IRgenIcmpImmRight(entrybb, 
                    BasicInstruction::ne,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)newreg)->GetRegNo());
        }else{
            IRgenFcmpImmRight(entrybb, 
                    BasicInstruction::ONE,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)newreg)->GetRegNo());
        }
        optype_map[max_reg] = operand_type::BOOL;
        currentop = newreg;
    }
    IRgenBrCond(genbb, 
        ((RegOperand*)currentop)->GetRegNo(), 
        truebbque.back()->block_id, 
        falsebbque.back()->block_id);
    genbb = newtruebb;
    truebbque.pop_back();
    eqExp->codeIR();

    if(genbb->Instruction_list.back()->GetOpcode() != BasicInstruction::ICMP && genbb->Instruction_list.back()->GetOpcode() != BasicInstruction::FCMP){
        auto currentopno = ((RegOperand*)currentop)->GetRegNo();
        auto currentoptype = optype_map[currentopno];
        auto newreg = GetNewRegOperand(++max_reg);
        if(currentoptype == operand_type::I32){
            IRgenIcmpImmRight(entrybb, 
                    BasicInstruction::ne,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)newreg)->GetRegNo());
        }else{
            IRgenFcmpImmRight(entrybb, 
                    BasicInstruction::ONE,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)newreg)->GetRegNo());
        }
        optype_map[max_reg] = operand_type::BOOL;
        currentop = newreg;
    }
    IRgenBrCond(genbb, 
        ((RegOperand*)currentop)->GetRegNo(), 
        truebbque.back()->block_id, 
        falsebbque.back()->block_id);
}

void LOrExp::codeIR() {
    currentop = nullptr;
    max_label++;
    auto newfalsebb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    falsebbque.push_back(newfalsebb);
    lOrExp->codeIR();
    auto &entrybb = GetCurrentBlock();
    if(entrybb->Instruction_list.back()->GetOpcode() != BasicInstruction::ICMP && entrybb->Instruction_list.back()->GetOpcode() != BasicInstruction::FCMP){
        auto currentopno = ((RegOperand*)currentop)->GetRegNo();
        auto currentoptype = optype_map[currentopno];
        auto newreg = GetNewRegOperand(++max_reg);
        if(currentoptype == operand_type::I32){
            IRgenIcmpImmRight(entrybb, 
                    BasicInstruction::ne,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)newreg)->GetRegNo());
        }else{
            IRgenFcmpImmRight(entrybb, 
                    BasicInstruction::ONE,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)newreg)->GetRegNo());
        }
        optype_map[max_reg] = operand_type::BOOL;
        currentop = newreg;
    }
    
    IRgenBrCond(genbb, 
        ((RegOperand*)currentop)->GetRegNo(), 
        truebbque.back()->block_id, 
        falsebbque.back()->block_id);
    genbb = newfalsebb;falsebbque.pop_back();
    lAndExp->codeIR();
    
    if(genbb->Instruction_list.back()->GetOpcode() != BasicInstruction::ICMP && genbb->Instruction_list.back()->GetOpcode() != BasicInstruction::FCMP){
        auto currentopno = ((RegOperand*)currentop)->GetRegNo();
        auto currentoptype = optype_map[currentopno];
        auto newreg = GetNewRegOperand(++max_reg);
        if(currentoptype == operand_type::I32){
            IRgenIcmpImmRight(entrybb, 
                    BasicInstruction::ne,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)newreg)->GetRegNo());
        }else{
            IRgenFcmpImmRight(entrybb, 
                    BasicInstruction::ONE,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)newreg)->GetRegNo());
        }
        optype_map[max_reg] = operand_type::BOOL;
        currentop = newreg;
    }
    IRgenBrCond(genbb, 
        ((RegOperand*)currentop)->GetRegNo(), 
        truebbque.back()->block_id, 
        falsebbque.back()->block_id);
}

void Lval::codeIR() {
    isRightlval = 1;

	// if(isParam){
	// 	std::cout << attribute.type->getString() << std::endl;
	// }

    if(dims == nullptr && !attribute.type->checkPointer()){  
        if(isParam || (isRightlval && !isLeftlval)){
            if(ptrmap.find(name) == ptrmap.end()){
                assert("returnop is not define in Lval::codeIR");
            }else{
                if(attribute.ConstTag){
                    if(attribute.type->getType() == BuiltinType::BuiltinKind::Int){
                        currentint = attribute.val.IntVal;
                        currentop = new ImmI32Operand(currentint);
                    }else if(attribute.type->getType() == BuiltinType::BuiltinKind::Float){
                        currentfloat = attribute.val.FloatVal;
                        currentop = new ImmF32Operand(currentfloat);
                    }
                    if(max_label == -1){
                        return;
                    }
                }
                auto &entrybb = GetCurrentBlock(); 
                auto newresultreg = GetNewRegOperand(++max_reg);
				Operand localReg = GetNewRegOperand(irgen_table.symbol_table.look(name));
				Operand globalSymbol = GetNewGlobalOperand(name->getName());
				Operand loadVar = (irgen_table.symbol_table.look(name) == -1) ? globalSymbol : localReg;

                if(attribute.type->getType() == BuiltinType::BuiltinKind::Int){
                    IRgenLoad(entrybb, 
                            BasicInstruction::LLVMType::I32, 
                            max_reg, 
                            loadVar);
                    optype_map[max_reg] = operand_type::I32;
                }else if(attribute.type->getType() == BuiltinType::BuiltinKind::Float){
                    IRgenLoad(entrybb, 
                            BasicInstruction::LLVMType::FLOAT32,
                            max_reg, 
                           	loadVar);
                    optype_map[max_reg] = operand_type::FLOAT32;
                }
                currentop = newresultreg;
            }
        }
        if(ptrmap.find(name) == ptrmap.end()){
            assert("Lvalptr is not define in Lval::codeIR");
        }else{
			Operand localReg = GetNewRegOperand(irgen_table.symbol_table.look(name));
			Operand globalSymbol = GetNewGlobalOperand(name->getName());
			currentptr = (irgen_table.symbol_table.look(name) == -1) ? globalSymbol :localReg;
        }
        currentname = name;
    }else{
        // dimcount = 1;
        std::vector<Operand> dim;
        isLeftlval = 0;
        isRightlval = 1;

        if(ptrmap[name]->GetOperandType() != BasicOperand::REG || ((RegOperand*)ptrmap[name])->GetRegNo() >= funcdefI->formals_reg.size()){
            // gep不加一个零的情况: 函数参数生成的 ptr
            dim.push_back(new ImmI32Operand(0));
        }
        if(dims != nullptr){
            for(auto exp : *dims){
				currentop = nullptr;
                exp->codeIR();
				if(currentop == nullptr) {  // intconst or floatconst
					auto constReg = GetNewRegOperand(++max_reg);
					auto &entrybb = GetCurrentBlock();
					if (attribute.type->getType() == BuiltinType::BuiltinKind::Int) {
						IRgenArithmeticI32ImmAll(entrybb,
								BasicInstruction::LLVMIROpcode::ADD, 
								0,
								exp->attribute.val.IntVal, max_reg);
						optype_map[max_reg] = operand_type::I32;
					} else if (attribute.type->getType() == BuiltinType::BuiltinKind::Float) {
						IRgenArithmeticF32ImmAll(entrybb,
								BasicInstruction::LLVMIROpcode::FADD, 
								0,
								exp->attribute.val.FloatVal, max_reg);
						optype_map[max_reg] = operand_type::FLOAT32;
					}
					dim.push_back(constReg);
				}
				else {
					dim.push_back(currentop);
				}
            }
        }
        
        auto ptrreg = GetNewRegOperand(++max_reg);
        auto &entrybb = GetCurrentBlock(); 

		// ptr类型新增了类型(INT_PTR 和 FLOAT_PTR)，但是不需要load
		Operand localReg = GetNewRegOperand(irgen_table.symbol_table.look(name));
		Operand globalSymbol = GetNewGlobalOperand(name->getName());
		Operand loadVar = (irgen_table.symbol_table.look(name) == -1) ? globalSymbol : localReg;
		std::vector<int> localDim = irgen_table.symboldim_table.look(name);
		std::vector<int> globalDim = semant_table.GlobalTable[name].dims;
		std::vector<int> loadDim = (irgen_table.symboldim_table.look(name) == std::vector<int> ({-1})) ? globalDim : localDim;
		// if(isParam){
		// 	std::cout << attribute.type->getString() << " loadDim.size()=" << loadDim.size() << std::endl;
		// }
		if(attribute.type->getType() == BuiltinType::BuiltinKind::Int || attribute.type->getType() == BuiltinType::BuiltinKind::IntPtr){
			IRgenGetElementptrIndexI32(entrybb, 
				BasicInstruction::LLVMType::I32, 
				max_reg,
				loadVar,
				loadDim,
				dim);
			optype_map[max_reg] = operand_type::I32_PTR;
		} else if(attribute.type->getType() == BuiltinType::BuiltinKind::Float ||  attribute.type->getType() == BuiltinType::BuiltinKind::FloatPtr) {
			IRgenGetElementptrIndexI32(entrybb, 
				BasicInstruction::LLVMType::FLOAT32, 
				max_reg,
				loadVar,
				loadDim,
				dim);
			optype_map[max_reg] = operand_type::FLOAT32_PTR;
		}
   		

		currentptr = ptrreg;
        if(attribute.type->getType() == BuiltinType::BuiltinKind::Int){
			auto resultreg = GetNewRegOperand(++max_reg);
            IRgenLoad(entrybb, 
                    BasicInstruction::LLVMType::I32, 
                    max_reg, 
                    currentptr);
            optype_map[max_reg] = operand_type::I32;
            currentop = resultreg;
        }else if(attribute.type->getType() == BuiltinType::BuiltinKind::Float){
            auto resultreg = GetNewRegOperand(++max_reg);
            IRgenLoad(entrybb, 
                    BasicInstruction::LLVMType::FLOAT32,
                    max_reg, 
                    currentptr);
            optype_map[max_reg] = operand_type::FLOAT32;
            currentop = resultreg;
        }else{
            currentop = currentptr;
        }
    }
}
void FuncRParams::codeIR() {
    isParam = 1;
	rParamsVec& paramsvec = irgen_table.FuncRParamsStack.top();
	std::vector<FuncFParam> fparamsvec = irgen_table.FuncFParamsStack.top();
	int paramsSz = fparamsvec.size();

	for(int i = 0; i < paramsSz; i++){
		ExprBase exp = rParams->at(i);
		auto fparam = fparamsvec[i];
		
		auto expType = exp->attribute.type->getType();
		auto fparamsType = fparam->type_decl->getType();

		exp->codeIR();

		// std::cerr << exp->attribute.type->checkPointer() << std::endl;
		// std::cerr << exp->attribute.type->getString() << std::endl;
		if (expType == BuiltinType::BuiltinKind::Float) {
			if (fparamsType == BuiltinType::BuiltinKind::Int) {
				BinaryFloattoIntConverse(GetCurrentBlock(), currentop);
				paramsvec.push_back(std::make_pair(BasicInstruction::LLVMType::I32, currentop));
			} else if (fparamsType == BuiltinType::BuiltinKind::Float) {
				paramsvec.push_back(std::make_pair(BasicInstruction::LLVMType::FLOAT32, currentop));
			}
		} else if (expType == BuiltinType::BuiltinKind::Int || expType == BuiltinType::BuiltinKind::Bool) {
			if (fparamsType == BuiltinType::BuiltinKind::Int || fparamsType == BuiltinType::BuiltinKind::Bool) {
				paramsvec.push_back(std::make_pair(BasicInstruction::LLVMType::I32, currentop));
			} else if (fparamsType == BuiltinType::BuiltinKind::Float) {
				BinaryInttoFloatConverse(GetCurrentBlock(), currentop);
				paramsvec.push_back(std::make_pair(BasicInstruction::LLVMType::FLOAT32, currentop));
			}
		} else {
			paramsvec.push_back(std::make_pair(BasicInstruction::LLVMType::PTR, currentop));
		}

	}

    isParam = 0;
}
void FuncCall::codeIR() {
	rParamsVec args;
	irgen_table.FuncRParamsStack.push(args);
	irgen_table.FuncFParamsStack.push(*semant_table.FunctionTable[name->getName()]->formals);
    if(funcRParams != nullptr){
        funcRParams->codeIR();
    }
    
    auto &entrybb = GetCurrentBlock(); 
	rParamsVec paramsvec = irgen_table.FuncRParamsStack.top();
    //std::cout<<"func_args_size= "<<paramsvec.size()<<"\n";
    if(attribute.type->getType() == BuiltinType::BuiltinKind::Void){
        IRgenCallVoid(entrybb, 
        BasicInstruction::LLVMType::VOID, paramsvec, name->getName());
    }else if(attribute.type->getType() == BuiltinType::BuiltinKind::Int){
        currentop = GetNewRegOperand(++max_reg);
        if(paramsvec.empty()){
            IRgenCallNoArgs(entrybb,
                    BasicInstruction::LLVMType::I32,
                    ((RegOperand*)currentop)->GetRegNo(),
                    name->getName());
            optype_map[max_reg] = operand_type::I32;
        }else{
            IRgenCall(entrybb,
                    BasicInstruction::LLVMType::I32,
                    ((RegOperand*)currentop)->GetRegNo(),
                    paramsvec,
                    name->getName());
            optype_map[max_reg] = operand_type::I32;
        }
    }else{
        currentop = GetNewRegOperand(++max_reg);
        if(paramsvec.empty()){
            IRgenCallNoArgs(entrybb,
                    BasicInstruction::LLVMType::FLOAT32,
                    ((RegOperand*)currentop)->GetRegNo(),
                    name->getName());
            optype_map[max_reg] = operand_type::FLOAT32;
        }else{
            IRgenCall(entrybb,
                    BasicInstruction::LLVMType::FLOAT32,
                    ((RegOperand*)currentop)->GetRegNo(),
                    paramsvec,
                    name->getName());
            optype_map[max_reg] = operand_type::FLOAT32;
        }
    }
	irgen_table.FuncRParamsStack.pop();
	irgen_table.FuncFParamsStack.pop();;
}

void UnaryExp::codeIR() {
	if(unaryOp.optype == OpType::Op::Sub || unaryOp.optype == OpType::Op::Add){
		GenUnaryAddSubIR(unaryOp.optype, attribute, unaryExp);
    } else {
		GenUnaryNotIR(attribute, unaryExp);
	}
}

void IntConst::codeIR() {
    currentint = val;
    if(!dimcount){
        auto newresultreg = GetNewRegOperand(++max_reg);
        auto &entrybb = GetCurrentBlock();
        IRgenArithmeticI32ImmAll(entrybb,
            BasicInstruction::LLVMIROpcode::ADD,
            val,
            0,
            max_reg);
        optype_map[max_reg] = operand_type::I32;
        currentop = newresultreg;
	}
}

void FloatConst::codeIR() {
    currentfloat = val;
    if(!dimcount){
        auto newresultreg = GetNewRegOperand(++max_reg);
        auto &entrybb = GetCurrentBlock();
        IRgenArithmeticF32ImmAll(entrybb,
            BasicInstruction::LLVMIROpcode::FADD,
            val,
            0,
            max_reg);
        optype_map[max_reg] = operand_type::FLOAT32;
        currentop = newresultreg;
    } 
}

void PrimaryExp::codeIR() {exp->codeIR();}

void AssignStmt::codeIR() {
    isRightlval = 0;
    exp->codeIR();
    auto tempop = currentop;
    isLeftlval = 1;
    lval->codeIR();
    
    isLeftlval = 0;
    isRightlval = 1;
    auto &entrybb = GetCurrentBlock();
	auto lvalType = lval->attribute.type->getType();
	auto expType = exp->attribute.type->getType();
	auto instType = (lvalType == BuiltinType::BuiltinKind::Float) ? BasicInstruction::LLVMType::FLOAT32 : BasicInstruction::LLVMType::I32;
    if(expType != lvalType && currentptr->GetOperandType() == BasicOperand::REG) {
		auto newresultreg_zext = GetNewRegOperand(++max_reg);
		IRgenTypeConverse(entrybb,
		expType,
		lvalType,
		max_reg,
		((RegOperand*)tempop)->GetRegNo());
		optype_map[max_reg] = operand_type::I32;
		tempop = newresultreg_zext;
	}
	IRgenStore(entrybb, instType, tempop, currentptr);
}

void ExprStmt::codeIR() {
	if(exp != nullptr) 
		exp->codeIR();
}

void BlockStmt::codeIR() {
	irgen_table.symbol_table.beginScope();
	irgen_table.symboldim_table.beginScope();
    b->codeIR();
	irgen_table.symbol_table.endScope();
	irgen_table.symboldim_table.endScope();
}

void IfStmt::codeIR() {
    max_label++;
    auto entrybb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    IRgenBRUnCond(llvmIR.function_block_map[funcdefI][max_label-1], entrybb->block_id);

    max_label++;
    auto truebb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    max_label++;
    auto falsebb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    truebbque.push_back(truebb);
    falsebbque.push_back(falsebb);
    
    genbb = entrybb;
    Cond->codeIR();
    genbb = nullptr;
    
    truebbque.pop_back();
    falsebbque.pop_back();

    auto icmpop = currentop;
    if(entrybb->Instruction_list.back()->GetOpcode() != BasicInstruction::ICMP && entrybb->Instruction_list.back()->GetOpcode() != BasicInstruction::FCMP){
        auto currentopno = ((RegOperand*)currentop)->GetRegNo();
        auto currentoptype = optype_map[currentopno];
        if(currentoptype == operand_type::I32){
            icmpop = GetNewRegOperand(++max_reg);
            IRgenIcmpImmRight(entrybb, 
                    BasicInstruction::ne,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)icmpop)->GetRegNo());
        }else{
            icmpop = GetNewRegOperand(++max_reg);
            IRgenFcmpImmRight(entrybb, 
                    BasicInstruction::ONE,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)icmpop)->GetRegNo());
        }
        
        optype_map[max_reg] = operand_type::BOOL;
    }
	if(elseStmt != nullptr) { // if else
		max_label++;
		auto ifstmtstartbb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
		ifStmt->codeIR();
		auto ifstmtendbb = llvmIR.function_block_map[funcdefI][max_label];

		max_label++;
		auto elsestmtstartbb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
		elseStmt->codeIR();
		auto elsestmtendbb = llvmIR.function_block_map[funcdefI][max_label];

		max_label++;
		auto exitbb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);

		IRgenBrCond(entrybb, ((RegOperand*)icmpop)->GetRegNo(), truebb->block_id, falsebb->block_id);
		IRgenBRUnCond(ifstmtendbb, exitbb->block_id);
		IRgenBRUnCond(elsestmtendbb, exitbb->block_id);


		if(truebb->Instruction_list.empty() || truebb->Instruction_list.back()->GetOpcode() != BasicInstruction::BR_UNCOND){
			IRgenBRUnCond(truebb, ifstmtstartbb->block_id);
		}   
		if(falsebb->Instruction_list.empty() || falsebb->Instruction_list.back()->GetOpcode() != BasicInstruction::BR_UNCOND){
			IRgenBRUnCond(falsebb, elsestmtstartbb->block_id);
		}
	} else { // if no else
		IRgenBrCond(entrybb, ((RegOperand*)icmpop)->GetRegNo(), truebb->block_id, falsebb->block_id);

		max_label++;
		auto stmtstartbb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
		ifStmt->codeIR();
		auto stmtendbb = llvmIR.function_block_map[funcdefI][max_label];
		
		max_label++;
		auto exitbb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
		
		if(truebb->Instruction_list.empty() || truebb->Instruction_list.back()->GetOpcode() != BasicInstruction::BR_UNCOND){
			IRgenBRUnCond(truebb, stmtstartbb->block_id);
		}
		if(falsebb->Instruction_list.empty() || falsebb->Instruction_list.back()->GetOpcode() != BasicInstruction::BR_UNCOND){
			IRgenBRUnCond(falsebb, exitbb->block_id);
		}
		
		IRgenBRUnCond(stmtendbb, exitbb->block_id);
	}
}

void WhileStmt::codeIR() {
	max_label++;
    auto entrybb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    IRgenBRUnCond(llvmIR.function_block_map[funcdefI][max_label-1], entrybb->block_id);
    max_label++;
    auto truebb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    max_label++;
    auto falsebb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    truebbque.push_back(truebb);
    falsebbque.push_back(falsebb);
    genbb = entrybb;
    Cond->codeIR();
    genbb = nullptr;

    truebbque.pop_back();
    falsebbque.pop_back();
    
    auto icmpop = currentop;
    if(entrybb->Instruction_list.back()->GetOpcode() != BasicInstruction::ICMP && entrybb->Instruction_list.back()->GetOpcode() != BasicInstruction::FCMP){
        auto currentopno = ((RegOperand*)currentop)->GetRegNo();
        auto currentoptype = optype_map[currentopno];
        if(currentoptype == operand_type::I32){
            icmpop = GetNewRegOperand(++max_reg);
            IRgenIcmpImmRight(entrybb, 
                    BasicInstruction::ne,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)icmpop)->GetRegNo());
        }else{
            icmpop = GetNewRegOperand(++max_reg);
            IRgenFcmpImmRight(entrybb, 
                    BasicInstruction::ONE,
                    ((RegOperand*)currentop)->GetRegNo(),
                    0,
                    ((RegOperand*)icmpop)->GetRegNo());
        }
        
        optype_map[max_reg] = operand_type::BOOL;
    }
    IRgenBrCond(entrybb, ((RegOperand*)icmpop)->GetRegNo(), truebb->block_id, falsebb->block_id);
    
    auto nowcontinueque_size = continuebbque.size();
    auto nowbreakque_size = breakbbque.size();
    max_label++;
    auto bodystartbb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    loopBody->codeIR();
    auto bodyendbb = llvmIR.function_block_map[funcdefI][max_label];

    max_label++;
    auto exitbb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    while(continuebbque.size() != nowcontinueque_size){
        auto nowbb = continuebbque.back();
        IRgenBRUnCond(nowbb, entrybb->block_id);
        continuebbque.pop_back();
    }
    while(breakbbque.size() != nowbreakque_size){
        auto nowbb = breakbbque.back();
        IRgenBRUnCond(nowbb, exitbb->block_id);
        breakbbque.pop_back();
    }
    if(!bodyendbb->Instruction_list.empty() && bodyendbb->Instruction_list.back()->GetOpcode() != BasicInstruction::RET){
        IRgenBRUnCond(bodyendbb, entrybb->block_id);
    }else if(!llvmIR.function_block_map[funcdefI][bodyendbb->block_id-1]->Instruction_list.empty() 
            && llvmIR.function_block_map[funcdefI][bodyendbb->block_id-1]->Instruction_list.back()->GetOpcode() != BasicInstruction::RET){
        IRgenBRUnCond(bodyendbb, entrybb->block_id);
    }
    if(truebb->Instruction_list.empty() || truebb->Instruction_list.back()->GetOpcode() != BasicInstruction::BR_UNCOND){
        IRgenBRUnCond(truebb, bodystartbb->block_id);
    }
    if(falsebb->Instruction_list.empty() || falsebb->Instruction_list.back()->GetOpcode() != BasicInstruction::BR_UNCOND){
        IRgenBRUnCond(falsebb, exitbb->block_id);
    }
}

void ContinueStmt::codeIR() {
	auto &entrybb = GetCurrentBlock();
    continuebbque.push_back(entrybb);
    max_label++;
    llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
}

void BreakStmt::codeIR() {
	auto &entrybb = GetCurrentBlock();
    breakbbque.push_back(entrybb);
    max_label++;
    llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
}

void RetStmt::codeIR() {
	if(retExp == nullptr) {
		auto &entrybb = GetCurrentBlock();
    	IRgenRetVoid(entrybb);
		return;
	}

    retExp->codeIR();
    auto &retbb = GetCurrentBlock();
	
	auto ty = attribute.type->getType();
	// std::cout << attribute.type->getString() << std::endl;
    if(ty == BuiltinType::BuiltinKind::Void){
        IRgenRetVoid(retbb);
    }else if(ty == BuiltinType::BuiltinKind::Int){
        auto currentopno = ((RegOperand*)currentop)->GetRegNo();
        if(optype_map[currentopno] == operand_type::BOOL){
            BinaryBoolCheckandConverse(retbb, currentop);
            currentopno = ((RegOperand*)currentop)->GetRegNo();
        }else if(optype_map[currentopno] == operand_type::FLOAT32){
            BinaryFloattoIntConverse(retbb, currentop);
            currentopno = ((RegOperand*)currentop)->GetRegNo();
        }
        IRgenRetReg(retbb, BasicInstruction::LLVMType::I32, currentopno);
    }else{
        auto currentopno = ((RegOperand*)currentop)->GetRegNo();
        if(optype_map[currentopno] == operand_type::BOOL){
            BinaryBoolCheckandConverse(retbb, currentop);
            BinaryInttoFloatConverse(retbb, currentop);
            currentopno = ((RegOperand*)currentop)->GetRegNo();
        }else if(optype_map[currentopno] == operand_type::I32){
            BinaryInttoFloatConverse(retbb, currentop);
            currentopno = ((RegOperand*)currentop)->GetRegNo();
        }
        IRgenRetReg(retbb, BasicInstruction::LLVMType::FLOAT32, currentopno);
    }
}

void ConstInitValList::codeIR() {
	int tail = 1;
	int subArraydim_copy;
	if(isTopVarInitVal) {
		isTopVarInitVal = false;
		for(int i = initdim.size() - 1; i >= 0; i--) {
			tail *= initdim[i];
		}
		tail--;
		subArraydim_copy = subArraydim = 1;
	} else {
		tail = getTail();
		subArraydim_copy = subArraydim;
	}

	if(initval != nullptr){
		for(auto init : *initval){
			if(head >= tail + 1) {
				break;
			}
			subArraydim = subArraydim_copy;
			init->codeIR();
		}
	}

	while(head <= tail) {
		head++;
		initarrayint.push_back(0);
		initarrayfloat.push_back(0.0);
	}
}

void ConstInitVal::codeIR() {
	if(dimcount && isGobal) {
		initarrayint.push_back(exp->attribute.val.IntVal);
		initarrayfloat.push_back(exp->attribute.val.FloatVal);
		head++;
		return;
	}

	exp->codeIR();

	if(dimcount && exp->attribute.ConstTag){  // constVal
		auto ty = attribute.type->getType();
		if (ty == BuiltinType::BuiltinKind::Int) {
			std::vector<Operand> indexs = offset_to_indexs(head, initdim);
			Operand elementPtrReg = GetNewRegOperand(++max_reg);
			IRgenGetElementptrIndexI32(GetCurrentBlock(), 
			BasicInstruction::LLVMType::I32, 
			max_reg,
			initarrayReg,
			initdim,
			indexs);
			optype_map[max_reg] = operand_type::I32_PTR;

			Operand storeValReg = GetNewRegOperand(++max_reg); 
			IRgenArithmeticI32ImmAll(GetCurrentBlock(),
			BasicInstruction::LLVMIROpcode::ADD,
			exp->attribute.val.IntVal,
			0,
			max_reg);
			optype_map[max_reg] = operand_type::I32;

			IRgenStore(GetCurrentBlock(), BasicInstruction::LLVMType::I32, storeValReg, elementPtrReg);	
		} else if (ty == BuiltinType::BuiltinKind::Float) {
			std::vector<Operand> indexs = offset_to_indexs(head, initdim);
			Operand elementPtrReg = GetNewRegOperand(++max_reg);
			IRgenGetElementptrIndexI32(GetCurrentBlock(), 
			BasicInstruction::LLVMType::FLOAT32, 
			max_reg,
			initarrayReg,
			initdim,
			indexs);
			optype_map[max_reg] = operand_type::FLOAT32_PTR;

			Operand storeValReg = GetNewRegOperand(++max_reg); 
			IRgenArithmeticF32ImmAll(GetCurrentBlock(),
			BasicInstruction::LLVMIROpcode::FADD,
			exp->attribute.val.FloatVal,
			0,
			max_reg);
			optype_map[max_reg] = operand_type::FLOAT32;

			IRgenStore(GetCurrentBlock(), BasicInstruction::LLVMType::FLOAT32, storeValReg, elementPtrReg);	
		}

		head++;
	}
}

void VarInitValList::codeIR() {
	int tail = 1;
	int subArraydim_copy;
	if(isTopVarInitVal) {
		isTopVarInitVal = false;
		for(int i = initdim.size() - 1; i >= 0; i--) {
			tail *= initdim[i];
		}
		tail--;
		subArraydim_copy = subArraydim = 1;
	} else {
		tail = getTail();
		subArraydim_copy = subArraydim;
	}
	
	if(initval != nullptr){
		for(auto init : *initval){
			if(head >= tail + 1) {
				break;
			}
			subArraydim = subArraydim_copy;
			init->codeIR();
		}
	}

	while(head <= tail) {
		head++;
		initarrayint.push_back(0);
		initarrayfloat.push_back(0.0);
	}
}

void VarInitVal::codeIR() {
	if(dimcount && isGobal) {
		initarrayint.push_back(exp->attribute.val.IntVal);
		initarrayfloat.push_back(exp->attribute.val.FloatVal);
		head++;
		return;
	}

	exp->codeIR();

	auto ty = attribute.type->getType();
	if(dimcount && exp->attribute.ConstTag){  // constVal (维护数组的常量 by semant.cc)
		if (ty == BuiltinType::BuiltinKind::Int) {
			std::vector<Operand> indexs = offset_to_indexs(head, initdim);
			Operand elementPtrReg = GetNewRegOperand(++max_reg);
			IRgenGetElementptrIndexI32(GetCurrentBlock(), 
			BasicInstruction::LLVMType::I32, 
			max_reg,
			initarrayReg,
			initdim,
			indexs);
			optype_map[max_reg] = operand_type::I32_PTR;

			Operand storeValReg = GetNewRegOperand(++max_reg); 
			IRgenArithmeticI32ImmAll(GetCurrentBlock(),
			BasicInstruction::LLVMIROpcode::ADD,
			exp->attribute.val.IntVal,
			0,
			max_reg);
			optype_map[max_reg] = operand_type::I32;

			IRgenStore(GetCurrentBlock(), BasicInstruction::LLVMType::I32, storeValReg, elementPtrReg);
		} else if (ty == BuiltinType::BuiltinKind::Float) {
			std::vector<Operand> indexs = offset_to_indexs(head, initdim);
			Operand elementPtrReg = GetNewRegOperand(++max_reg);
			IRgenGetElementptrIndexI32(GetCurrentBlock(), 
			BasicInstruction::LLVMType::FLOAT32, 
			max_reg,
			initarrayReg,
			initdim,
			indexs);
			optype_map[max_reg] = operand_type::FLOAT32_PTR;

			Operand storeValReg = GetNewRegOperand(++max_reg); 
			IRgenArithmeticF32ImmAll(GetCurrentBlock(),
			BasicInstruction::LLVMIROpcode::FADD,
			exp->attribute.val.FloatVal,
			0,
			max_reg);
			optype_map[max_reg] = operand_type::FLOAT32;

			IRgenStore(GetCurrentBlock(), BasicInstruction::LLVMType::FLOAT32, storeValReg, elementPtrReg);
		}
		
		head++;

	} else if(dimcount) { // Lval

		if (ty == BuiltinType::BuiltinKind::Int) {
			Operand storeValReg = GetNewRegOperand(max_reg); 

			std::vector<Operand> indexs = offset_to_indexs(head, initdim);
			Operand elementPtrReg = GetNewRegOperand(++max_reg);
			IRgenGetElementptrIndexI32(GetCurrentBlock(), 
			BasicInstruction::LLVMType::I32, 
			max_reg,
			initarrayReg,
			initdim,
			indexs);
			optype_map[max_reg] = operand_type::I32_PTR;

			IRgenStore(GetCurrentBlock(), BasicInstruction::LLVMType::I32, storeValReg, elementPtrReg);
		} else if (ty == BuiltinType::BuiltinKind::Float) {
			Operand storeValReg = GetNewRegOperand(max_reg); 

			std::vector<Operand> indexs = offset_to_indexs(head, initdim);
			Operand elementPtrReg = GetNewRegOperand(++max_reg);
			IRgenGetElementptrIndexI32(GetCurrentBlock(), 
			BasicInstruction::LLVMType::FLOAT32, 
			max_reg,
			initarrayReg,
			initdim,
			indexs);
			optype_map[max_reg] = operand_type::FLOAT32_PTR;

			IRgenStore(GetCurrentBlock(), BasicInstruction::LLVMType::FLOAT32, storeValReg, elementPtrReg);
		} 

        head++;
	}
}
void VarDef_no_init::codeIR() {
	// global var
	auto ty = attribute.type->getType();
    if(irgen_table.symbol_table.get_current_scope() == 0)
	{
        auto varinfo = semant_table.GlobalTable[name];

        if(dims == nullptr){

            ptrmap[name] = GetNewGlobalOperand(name->getName());
            if(ty == BuiltinType::BuiltinKind::Int){
                IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::I32, new ImmI32Operand(0),false,false);
            }else{
                IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::FLOAT32, new ImmF32Operand(0),false,false);
            }
        }else{
            ptrmap[name] = GetNewGlobalOperand(name->getName());
            dimcount = 1;
            std::vector<int> dim;
            for(auto exp : *dims){
                exp->codeIR();
                dim.push_back(currentint);
            }
			irgen_table.symboldim_table.enter(name, dim);
            dimcount = 0; 
            if(ty == BuiltinType::BuiltinKind::Int){
                IRgenGlobalVarDefineArray(name->getName(), BasicInstruction::LLVMType::I32, VarAttribute(dim, std::vector<int>({})),false);
            }else{
                IRgenGlobalVarDefineArray(name->getName(), BasicInstruction::LLVMType::FLOAT32, VarAttribute(dim,std::vector<int>({})),false);
            }
        }

    } else {
        auto entrybb = llvmIR.function_block_map[funcdefI][0];
        auto type = ty;
        ptrmap[name] = GetNewRegOperand(++max_reg);
		irgen_table.symbol_table.enter(name, max_reg);

        if(dims == nullptr){
			auto &gbb = GetCurrentBlock();
 			ptrmap[name] = GetNewRegOperand(++max_reg);//result reg
 			irgen_table.symbol_table.enter(name, max_reg);
            if(ty == BuiltinType::BuiltinKind::Int){
                IRgenAlloca(entrybb, BasicInstruction::I32, max_reg);
                optype_map[max_reg] = operand_type::I32_PTR;
				IRgenArithmeticI32ImmAll(gbb,BasicInstruction::LLVMIROpcode::ADD,0,0,++max_reg);
				IRgenStore(gbb, BasicInstruction::LLVMType::I32, GetNewRegOperand(max_reg),ptrmap[name]);
            }else{
                IRgenAlloca(entrybb, BasicInstruction::FLOAT32, max_reg);
                optype_map[max_reg] = operand_type::FLOAT32_PTR;
				IRgenArithmeticF32ImmAll(gbb,BasicInstruction::LLVMIROpcode::FADD,0,0,++max_reg);
				IRgenStore(gbb, BasicInstruction::LLVMType::FLOAT32, GetNewRegOperand(max_reg), ptrmap[name]);
            }
        }else{
            dimcount = 1;
            std::vector<int> dim;
            for(auto exp : *dims){
				dim.push_back(exp->attribute.val.IntVal);
            }
            if(type == BuiltinType::BuiltinKind::Float){
                optype_map[max_reg] = operand_type::FLOAT32_PTR;
                IRgenAllocaArray(entrybb, BasicInstruction::LLVMType::FLOAT32, ((RegOperand*)ptrmap[name])->GetRegNo(), dim);
            }else{
                optype_map[max_reg] = operand_type::I32_PTR;
                IRgenAllocaArray(entrybb, BasicInstruction::LLVMType::I32, ((RegOperand*)ptrmap[name])->GetRegNo(), dim);
            }
            dimcount = 0;

			irgen_table.symboldim_table.enter(name, dim);
        }
    }	
}
void VarDef::codeIR() {

    auto ty = attribute.type->getType();
	auto instType = (ty == BuiltinType::BuiltinKind::Int) ? BasicInstruction::LLVMType::I32 : BasicInstruction::LLVMType::FLOAT32;

	// global var
    if (irgen_table.symbol_table.get_current_scope() == 0)
	{	
        
		auto varinfo = semant_table.GlobalTable[name];
        ptrmap[name] = GetNewGlobalOperand(name->getName());
		
        if(dims == nullptr){
            if(ty == BuiltinType::BuiltinKind::Int){
                if(varinfo.IntInitVals.empty()){
                    IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::I32, new ImmI32Operand(varinfo.FloatInitVals.front()),false,true);
                }else{
                    IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::I32, new ImmI32Operand(varinfo.IntInitVals.front()),false,true);
                }
            }else{
                if(varinfo.FloatInitVals.empty()){
                    IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::FLOAT32, new ImmF32Operand(varinfo.IntInitVals.front()),false,true);
                }else{
                    IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::FLOAT32, new ImmF32Operand(varinfo.FloatInitVals.front()),false,true);
                }
            }
        }else{
            dimcount = 1;
			isGobal = true;

			initdim.clear();
			int arraySize = 1;
            for(auto exp : *dims){
                exp->codeIR();
				initdim.push_back(currentint);
				arraySize *= currentint;
            }
			irgen_table.symboldim_table.enter(name, initdim);

			if(ty == BuiltinType::BuiltinKind::Int){
                initarrayint.clear();
            }else{
                initarrayfloat.clear();
            }

			head = 0;
			isTopVarInitVal = true;
            init->codeIR();
			
            dimcount = 0;
			isGobal = false;

            if(ty == BuiltinType::BuiltinKind::Int){
				while(initarrayint.size() < arraySize) initarrayint.push_back(0);
                IRgenGlobalVarDefineArray(name->getName(), BasicInstruction::LLVMType::I32, VarAttribute(initdim,initarrayint),false);
            }else{
				while(initarrayfloat.size() < arraySize) initarrayfloat.push_back(0.0);
                IRgenGlobalVarDefineArray(name->getName(), BasicInstruction::LLVMType::FLOAT32, VarAttribute(initdim,initarrayfloat),false);
            }

        }        
        
    } else {
        if(dims == nullptr){
			auto entrybb = llvmIR.function_block_map[funcdefI][0];
			ptrmap[name] = GetNewRegOperand(++max_reg);
			irgen_table.symbol_table.enter(name, max_reg);
			IRgenAlloca(entrybb, instType, max_reg);
			if(ty == BuiltinType::BuiltinKind::Float){
                optype_map[max_reg] = operand_type::FLOAT32_PTR;
            }else{
                optype_map[max_reg] = operand_type::I32_PTR;
            }
            init->codeIR();
            auto &gbb = GetCurrentBlock();

            if(ty == BuiltinType::BuiltinKind::Int){
                bool PtrisFloat = 0;
                if(ptrmap[name]->GetOperandType() != BasicOperand::REG){}
				else{
                    PtrisFloat = optype_map[((RegOperand*)currentop)->GetRegNo()] == operand_type::FLOAT32;
                }
                if(PtrisFloat){
                    BinaryFloattoIntConverse(gbb, currentop);
                }
                IRgenStore(gbb, BasicInstruction::LLVMType::I32, currentop, ptrmap[name]);
            }else{
                bool PtrisINT = 0;
                if(ptrmap[name]->GetOperandType() != BasicOperand::REG){}
				else{
                    
                    PtrisINT = optype_map[((RegOperand*)currentop)->GetRegNo()] == operand_type::I32;
                }
                if(PtrisINT){
                    BinaryInttoFloatConverse(gbb, currentop);
                }
                IRgenStore(gbb, BasicInstruction::LLVMType::FLOAT32, currentop, ptrmap[name]);
            }

        } else {
			dimcount = 1;
			initdim.clear();
			int arraySize = 1;
            for(auto exp : *dims){
                exp->codeIR();
                initdim.push_back(currentint);
				arraySize *= currentint;
            }
			
			irgen_table.symboldim_table.enter(name, initdim);
			
			if(ty == BuiltinType::BuiltinKind::Int){
                initarrayint.clear();
            }else{
                initarrayfloat.clear();
            }

			auto entrybb = llvmIR.function_block_map[funcdefI][0];
			ptrmap[name] = GetNewRegOperand(++max_reg);
			initarrayReg = ptrmap[name];
			irgen_table.symbol_table.enter(name, max_reg);
            if(ty == BuiltinType::BuiltinKind::Int){
                optype_map[max_reg] = operand_type::I32_PTR;
                IRgenAllocaArray(entrybb, BasicInstruction::LLVMType::I32, max_reg, initdim);
            }else{
                optype_map[max_reg] = operand_type::FLOAT32_PTR;
                IRgenAllocaArray(entrybb, BasicInstruction::LLVMType::FLOAT32, max_reg, initdim);
            }
			
			// args: addr, fillVal, fillLen, isvolatile
			std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args;
			args.push_back(std::pair<enum BasicInstruction::LLVMType, Operand>(BasicInstruction::LLVMType::PTR, GetNewRegOperand(max_reg))); 
			args.push_back(std::pair<enum BasicInstruction::LLVMType, Operand>(BasicInstruction::LLVMType::I8, new ImmI32Operand(0))); 
			args.push_back(std::pair<enum BasicInstruction::LLVMType, Operand>(BasicInstruction::LLVMType::I32, new ImmI32Operand(arraySize * sizeof(int)))); 
			args.push_back(std::pair<enum BasicInstruction::LLVMType, Operand>(BasicInstruction::LLVMType::I1, new ImmI32Operand(0)));   
			CallInstruction *memsetCall = new CallInstruction(BasicInstruction::LLVMType::VOID, nullptr, std::string("llvm.memset.p0.i32"), args);
			llvmIR.function_block_map[funcdefI][max_label]->InsertInstruction(1, memsetCall);

			head = 0;
			isTopVarInitVal = true;
            init->codeIR();
			dimcount = 0;
        }        
    }
}
void ConstDef::codeIR() {
    auto ty = attribute.type->getType();
	auto instType = (ty == BuiltinType::BuiltinKind::Int) ? BasicInstruction::LLVMType::I32 : BasicInstruction::LLVMType::FLOAT32;
	
	// global var
    if (irgen_table.symbol_table.get_current_scope() == 0)
	{
		auto varinfo = semant_table.GlobalTable[name];
        ptrmap[name] = GetNewGlobalOperand(name->getName());
		
        if(dims == nullptr){
            if(ty == BuiltinType::BuiltinKind::Int){
                if(varinfo.IntInitVals.empty()){
                    IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::I32, new ImmI32Operand(varinfo.FloatInitVals.front()),true,true);
                }else{
                    IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::I32, new ImmI32Operand(varinfo.IntInitVals.front()),true,true);
                }
                
            }else{
                if(varinfo.FloatInitVals.empty()){
                    IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::FLOAT32, new ImmF32Operand(varinfo.IntInitVals.front()),true,true);
                }else{
                    IRgenGlobalVarDefine(name->getName(), BasicInstruction::LLVMType::FLOAT32, new ImmF32Operand(varinfo.FloatInitVals.front()),true,true);
                }
            }
        }else{
			isGobal = true;
			isTopVarInitVal = true;
            dimcount = 1;
			head = 0;
			initdim.clear();
			int arraySize = 1;
            for(auto exp : *dims){
                exp->codeIR();
				initdim.push_back(currentint);
				arraySize *= currentint;
            }
			irgen_table.symboldim_table.enter(name, initdim);

			if(ty == BuiltinType::BuiltinKind::Int){
                initarrayint.clear();
            }else{
                initarrayfloat.clear();
            }
		
            init->codeIR();
            dimcount = 0;
			isGobal = false;
            if(ty == BuiltinType::BuiltinKind::Int){
				while(initarrayint.size() < arraySize) initarrayint.push_back(0);
                IRgenGlobalVarDefineArray(name->getName(), BasicInstruction::LLVMType::I32, VarAttribute(initdim,initarrayint),true);
            }else{
				while(initarrayfloat.size() < arraySize) initarrayfloat.push_back(0.0);
                IRgenGlobalVarDefineArray(name->getName(), BasicInstruction::LLVMType::FLOAT32, VarAttribute(initdim,initarrayfloat),true);
            }
        }        
        
    } else {
        if(dims == nullptr){
			auto entrybb = llvmIR.function_block_map[funcdefI][0];
			ptrmap[name] = GetNewRegOperand(++max_reg);
			irgen_table.symbol_table.enter(name, max_reg);
			IRgenAlloca(entrybb, instType, max_reg);
            if(ty == BuiltinType::BuiltinKind::Int){
                optype_map[max_reg] = operand_type::I32_PTR;
            }else{
                optype_map[max_reg] = operand_type::FLOAT32_PTR;
            }
			init->codeIR();
			if(ty == BuiltinType::BuiltinKind::Int){
                optype_map[max_reg] = operand_type::I32;
				IRgenStore(llvmIR.function_block_map[funcdefI][max_label], BasicInstruction::LLVMType::I32, currentop, ptrmap[name]);
			}else{
				IRgenStore(llvmIR.function_block_map[funcdefI][max_label], BasicInstruction::LLVMType::FLOAT32, currentop, ptrmap[name]);
			}
        }else{
			dimcount = 1;
			initdim.clear();
			int arraySize = 1;
            for(auto exp : *dims){
                exp->codeIR();
                initdim.push_back(currentint);
				arraySize *= currentint;
            }

			irgen_table.symboldim_table.enter(name, initdim);
			
			if(ty == BuiltinType::BuiltinKind::Int){
                initarrayint.clear();
            }else{
                initarrayfloat.clear();
            }

			auto entrybb = llvmIR.function_block_map[funcdefI][0];
			ptrmap[name] = GetNewRegOperand(++max_reg);
			initarrayReg = ptrmap[name];
			irgen_table.symbol_table.enter(name, max_reg);
			IRgenAllocaArray(entrybb, instType, max_reg, initdim);

			// 局部变量初始化为 0
			std::vector<std::pair<enum BasicInstruction::LLVMType, Operand>> args;
			args.push_back(std::pair<enum BasicInstruction::LLVMType, Operand>(BasicInstruction::LLVMType::PTR, GetNewRegOperand(max_reg))); 
			args.push_back(std::pair<enum BasicInstruction::LLVMType, Operand>(BasicInstruction::LLVMType::I8, new ImmI32Operand(0))); 
			args.push_back(std::pair<enum BasicInstruction::LLVMType, Operand>(BasicInstruction::LLVMType::I32, new ImmI32Operand(arraySize * sizeof(int)))); 
			args.push_back(std::pair<enum BasicInstruction::LLVMType, Operand>(BasicInstruction::LLVMType::I1, new ImmI32Operand(0)));   
			CallInstruction *memsetCall = new CallInstruction(BasicInstruction::LLVMType::VOID, nullptr, std::string("llvm.memset.p0.i32"), args);
			llvmIR.function_block_map[funcdefI][max_label]->InsertInstruction(1, memsetCall);

			head = 0;
			isTopVarInitVal = true;
            init->codeIR();
			dimcount = 0;
        }        
    }

}
void VarDecl::codeIR() {
	for(auto def : *var_def_list){
        def->codeIR();
    }
}

void ConstDecl::codeIR() {
    for(auto def : *var_def_list){
        def->codeIR();
    }
}

void BlockItem_Decl::codeIR() { decl->codeIR();}
void BlockItem_Stmt::codeIR() { stmt->codeIR();}
void __Block::codeIR() {
	irgen_table.symbol_table.beginScope();
	irgen_table.symboldim_table.beginScope();
    int caaa = 0;
    for(auto blockitem : *item_list){
        blockitem->codeIR();
    }
	irgen_table.symbol_table.endScope();
	irgen_table.symboldim_table.endScope();
}
void __FuncFParam::codeIR() {
	initdim.clear();
    dimcount = 1;
    
	for(auto exp : *dims){
        if(exp != nullptr){
            exp->codeIR();
            initdim.push_back(currentint);
        }
    }
	irgen_table.symboldim_table.enter(name, initdim);
    dimcount = 0;
}
void __FuncDef::codeIR() {
	irgen_table.symbol_table.beginScope();
	irgen_table.symboldim_table.beginScope();
    if(return_type->getType() == BuiltinType::BuiltinKind::Int){
        funcdefI = new FunctionDefineInstruction(BasicInstruction::I32,name->getName());
    }else if(return_type->getType() == BuiltinType::BuiltinKind::Float){
        funcdefI = new FunctionDefineInstruction(BasicInstruction::FLOAT32,name->getName());
    }else{
        funcdefI = new FunctionDefineInstruction(BasicInstruction::VOID,name->getName());
    }

    max_reg = -1;
    max_label = -1;
    paramptr.clear();
    optype_map.clear();

    std::deque<Instruction> allocaIque;
    std::deque<Instruction> storeIque;
    for(auto para : *formals){
        auto parareg = GetNewRegOperand(++max_reg);
        ptrmap[para->name] = parareg;
    }
    for(auto para : *formals){
        auto parareg = ptrmap[para->name];
        if(para->attribute.type->getType() == BuiltinType::BuiltinKind::Int){
            funcdefI->InsertFormal(BasicInstruction::I32);
            auto allocaptr = GetNewRegOperand(++max_reg);
            allocaIque.push_back(new AllocaInstruction(BasicInstruction::I32,allocaptr));
            storeIque.push_back(new StoreInstruction(BasicInstruction::I32, allocaptr, parareg));
            ptrmap[para->name] = allocaptr;
		    irgen_table.symbol_table.enter(para->name, max_reg);
        }else if(para->attribute.type->getType() == BuiltinType::BuiltinKind::Float){
            funcdefI->InsertFormal(BasicInstruction::FLOAT32);
            auto allocaptr = GetNewRegOperand(++max_reg);
            allocaIque.push_back(new AllocaInstruction(BasicInstruction::FLOAT32,allocaptr));
            storeIque.push_back(new StoreInstruction(BasicInstruction::FLOAT32, allocaptr, parareg));
            ptrmap[para->name] = allocaptr;
            irgen_table.symbol_table.enter(para->name, max_reg);
        }else{
            funcdefI->InsertFormal(BasicInstruction::PTR);
            irgen_table.symbol_table.enter(para->name, ((RegOperand*)parareg)->GetRegNo());
            para->codeIR();
        }
    }
    llvmIR.NewFunction(funcdefI);
    
    ++max_label;
    auto entrybb = llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    while(!allocaIque.empty()){
        entrybb->InsertInstruction(1,allocaIque.back());
        allocaIque.pop_back();
    }
    while(!storeIque.empty()){
        entrybb->InsertInstruction(1,storeIque.back());
        storeIque.pop_back();
    }
    ++max_label;
    llvmIR.function_block_map[funcdefI][max_label] = llvmIR.NewBlock(funcdefI,max_label);
    IRgenBRUnCond(llvmIR.function_block_map[funcdefI][max_label-1], max_label);
    
    genbb = llvmIR.function_block_map[funcdefI][max_label];
    block->codeIR();
    auto &exitbb = llvmIR.function_block_map[funcdefI][max_label];
    if(return_type->getType() == BuiltinType::BuiltinKind::Void){
        IRgenRetVoid(exitbb);
    }else if(return_type->getType() == BuiltinType::BuiltinKind::Int){
        auto newresultreg = GetNewRegOperand(++max_reg);
        IRgenArithmeticI32ImmAll(exitbb,
                BasicInstruction::LLVMIROpcode::ADD, 
                0,
                0,max_reg);
        optype_map[max_reg] = operand_type::I32;
        IRgenRetReg(exitbb, BasicInstruction::LLVMType::I32, ((RegOperand*)newresultreg)->GetRegNo());
    }else{
        auto newresultreg = GetNewRegOperand(++max_reg);
        IRgenArithmeticF32ImmAll(exitbb,
                BasicInstruction::LLVMIROpcode::FADD, 
                0,
                0,max_reg);
        optype_map[max_reg] = operand_type::FLOAT32;
        IRgenRetReg(exitbb, BasicInstruction::LLVMType::FLOAT32, ((RegOperand*)newresultreg)->GetRegNo());

    }
    
    for(int i = 0; i <= max_label; ++i){
        if(llvmIR.function_block_map[funcdefI].find(i) !=  llvmIR.function_block_map[funcdefI].end() && llvmIR.function_block_map[funcdefI][i]->Instruction_list.empty()){
            llvmIR.function_block_map[funcdefI].erase(i);
        }
    }
    
    llvmIR.function_max_reg[funcdefI] = max_reg;
    llvmIR.function_max_label[funcdefI] = max_label;
    llvmIR.FunctionNameTable[funcdefI->GetFunctionName()] = funcdefI;//FunctionInlinePass

    irgen_table.symbol_table.endScope();
    irgen_table.symboldim_table.endScope();
}
void CompUnit_Decl::codeIR() { decl->codeIR();}
void CompUnit_FuncDef::codeIR() { func_def->codeIR();}
void __Program::codeIR() {
	irgen_table.symbol_table.beginScope();
	irgen_table.symboldim_table.beginScope();
    AddLibFunctionDeclare();
    auto comp_vector = *comp_list;
    for (auto comp : comp_vector) {
        comp->codeIR();
    }
}

void AddLibFunctionDeclare() {
    FunctionDeclareInstruction *getint = new FunctionDeclareInstruction(BasicInstruction::I32, "getint");
    llvmIR.function_declare.push_back(getint);

    FunctionDeclareInstruction *getchar = new FunctionDeclareInstruction(BasicInstruction::I32, "getch");
    llvmIR.function_declare.push_back(getchar);

    FunctionDeclareInstruction *getfloat = new FunctionDeclareInstruction(BasicInstruction::FLOAT32, "getfloat");
    llvmIR.function_declare.push_back(getfloat);

    FunctionDeclareInstruction *getarray = new FunctionDeclareInstruction(BasicInstruction::I32, "getarray");
    getarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(getarray);

    FunctionDeclareInstruction *getfloatarray = new FunctionDeclareInstruction(BasicInstruction::I32, "getfarray");
    getfloatarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(getfloatarray);

    FunctionDeclareInstruction *putint = new FunctionDeclareInstruction(BasicInstruction::VOID, "putint");
    putint->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(putint);

    FunctionDeclareInstruction *putch = new FunctionDeclareInstruction(BasicInstruction::VOID, "putch");
    putch->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(putch);

    FunctionDeclareInstruction *putfloat = new FunctionDeclareInstruction(BasicInstruction::VOID, "putfloat");
    putfloat->InsertFormal(BasicInstruction::FLOAT32);
    llvmIR.function_declare.push_back(putfloat);

    FunctionDeclareInstruction *putarray = new FunctionDeclareInstruction(BasicInstruction::VOID, "putarray");
    putarray->InsertFormal(BasicInstruction::I32);
    putarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(putarray);

    FunctionDeclareInstruction *putfarray = new FunctionDeclareInstruction(BasicInstruction::VOID, "putfarray");
    putfarray->InsertFormal(BasicInstruction::I32);
    putfarray->InsertFormal(BasicInstruction::PTR);
    llvmIR.function_declare.push_back(putfarray);

    FunctionDeclareInstruction *starttime = new FunctionDeclareInstruction(BasicInstruction::VOID, "_sysy_starttime");
    starttime->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(starttime);

    FunctionDeclareInstruction *stoptime = new FunctionDeclareInstruction(BasicInstruction::VOID, "_sysy_stoptime");
    stoptime->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(stoptime);

    // 一些llvm自带的函数，也许会为你的优化提供帮助
    FunctionDeclareInstruction *llvm_memset =
    new FunctionDeclareInstruction(BasicInstruction::VOID, "llvm.memset.p0.i32");
    llvm_memset->InsertFormal(BasicInstruction::PTR);
    llvm_memset->InsertFormal(BasicInstruction::I8);
    llvm_memset->InsertFormal(BasicInstruction::I32);
    llvm_memset->InsertFormal(BasicInstruction::I1);
    llvmIR.function_declare.push_back(llvm_memset);

    FunctionDeclareInstruction *llvm_umax = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.umax.i32");
    llvm_umax->InsertFormal(BasicInstruction::I32);
    llvm_umax->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_umax);

    FunctionDeclareInstruction *llvm_umin = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.umin.i32");
    llvm_umin->InsertFormal(BasicInstruction::I32);
    llvm_umin->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_umin);

    FunctionDeclareInstruction *llvm_smax = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.smax.i32");
    llvm_smax->InsertFormal(BasicInstruction::I32);
    llvm_smax->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_smax);

    FunctionDeclareInstruction *llvm_smin = new FunctionDeclareInstruction(BasicInstruction::I32, "llvm.smin.i32");
    llvm_smin->InsertFormal(BasicInstruction::I32);
    llvm_smin->InsertFormal(BasicInstruction::I32);
    llvmIR.function_declare.push_back(llvm_smin);
}