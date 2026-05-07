#include "SemanticAnalyzer.h"
#include<stack>
#include<algorithm>
#include<cmath>
#include<unordered_map>

static std::unordered_map<std::string, int>declmap;

ASTContext* stx;

//���ű���һ�����⣬���﷨���������Ժ����޷��پ���ͬ����˳��ȥ����һ�η��ű��������������׶Σ���Կ��ܳ��ֵĺ���������飬�洢��������
std::map<std::string, FuncType*> FuncMap;
//��ǰ���ڷ����ĺ�����������֤return���
static std::string CurrentFunc;
//�����Ƿ���main����
bool hasMain = false;
//�жϵ�ǰ�Ƿ��������ж����֮��
bool isInCondition = false;
//�жϵ�ǰ�Ƿ���ȫ��
bool isInGlobal = true;

//�жϵ�ǰ�������Ƿ�ȫ���ǳ���(��������ʱ��)
bool isAllConst = true;

//��Ҫ�ͷŵ�
std::vector<ValType*>ToDelete;

//��������׶Σ�����ÿһ���ڵ㣬���Ǹ���һЩ���������Խ�����֤���޸ĸýڵ�

//***************���ߺ���************************//

ValType* addNewValType()
{
	ValType* valtype = new ValType();
	ToDelete.push_back(valtype);
	return valtype;
}

//*********��ʽ����ת��***********//

//������ʽ����ת���ĺ�����ֻ����IntתFloat
static std::shared_ptr<ExprAST> AddImplicitCastForOp(ValType* valtype,std::shared_ptr<ExprAST> Expr)
{
	ValType* FromType = new ValType();
	ValType* ToType = new ValType();
	FromType->setType(new IntType());
	ToType->setType(new FloatType());
	std::shared_ptr<ImplCastExprAST>impl = std::make_shared<ImplCastExprAST>(FromType, ToType, std::move(Expr));
	impl->CastString = "%i32Tf32";
	if (valtype->isConstQualified()) {
		impl->ToType->setConst();
		impl->FromType->setConst();
	}
	return impl;
}

//���ȺŴ�������ת��
static std::shared_ptr<ExprAST> AddImplicitCastForEqual(ValType* valtype, Type::TypeID LID, Type::TypeID RID,std::shared_ptr<ExprAST> RHS)
{
	ValType* FromType = new ValType();
	ValType* ToType = new ValType();

	//���˵�ǵȺţ���ô�Ⱥ��ұߵ�Ҫת����ߵ�
	//****************�Ȳ��������飬ֻ��������������
	if ((LID != Type::TypeID::Int && LID != Type::TypeID::Float) || (RID != Type::TypeID::Int && RID != Type::TypeID::Float)) {
		printf("type error: only int/float in valdecl\n");
		return nullptr;
	}

	if (LID == Type::TypeID::Int && RID == Type::TypeID::Float) {
		FromType->setType(new FloatType());
		ToType->setType(new IntType());
		std::shared_ptr<ImplCastExprAST>impl = std::make_shared<ImplCastExprAST>(FromType,ToType,RHS);
		impl->CastString = "%f32Ti32";
		if (valtype->isConstQualified()) {
			impl->ToType->setConst();
			impl->FromType->setConst();
		}
		return impl;
	}
	else if (LID == Type::TypeID::Float && RID == Type::TypeID::Int) {
		FromType->setType(new IntType());
		ToType->setType(new FloatType());
		std::shared_ptr<ImplCastExprAST>impl = std::make_shared<ImplCastExprAST>(FromType, ToType, RHS);
		impl->CastString = "%i32Tf32";
		if (valtype->isConstQualified()) {
			impl->ToType->setConst();
			impl->FromType->setConst();
		}
		return impl;
	}
	else {
		printf("type error: only int/float in BOP\n");
		return nullptr;
	}
}


//������Ԫ����������ͷ��ؽ��
ValType* BinaryExprAST::JudgeType(ValType* otherType, ValType* resultType)
{
	ValType* valtype = new ValType();
	char op = this->Op;
	if (resultType->getBaseType()->getTypeID() == Type::TypeID::Int) {
		IntType* inttype = new IntType();
		valtype->setType(inttype);
	}
	else {
		if (this->Op == 62 || this->Op == 262 || this->Op == 60 || this->Op == 261 || this->Op == 259 || this->Op == 260 || this->Op == 33 || this->Op == 257 || this->Op == 258) {
			IntType* inttype = new IntType();
			valtype->setType(inttype);
		}
		else{
			FloatType* floattype = new FloatType();
			valtype->setType(floattype);
		}
	}

	//�ж������ǲ��ǳ������ܲ���ֱ�Ӽ������
	if (resultType->isConstQualified() && otherType->isConstQualified()) {
		valtype->setConst();
	}
	return valtype;
}

std::shared_ptr<ExprAST> ParseArrayRef()
{
	return nullptr;
}

//*********����ά�ȼ���***********//

//��������ڱ�����������鳤�ȼ��㺯���зֱ���ɣ��Ƿ񸳳�ֵ�����AnaExpr�׶����
//�����Ҫevaluate˵��һ���ǳ���
ValTypeStruct  DeclRefAST::Evaluate()
{
	ValTypeStruct val;
	if (this->UseDecl->valtype->getBaseType()->getTypeID() == Type::TypeID::Int) {
		val.type = "int";
		val.iVal = this->UseDecl->constValue.iVal;
	}
	else if (this->UseDecl->valtype->getBaseType()->getTypeID() == Type::TypeID::Float) {
		val.type = "float";
		val.fVal = this->UseDecl->constValue.fVal;
	}
	else {
		//������һ����������������﷨���������
		ArrayType* arrayType = (ArrayType*)this->UseDecl->valtype->getBaseType();
		
	}
	return val;
}

ValTypeStruct IntegerLiteralAST::Evaluate()
{
	ValTypeStruct val;
	val.type = "int";
	val.iVal = this->Val;
	return val;
}


ValTypeStruct  FloatLiteralAST::Evaluate()
{
	ValTypeStruct val;
	val.type = "float";
	val.fVal = this->Val;
	return val;
}

//�����ݴ�
std::vector<int>globalOffsetVec;
static std::stack<std::vector<int>> globalOffsetStack;
ValTypeStruct ArraySubscripAST::Evaluate()
{
	//�ݴ�
	globalOffsetStack.push(globalOffsetVec);
	globalOffsetVec.clear();

	ValTypeStruct val = this->ArraySub->Evaluate();

	globalOffsetVec = globalOffsetStack.top();
	globalOffsetStack.pop();

	globalOffsetVec.push_back(val.iVal);

	if (this->Addr->getClassName() != "DeclRef") {
		return this->Addr->Evaluate();
	}

	ValTypeStruct result;
	std::shared_ptr<DeclRefAST> declref = std::dynamic_pointer_cast<DeclRefAST>(this->Addr);
	
	int offset = 0;
	std::reverse(globalOffsetVec.begin(), globalOffsetVec.end());
	for (size_t i = 0;i < declref->UseDecl->dimensions.size();i++) {
		offset += globalOffsetVec[i] * declref->UseDecl->offsetVec[i];
	}
	

	if (declref->UseDecl->type == ResultType::iarray) {
		result.type = "int";
		auto it = declref->UseDecl->constValue.imap.find(offset);
		if (it != declref->UseDecl->constValue.imap.end()) {
			result.iVal = it->second;
		}
		else {
			result.iVal = 0;
		}
		
	}
	else {
		result.type = "float";
		auto it = declref->UseDecl->constValue.fmap.find(offset);
		if (it != declref->UseDecl->constValue.fmap.end()) {
			result.fVal = declref->UseDecl->constValue.fmap[offset];
		}
		else {
			result.fVal = 0;
		}
	}

	globalOffsetVec.clear();
	return result;
}

//���㵱ǰ��Ԫ������ڵ������ֵ��ֻ�����ϲ���ֵ�Ƿ����0����ÿһ�����Ƿ��ǳ������Ƿ��Ǹ���
ValTypeStruct  BinaryExprAST::Evaluate()
{
	ValTypeStruct leftval = this->LHS->Evaluate();
	ValTypeStruct rightval = this->RHS->Evaluate();
	//&& -257, || -258, == -259, != -260, <= -261, >= -262
	ValTypeStruct val;
	val.type = leftval.type;//��������֮��϶����ߵĽڵ�������ͬ��
	switch (this->Op)
	{
	//��ֻ����+��*********************************************
	case '+': {
		if (val.type == "int") {
			val.iVal = leftval.iVal + rightval.iVal;
			return val;
		}
		else {
			val.fVal = leftval.fVal + rightval.fVal;
			return val;
		}
	}
	case '-': {
		if (val.type == "int") {
			val.iVal = leftval.iVal - rightval.iVal;
			return val;
		}
		else {
			val.fVal = leftval.fVal - rightval.fVal;
			return val;
		}
	}
	case '*': {
		if (val.type == "int") {
			val.iVal = leftval.iVal * rightval.iVal;
			return val;
		}
		else {
			val.fVal = leftval.fVal * rightval.fVal;
			return val;
		}
	}
	case '/': {
		if (val.type == "int") {
			val.iVal = leftval.iVal / rightval.iVal;
			return val;
		}
		else {
			val.fVal = leftval.fVal / rightval.fVal;
			return val;
		}
	}
	case '>': {
		if (val.type == "int") {
			val.iVal = leftval.iVal > rightval.iVal?1:0;
			return val;
		}
		else {
			val.type = "int";//һ��Ҫ��int
			val.iVal = leftval.fVal > rightval.fVal?1:0;
			return val;
		}
	}
	case '%': {
		if (val.type == "int") {
			val.iVal = leftval.iVal % rightval.iVal;
			return val;
		}
	}
	case '<': {
		if (val.type == "int") {
			val.iVal = leftval.iVal < rightval.iVal ? 1 : 0;
			return val;
		}
		else {
			val.type = "int";//һ��Ҫ��int
			val.iVal = leftval.fVal < rightval.fVal ? 1 : 0;
			return val;
		}
	}
	case 257: {
		if (val.type == "int") {
			val.iVal = leftval.iVal && rightval.iVal ;
			return val;
		}
		else {
			val.type = "int";//һ��Ҫ��int
			val.iVal = leftval.fVal && rightval.fVal ;
			return val;
		}
	}
	case 258: {
		if (val.type == "int") {
			val.iVal = leftval.iVal || rightval.iVal;
			return val;
		}
		else {
			val.type = "int";//һ��Ҫ��int
			val.iVal = leftval.fVal || rightval.fVal;
			return val;
		}
	}
	case 259: {
		if (val.type == "int") {
			val.iVal = leftval.iVal == rightval.iVal?1:0;
			return val;
		}
		else {
			val.type = "int";//һ��Ҫ��int
			val.iVal = leftval.fVal == rightval.fVal?1:0;
			return val;
		}
	}
	case 260: {
		if (val.type == "int") {
			val.iVal = leftval.iVal != rightval.iVal?1:0;
			return val;
		}
		else {
			val.type = "int";//һ��Ҫ��int
			val.iVal = leftval.fVal != rightval.fVal?1:0;
			return val;
		}
	}
	case 261: {
		if (val.type == "int") {
			val.iVal = leftval.iVal <=rightval.iVal?1:0;
			return val;
		}
		else {
			val.type = "int";//һ��Ҫ��int
			val.iVal = leftval.fVal <= rightval.fVal?1:0;
			return val;
		}
	}
	case 262: {
		if (val.type == "int") {
			val.iVal = leftval.iVal >= rightval.iVal?1:0;
			return val;
		}
		else {
			val.type = "int";//һ��Ҫ��int
			val.iVal = leftval.fVal >= rightval.fVal?1:0;
			return val;
		}
	}
	default:
		break;
	}
	return val;
}

ValTypeStruct  UnaryExprAST::Evaluate()
{
	char op = this->Op;
	ValTypeStruct rightval = this->RHS->Evaluate();
	ValTypeStruct val;
	val.type = rightval.type;
	switch (op)
	{
	case '+':
		if (val.type == "int") {
			val.iVal = + rightval.iVal;
			return val;
		}
		else {
			val.fVal = + rightval.fVal;
			return val;
		}
	case '-':
		if (val.type == "int") {
			val.iVal = -rightval.iVal;
			return val;
		}
		else {
			val.fVal = -rightval.fVal;
			return val;
		}
	case '!':
		if (val.type == "int") {
			val.iVal = rightval.iVal == 0 ? 1 : 0;
			return val;
		}
		else {
			val.type = "int";
			val.iVal = rightval.fVal == 0 ? 1:0;
			return val;
		}
	default:
		break;
	}
	return val;
}

//������ʽת�����͵���ֵ���㣬���ǽ�������ת��
ValTypeStruct ImplCastExprAST::Evaluate()
{
	ValTypeStruct val = this->Val->Evaluate();
	if (this->ToType->getBaseType()->getTypeID() == Type::TypeID::Int) {
		val.type = "int";
		val.iVal = (int)std::trunc(val.fVal);
	}
	else if (this->ToType->getBaseType()->getTypeID() == Type::TypeID::Float) {
		val.type = "float";
		val.fVal = (float)val.iVal;
	}
	
	return val;
}

//���������ά��ֵ
int ArrayType::ParseExprsToSize()
{
	for (size_t i = 0;i < exprs.size();i++) {
		//�ȷ������ټ���
		ValType* valtype = exprs[i]->AnaExpr();
		if (!valtype || !valtype->isConstQualified()) {
			printf("error array expr\n");
			return -1;
		}

		ValTypeStruct ret = exprs[i]->Evaluate();
		if (ret.type == "float") {//����������
			printf("erray init need int type\n");
			return -1;
		}
		if (ret.iVal < 0) {//�����ǷǸ����� ***********ָ���һά�����0�����
			printf("error array init <0\n");
			return -1;
		}
		Size.push_back(ret.iVal);
	}

	return 0;
}

//��ȡ���齵ά�Ժ������
ValType* getLowerValType(ValType* valtype)
{
	ValType* type = new ValType();

	//�޷��ٽ�ά
	if (valtype->getBaseType()->getTypeID() != Type::TypeID::Array) {
		return nullptr;
	}

	ArrayType* arrayType = (ArrayType*)valtype->getBaseType();
	if (arrayType->Size.size() > 1) {
		std::vector<int> Size_2;
		for (size_t i = 1;i < arrayType->Size.size();i++) {
			Size_2.push_back(arrayType->Size[i]);
		}
		type->setType(new ArrayType(arrayType->ElementType, Size_2));
	}
	else {
		//��ά��һά���˻�Ϊ��������
		type->setType(arrayType->ElementType->getBaseType());
	}
	if (valtype->isConstQualified())
		type->setConst();
	return type;
}

std::shared_ptr<ExprAST> getInitList(std::vector<std::shared_ptr<ExprAST>>& List, int& k, ValType* valtype) {
	std::vector<std::shared_ptr<ExprAST>>newList;
	int i = 0;//��ǰά�Ѿ����˼��������ܳ���
	int currentMaxDim = ((ArrayType*)valtype->getBaseType())->Size[0];
	while (k < List.size() && (i < currentMaxDim))
	{
		if (List[k]->getClassName() == "InitList") {
			//�������{ }��ʼ���������������ʼ���ˣ�ֱ��������Բ���initlist��λ��
			auto E = List[k]->AnaExpr();
			if (!E)
				return nullptr;
			if (((ArrayType*)E->getBaseType())->Size.size() > ((ArrayType*)valtype->getBaseType())->Size.size()) {
				break;
			}
			else {
				newList.push_back(List[k]);
				i++;
				k++;
			}
		}
		else {
			//�����Ҫ��ά���ͽ�ά������һάȥ����
			if (((ArrayType*)valtype->getBaseType())->Size.size() > 1) {
				auto E = getInitList(List, k, getLowerValType(valtype));
				if (!E)
					return nullptr;
				newList.push_back(E);
				i++;
			}
			//�Ѿ����������ά
			else {
				auto E = List[k]->AnaExpr();
				if (!E || (E->getBaseType()->getTypeID() != Type::TypeID::Int && E->getBaseType()->getTypeID() != Type::TypeID::Float))
					return nullptr;
				if (!E->isConstQualified()) {//�������
					isAllConst = false;
				}
				if (((ArrayType*)valtype->getBaseType())->ElementType->getBaseType()->getTypeID() != E->getBaseType()->getTypeID()) {
					if (((ArrayType*)valtype->getBaseType())->ElementType->getBaseType()->getTypeID() == Type::TypeID::Int) {
						printf("use float in int array\n");
						return nullptr;
					}
					auto IM = AddImplicitCastForEqual(E, ((ArrayType*)valtype->getBaseType())->ElementType->getBaseType()->getTypeID(), E->getBaseType()->getTypeID(), List[k]);
					newList.push_back(IM);
				}
				else {
					newList.push_back(List[k]);
				}
				i++;
				k++;
			}
		}
	}
	//��ǰά�ȷ�������������/�����µ�initlist���������������û�дﵽ��ǰά�ȵ����ֵ�����0
	if (i == currentMaxDim){
		return std::make_shared<InitListAST>(valtype,newList);
	}
	else {
		newList.push_back(std::make_shared<ImplicitValueInit>(getLowerValType(valtype)));
		return std::make_shared<InitListAST>(valtype, newList);
	}
}
static int currentDim = -1;
//std::shared_ptr<ExprAST> getInitList(std::vector<std::shared_ptr<ExprAST>>& List, ValType* valtype) 
//{
//	currentDim += 1;
//	std::vector < std::shared_ptr<ExprAST> > newList;
//	for (auto expr : List) {
//		if (expr->getClassName() == "InitList") {
//			expr->AnaExpr();
//			newList.push_back(expr);
//		}
//		else {
//			
//		}
//	}
//}

void ParseIOffsetRecursion(int offset, int level,std::vector<int> dimensions,std::map<int, int>& result,std::shared_ptr<ExprAST>initList) {
	int maxsize = std::dynamic_pointer_cast<InitListAST>(initList)->List.size();
	int basic_offset = 1;//����һ���У�iΪ����˵����Ҫƫ��offset + i * basic_offset
	if (level + 1 < dimensions.size()) {
		for (int i = level + 1;i < dimensions.size();i++)
			basic_offset *= dimensions[i];
	}
	
	for (int i = 0;i < maxsize;i++) {
		if (std::dynamic_pointer_cast<InitListAST>(initList)->List[i]->getClassName() == "InitList") {
			ParseIOffsetRecursion(offset + i * basic_offset, level + 1, dimensions, result, std::dynamic_pointer_cast<InitListAST>(initList)->List[i]);
		}
		else if (std::dynamic_pointer_cast<InitListAST>(initList)->List[i]->getClassName() == "ImplicitValueInit") {
			continue;
		}
		else {
			result[offset + i] = std::dynamic_pointer_cast<InitListAST>(initList)->List[i]->Evaluate().iVal;
		}
	}
}

void ParseFOffsetRecursion(int offset, int level, std::vector<int> dimensions, std::map<int, float>& result, std::shared_ptr<ExprAST>initList) {
	int maxsize = std::dynamic_pointer_cast<InitListAST>(initList)->List.size();
	int basic_offset = 1;//����һ���У�iΪ����˵����Ҫƫ��offset + i * basic_offset
	if (level + 1 < dimensions.size()) {
		for (int i = level + 1;i < dimensions.size();i++)
			basic_offset *= dimensions[i];
	}

	for (int i = 0;i < maxsize;i++) {
		if (initList->getClassName() == "InitList") {
			ParseFOffsetRecursion(offset + i * basic_offset, level + 1, dimensions, result, std::dynamic_pointer_cast<InitListAST>(initList)->List[i]);
		}
		else if (initList->getClassName() == "ImplicitValueInit") {
			continue;
		}
		else {
			result[offset + i] = std::dynamic_pointer_cast<InitListAST>(initList)->List[i]->Evaluate().fVal;
		}
	}
}

std::map<int, int> ParseIOffset(std::vector<int> dimensions,std::shared_ptr<ExprAST>initList) {
	std::map<int, int> result ;
	ParseIOffsetRecursion(0, 0, dimensions,result, initList);
	return result;
}

std::map<int, float> ParseFOffset(std::vector<int> dimensions, std::shared_ptr<ExprAST>initList) {
	std::map<int, float> result;
	ParseFOffsetRecursion(0, 0, dimensions, result, initList);
	return result;
}



//***************���ߺ������������������ͷ���************************//

ValType*  StringAST::AnaExpr()
{
	return addNewValType();
}

//���ڴ����֣�����ֻ��Ҫ֪�����ǵ�����
ValType* FloatLiteralAST::AnaExpr()
{
	return this->Type;
}

//���ڴ�����
ValType* IntegerLiteralAST::AnaExpr()
{
	return this->Type;
}

//�������õı���
ValType* DeclRefAST::AnaExpr()
{
	return this->UseDecl->valtype;	
}

//���������
ValType* ArraySubscripAST::AnaExpr() 
{
	ValType* DType = Addr->AnaExpr();//���õĵ�ַ����
	ValType* NType = ArraySub->AnaExpr();//ȡ��ֵ

	if (!DType || !NType)
		return nullptr;

	if (NType->getBaseType()->getTypeID() != Type::TypeID::Int) {
		printf("need int in arraysub\n");
		return nullptr;
	}

	if (DType->getBaseType()->getTypeID() != Type::TypeID::Array) {
		printf("need point or array in arraysub\n");
		return nullptr;
	}

	ArrayType* arrayType = (ArrayType*)DType->getBaseType();
	ValType* newValType = new ValType();
	if (arrayType->ElementType->getBaseType()->getTypeID() == Type::TypeID::Int) {
		std::vector<int> newSize;
		//��ȡ��һ������
		if (arrayType->Size.size() > 1) {
			std::vector<int> newSizeTmp(arrayType->Size.begin() + 1, arrayType->Size.end());
			newSize = newSizeTmp;
			ValType* newArrayContain = new ValType();
			newArrayContain->setType(new IntType());
			ArrayType* newArrayType = new ArrayType(newArrayContain,newSize);
			newValType->setType(newArrayType);
		}
		else {
			//�����������һ�����ͱ��������
			newValType->setType(new IntType());
		}
		if (DType->isConstQualified() && NType->isConstQualified())
			newValType->setConst();
		this->Type = newValType;
	}
	else if (arrayType->ElementType->getBaseType()->getTypeID() == Type::TypeID::Float) {
		std::vector<int> newSize;
		//��ȡ��һ������
		if (arrayType->Size.size() > 1) {
			std::vector<int> newSizeTmp(arrayType->Size.begin() + 1, arrayType->Size.end());
			newSize = newSizeTmp;
			ValType* newArrayContain = new ValType();
			newArrayContain->setType(new FloatType());
			ArrayType* newArrayType = new ArrayType(newArrayContain, newSize);
			newValType->setType(newArrayType);
		}
		else {
			//�����������һ�����ͱ��������
			newValType->setType(new FloatType());
		}
		if (DType->isConstQualified())
			newValType->setConst();
		this->Type = newValType;
	}
	return newValType;
	
}

//���ڶ�Ԫ�������ÿһ�����Ŷ��������ԣ�����ʽֻ��int��float����Ҫ�ж��ǲ��ǳ�������ʽ
ValType* BinaryExprAST::AnaExpr()
{
	ValType* LType = LHS->AnaExpr();
	ValType* RType = RHS->AnaExpr();

	//���ж������ǲ����г�����
	if (!LType || !RType)
		return nullptr;

	Type::TypeID LID = LType->getBaseType()->getTypeID();
	Type::TypeID RID = RType->getBaseType()->getTypeID();


	//�������ID���ȣ�Ҳ��һ����������ת��
	if (LID != RID) {
		//���ǵȺŵĶ�Ԫ�������תFloat����Ҫ��֤�ǲ���һ��Intһ��Float
		if (this->Op != '=') {
			if (LID == Type::TypeID::Int&& RID == Type::TypeID::Float) {
				auto E = AddImplicitCastForOp(LType,LHS);
				this->LHS = E;
				this->Type = this->JudgeType(LType, RType);
				return Type;
			}
			else if (LID == Type::TypeID::Float && RID == Type::TypeID::Int) {
				auto E = AddImplicitCastForOp(RType,RHS);
				this->RHS = E;
				this->Type = this->JudgeType(RType, LType);
				return Type;
			}
			else {
				printf("type error: only int/float in BOP\n");
				return nullptr;
			}
		}
		else {
			//�Ⱥŵ�����£��������ǳ�����ô�ͱ���
			if (LType->isConstQualified()) {
				printf("error try to change const val");
				return nullptr;
			}
			auto E = AddImplicitCastForEqual(RType,LID, RID, this->RHS);
			this->RHS = E;
			this->Type = this->JudgeType(RType,LType);
			return Type;
		}
	}
	else {
		if (this->Op == '=') {
			//����ǵȺţ���Ҫ�������ǲ���const
			if (LType->isConstQualified()) {
				printf("error try to change const val");
				return nullptr;
			}
			this->Type = this->JudgeType(RType, LType);
			return this->Type;
		}
		else {
			if (LID != Type::TypeID::Int && LID != Type::TypeID::Float) {
				printf("type error: only int/float in valdecl\n");
				return nullptr;
			}
			this->Type = this->JudgeType(RType, LType);
			return this->Type;
		}
	}
}

//һԪ�������Ҫ�жϳ��֣���ʱ���ǲ�����������ʽ
ValType* UnaryExprAST::AnaExpr()
{
	char op = this->Op;
	auto E=this->RHS->AnaExpr();
	if (!E)
		return nullptr;
	//if (op == '!') {
	//	if (!isInCondition)
	//	{
	//		printf("error use ! out of condition\n");
	//		return nullptr;
	//	}
	//	//һ����Int���ͣ������ֵΪ��������ôҲ���س���
	//	ValType* valtype = new ValType();
	//	valtype->setType(new IntType());
	//	if (E->isConstQualified())
	//		valtype->setConst();
	//	this->Type = valtype;
	//	return valtype;
	//}
	//else {
		this->Type = E;
		return E;
	//}
}

ValType* BreakAST::AnaExpr()
{
	return addNewValType();
}

ValType* ContinueAST::AnaExpr()
{
	return addNewValType();
}

ValType* ReturnExprAST::AnaExpr()
{
	FuncType* functype = FuncMap[CurrentFunc];
	Type::TypeID returntype = functype->getReturnType()->getBaseType()->getTypeID();

	if (this->Stmt) {
		auto RType = this->Stmt->AnaExpr();
		if (!RType)
			return nullptr;
		Type::TypeID RID = RType->getBaseType()->getTypeID();
		if (returntype != RID) {
			if (RID == Type::TypeID::Int || RID == Type::TypeID::Float) {
				auto S = AddImplicitCastForEqual(RType,returntype, RID, this->Stmt);
				this->Stmt = S;
				this->Type = functype->getReturnType();
				return this->Type;
			}
			else {
				printf("error return type need int/float");
				return nullptr;
			}
		}
		this->Type = functype->getReturnType();
	}
	else {
		if (returntype != Type::TypeID::Void) {
			printf("return type error\n");
			return nullptr;
		}
	}

	return addNewValType();
}

ValType* WhileExprAST::AnaExpr()
{
	isInCondition = true;
	ValType* cType = this->Condition->AnaExpr();
	isInCondition = false;

	if (!cType)
		return nullptr;
	ValType* stmttype = this->Stmt->AnaExpr();
	if (!stmttype)
		return nullptr;
	return addNewValType();
}

ValType* IfElseExprAST::AnaExpr()
{
	isInCondition = true;
	ValType* cType=this->Condition->AnaExpr();
	isInCondition = false;

	if (!cType)
		return nullptr;
	if (cType->getBaseType()->getTypeID() != Type::TypeID::Int && cType->getBaseType()->getTypeID() != Type::TypeID::Float) {
		printf("error condition need int/float type\n");
		return nullptr;
	}
	ValType* stmttype = this->Stmts->AnaExpr();
	if (!stmttype)
		return nullptr;
	if (this->ElseStmt) {
		ValType* elsetype = this->ElseStmt->AnaExpr();
		if (!elsetype)
			return nullptr;
	}
	return addNewValType();
}

//������ʼ���ڵ�
ValType* InitListAST::AnaExpr()
{
	ArrayType* arrayType = (ArrayType*)this->Type->getBaseType();
	if (arrayType->Size.size() == 0) {
		//��ʼ��Size
		if (arrayType->ParseExprsToSize() == -1) 
			return nullptr;
	}
	else {
		//����Ѿ����˾��������ά�ȣ�˵���Ѿ���ʼ�����ˣ�����ֱ�ӷ���
		return this->Type;
	}
	if (this->List.size() == 0) {
		List.push_back(std::make_shared<ImplicitValueInit>(getLowerValType(this->Type)));
		return this->Type;
	}
	int i = 0;
	//�ӵ�0λ��ʼ���ݹ鹹���µ�InitList
	auto E = getInitList(this->List, i,this->Type);
	if (!E)
		return nullptr;
	if (i < List.size()) {
		printf("error too many nums in array\n");
		return nullptr;
	}
	this->List = std::dynamic_pointer_cast<InitListAST>(E)->List;//************���������������
	return this->Type;
}

//���������ڵ㣬���ز�Ϊ�յĶ���Ϳ����ˣ�ȫ�ֱ�����Ҫ�ж��ǲ��ǳ�������ʽ��������ֵ;�������������ж�
//ȫ�ֱ�����Ҫ��ʼ����δ��������
ValType* ValDeclAST::AnaExpr()
{
	Type::TypeID VType = this->valType->getBaseType()->getTypeID();
	//������������***********�ǵó��󵽷��ű�
	if (VType == Type::TypeID::Array) {
		//�Ƚ��������ά��
		ArrayType* arrayType = (ArrayType*)this->valType->getBaseType();
		if (arrayType->ParseExprsToSize() == -1)
			return nullptr;
		this->symbol->dimensions = arrayType->Size;
		//计算偏移相乘数组
		for (size_t i = this->symbol->dimensions.size();i > 0;i--) {
				if (i == this->symbol->dimensions.size()) {
				this->symbol->offsetVec.push_back(1);
			}
			else {
				int mul = 1;
				for (int j = i;j < this->symbol->dimensions.size();j++) {
					mul *= this->symbol->dimensions[j];
				}
				this->symbol->offsetVec.push_back(mul);
			}
		}
		std::reverse(this->symbol->offsetVec.begin(), this->symbol->offsetVec.end());
		//��������ʼ���б�Ϊ��
		if (!this->Val) {
			if (this->valType->isConstQualified()) {
				printf("no initial val\n");
				return nullptr;
			}
			if (isInGlobal) {
				//�����ȫ�ֵĻ��������������нڵ㸳ֵ0
				std::vector<std::shared_ptr<ExprAST>> InitList;
				InitList.push_back(std::make_shared<ImplicitValueInit>(getLowerValType(this->valType)));
				this->Val = std::make_shared<InitListAST>(this->valType, InitList);
			}
			if (arrayType->getElementType()->getBaseType()->getTypeID() == Type::TypeID::Int) {
				this->symbol->type = ResultType::iarray;
			}
			else {
				this->symbol->type = ResultType::farray;
			}
			return addNewValType();
		}
		else {
			//��ֵ������£��ݹ����ʼ��������ԭΪ��ȷ����ʽ
			//�ȷ�����ɣ�1.����ǳ�������鳣�� 2.���ÿһά�����ͺ�����
			isAllConst = true;
			currentDim = -1;
			auto E=this->Val->AnaExpr();
			if (!E)
				return nullptr;
			if (this->valType->isConstQualified()) {
				if (!isAllConst) {
					printf("error not all const in const array\n");
					return nullptr;
				}
				if (arrayType->getElementType()->getBaseType()->getTypeID() == Type::TypeID::Int) {
					this->symbol->type = ResultType::iarray;
					this->symbol->isConst = true;
					this->symbol->constValue.imap = ParseIOffset(this->symbol->dimensions, this->Val);
				}
				else if (arrayType->getElementType()->getBaseType()->getTypeID() == Type::TypeID::Float) {
					this->symbol->type = ResultType::farray;
					this->symbol->isConst = true;
					this->symbol->constValue.fmap = ParseFOffset(this->symbol->dimensions, this->Val);
				}
			}
			if(isInGlobal) {
				if (!isAllConst) {
					printf("error not all const in const array\n");
					return nullptr;
				}
				if (arrayType->getElementType()->getBaseType()->getTypeID() == Type::TypeID::Int) {
					this->symbol->type = ResultType::iarray;
					this->symbol->globalValue.imap = ParseIOffset(this->symbol->dimensions,this->Val);
				}
				else if (arrayType->getElementType()->getBaseType()->getTypeID() == Type::TypeID::Float) {
					this->symbol->type = ResultType::farray;
					this->symbol->globalValue.fmap = ParseFOffset(this->symbol->dimensions, this->Val);
				}
			}
			if (arrayType->getElementType()->getBaseType()->getTypeID() == Type::TypeID::Int) {
				this->symbol->type = ResultType::iarray;
			}
			else if (arrayType->getElementType()->getBaseType()->getTypeID() == Type::TypeID::Float) {
				this->symbol->type = ResultType::farray;
			}
			return addNewValType();
		}
	}

	//���ű�����
	if (VType == Type::TypeID::Int) {
		this->symbol->type = ResultType::i32;
		if (this->valType->isConstQualified()) {
			this->symbol->isConst = true;
		}
	}
	else if (VType == Type::TypeID::Float) {
		this->symbol->type = ResultType::f32;
		if (this->valType->isConstQualified()) {
			this->symbol->isConst = true;
		}
	}

	//��ʼ��ֵΪ��
	if (!this->Val) {
		if (this->valType->isConstQualified()) {
			//����ǳ�����û�г�ʼֵ������
			printf("const val need initialize\n");
			return nullptr;
		}
		return addNewValType();
	}

	//����ǳ�����ȫ�ֱ�������ôֱ�Ӽ�������ֵ
	if (this->valType->isConstQualified() || isInGlobal) {
		//��ʼֵ��Ϊ�գ��Ƚ��з����������ʽת����Ȼ���ټ���ֵ
		auto E = this->Val->AnaExpr();
		if (!E||!E->isConstQualified())//�����ǳ�������ʽ
			return nullptr;
		//����ת��
		Type::TypeID EType = E->getBaseType()->getTypeID();
		if (VType != EType) {
			auto IM = AddImplicitCastForEqual(E,VType, EType, this->Val);
			if (!IM) {
				return nullptr;
			}
			this->Val = IM;
		}

		//��ȡ����ֵ
		auto val = this->Val->Evaluate();

		if (isInGlobal) {
			//�����ȫ������
			if (VType == Type::TypeID::Int) {
				this->symbol->globalValue.iVal = val.iVal;
			}
			else if (VType == Type::TypeID::Float) {
				this->symbol->globalValue.fVal = val.fVal;
			}
		}
		if (this->valType->isConstQualified()) {
			//����ǳ�������
			if (VType == Type::TypeID::Int) {
				this->symbol->constValue.iVal = val.iVal;
			}
			else if (VType == Type::TypeID::Float) {
				this->symbol->constValue.fVal = val.fVal;
			}
		}
		
		return addNewValType();
		
	}
	else {
		//�������������洦���ˣ���������ֻ��Ҫ����int/float
		auto E = this->Val->AnaExpr();//�õ���ֵ��������
		if (!E)
			return nullptr;
		Type::TypeID EType = E->getBaseType()->getTypeID();
		if (VType != EType) {
			auto IM = AddImplicitCastForEqual(E,VType, EType, this->Val);
			if (!IM) {
				return nullptr;
			}
			this->Val = IM;
			return addNewValType();
		}
		else {
			return addNewValType();
		}
		
	}
}

//�������ýڵ㣬����������
ValType* CallExprAST::AnaExpr()
{
	if (this->Callee == "putf") {
		for (size_t i = 0;i < this->Args.size();i++) {
			this->Args[i]->AnaExpr();
		}
		return this->UseFunc->functype->getReturnType();
	}

	//��ȡ���õĺ���
	//��������
	ValType* returnType = this->UseFunc->functype->getReturnType();
	//���������б�
	std::vector<ValType*> paramTypes = this->UseFunc->functype->getParamTypes();
	std::vector<Symbol*> useParam = this->UseFunc->ParamSymbols;
	//�������������һ����һ������
	if (paramTypes.size() != this->Args.size())
	{
		printf("error params num\n");
		return nullptr;
	}
	//�����������һ������ô�ͼ��������ͣ�int/float����ʽ������������
	for (size_t i = 0;i < paramTypes.size();i++) {
		Type::TypeID PID = paramTypes[i]->getBaseType()->getTypeID();//��Ҫ��type
		//��������������
		auto E=Args[i]->AnaExpr();
		if (!E)
			return nullptr;

		Type::TypeID AID = E->getBaseType()->getTypeID();
		if (PID == AID) 
			continue;
		else {
			//��������ȷ�Ĳ�����ͬ�����ͣ�int-float,float-int
			if (PID == Type::TypeID::Int && AID == Type::TypeID::Float) {
				auto E1 = AddImplicitCastForEqual(E,PID, AID, Args[i]);
				Args[i] = E1;
			}
			else if(PID == Type::TypeID::Float && AID == Type::TypeID::Int) {
				auto E1 = AddImplicitCastForEqual(E,PID, AID, Args[i]);
				Args[i] = E1;
			}
			//else if(PID == Type::TypeID::Point && AID == Type::TypeID::Array) {//*******************************�������ﶼ���Դ��Ż����ϲ��ȵ�**************
			//	std::vector<int> newSize;
			//	ArrayType* arrayType = (ArrayType*)(E->getBaseType());
			//	//��ȡ��һ������
			//	if (arrayType->Size.size() > 1) {
			//		std::vector<int> newSizeTmp(arrayType->Size.begin() + 1, arrayType->Size.end());
			//		newSize = newSizeTmp;
			//	}
			//	//�ж�ָ�������Ƿ����
			//	if (newSize.size()!= useParam[i]->dimensions.size()||(!std::equal(newSize.begin(), newSize.end(), useParam[i]->dimensions.begin()))) {
			//		printf("point not  equal\n");
			//		return nullptr;
			//	}
			//	PointType* tempptype = (PointType*)paramTypes[i]->getBaseType();
			//	if (tempptype->ElementType->getBaseType()->getTypeID() != arrayType->ElementType->getBaseType()->getTypeID()) {
			//		printf("point not equal\n");
			//		return nullptr;
			//	}
			//	ValType* valtype = new ValType();
			//	if (arrayType->ElementType->getBaseType()->getTypeID()==Type::TypeID::Int) {
			//		valtype->setType(new IntType());
			//	}
			//	else {
			//		valtype->setType(new FloatType());
			//	}
			//	PointType* pointType = new PointType(valtype, newSize);
			//	ValType* ToType = new ValType();
			//	ToType->setType(pointType);
			//	std::string castString = "ArrayToPoint";
			//	std::shared_ptr<ExprAST> im = std::make_shared<ImplCastExprAST>(E, ToType, Args[i], castString);
			//	this->Args[i] = im;
			//}
			else {
				printf("error param type\n");
				return nullptr;
			}
		}
	}
	return returnType;
}

static int var_sort = 0;
//���������������
ValType* DeclStmtAST::AnaExpr()
{
	if (this->ValDecls.size() == 1) {
		declmap[std::dynamic_pointer_cast<ValDeclAST>(this->ValDecls[0])->Name] = var_sort;
	}	
	for (size_t i = 0;i < this->ValDecls.size();i++)
	{
		auto E = this->ValDecls[i]->AnaExpr();
		if (!E)
			return nullptr;
	}
	return addNewValType();
}

//����������
ValType* CompoundStmtAST::AnaExpr()
{
	declmap.clear();
	var_sort = 0;
	//��һ�����⣬Ҫע��delete����Ҫ��ָ��
	for (size_t i = 0;i < this->Stmts.size();i++)
	{
		if (Stmts[i]->getClassName() == "DeclStmt") {
			var_sort = i;
		}
		auto E = this->Stmts[i]->AnaExpr();		
		if (!E)
			return nullptr;
	}

	if (declmap.size() == Stmts.size() - 2) {
		if (Stmts[this->Stmts.size() - 2]->getClassName() == "BinaryExpr" && std::dynamic_pointer_cast<BinaryExprAST>(this->Stmts[this->Stmts.size() - 2])->Op == '=') {
			if (Stmts[this->Stmts.size() - 1]->getClassName() == "ReturnExpr") {
				if (!std::dynamic_pointer_cast<ReturnExprAST>(Stmts[Stmts.size() - 1])->Stmt) {
					std::shared_ptr<BinaryExprAST> b = std::dynamic_pointer_cast<BinaryExprAST>(this->Stmts[this->Stmts.size() - 2]);
					if (b->RHS->getClassName() == "DeclRef") {
						std::string usename = std::dynamic_pointer_cast<DeclRefAST>(b->RHS)->Name;
						if (declmap.find(usename) != declmap.end()) {
							auto temp1 = Stmts[this->Stmts.size() - 2];
							auto temp2 = Stmts[this->Stmts.size() - 1];
							auto temp3 = Stmts[declmap[usename]];
							this->Stmts.clear();
							Stmts.push_back(temp3);
							Stmts.push_back(temp1);
							Stmts.push_back(temp2);
						}
						else {
							auto temp1 = Stmts[this->Stmts.size() - 2];
							auto temp2 = Stmts[this->Stmts.size() - 1];
							this->Stmts.clear();
							Stmts.push_back(temp1);
							Stmts.push_back(temp2);
						}
					}
				}
			}
		}
	}
	return addNewValType();
}

//�жϺ��������ǲ���ָ��
bool isPoint = false;
ValType* ParamDeclAST::AnaExpr() 
{
	//��������
	if (this->Type->getBaseType()->getTypeID() == Type::TypeID::Int) 
		this->symbol->type = ResultType::i32;
	else if (this->Type->getBaseType()->getTypeID() == Type::TypeID::Float)
		this->symbol->type = ResultType::f32;
	else {
		isPoint = true;
		ArrayType* pointType = (ArrayType*)this->Type->getBaseType();
		if (pointType->ElementType->getBaseType()->getTypeID() == Type::TypeID::Int)
			this->symbol->type = ResultType::ipoint;
		else
			this->symbol->type = ResultType::fpoint;
	}
	return addNewValType();
}


//�����ڵ�
ValType* FunctionAST::AnaExpr()
{
	isInGlobal = false;

	//����ֵ
	if (this->getFunctype()->getReturnType()->getBaseType()->getTypeID() == Type::TypeID::Int) {
		this->symbol->returnType = ResultType::i32;
	}
	else if (this->getFunctype()->getReturnType()->getBaseType()->getTypeID() == Type::TypeID::Float) {
		this->symbol->returnType = ResultType::f32;
	}
	else if (this->getFunctype()->getReturnType()->getBaseType()->getTypeID() == Type::TypeID::Void) {
		this->symbol->returnType = ResultType::VOID;
	}

	//��Ҫ��ָ������Ҫ���д���*********************
	if (this->Params.size() != 0) {
		for (size_t i = 0;i < Params.size();i++) {
			isPoint = false;
			this->Params[i]->AnaExpr();
			if (isPoint) {
				std::shared_ptr<ParamDeclAST> pd = std::dynamic_pointer_cast<ParamDeclAST>(this->Params[i]);
				ArrayType* pointType = (ArrayType*)(pd->Type)->getBaseType();
				for (size_t j = 0;j < pointType->exprs.size();j++) {
					ValType* DType = pointType->exprs[j]->AnaExpr();
					if (!DType || !DType->isConstQualified()|| DType->getBaseType()->getTypeID()!= Type::TypeID::Int) {
						printf("point init fail\n");
						return nullptr;
					}
					ValTypeStruct value = pointType->exprs[j]->Evaluate();
					pd->symbol->dimensions.push_back(value.iVal);
					((ArrayType*)(this->Type->getParamTypes()[i]->getBaseType()))->Size.push_back(value.iVal);
				}
				//计算偏移相乘数组
				for (size_t i = pd->symbol->dimensions.size();i > 0;i--) {
					if (i == pd->symbol->dimensions.size()) {
						pd->symbol->offsetVec.push_back(1);
					}
					else {
						int mul = 1;
						for (int j = i;j < pd->symbol->dimensions.size();j++) {
							mul *= pd->symbol->dimensions[j];
						}
						pd->symbol->offsetVec.push_back(mul);
					}
				}
				std::reverse(pd->symbol->offsetVec.begin(), pd->symbol->offsetVec.end());
			}
		}
	}

	//�ں���Map�����ӱ����������ں������õ�У��
	FuncMap.insert(std::make_pair<std::string, FuncType*>(this->getName(), this->getFunctype()));
	if (this->getName() == "main") {
		hasMain = true;
		//main��������int���޲���
		if (this->getFunctype()->getReturnType()->getBaseType()->getTypeID() != Type::TypeID::Int || this->getFunctype()->getParamTypes().size() != 0) {
			printf("error main func def");
			return nullptr;
		}
	}
		
	
	CurrentFunc = this->getName();

	ValType* ret = this->CompoundStmt->AnaExpr();

	isInGlobal = true;

	if (!ret)
		return nullptr;
	return ret;
}

//������ڵ㣬��һ��ͳһ���麯��ȥ����ValType*
ValType* CompUnitAST::AnaExpr()
{
	
	for (size_t i = 0;i < this->TopExpr.size();i++) {
		auto E = TopExpr[i]->AnaExpr();
		if (!E) 
			return nullptr;
	}
	
	return addNewValType();
}


int SemanticAnalyze(ASTContext* ctx)
{
	stx = ctx;
	
	ValType* ret = stx->CompileUnit->AnaExpr();
	if (!ret) {
		return -1;
	}
	else if (!hasMain) {
		printf("no main function\n");
		return -1;
	}
	for (ValType* valtype: ToDelete) 
		delete valtype;
	return 0;
}