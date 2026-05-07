#pragma once

#include "Common.h"
#include "Values.h"
#include "Instruction.h"

namespace ir {
    DataType getOperandCastTargetDataType(const ir::OperInst::Operator &op, const DataType &lhsDataType, const DataType &rhsDataType);
    ir::Literal foldOperation(const ir::OperInst::Operator &op, ir::Literal lhsLiteral, ir::Literal rhsLiteral);
}
