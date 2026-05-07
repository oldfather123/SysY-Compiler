//
// Created by q1w2e3r4 on 23-7-10.
//

#include "LLVM.h"

bool Both_Const(ValueRef* lhs, ValueRef* rhs);
bool Both_Int(ValueRef* lhs, ValueRef* rhs);

ValueRef* BuildLT(Builder* builder, ValueRef* lhs, ValueRef* rhs);
ValueRef* BuildLE(Builder* builder, ValueRef* lhs, ValueRef* rhs);
ValueRef* BuildEquals(Builder* builder, ValueRef* lhs, ValueRef* rhs);
ValueRef* BuildUnequal(Builder* builder, ValueRef* lhs, ValueRef* rhs);

ValueRef* BuildXorTrue(Builder* builder,ValueRef* ref); // SysY语法里没有xor,唯一用到的是和true的异或

string zeroInitializer(Type *type);
string nonZeroInitializer(std::any values,Type* type);

ValueRef* BuildAdd(Builder* builder, ValueRef* lhs, ValueRef* rhs){
    if(Both_Const(lhs,rhs)){
        if(lhs->type == rhs->type && lhs->type == IntConst){
            int op1 = stoi(lhs->get_Ref());
            int op2 = stoi(rhs->get_Ref());
            return new Int_Const(op1 + op2);
        }
        else{
            lhs = BuildSitofp(builder,lhs);
            rhs = BuildSitofp(builder,rhs);
            float op1 = dynamic_cast<Float_Const*>(lhs)->value;
            float op2 = dynamic_cast<Float_Const*>(rhs)->value;
            return new Float_Const(op1 + op2);
        }
    }
    else if(Both_Int(lhs,rhs)){
        auto* ret = new Int_Var();

        IRInstruction * instr = new BinaryInstruction(ret,lhs,rhs,ADD);
        builder->AppendCode(instr);
        return ret;
    }
    else{ // any_is_float
        ValueRef* v1 = BuildSitofp(builder, lhs);
        ValueRef* v2 = BuildSitofp(builder, rhs);
        auto ret = new Float_Var();

        IRInstruction * instr = new BinaryInstruction(ret,v1,v2,ADD);
        builder->AppendCode(instr);
        return ret;
    }
}

ValueRef* BuildSub(Builder* builder, ValueRef* lhs, ValueRef* rhs){
    if(Both_Const(lhs,rhs)){
        if(lhs->type == rhs->type && lhs->type == IntConst){
            int op1 = stoi(lhs->get_Ref());
            int op2 = stoi(rhs->get_Ref());
            return new Int_Const(op1 - op2);
        }
        else{
            lhs = BuildSitofp(builder,lhs);
            rhs = BuildSitofp(builder,rhs);
            float op1 = dynamic_cast<Float_Const*>(lhs)->value;
            float op2 = dynamic_cast<Float_Const*>(rhs)->value;
            return new Float_Const(op1 - op2);
        }
    }
    else if(Both_Int(lhs,rhs)){
        auto ret = new Int_Var();

        IRInstruction * instr = new BinaryInstruction(ret,lhs,rhs,SUB);
        builder->AppendCode(instr);
        return ret;
    }
    else{
        ValueRef* v1 = BuildSitofp(builder, lhs);
        ValueRef* v2 = BuildSitofp(builder, rhs);
        auto ret = new Float_Var();

        IRInstruction * instr = new BinaryInstruction(ret,v1,v2,SUB);
        builder->AppendCode(instr);
        return ret;
    }
}

ValueRef* BuildMul(Builder* builder, ValueRef* lhs, ValueRef* rhs){
    if(Both_Const(lhs,rhs)){
        if(lhs->type == rhs->type && lhs->type == IntConst){
            int op1 = stoi(lhs->get_Ref());
            int op2 = stoi(rhs->get_Ref());
            return new Int_Const(op1 * op2);
        }
        else{
            lhs = BuildSitofp(builder,lhs);
            rhs = BuildSitofp(builder,rhs);
            float op1 = dynamic_cast<Float_Const*>(lhs)->value;
            float op2 = dynamic_cast<Float_Const*>(rhs)->value;
            return new Float_Const(op1 * op2);
        }
    }
    else if(Both_Int(lhs,rhs)){
        auto ret = new Int_Var();

        IRInstruction * instr = new BinaryInstruction(ret,lhs,rhs,MUL);
        builder->AppendCode(instr);
        return ret;
    }
    else{
        ValueRef *v1 = BuildSitofp(builder, lhs);
        ValueRef *v2 = BuildSitofp(builder, rhs);
        auto ret = new Float_Var();

        IRInstruction * instr = new BinaryInstruction(ret,v1,v2,MUL);
        builder->AppendCode(instr);
        return ret;
    }
}

ValueRef* BuildDiv(Builder* builder, ValueRef* lhs, ValueRef* rhs){

    if(Both_Const(lhs,rhs)){
        if(lhs->type == rhs->type && lhs->type == IntConst){
            int op1 = stoi(lhs->get_Ref());
            int op2 = stoi(rhs->get_Ref());
            return new Int_Const(op1 / op2);
        }
        else{
            lhs = BuildSitofp(builder,lhs);
            rhs = BuildSitofp(builder,rhs);
            float op1 = dynamic_cast<Float_Const*>(lhs)->value;
            float op2 = dynamic_cast<Float_Const*>(rhs)->value;
            return new Float_Const(op1 / op2);
        }
    }
    else if(Both_Int(lhs,rhs)){
        auto ret = new Int_Var();

        IRInstruction * instr = new BinaryInstruction(ret,lhs,rhs,DIV);
        builder->AppendCode(instr);
        return ret;
    }
    else{
        ValueRef* v1 = BuildSitofp(builder, lhs);
        ValueRef* v2 = BuildSitofp(builder, rhs);
        auto ret = new Float_Var();

        IRInstruction * instr = new BinaryInstruction(ret,v1,v2,DIV);
        builder->AppendCode(instr);
        return ret;
    }
}

ValueRef* BuildMod(Builder* builder, ValueRef* lhs, ValueRef* rhs){
    if(Both_Const(lhs,rhs)){
        if(lhs->type == rhs->type && lhs->type == IntConst){
            int op1 = stoi(lhs->get_Ref());
            int op2 = stoi(rhs->get_Ref());
            return new Int_Const(op1 % op2);
        }
        else{
            cerr << "invalid operands to binary expression('float' to 'float')" << endl;
            throw exception();
        }
    }
    else if(Both_Int(lhs,rhs)){
        auto ret = new Int_Var();

        IRInstruction * instr = new BinaryInstruction(ret,lhs,rhs,MOD);
        builder->AppendCode(instr);
        return ret;
    }
    else{
        cerr << "invalid operands to binary expression('float' to 'float')" << endl;
        throw exception();
    }
}


ValueRef* BuildLT(Builder* builder, ValueRef* lhs, ValueRef* rhs){
    lhs = BuildZext(builder,lhs); // lhs与rhs可能为bool型，因此先做一轮转化
    rhs = BuildZext(builder,rhs);

    if(Both_Const(lhs,rhs)){
        if(lhs->type == rhs->type && lhs->type == IntConst){
            int op1 = stoi(lhs->get_Ref());
            int op2 = stoi(rhs->get_Ref());
            ValueRef* res = op1<op2 ? new Bool_Const(true) : new Bool_Const(false);
            return res;
        }
        else{
            lhs = BuildSitofp(builder,lhs);
            rhs = BuildSitofp(builder,rhs);
            float op1 = dynamic_cast<Float_Const*>(lhs)->value;
            float op2 = dynamic_cast<Float_Const*>(rhs)->value;
            ValueRef* res = op1<op2 ? new Bool_Const(true) : new Bool_Const(false);
            return res;
        }
    }
    else if(Both_Int(lhs,rhs)){
        auto result = new Bool_Var();

        IRInstruction* instr = new CmpInstruction(result,lhs,rhs,LT);

        builder->AppendCode(instr);
        return result;
    }
    else{
        lhs = BuildSitofp(builder,lhs); // int -> float
        rhs = BuildSitofp(builder,rhs);
        auto result = new Bool_Var();

        IRInstruction* instr = new CmpInstruction(result,lhs,rhs,LT);
        builder->AppendCode(instr);

        return result;
    }
}

ValueRef* BuildLE(Builder* builder, ValueRef* lhs, ValueRef* rhs){
    lhs = BuildZext(builder,lhs); // lhs与rhs可能为bool型，因此先做一轮转化
    rhs = BuildZext(builder,rhs);

    if(Both_Const(lhs,rhs)){
        if(lhs->type == rhs->type && lhs->type == IntConst){
            int op1 = stoi(lhs->get_Ref());
            int op2 = stoi(rhs->get_Ref());
            ValueRef* res = op1<=op2 ? new Bool_Const(true) : new Bool_Const(false);
            return res;
        }
        else{
            lhs = BuildSitofp(builder,lhs);
            rhs = BuildSitofp(builder,rhs);
            float op1 = dynamic_cast<Float_Const*>(lhs)->value;
            float op2 = dynamic_cast<Float_Const*>(rhs)->value;
            ValueRef* res = op1<=op2 ? new Bool_Const(true) : new Bool_Const(false);
            return res;
        }
    }
    else if(Both_Int(lhs,rhs)){
        auto result = new Bool_Var();

        IRInstruction* instr = new CmpInstruction(result,lhs,rhs,LE);
        builder->AppendCode(instr);

        return result;
    }
    else{
        lhs = BuildSitofp(builder,lhs); // int -> float
        rhs = BuildSitofp(builder,rhs);
        auto result = new Bool_Var();

        IRInstruction* instr = new CmpInstruction(result,lhs,rhs,LE);
        builder->AppendCode(instr);

        return result;
    }
}

ValueRef* BuildEquals(Builder* builder, ValueRef* lhs, ValueRef* rhs){
    lhs = BuildZext(builder,lhs); // lhs与rhs可能为bool型，因此先做一轮转化
    rhs = BuildZext(builder,rhs);

    if(Both_Const(lhs,rhs)){
        if(lhs->type == rhs->type && lhs->type == IntConst){
            int op1 = stoi(lhs->get_Ref());
            int op2 = stoi(rhs->get_Ref());
            ValueRef* res = op1==op2 ? new Bool_Const(true) : new Bool_Const(false);
            return res;
        }
        else{
            lhs = BuildSitofp(builder,lhs);
            rhs = BuildSitofp(builder,rhs);
            float op1 = dynamic_cast<Float_Const*>(lhs)->value;
            float op2 = dynamic_cast<Float_Const*>(rhs)->value;
            ValueRef* res = op1==op2 ? new Bool_Const(true) : new Bool_Const(false);
            return res;
        }
    }
    else if(Both_Int(lhs,rhs)){
        auto result = new Bool_Var();

        IRInstruction* instr = new CmpInstruction(result,lhs,rhs,EQ);
        builder->AppendCode(instr);

        return result;
    }
    else{
        lhs = BuildSitofp(builder,lhs); // int -> float
        rhs = BuildSitofp(builder,rhs);
        auto result = new Bool_Var();

        IRInstruction* instr = new CmpInstruction(result,lhs,rhs,EQ);
        builder->AppendCode(instr);

        return result;
    }
}

ValueRef* BuildUnequal(Builder* builder, ValueRef* lhs, ValueRef* rhs){
    lhs = BuildZext(builder,lhs); // lhs与rhs可能为bool型，因此先做一轮转化
    rhs = BuildZext(builder,rhs);

    if(Both_Const(lhs,rhs)){
        if(lhs->type == rhs->type && lhs->type == IntConst){
            int op1 = stoi(lhs->get_Ref());
            int op2 = stoi(rhs->get_Ref());
            ValueRef* res = op1!=op2 ? new Bool_Const(true) : new Bool_Const(false);
            return res;
        }
        else{
            lhs = BuildSitofp(builder,lhs);
            rhs = BuildSitofp(builder,rhs);
            float op1 = dynamic_cast<Float_Const*>(lhs)->value;
            float op2 = dynamic_cast<Float_Const*>(rhs)->value;
            ValueRef* res = op1!=op2 ? new Bool_Const(true) : new Bool_Const(false);
            return res;
        }
    }
    else if(Both_Int(lhs,rhs)){
        auto result = new Bool_Var();

        IRInstruction* instr = new CmpInstruction(result,lhs,rhs,NE);
        builder->AppendCode(instr);

        return result;
    }
    else{
        lhs = BuildSitofp(builder,lhs); // int -> float
        rhs = BuildSitofp(builder,rhs);
        auto result = new Bool_Var();

        IRInstruction* instr = new CmpInstruction(result,lhs,rhs,NE);
        builder->AppendCode(instr);

        return result;
    }
}

ValueRef* BuildNeg(Builder* builder, ValueRef* ref){
    switch(ref->type){
        case IntConst:{
            int val = stoi(ref->get_Ref());
            return new Int_Const(-val);
        }
        case FloatConst:{
            float val = dynamic_cast<Float_Const*>(ref)->value;
            return new Float_Const(-val);
        }
        case IntVar:{
            return BuildSub(builder,new Int_Const(0),ref);
        }
        case FloatVar:{
            auto *ret = new Float_Var();

            IRInstruction* instr = new FNegInstruction(ret,ref);
            builder->AppendCode(instr);
            return ret;
        }
        default:{
            cerr << "not defined!" << endl;
            throw exception();
        }
    }
}

ValueRef* BuildNot(Builder* builder, ValueRef* ref){
    switch(ref->type){
        case IntConst:{
            int val = stoi(ref->get_Ref());
            return new Int_Const(!val);
        }
        case FloatConst:{
            float val = dynamic_cast<Float_Const*>(ref)->value;
            return new Int_Const(val == 0.0);
        }
        case IntVar:{
            ValueRef * temp1 = BuildCmp(builder,ref,new Int_Const(0),NE); // icmp ne 0
            ValueRef * temp2 = BuildXorTrue(builder,temp1);
            ValueRef * ret = BuildZext(builder,temp2);
            return ret;
        }
        case FloatVar:{
            ValueRef * temp1 = BuildCmp(builder,ref,new Float_Const(0.0),NE); //fcmp ne 0.0
            ValueRef * temp2 = BuildXorTrue(builder,temp1);
            ValueRef * ret = BuildZext(builder,temp2);
            return ret;
        }
        default:{
            cerr << "not defined!" << endl;
            throw exception();
        }
    }
}

ValueRef* BuildZext(Builder* builder, ValueRef* ref){
    if(ref->type == BoolConst){
        return new Int_Const(dynamic_cast<Bool_Const*>(ref)->value);
    }
    if(ref->type != BoolVar){
        return ref;
    }
    auto* ret = new Int_Var();
    IRInstruction* instr = new ZExtInstruction(ret,ref);
    builder->AppendCode(instr);

    return ret;
}

ValueRef* BuildSitofp(Builder* builder, ValueRef* ref){
    if(ref->type == FloatConst || ref->type == FloatVar){
        return ref;
    }
    else if(ref->type == IntConst){
        return new Float_Const((float)stoi(ref->get_Ref()));
    }
    else if(ref->type == IntVar){ //IntVar
        auto* ret = new Float_Var();
        IRInstruction* instr = new ITFPInstruction(ret,ref);
        builder->AppendCode(instr);
        return ret;
    }
    else{
        cerr << "not a numeric type!" << endl;
        throw exception();
    }
}

ValueRef* BuildFPTrunc(Builder* builder, ValueRef* ref){
    if(ref->type == IntConst || ref->type == IntVar){
        return ref;
    }
    else if(ref->type == FloatConst){
        float val = dynamic_cast<Float_Const*>(ref)->value;
        return new Int_Const((int)val);
    }
    else if(ref->type == FloatVar){ //IntVar
        auto* ret = new Int_Var();
        IRInstruction* instr = new FPTIInstruction(ret,ref);
        builder->AppendCode(instr);
        return ret;
    }
    else{
        cerr << "not a numeric type!" << endl;
        throw exception();
    }
}

ValueRef* BuildXorTrue(Builder* builder,ValueRef* ref){
    auto* ret = new Bool_Var();
    IRInstruction* instr = new XorInstruction(ret,ref);
    builder->AppendCode(instr);

    return ret;
}

ValueRef* BuildLoad(Builder* builder,Pointer *ptr){ // load ptr指向的位置(用于数组load)
    Type* eleType = ptr->pointerType->elementType;
    switch(eleType->type){
        case INT32TYPE:{
            auto ret = new Int_Var();
            IRInstruction* instr = new LoadInstruction(ret,ptr);
            builder->AppendCode(instr);

            return ret;
        }
        case FLOATTYPE:{
            auto ret = new Float_Var();
            IRInstruction* instr = new LoadInstruction(ret,ptr);
            builder->AppendCode(instr);

            return ret;
        }
        case POINTERTYPE:{
            PointerType* p = ptr->pointerType;
            Pointer* ret = (Pointer*)(ptr->getElement());
            IRInstruction* instr = new LoadInstruction(ret,ptr);
            builder->AppendCode(instr);

            return ret;
        }
        case ARRAYTYPE:
        case FUNCTYPE:
        case VOIDTYPE: {
            cerr << "Non-numeric Type can't be loaded!" << endl;
            throw exception();
        }
    }
}

ValueRef* BuildLoad(Builder* builder,Symbol *symbol){ // load symbol(符号load)
    Type* eleType = symbol->symbolType;
    switch(eleType->type){
        case INT32TYPE:{
            auto ret = new Int_Var();
            IRInstruction* instr = new LoadInstruction(ret,symbol);
            builder->AppendCode(instr);

            return ret;
        }
        case FLOATTYPE:{
            auto ret = new Float_Var();
            IRInstruction* instr = new LoadInstruction(ret,symbol);
            builder->AppendCode(instr);

            return ret;
        }
        case POINTERTYPE:{
            Pointer* ret = new Pointer((PointerType*) eleType);
            IRInstruction* instr = new LoadInstruction(ret,symbol);
            builder->AppendCode(instr);
            return ret;
        }
        case ARRAYTYPE:
        case FUNCTYPE:
        case VOIDTYPE: {
            cerr << "Non-numeric Type can't be loaded!" << endl;
            throw exception();
        }
    }
}

void BuildStore(Builder* builder,Pointer* ptr,ValueRef* ref){ // save ref -> ptr指向的位置(数组用)
    PointerType *p = ptr->pointerType;

    IRInstruction* instr = new StoreInstruction(ptr,ref);
    builder->AppendCode(instr);
}

void BuildStore(Builder* builder,Symbol* symbol,ValueRef* ref){ // save ref -> symbol(符号用)
    Type* eleType = symbol->symbolType;

    IRInstruction* instr = new StoreInstruction(symbol,ref);
    builder->AppendCode(instr);
}


Symbol* BuildAlloc(Builder* builder,Type* type,const std::string& name,bool is_const) { // 局部变量
    switch (type->type) {
        case VOIDTYPE: {
            break;
        }
        case INT32TYPE: {
            string varname = Utils::getNewLabel(name);
            Symbol* ret = new Symbol(type,varname,is_const);

            string debugInfo = varname + " = alloca i32, align 4"; // e,g %a = alloca i32, align 4
            IRInstruction* instr = new AllocaInstruction(ret,type,debugInfo);
            builder->AppendCode(instr);

            return ret;
        }
        case FLOATTYPE: {
            string varname = Utils::getNewLabel(name);
            Symbol* ret = new Symbol(type,varname,is_const);

            string debugInfo = varname + " = alloca float, align 4"; // e.g %a = alloca float, align 4
            IRInstruction* instr = new AllocaInstruction(ret,type,debugInfo);
            builder->AppendCode(instr);

            return ret;
        }
        case ARRAYTYPE: {
            string varname = Utils::getNewLabel(name);
            Symbol* ret = new Symbol(type,varname,is_const);

            string debugInfo = varname + " = alloca " + type->getTypeName() + ", align 16"; // e.g %2 = alloca [2 x [2 x i32]], align 16
            IRInstruction* instr = new AllocaInstruction(ret,type,debugInfo);
            builder->AppendCode(instr);

            return ret;
        }
        case POINTERTYPE:{
            string varname = Utils::getNewLabel(name);
            Symbol* ret = new Symbol(type,varname,is_const);

            string debugInfo = varname + " = alloca " + type->getTypeName() + ", align 8"; // e.g %2 = alloca i32*, align 8
            IRInstruction* instr = new AllocaInstruction(ret,type,debugInfo);
            builder->AppendCode(instr);

            return ret;
        }
        default: {
            break;
        }
    }
    return nullptr;
}

Symbol* AddGlobal(Builder* builder,Type* type,const std::string& name,ValueRef *initVal,bool is_const) { // 全局变量
    Symbol * symbol = new Symbol(type,name,is_const);
    if(initVal == nullptr){
        string debugInfo;
        switch(type->type){
            case INT32TYPE: debugInfo = name + " = dso_local global i32 0, align 4"; break;
            case FLOATTYPE: debugInfo = name + " = dso_local global float 0.000000e+00, align 4"; break;
            case ARRAYTYPE: debugInfo = name + " = dso_local global " + zeroInitializer(type) + ", align 16"; break;
            default: throw exception();
        }

        IRInstruction* instr = new AddGlobalInstruction(symbol,type,initVal,debugInfo);
        builder->AppendCode(instr);

        switch (symbol->symbolType->type) {
            case INT32TYPE: {
                symbol->constVal = new Int_Const(0);
                break;
            }
            case FLOATTYPE: {
                symbol->constVal = new Float_Const(0.0);
                break;
            }
            case ARRAYTYPE:{
                vector<any> v;
                symbol->constVal = new Array_Const(v);
                break;
            }
            default: throw exception();
        }
    }
    else{ // has initVal
        if(type->type != ARRAYTYPE) {
            if (initVal->type != IntConst && initVal->type != FloatConst) {
                cerr << "initializer element is not a compile-time constant" << endl;
                throw exception();
            }
            string debugInfo = name + " = dso_local global " + type->getTypeName() + ' ' + initVal->get_Ref() + ", align 4";

            IRInstruction* instr = new AddGlobalInstruction(symbol,type,initVal,debugInfo);
            builder->AppendCode(instr);
            symbol->constVal = initVal;
        }
        else{ //数组单独考虑
            symbol->constVal = initVal;
            string debugInfo = name + " = dso_local global " + nonZeroInitializer(((Array_Const*)initVal)->values ,type) + ", align 16";
            IRInstruction* instr = new AddGlobalInstruction(symbol,type,initVal,debugInfo);
            builder->AppendCode(instr);
        }
    }
    return symbol;
}

string zeroInitializer(Type *type) {
    if(type->type == ARRAYTYPE){
        return type->getTypeName() + " zeroinitializer";
    }
    else{
        return type->type == INT32TYPE? "i32 0":"float 0.000000e+00";
    }
    /*
    string ret = "";
    ret += type->getTypeName() + ' ';
    if(type->type == ARRAYTYPE){
        ret += '[';
        string code = zeroInitializer(((ArrayType*) type)->elementType);
        for(int i=0;i<((ArrayType*) type)->ele_cnt;i++){
            if(i != 0) ret += ", ";
            ret += code;
        }
        ret += ']';
        return ret;
    }
    else{
        return type->type == INT32TYPE? "i32 0":"float 0.000000e+00";
    }
    */
}


string nonZeroInitializer(std::any values,Type* type){
    string ret = "";
    ret += type->getTypeName() + ' ';
    if(type->type == ARRAYTYPE){
        ret += '[';

        auto v = any_cast<vector<std::any>>(values);
        int size = v.size();

        for(int i=0;i<((ArrayType*) type)->ele_cnt;i++){
            if(i != 0) ret += ", ";
            if(i < size)
                ret += nonZeroInitializer(v[i],((ArrayType*) type)->elementType);
            else
                ret += zeroInitializer(((ArrayType*) type)->elementType);
        }
        ret += ']';
        return ret;
    }
    else{
        auto initVal = any_cast<ValueRef*>(values);
        return initVal->get_TypeName() + ' ' + initVal->get_Ref();
    }
}


ValueRef* BuildCmp(Builder* builder,ValueRef* lhs, ValueRef* rhs,cmpType type){
    //EQ,NE,UGT,UGE,ULT,ULE,SGT,SGE,SLT,SLE
    switch(type){
        case EQ: return BuildEquals(builder,lhs,rhs);
        case NE: return BuildUnequal(builder,lhs,rhs);
        case GT: return BuildLT(builder,rhs,lhs);
        case GE: return BuildLE(builder,rhs,lhs);
        case LT: return BuildLT(builder,lhs,rhs);
        case LE: return BuildLE(builder,lhs,rhs);
    }
}

ValueRef *BuildCall(Builder *builder,Function *func, std::vector<ValueRef *>& args) {
    //e.g %0 = call i32 @func(i32 %1) -> normal call
    //    call void @foo(i32 %2)      -> void call
    ValueRef* ret = nullptr;
    switch(func->funcType->retType->type){
        case VOIDTYPE:{
            break;
        }
        case INT32TYPE:{
            ret = new Int_Var();
            break;
        }
        case FLOATTYPE:{
            ret = new Float_Var();
            break;
        }
        default: {
            cerr << "Return type is not basic type!!!" << endl;
            throw exception();
        }
    }

    IRInstruction* instr = new CallInstruction(ret,func,args);
    builder->AppendCode(instr);
    return ret;
}

void BuildBr(Builder *builder, BasicBlock *block) {
    builder->LinkBlock(block);

    IRInstruction* instr = new BrInstruction(block);
    builder->AppendCode(instr);
}

void BuildCondBr(Builder *builder, ValueRef *cond, BasicBlock *ifTrue, BasicBlock *ifFalse) {
    builder->LinkBlock(ifTrue);
    builder->LinkBlock(ifFalse);

    IRInstruction* instr = new CondBrInstruction(cond,ifTrue,ifFalse);
    builder->AppendCode(instr);
}

ValueRef *BuildGEP(Builder* builder,ValueRef *ptr, std::vector<ValueRef *>& idxs) {
    // GEP返回值只可能是Pointer,因此Array在最后要多做一次IndexOfArray以转化为Pointer

    ValueRef *temp = ptr;
    for(ValueRef * i : idxs){
        temp = IndexOfArray(builder,temp,i);
    }
    if(temp->type == Ptr){
        return (Pointer*)temp;
    }
    else { // temp->type == Arr
        // GEP返回值只可能是Pointer,因此Array在最后要多做一次IndexOfArray以转化为Pointer
        ValueRef *ret = IndexOfArray(builder,temp,new Int_Const(0));
        if(ret->type == Ptr){
            return (Pointer*)ret;
        }
        else{ // 返回值仍为Array,直接用构造函数转化类型
            return new Pointer(ret);
        }
    }
}

ValueRef *IndexOfArray(Builder* builder,ValueRef *ref,ValueRef * index){
    Type *ptrType; // 只有可能是指针类型或数组类型
    if(ref->type == Arr){
        ptrType = ((Array*)ref)->arrayType;
    }
    else if(ref->type == Ptr){
        ptrType = ((Pointer*)ref)->pointerType;
    }
    else{ // ref->type == Symbol
        ptrType = ((Symbol*)ref)->symbolType;
    }

    ValueRef *ret = ref->getElementPtr();
    if(ptrType->type == ARRAYTYPE){
        IRInstruction* instr = new GEPInstruction(ret,ref,index);
        builder->AppendCode(instr);
    }
    else if(ptrType->type == POINTERTYPE){
        //指针的GEP: 元素类型* -> 元素类型
        string eleName = ((PointerType*)ptrType)->elementType->getTypeName();

        IRInstruction* instr = new GEPInstruction(ret,ref,index);
        builder->AppendCode(instr);
    }
    else{
        cerr << "Not a pointer/arr type!" << endl;
        throw exception();
    }
    return ret;
}

void BuildRet(Builder *builder, ValueRef *retVal) {
    IRInstruction* instr = new RetInstruction(retVal);
    builder->AppendCode(instr);
}

bool Both_Const(ValueRef* lhs, ValueRef* rhs){
    return lhs->type <= FloatConst && rhs->type <= FloatConst;
}

bool Both_Int(ValueRef* lhs, ValueRef* rhs){
    return (lhs->type == IntConst || lhs->type == IntVar) && (rhs->type == IntConst || rhs->type == IntVar);
}
