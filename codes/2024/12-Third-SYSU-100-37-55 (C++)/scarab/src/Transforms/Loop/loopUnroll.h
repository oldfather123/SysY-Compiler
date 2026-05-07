#pragma once
#include <iostream>
#include <vector>
#include <queue>
#include "Module.h"
#include "Block.h"

void loopUnroll(FunctionPtr func);
InstructionPtr cloneInstructionForUnroll(InstructionPtr old, unordered_map<ValuePtr,ValuePtr>&ValueMap,
                                        unordered_map<LabelPtr,LabelPtr>&LabelMap, FunctionPtr func);
                                    
void removeAllOperandUses(InstructionPtr instr);
void updateCurrentValuesMapping(InstructionPtr instr, unordered_map<ValuePtr, ValuePtr>& CurrentIncomingValue);
ValuePtr mapOperandForUnroll(ValuePtr old, unordered_map<ValuePtr, ValuePtr>&ValueMap);