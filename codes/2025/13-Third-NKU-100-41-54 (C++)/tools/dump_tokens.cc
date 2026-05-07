#include "../front_end/sysy_parser.tab.hh"

const char* getTokenName(int token) {
    switch(token) {
        case LEQ: return "LEQ";
        case GEQ: return "GEQ";
        case EQ: return "EQ";
        case NE: return "NE";
        case AND: return "AND";
        case OR: return "OR";
        case CONST: return "CONST";
        case IF: return "IF";
        case ELSE: return "ELSE";
        case WHILE: return "WHILE";
        case NONE_TYPE: return "NONE_TYPE";
        case INT: return "INT";
        case FLOAT: return "FLOAT";
        case RETURN: return "RETURN";
        case BREAK: return "BREAK";
        case CONTINUE: return "CONTINUE";
        case '{': return "LBRACE";
        case '}': return "RBRACE";
        case ';': return "SEMICOLON";
        case '(': return "LPAREN";
        case ')': return "RPAREN";
        case ',': return "COMMA";
        case '[': return "LBRACKET";
        case ']': return "RBRACKET";
        case '.': return "DOT";
        case '+': return "PLUS";
        case '-': return "MINUS";
        case '*': return "MUL";
        case '/': return "DIV";
        case '=': return "ASSIGN";
        case '<': return "LT";
        case '!': return "NOT";
        case '%': return "MOD";
        case '>': return "GT";
        case IDENT: return "IDENT";
        case INT_CONST: return "INT_CONST";
        case FLOAT_CONST: return "FLOAT_CONST";
        case ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

extern void dumpTokens(FILE* output, int token, int line_number, char *yytext, YYSTYPE yylval) {
    const char* token_name = getTokenName(token);
    
    switch(token) {
        case IDENT:
            fprintf(output, "%-10d %-20s %-15s %-10s\n", 
                    line_number, yytext, token_name, 
                    (yylval.symbol_token->getName()).c_str());
            break;
        case INT_CONST:
            fprintf(output, "%-10d %-20s %-15s %-10d\n", 
                    line_number, yytext, token_name, yylval.int_token);
            break;
        case FLOAT_CONST:
            fprintf(output, "%-10d %-20s %-15s %-10f\n", 
                    line_number, yytext, token_name, yylval.float_token);
            break;
        case ERROR:
            fprintf(output, "%-10d %-20s %-15s %-10s\n", 
                    line_number, yytext, token_name, "Invalid token");
            break;
        default:
            fprintf(output, "%-10d %-20s %-15s\n", 
                    line_number, yytext, token_name);
            break;
    }
}