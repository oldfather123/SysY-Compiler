package utils.type;

public enum SyntaxType {
    PUTF("putf"),
    B_TYPE("btype"),
    HEX_FLOAT_CONST("hexfloatconst"),
    DEC_FLOAT_CONST("decfloatconst"),
    HEX_CONST("hexconst"),
    OCT_CONST("oct"),
    DEC_CONST("decConst"),
    PUT_ARRAY("putarray"),
    PUT_F_ARRAY("pufarray"),
    GET_ARRAY_STMT("getarray"),
    GET_CH_STMT("getch"),
    GET_F_ARRAY_STMT("getfarray"),
    GET_FLOAT_STMT("getfloat"),
    PUTCH_STMT("putch"),
    PUTFLOAT_STMT("putfloat"),
    PUTINT_STMT("putint"),
    STOPTIME_STMT("stoptime"),
    WHILE_STMT("while"),
    STARTTIME_STMT("starttime"),
    IF_STMT("Stmt"),
    ADD_EXP("AddExp"),
    MUL_EXP("MulExp"),
    UNARY_EXP("UnaryExp"),
    PRIMARY_EXP("PrimaryExp"),
    LVAL("LVal"),
    EXP("Exp"),
    UNARY_OP("UnaryOp"),
    NUMBER("Number"),
    CONST_EXP("ConstExp"),
    LAND_EXP("LAndExp"),
    EQ_EXP("EqExp"),
    REL_EXP("RelExp"),
    LOR_EXP("LOrExp"),
    COND("Cond"),
    COMP_UNIT("CompUnit"),
    CONST_DECL("ConstDecl"),
    CONST_DEF("ConstDef"),
    CONST_INIT_VAL("ConstInitVal"),
    VAR_DECL("VarDecl"),
    VAR_DEF("VarDef"),
    INIT_VAL("InitVal"),
    FUNC_DEF("FuncDef"),
    MAIN_FUNC_DEF("MainFuncDef"),
    FUNC_TYPE("FuncType"),
    FUNC_F_PARAMS("FuncFParams"),
    FUNC_R_PARAMS("FuncRParams"),
    FUNC_F_PARAM("FuncFParam"),
    BLOCK("Block"),
    STMT("Stmt"),
    ASSIGN_STMT("Stmt"),
    BLOCK_STMT("Stmt"),
    BREAK_STMT("Stmt"),
    CONTINUE_STMT("Stmt"),
    EXP_STMT("Stmt"),
    GET_INT_STMT("Stmt"),
    PRINTF_STMT("Stmt"),
    RETURN_STMT("Stmt"),
    LOOP_FOR_STMT("Stmt"),
    FOR_STMT("ForStmt"),
    TOKEN_NODE("TokenNode");

    private final String STName;

    SyntaxType(String STName) {
        this.STName = STName;
    }

    public String toString() {
        return STName;
    }

}
