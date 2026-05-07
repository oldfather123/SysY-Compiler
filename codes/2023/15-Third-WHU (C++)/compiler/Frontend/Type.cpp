#include"Type.h"
#include"AST.h"

std::string str = "";

void ParseArrayType(std::string& str, Type* type)
{
	ArrayType* at = (ArrayType*)type;
	ValType* arraytype = at->getElementType();
	if (arraytype->getBaseType()->getTypeID() == Type::TypeID::Int) {
		str += "int";
		for (int a : at->Size) {
			str += "[";
			str += std::to_string(a);
			str += "]";
		}
	}
	else
	{
		str += "float";
		for (int a : at->Size) {
			str += "[";
			str += std::to_string(a);
			str += "]";
		}
	}
}

//void ParsePointType(std::string & str, Type * type)
//{
//	PointType* pt = (PointType*)type;
//	ValType* pointtype = pt->getElementType();
//	if (pointtype->getBaseType()->getTypeID() == Type::TypeID::Int) {
//		str += "int(*)";
//		for (int a : pt->Size) {
//			str += "[";
//			str += std::to_string(a);
//			str += "]";
//		}
//	}
//	else
//	{
//		str += "float(*)";
//		for (int a : pt->Size) {
//			str += "[";
//			str += std::to_string(a);
//			str += "]";
//		}
//	}
//}

std::string ValType::getTypeStr()
{
	if (!this->getBaseType())
		return "unknown type";
	str = "";
	if (this->isConstQualified()) {
		str += "const ";
	}
	Type* type = this->getBaseType();
	switch (type->getTypeID())
	{
	case Type::TypeID::Int:
		str += "int";
		break;
	case Type::TypeID::Float:
		str += "float";
		break;
	case Type::TypeID::Void:
		str += "void";
		break;
	case Type::TypeID::Array:
		ParseArrayType(str,type);
		break;
	default:
		break;
	}
	return str;
}

std::string FuncType::getTypeStr() {
	ValType* returnType = this->getReturnType();
	str = "";
	switch ((returnType->getBaseType())->getTypeID()) {
	case Type::TypeID::Int:
		str += "int(";
		break;
	case Type::TypeID::Float:
		str += "float(";
		break;
	case Type::TypeID::Void:
		str += "void(";
		break;
	default:
		break;
	}
	std::vector<ValType*> vec= this->getParamTypes();
	if (vec.size() != 0) {
		for (int i = 0;i < this->getParamTypes().size();i++) {
			switch ((vec[i]->getBaseType())->getTypeID()) {
			case Type::TypeID::Int:
				str += "int,";
				break;
			case Type::TypeID::Float:
				str += "float,";
				break;
			case Type::TypeID::Array: {
				ArrayType* pt = (ArrayType*)(vec[i]->getBaseType());
				ValType* arraytype = pt->getElementType();
				if (arraytype->getBaseType()->getTypeID() == Type::TypeID::Int) {
					str += "int(*)";
					for (int a : pt->Size) {
						if (a == 0)
							continue;
						str += "[";
						str += std::to_string(a);
						str += "]";
					}
				}
				else
				{
					str += "float(*),";
					for (int a : pt->Size) {
						str += "[";
						str += std::to_string(a);
						str += "]";
					}
				}
				str += ",";
				break;
			}
			default:
				break;
			}
		}
		str = str.substr(0, str.length() - 1);
	}
	str += ")";
	return str;
}

