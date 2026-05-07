#pragma once

#include "../utility/System.h"
#include "../utility/Type.h"
#include "../midend/Value.h"
#include "../midend/MIRInstruction.h"
#include "../utility/Cursor.h"

//Only for arithmetic

void Check(shared_ptr<Value>& value,DataType expected);
void CheckBinary(shared_ptr<Value>&a,shared_ptr<Value>&b);

void CheckCopy(shared_ptr<Copy>& copy);

void CheckOr(shared_ptr<Or>& _or);
void CheckAnd(shared_ptr<And>& _and);
void CheckNot(shared_ptr<Not>& _not);

void CheckEqual(shared_ptr<Equal>& equal);
void CheckNotEqual(shared_ptr<NotEqual>& not_equal);
void CheckLess(shared_ptr<Less>& less);
void CheckLessEqual(shared_ptr<LessEqual>& less_equal);
void CheckGreater(shared_ptr<Greater>& greater);
void CheckGreaterEqual(shared_ptr<GreaterEqual>& greater_equal);

void CheckAdd(shared_ptr<Add>& add);
void CheckSub(shared_ptr<Sub>& sub);
void CheckMul(shared_ptr<Mul>& mul);
void CheckDiv(shared_ptr<Div>& div);
void CheckMod(shared_ptr<Mod>& mod,Cursor location);
