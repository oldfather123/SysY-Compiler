#pragma once
#include <queue>
#include<bitset>
#include"../IR/DAG.h"
#include "map"
#define SIZE 1000000
using namespace std;

enum ConstantFolding_type { NAC, UNDEF, FOLDING_C};

//定值为常量
struct FoldingValue {
	ConstantFolding_type type;
	ResultType valueType;
	Value constanValue;
	
	//判断两个的value是否相等
	bool value_equal(FoldingValue value1, FoldingValue value2) {
		if (value1.valueType != value2.valueType)
			return false;
		if (value1.valueType == i32) {
			return value1.constanValue.iValue == value2.constanValue.iValue;
		}
		else if(value1.valueType == f32){
			return value1.constanValue.fValue == value2.constanValue.fValue;
		}
		return false;
	}

	bool equal(FoldingValue value1) {
		if (this->type != value1.type)
			return false;
		if (this->type == NAC || this->type == UNDEF)
			return true;
		if (value1.valueType == i32 && this->valueType==value1.valueType) {
			return value1.constanValue.iValue == this->constanValue.iValue;
		}
		else if (value1.valueType == f32 && this->valueType == value1.valueType) {
			return value1.constanValue.fValue == this->constanValue.fValue;
		}
		return false;
	}
};

class BlockDataInfo
{
public:
	vector<string> variableList;//变量合集
	//到达定值
	bitset<SIZE> defGens;
	bitset<SIZE> defKill;
	bitset<SIZE> defInSet;
	bitset<SIZE> defOutSet;
	//活跃变量
	bitset<SIZE> liveUses;
	bitset<SIZE> liveDefs;
	bitset<SIZE> liveInSet;
	bitset<SIZE> liveOutSet;
	//可用表达式
	bitset<SIZE> AvaGens;
	bitset<SIZE> AvaKills;
	bitset<SIZE> AvaInSet;
	bitset<SIZE> AvaOutSet;

	bitset<10*SIZE> edges;//1表示实边，0表示虚边

	//常量传播
	map<string,FoldingValue> foldingMap;

	BlockDataInfo() {
		//初始化
		defOutSet = 0;
		defInSet = 0;
		defGens = 0;
		defKill = 0;
		liveUses = 0;
		liveDefs = 0;
		liveInSet = 0;
		liveOutSet = 0;
		AvaInSet = 0;
		AvaOutSet.set();//全部置1
		AvaGens = 0;
		AvaKills = 0;
		variableList.clear();
		foldingMap.clear();
	}

	//设置到达定值输出
	void setDefOut() {
		defOutSet = defInSet & ~defKill;
		defOutSet |= defGens;
	}

	//设置活跃变量的输入
	void setLiveIn() {
		liveInSet = liveOutSet & ~liveDefs;
		liveInSet |= liveUses;
	}

	//设置可用表达式的输出
	void setAvaOut() {
		AvaOutSet = AvaInSet & ~AvaKills;
		AvaOutSet |= AvaGens;
	}

	//将位向量中的信息转化为对应的变量集合
	vector<string> bitSet_to_vector(bitset<SIZE> set,vector<string> collection) {
		vector<string> result;
		for (int i = 0; i < collection.size(); i++) {
			if (set.test(i)) {
				result.push_back(collection[i]);
			}
		}
		return result;
	}
	void Display(bitset<SIZE> set, vector<string> collection) {
		//vector<string> temp = bitSet_to_vector(set, collection);
		//for (int i = 0; i < temp.size(); i++) {
		//	//cout << temp[i] << "  ";
		//}
		////cout << endl;
	}
};

