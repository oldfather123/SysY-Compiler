#pragma once
#ifndef RVGenerate_H
#define RVGenerate_H

#include <string>
#include "../IR/DAG.h"
#include "../IR/GCFG.h"

//生成汇编代码函数，s为生成字符串
void gene(std::string s);

//在汇编代码中添加标签
void addlabel(std::string label);

//在汇编代码中添加全局变量
void addglobaliarray(std::string arrayname, map<int, int> val, int dimension);

void addglobalfarray(std::string arrayname, map<int, int> val, int dimension);

void addglobalvar(std::string varname,int varvalue);

void addglobalconstvar(std::string varname, int varvalue);

void addglobalstring(std::string varname, std::string varvalue);

void globalVar(SymbolTable* GlobalSymbolTable);

void generateWithoutEnter(std::string s);

void generateWithoutTab(std::string s);

//void instrunctionGenerate(GCFG* gcfg, SymbolTable* GlobalSymbolTable, string targetname);

#endif