%filenames = "scanner"

digit [0-9]
nonzero [1-9]
hexdigit [0-9a-fA-F]
letter [a-zA-Z]

%x COMMENT_TYPE1 COMMENT_TYPE2

%%

"if"        { adjust(); return Parser::IF; }
"else"      { adjust(); return Parser::ELSE; }
"while"     { adjust(); return Parser::WHILE; }
"break"     { adjust(); return Parser::BREAK; }
"continue"  { adjust(); return Parser::CONTINUE; }
"return"    { adjust(); return Parser::RETURN; }
"const"     { adjust(); return Parser::CONST; }
"int"       { adjust(); return Parser::INT_TY; }
"float"     { adjust(); return Parser::FLOAT_TY; }
"void"      { adjust(); return Parser::VOID_Ty; }

","         { adjust(); return Parser::COMMA; }
";"         { adjust(); return Parser::SEMICOLON; }
"("         { adjust(); return Parser::LPAREN; }
")"         { adjust(); return Parser::RPAREN; }
"["         { adjust(); return Parser::LBRACK; }
"]"         { adjust(); return Parser::RBRACK; }
"{"         { adjust(); return Parser::LBRACE; }
"}"         { adjust(); return Parser::RBRACE; }
"+"         { adjust(); return Parser::PLUS; }
"-"         { adjust(); return Parser::MINUS; }
"*"         { adjust(); return Parser::TIMES; }
"/"         { adjust(); return Parser::DIVIDE; }
"%"         { adjust(); return Parser::MODULE; }
"=="        { adjust(); return Parser::EQ; }
"!="        { adjust(); return Parser::NEQ; }
"<"         { adjust(); return Parser::LT; }
"<="        { adjust(); return Parser::LE; }
">"         { adjust(); return Parser::GT; }
">="        { adjust(); return Parser::GE; }
"&&"        { adjust(); return Parser::AND; }
"||"        { adjust(); return Parser::OR; }
"!"         { adjust(); return Parser::NEG; }
"="         { adjust(); return Parser::ASSIGN; }

("_"|{letter})({letter}|{digit}|"_")*   { adjust(); return Parser::IDENT; }

{nonzero}{digit}*               { adjust(); return Parser::INT; }
0[xX]{hexdigit}+                { adjust(); return Parser::INT; }
0[0-7]*                         { adjust(); return Parser::INT; }

(({digit}*\.{digit}+)|({digit}+\.))([eE][+-]?{digit}+)?                 { adjust(); return Parser::FLOAT; }
{digit}+[eE][+-]?{digit}+                                               { adjust(); return Parser::FLOAT; }
0[xX](({hexdigit}*\.{hexdigit}+)|({hexdigit}+\.))([pP][+-]?{digit}+)?   { adjust(); return Parser::FLOAT; }
0[xX]{hexdigit}+[pP][+-]?{digit}+                                       { adjust(); return Parser::FLOAT; }

"//"        { adjust(); begin(StartCondition_::COMMENT_TYPE1); }
<COMMENT_TYPE1> {
    \n      { adjust(); begin(StartCondition_::INITIAL); errmsg->NewLine(); }
    .       { adjust(false); }
}

"*/"        { adjust(); errmsg->InsertErrAt(errmsg->token_pos,"mismatch comment pair"); }
"/*"        { adjust(); begin(StartCondition_::COMMENT_TYPE2); }
<COMMENT_TYPE2> {
    "*/"    { adjust(); begin(StartCondition_::INITIAL);}
    \n      { adjust(false); errmsg->NewLine(); }
    .       { adjust(false); }
}

[ \t\f]+    { adjust(); }
\n          { adjust(); errmsg->NewLine(); }
.           { adjust(); errmsg->InsertErrAt(errmsg->token_pos,"illegal token"); }
