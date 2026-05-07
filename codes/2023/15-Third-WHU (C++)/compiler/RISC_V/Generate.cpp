#include <iostream>

#include "Generate.h"
#include "InstructionSelection.h"
#include "Linearize.h"

#include <fstream>

ofstream output;

void generateWithoutEnter(std::string s) {
	//std::cout << s;
	output << s;
}

void generateWithoutTab(std::string s) {
	generateWithoutEnter(s + "\n");
}

//生成汇编代码函数，s为生成字符串
void gene(std::string s) {
	generateWithoutTab("\t" + s);
}

//在汇编代码中添加标签
void addlabel(std::string label) {
	generateWithoutTab(label + ":");
}

void addglobaliarray(std::string arrayname, map<int, int> val, int dimension) {
	gene(".type\t" + arrayname + ",@object");
	gene(".section\t.data,\"aw\"");
	gene(".globl\t" + arrayname);
	gene(".align\t2");
	gene(".size\t" + arrayname + ", " + to_string(dimension));
	addlabel(arrayname);
	int last = -1;
	for (auto it = val.begin(); it != val.end(); it++) {
		if ((it->first - last) != 1) {
			gene(".zero\t" + to_string(4 * (it->first - last - 1)));
		}
		gene(".word\t" + to_string(it->second));

		last = it->first;
	}
	last = last * 4;
	if (last != dimension - 4) {
		gene(".zero\t" + to_string(dimension - last - 4));
	}

	generateWithoutTab("");
}

void addglobalfarray(std::string arrayname, map<int, int> val, int dimension) {
	gene(".type\t" + arrayname + ",@object");
	gene(".section\t.data,\"aw\"");
	gene(".globl\t" + arrayname);
	gene(".align\t2");
	gene(".size\t" + arrayname + ", " + to_string(dimension));
	addlabel(arrayname);
	int last = -1;
	for (auto it = val.begin(); it != val.end(); it++) {
		if ((it->first - last) != 1) {
			gene(".zero\t" + to_string(4 * (it->first - last - 1)));
		}
		else gene(".word\t" + to_string(f32Toi32(it->second)));

		last = it->first;
	}
	last = last * 4;
	if (last != dimension) {
		gene(".zero\t" + to_string(4 * dimension - last));
	}

	generateWithoutTab("");

}


void addglobalvar(std::string varname, int varvalue){
	gene(".type\t" + varname + ",@object");
	gene(".section\t.data,\"aw\"");
	gene(".globl\t" + varname);
	gene(".align\t2");
	gene(".size\t" + varname + ", 4");
	addlabel(varname);
	if (varvalue == 0) gene(".zero\t4");
	else gene(".word\t" + std::to_string(varvalue));
	generateWithoutTab("");
}

void addglobalconstvar(std::string varname, int varvalue) {
	gene(".type\t" + varname + ",@object");
	gene(".section\t.bss,\"aw\"");
	gene(".globl\t" + varname);
	gene(".align\t2");
	gene(".size\t" + varname + ",4");
	addlabel(varname);
	if(varvalue == 0) gene(".zero\t4");
	else gene(".word\t" + std::to_string(varvalue));
	generateWithoutTab("");
}

void addglobalstring(std::string varname, std::string varvalue) {
	gene(".section\t.data,\"aw\"");
	gene(".globl\t" + varname);
	addlabel(varname);
	gene(".string\t \"" + varvalue + "\"");
	generateWithoutTab("");
}

void globalVar(SymbolTable* GlobalSymbolTable) {
	vector<Symbol*> vars = GlobalSymbolTable->getGlobalSymbol();

	for (int i = 0; i < vars.size(); i++) {
		if (vars[i]->type == i32) {
			//if (vars[i]->isConst) {
			//	addglobalconstvar(vars[i]->name, vars[i]->constValue.iVal);
			//}
			//else addglobalvar(vars[i]->name, vars[i]->globalValue.iVal);
			addglobalvar(vars[i]->name, vars[i]->globalValue.iVal);
		}
		else if (vars[i]->type == f32) {
			//if (vars[i]->isConst) {
			//	addglobalconstvar(vars[i]->name, f32Toi32(vars[i]->constValue.fVal));
			//}
			//else addglobalvar(vars[i]->name, f32Toi32(vars[i]->globalValue.fVal));
			addglobalvar(vars[i]->name, f32Toi32(vars[i]->globalValue.fVal));
		}
		else if (vars[i]->type == iarray) {
			int dimension = 4;
			for (int j = 0; j < vars[i]->dimensions.size(); j++) {
				dimension *= vars[i]->dimensions[j];
			}
			addglobaliarray(vars[i]->name, vars[i]->globalValue.imap, dimension);
		}
		else if (vars[i]->type == farray) {
			int dimension = 4;
			for (int j = 0; j < vars[i]->dimensions.size(); j++) {
				dimension *= vars[i]->dimensions[j];
			}
			addglobalfarray(vars[i]->name, vars[i]->globalValue.imap, dimension);
		}

	}
};

//void instrunctionGenerate(GCFG* gcfg, SymbolTable* GlobalSymbolTable, string targetname) {
//	output.open(targetname);
//
//	
//	instructionSelection(gcfg, GlobalSymbolTable);
//
//	//generateWithoutTab(".text");
//	generateWithoutTab(".section\t.text");
//
//	Linearlize(gcfg, GlobalSymbolTable);
//
//	globalVar(GlobalSymbolTable);
//
//}
