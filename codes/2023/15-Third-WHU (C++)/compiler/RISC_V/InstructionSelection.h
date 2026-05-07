#pragma once
#ifndef InstructionSelection_H
#define InstructionSelection_H

#include "../IR/DAG.h"
#include "../IR/GCFG.h"

//÷∏¡Ó—°‘Ò
void instructionSelection(GCFG* gcfg, SymbolTable* GlobalSymbolTable);

#endif