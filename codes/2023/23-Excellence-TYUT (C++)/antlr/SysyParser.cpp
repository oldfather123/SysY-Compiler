
// Generated from Sysy.g4 by ANTLR 4.12.0


#include "SysyVisitor.h"

#include "SysyParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

    struct SysyParserStaticData final {
        SysyParserStaticData(std::vector<std::string> ruleNames,
                             std::vector<std::string> literalNames,
                             std::vector<std::string> symbolicNames)
                : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
                  symbolicNames(std::move(symbolicNames)),
                  vocabulary(this->literalNames, this->symbolicNames) {}

        SysyParserStaticData(const SysyParserStaticData &) = delete;

        SysyParserStaticData(SysyParserStaticData &&) = delete;

        SysyParserStaticData &operator=(const SysyParserStaticData &) = delete;

        SysyParserStaticData &operator=(SysyParserStaticData &&) = delete;

        std::vector<antlr4::dfa::DFA> decisionToDFA;
        antlr4::atn::PredictionContextCache sharedContextCache;
        const std::vector<std::string> ruleNames;
        const std::vector<std::string> literalNames;
        const std::vector<std::string> symbolicNames;
        const antlr4::dfa::Vocabulary vocabulary;
        antlr4::atn::SerializedATNView serializedATN;
        std::unique_ptr<antlr4::atn::ATN> atn;
    };

    ::antlr4::internal::OnceFlag sysyParserOnceFlag;
    SysyParserStaticData *sysyParserStaticData = nullptr;

    void sysyParserInitialize() {
        assert(sysyParserStaticData == nullptr);
        auto staticData = std::make_unique<SysyParserStaticData>(
                std::vector<std::string>{
                        "compUnit", "decl", "constDecl", "bType", "constDef", "constInitVal",
                        "varDecl", "varDef", "initVal", "funcDef", "funcType", "funcFParams",
                        "funcFParam", "block", "blockItem", "stmt", "exp", "cond", "lVal",
                        "primaryExp", "number", "unaryExp", "unaryOp", "funcRParams", "funcRParam",
                        "mulExp", "addExp", "relExp", "eqExp", "lAndExp", "lOrExp", "constExp"
                },
                std::vector<std::string>{
                        "", "'='", "", "", "", "'int'", "'float'", "'void'", "'const'", "'return'",
                        "'if'", "'else'", "'for'", "'while'", "'do'", "'break'", "'continue'",
                        "'('", "')'", "'['", "']'", "'{'", "'}'", "','", "';'", "'\\u003F'",
                        "':'", "'-'", "'!'", "'~'", "'+'", "'*'", "'/'", "'%'", "'&&'", "'||'",
                        "'=='", "'!='", "'<'", "'<='", "'>'", "'>='"
                },
                std::vector<std::string>{
                        "", "", "COMMENT", "COMMENT_TOKEN", "SPACE", "KW_Int", "KW_Float",
                        "KW_Void", "KW_Const", "KW_Return", "KW_If", "KW_Else", "KW_For",
                        "KW_While", "KW_Do", "KW_Break", "KW_Continue", "Lparen", "Rparen",
                        "Lbarck", "Rbarck", "Lbrace", "Rbrace", "Comma", "Semi", "Ques", "Colon",
                        "Minus", "Point", "Tilde", "Add", "Mul", "Div", "Mode", "LAND", "LOR",
                        "EQ", "NEQ", "LT", "LE", "GT", "GE", "Int", "Ident", "STRING", "Float"
                }
        );
        static const int32_t serializedATNSegment[] = {
                4, 1, 45, 378, 2, 0, 7, 0, 2, 1, 7, 1, 2, 2, 7, 2, 2, 3, 7, 3, 2, 4, 7, 4, 2, 5, 7, 5, 2, 6, 7, 6, 2,
                7, 7, 7, 2, 8, 7, 8, 2, 9, 7, 9, 2, 10, 7, 10, 2, 11, 7, 11, 2, 12, 7, 12, 2, 13, 7, 13, 2, 14, 7,
                14, 2, 15, 7, 15, 2, 16, 7, 16, 2, 17, 7, 17, 2, 18, 7, 18, 2, 19, 7, 19, 2, 20, 7, 20, 2, 21, 7,
                21, 2, 22, 7, 22, 2, 23, 7, 23, 2, 24, 7, 24, 2, 25, 7, 25, 2, 26, 7, 26, 2, 27, 7, 27, 2, 28, 7,
                28, 2, 29, 7, 29, 2, 30, 7, 30, 2, 31, 7, 31, 1, 0, 1, 0, 5, 0, 67, 8, 0, 10, 0, 12, 0, 70, 9, 0,
                1, 0, 1, 0, 1, 1, 1, 1, 3, 1, 76, 8, 1, 1, 2, 1, 2, 1, 2, 1, 2, 1, 2, 5, 2, 83, 8, 2, 10, 2, 12, 2, 86,
                9, 2, 1, 2, 1, 2, 1, 3, 1, 3, 1, 4, 1, 4, 1, 4, 1, 4, 1, 4, 5, 4, 97, 8, 4, 10, 4, 12, 4, 100, 9, 4,
                1, 4, 1, 4, 1, 4, 1, 5, 1, 5, 1, 5, 1, 5, 1, 5, 5, 5, 110, 8, 5, 10, 5, 12, 5, 113, 9, 5, 3, 5, 115,
                8, 5, 1, 5, 3, 5, 118, 8, 5, 1, 6, 1, 6, 1, 6, 1, 6, 5, 6, 124, 8, 6, 10, 6, 12, 6, 127, 9, 6, 1, 6,
                1, 6, 1, 7, 1, 7, 1, 7, 1, 7, 1, 7, 5, 7, 136, 8, 7, 10, 7, 12, 7, 139, 9, 7, 1, 7, 1, 7, 1, 7, 1, 7,
                1, 7, 5, 7, 146, 8, 7, 10, 7, 12, 7, 149, 9, 7, 1, 7, 1, 7, 3, 7, 153, 8, 7, 1, 8, 1, 8, 1, 8, 1, 8,
                1, 8, 5, 8, 160, 8, 8, 10, 8, 12, 8, 163, 9, 8, 3, 8, 165, 8, 8, 1, 8, 3, 8, 168, 8, 8, 1, 9, 1, 9,
                1, 9, 1, 9, 3, 9, 174, 8, 9, 1, 9, 1, 9, 1, 9, 1, 10, 1, 10, 1, 11, 1, 11, 1, 11, 5, 11, 184, 8, 11,
                10, 11, 12, 11, 187, 9, 11, 1, 12, 1, 12, 1, 12, 1, 12, 1, 12, 1, 12, 1, 12, 1, 12, 5, 12, 197,
                8, 12, 10, 12, 12, 12, 200, 9, 12, 3, 12, 202, 8, 12, 1, 13, 1, 13, 5, 13, 206, 8, 13, 10, 13,
                12, 13, 209, 9, 13, 1, 13, 1, 13, 1, 14, 1, 14, 3, 14, 215, 8, 14, 1, 15, 1, 15, 1, 15, 1, 15,
                1, 15, 1, 15, 3, 15, 223, 8, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15,
                1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 1, 15,
                1, 15, 1, 15, 1, 15, 1, 15, 1, 15, 3, 15, 253, 8, 15, 1, 15, 3, 15, 256, 8, 15, 1, 16, 1, 16, 1,
                17, 1, 17, 1, 18, 1, 18, 1, 18, 1, 18, 1, 18, 5, 18, 267, 8, 18, 10, 18, 12, 18, 270, 9, 18, 1,
                19, 1, 19, 1, 19, 1, 19, 1, 19, 1, 19, 3, 19, 278, 8, 19, 1, 20, 1, 20, 3, 20, 282, 8, 20, 1, 21,
                1, 21, 1, 21, 1, 21, 3, 21, 288, 8, 21, 1, 21, 1, 21, 1, 21, 1, 21, 3, 21, 294, 8, 21, 1, 22, 1,
                22, 1, 23, 1, 23, 1, 23, 5, 23, 301, 8, 23, 10, 23, 12, 23, 304, 9, 23, 1, 24, 1, 24, 3, 24, 308,
                8, 24, 1, 25, 1, 25, 1, 25, 1, 25, 1, 25, 1, 25, 5, 25, 316, 8, 25, 10, 25, 12, 25, 319, 9, 25,
                1, 26, 1, 26, 1, 26, 1, 26, 1, 26, 1, 26, 5, 26, 327, 8, 26, 10, 26, 12, 26, 330, 9, 26, 1, 27,
                1, 27, 1, 27, 1, 27, 1, 27, 1, 27, 5, 27, 338, 8, 27, 10, 27, 12, 27, 341, 9, 27, 1, 28, 1, 28,
                1, 28, 1, 28, 1, 28, 1, 28, 5, 28, 349, 8, 28, 10, 28, 12, 28, 352, 9, 28, 1, 29, 1, 29, 1, 29,
                1, 29, 1, 29, 1, 29, 5, 29, 360, 8, 29, 10, 29, 12, 29, 363, 9, 29, 1, 30, 1, 30, 1, 30, 1, 30,
                1, 30, 1, 30, 5, 30, 371, 8, 30, 10, 30, 12, 30, 374, 9, 30, 1, 31, 1, 31, 1, 31, 0, 6, 50, 52,
                54, 56, 58, 60, 32, 0, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, 34, 36, 38,
                40, 42, 44, 46, 48, 50, 52, 54, 56, 58, 60, 62, 0, 7, 1, 0, 5, 6, 1, 0, 5, 7, 2, 0, 27, 28, 30,
                30, 1, 0, 31, 33, 2, 0, 27, 27, 30, 30, 1, 0, 38, 41, 1, 0, 36, 37, 391, 0, 68, 1, 0, 0, 0, 2, 75,
                1, 0, 0, 0, 4, 77, 1, 0, 0, 0, 6, 89, 1, 0, 0, 0, 8, 91, 1, 0, 0, 0, 10, 117, 1, 0, 0, 0, 12, 119,
                1, 0, 0, 0, 14, 152, 1, 0, 0, 0, 16, 167, 1, 0, 0, 0, 18, 169, 1, 0, 0, 0, 20, 178, 1, 0, 0, 0, 22,
                180, 1, 0, 0, 0, 24, 188, 1, 0, 0, 0, 26, 203, 1, 0, 0, 0, 28, 214, 1, 0, 0, 0, 30, 255, 1, 0, 0,
                0, 32, 257, 1, 0, 0, 0, 34, 259, 1, 0, 0, 0, 36, 261, 1, 0, 0, 0, 38, 277, 1, 0, 0, 0, 40, 281,
                1, 0, 0, 0, 42, 293, 1, 0, 0, 0, 44, 295, 1, 0, 0, 0, 46, 297, 1, 0, 0, 0, 48, 307, 1, 0, 0, 0, 50,
                309, 1, 0, 0, 0, 52, 320, 1, 0, 0, 0, 54, 331, 1, 0, 0, 0, 56, 342, 1, 0, 0, 0, 58, 353, 1, 0, 0,
                0, 60, 364, 1, 0, 0, 0, 62, 375, 1, 0, 0, 0, 64, 67, 3, 2, 1, 0, 65, 67, 3, 18, 9, 0, 66, 64, 1,
                0, 0, 0, 66, 65, 1, 0, 0, 0, 67, 70, 1, 0, 0, 0, 68, 66, 1, 0, 0, 0, 68, 69, 1, 0, 0, 0, 69, 71, 1,
                0, 0, 0, 70, 68, 1, 0, 0, 0, 71, 72, 5, 0, 0, 1, 72, 1, 1, 0, 0, 0, 73, 76, 3, 4, 2, 0, 74, 76, 3,
                12, 6, 0, 75, 73, 1, 0, 0, 0, 75, 74, 1, 0, 0, 0, 76, 3, 1, 0, 0, 0, 77, 78, 5, 8, 0, 0, 78, 79, 3,
                6, 3, 0, 79, 84, 3, 8, 4, 0, 80, 81, 5, 23, 0, 0, 81, 83, 3, 8, 4, 0, 82, 80, 1, 0, 0, 0, 83, 86,
                1, 0, 0, 0, 84, 82, 1, 0, 0, 0, 84, 85, 1, 0, 0, 0, 85, 87, 1, 0, 0, 0, 86, 84, 1, 0, 0, 0, 87, 88,
                5, 24, 0, 0, 88, 5, 1, 0, 0, 0, 89, 90, 7, 0, 0, 0, 90, 7, 1, 0, 0, 0, 91, 98, 5, 43, 0, 0, 92, 93,
                5, 19, 0, 0, 93, 94, 3, 62, 31, 0, 94, 95, 5, 20, 0, 0, 95, 97, 1, 0, 0, 0, 96, 92, 1, 0, 0, 0, 97,
                100, 1, 0, 0, 0, 98, 96, 1, 0, 0, 0, 98, 99, 1, 0, 0, 0, 99, 101, 1, 0, 0, 0, 100, 98, 1, 0, 0, 0,
                101, 102, 5, 1, 0, 0, 102, 103, 3, 10, 5, 0, 103, 9, 1, 0, 0, 0, 104, 118, 3, 62, 31, 0, 105,
                114, 5, 21, 0, 0, 106, 111, 3, 10, 5, 0, 107, 108, 5, 23, 0, 0, 108, 110, 3, 10, 5, 0, 109, 107,
                1, 0, 0, 0, 110, 113, 1, 0, 0, 0, 111, 109, 1, 0, 0, 0, 111, 112, 1, 0, 0, 0, 112, 115, 1, 0, 0,
                0, 113, 111, 1, 0, 0, 0, 114, 106, 1, 0, 0, 0, 114, 115, 1, 0, 0, 0, 115, 116, 1, 0, 0, 0, 116,
                118, 5, 22, 0, 0, 117, 104, 1, 0, 0, 0, 117, 105, 1, 0, 0, 0, 118, 11, 1, 0, 0, 0, 119, 120, 3,
                6, 3, 0, 120, 125, 3, 14, 7, 0, 121, 122, 5, 23, 0, 0, 122, 124, 3, 14, 7, 0, 123, 121, 1, 0,
                0, 0, 124, 127, 1, 0, 0, 0, 125, 123, 1, 0, 0, 0, 125, 126, 1, 0, 0, 0, 126, 128, 1, 0, 0, 0, 127,
                125, 1, 0, 0, 0, 128, 129, 5, 24, 0, 0, 129, 13, 1, 0, 0, 0, 130, 137, 5, 43, 0, 0, 131, 132,
                5, 19, 0, 0, 132, 133, 3, 62, 31, 0, 133, 134, 5, 20, 0, 0, 134, 136, 1, 0, 0, 0, 135, 131, 1,
                0, 0, 0, 136, 139, 1, 0, 0, 0, 137, 135, 1, 0, 0, 0, 137, 138, 1, 0, 0, 0, 138, 153, 1, 0, 0, 0,
                139, 137, 1, 0, 0, 0, 140, 147, 5, 43, 0, 0, 141, 142, 5, 19, 0, 0, 142, 143, 3, 62, 31, 0, 143,
                144, 5, 20, 0, 0, 144, 146, 1, 0, 0, 0, 145, 141, 1, 0, 0, 0, 146, 149, 1, 0, 0, 0, 147, 145,
                1, 0, 0, 0, 147, 148, 1, 0, 0, 0, 148, 150, 1, 0, 0, 0, 149, 147, 1, 0, 0, 0, 150, 151, 5, 1, 0,
                0, 151, 153, 3, 16, 8, 0, 152, 130, 1, 0, 0, 0, 152, 140, 1, 0, 0, 0, 153, 15, 1, 0, 0, 0, 154,
                168, 3, 32, 16, 0, 155, 164, 5, 21, 0, 0, 156, 161, 3, 16, 8, 0, 157, 158, 5, 23, 0, 0, 158,
                160, 3, 16, 8, 0, 159, 157, 1, 0, 0, 0, 160, 163, 1, 0, 0, 0, 161, 159, 1, 0, 0, 0, 161, 162,
                1, 0, 0, 0, 162, 165, 1, 0, 0, 0, 163, 161, 1, 0, 0, 0, 164, 156, 1, 0, 0, 0, 164, 165, 1, 0, 0,
                0, 165, 166, 1, 0, 0, 0, 166, 168, 5, 22, 0, 0, 167, 154, 1, 0, 0, 0, 167, 155, 1, 0, 0, 0, 168,
                17, 1, 0, 0, 0, 169, 170, 3, 20, 10, 0, 170, 171, 5, 43, 0, 0, 171, 173, 5, 17, 0, 0, 172, 174,
                3, 22, 11, 0, 173, 172, 1, 0, 0, 0, 173, 174, 1, 0, 0, 0, 174, 175, 1, 0, 0, 0, 175, 176, 5, 18,
                0, 0, 176, 177, 3, 26, 13, 0, 177, 19, 1, 0, 0, 0, 178, 179, 7, 1, 0, 0, 179, 21, 1, 0, 0, 0, 180,
                185, 3, 24, 12, 0, 181, 182, 5, 23, 0, 0, 182, 184, 3, 24, 12, 0, 183, 181, 1, 0, 0, 0, 184,
                187, 1, 0, 0, 0, 185, 183, 1, 0, 0, 0, 185, 186, 1, 0, 0, 0, 186, 23, 1, 0, 0, 0, 187, 185, 1,
                0, 0, 0, 188, 189, 3, 6, 3, 0, 189, 201, 5, 43, 0, 0, 190, 191, 5, 19, 0, 0, 191, 198, 5, 20,
                0, 0, 192, 193, 5, 19, 0, 0, 193, 194, 3, 62, 31, 0, 194, 195, 5, 20, 0, 0, 195, 197, 1, 0, 0,
                0, 196, 192, 1, 0, 0, 0, 197, 200, 1, 0, 0, 0, 198, 196, 1, 0, 0, 0, 198, 199, 1, 0, 0, 0, 199,
                202, 1, 0, 0, 0, 200, 198, 1, 0, 0, 0, 201, 190, 1, 0, 0, 0, 201, 202, 1, 0, 0, 0, 202, 25, 1,
                0, 0, 0, 203, 207, 5, 21, 0, 0, 204, 206, 3, 28, 14, 0, 205, 204, 1, 0, 0, 0, 206, 209, 1, 0,
                0, 0, 207, 205, 1, 0, 0, 0, 207, 208, 1, 0, 0, 0, 208, 210, 1, 0, 0, 0, 209, 207, 1, 0, 0, 0, 210,
                211, 5, 22, 0, 0, 211, 27, 1, 0, 0, 0, 212, 215, 3, 2, 1, 0, 213, 215, 3, 30, 15, 0, 214, 212,
                1, 0, 0, 0, 214, 213, 1, 0, 0, 0, 215, 29, 1, 0, 0, 0, 216, 217, 3, 36, 18, 0, 217, 218, 5, 1,
                0, 0, 218, 219, 3, 32, 16, 0, 219, 220, 5, 24, 0, 0, 220, 256, 1, 0, 0, 0, 221, 223, 3, 32, 16,
                0, 222, 221, 1, 0, 0, 0, 222, 223, 1, 0, 0, 0, 223, 224, 1, 0, 0, 0, 224, 256, 5, 24, 0, 0, 225,
                256, 3, 26, 13, 0, 226, 227, 5, 10, 0, 0, 227, 228, 5, 17, 0, 0, 228, 229, 3, 34, 17, 0, 229,
                230, 5, 18, 0, 0, 230, 231, 3, 30, 15, 0, 231, 256, 1, 0, 0, 0, 232, 233, 5, 10, 0, 0, 233, 234,
                5, 17, 0, 0, 234, 235, 3, 34, 17, 0, 235, 236, 5, 18, 0, 0, 236, 237, 3, 30, 15, 0, 237, 238,
                5, 11, 0, 0, 238, 239, 3, 30, 15, 0, 239, 256, 1, 0, 0, 0, 240, 241, 5, 13, 0, 0, 241, 242, 5,
                17, 0, 0, 242, 243, 3, 34, 17, 0, 243, 244, 5, 18, 0, 0, 244, 245, 3, 30, 15, 0, 245, 256, 1,
                0, 0, 0, 246, 247, 5, 15, 0, 0, 247, 256, 5, 24, 0, 0, 248, 249, 5, 16, 0, 0, 249, 256, 5, 24,
                0, 0, 250, 252, 5, 9, 0, 0, 251, 253, 3, 32, 16, 0, 252, 251, 1, 0, 0, 0, 252, 253, 1, 0, 0, 0,
                253, 254, 1, 0, 0, 0, 254, 256, 5, 24, 0, 0, 255, 216, 1, 0, 0, 0, 255, 222, 1, 0, 0, 0, 255,
                225, 1, 0, 0, 0, 255, 226, 1, 0, 0, 0, 255, 232, 1, 0, 0, 0, 255, 240, 1, 0, 0, 0, 255, 246, 1,
                0, 0, 0, 255, 248, 1, 0, 0, 0, 255, 250, 1, 0, 0, 0, 256, 31, 1, 0, 0, 0, 257, 258, 3, 52, 26,
                0, 258, 33, 1, 0, 0, 0, 259, 260, 3, 60, 30, 0, 260, 35, 1, 0, 0, 0, 261, 268, 5, 43, 0, 0, 262,
                263, 5, 19, 0, 0, 263, 264, 3, 32, 16, 0, 264, 265, 5, 20, 0, 0, 265, 267, 1, 0, 0, 0, 266, 262,
                1, 0, 0, 0, 267, 270, 1, 0, 0, 0, 268, 266, 1, 0, 0, 0, 268, 269, 1, 0, 0, 0, 269, 37, 1, 0, 0,
                0, 270, 268, 1, 0, 0, 0, 271, 272, 5, 17, 0, 0, 272, 273, 3, 32, 16, 0, 273, 274, 5, 18, 0, 0,
                274, 278, 1, 0, 0, 0, 275, 278, 3, 36, 18, 0, 276, 278, 3, 40, 20, 0, 277, 271, 1, 0, 0, 0, 277,
                275, 1, 0, 0, 0, 277, 276, 1, 0, 0, 0, 278, 39, 1, 0, 0, 0, 279, 282, 5, 45, 0, 0, 280, 282, 5,
                42, 0, 0, 281, 279, 1, 0, 0, 0, 281, 280, 1, 0, 0, 0, 282, 41, 1, 0, 0, 0, 283, 294, 3, 38, 19,
                0, 284, 285, 5, 43, 0, 0, 285, 287, 5, 17, 0, 0, 286, 288, 3, 46, 23, 0, 287, 286, 1, 0, 0, 0,
                287, 288, 1, 0, 0, 0, 288, 289, 1, 0, 0, 0, 289, 294, 5, 18, 0, 0, 290, 291, 3, 44, 22, 0, 291,
                292, 3, 42, 21, 0, 292, 294, 1, 0, 0, 0, 293, 283, 1, 0, 0, 0, 293, 284, 1, 0, 0, 0, 293, 290,
                1, 0, 0, 0, 294, 43, 1, 0, 0, 0, 295, 296, 7, 2, 0, 0, 296, 45, 1, 0, 0, 0, 297, 302, 3, 48, 24,
                0, 298, 299, 5, 23, 0, 0, 299, 301, 3, 48, 24, 0, 300, 298, 1, 0, 0, 0, 301, 304, 1, 0, 0, 0,
                302, 300, 1, 0, 0, 0, 302, 303, 1, 0, 0, 0, 303, 47, 1, 0, 0, 0, 304, 302, 1, 0, 0, 0, 305, 308,
                3, 32, 16, 0, 306, 308, 5, 44, 0, 0, 307, 305, 1, 0, 0, 0, 307, 306, 1, 0, 0, 0, 308, 49, 1, 0,
                0, 0, 309, 310, 6, 25, -1, 0, 310, 311, 3, 42, 21, 0, 311, 317, 1, 0, 0, 0, 312, 313, 10, 1,
                0, 0, 313, 314, 7, 3, 0, 0, 314, 316, 3, 42, 21, 0, 315, 312, 1, 0, 0, 0, 316, 319, 1, 0, 0, 0,
                317, 315, 1, 0, 0, 0, 317, 318, 1, 0, 0, 0, 318, 51, 1, 0, 0, 0, 319, 317, 1, 0, 0, 0, 320, 321,
                6, 26, -1, 0, 321, 322, 3, 50, 25, 0, 322, 328, 1, 0, 0, 0, 323, 324, 10, 1, 0, 0, 324, 325,
                7, 4, 0, 0, 325, 327, 3, 50, 25, 0, 326, 323, 1, 0, 0, 0, 327, 330, 1, 0, 0, 0, 328, 326, 1, 0,
                0, 0, 328, 329, 1, 0, 0, 0, 329, 53, 1, 0, 0, 0, 330, 328, 1, 0, 0, 0, 331, 332, 6, 27, -1, 0,
                332, 333, 3, 52, 26, 0, 333, 339, 1, 0, 0, 0, 334, 335, 10, 1, 0, 0, 335, 336, 7, 5, 0, 0, 336,
                338, 3, 52, 26, 0, 337, 334, 1, 0, 0, 0, 338, 341, 1, 0, 0, 0, 339, 337, 1, 0, 0, 0, 339, 340,
                1, 0, 0, 0, 340, 55, 1, 0, 0, 0, 341, 339, 1, 0, 0, 0, 342, 343, 6, 28, -1, 0, 343, 344, 3, 54,
                27, 0, 344, 350, 1, 0, 0, 0, 345, 346, 10, 1, 0, 0, 346, 347, 7, 6, 0, 0, 347, 349, 3, 54, 27,
                0, 348, 345, 1, 0, 0, 0, 349, 352, 1, 0, 0, 0, 350, 348, 1, 0, 0, 0, 350, 351, 1, 0, 0, 0, 351,
                57, 1, 0, 0, 0, 352, 350, 1, 0, 0, 0, 353, 354, 6, 29, -1, 0, 354, 355, 3, 56, 28, 0, 355, 361,
                1, 0, 0, 0, 356, 357, 10, 1, 0, 0, 357, 358, 5, 34, 0, 0, 358, 360, 3, 56, 28, 0, 359, 356, 1,
                0, 0, 0, 360, 363, 1, 0, 0, 0, 361, 359, 1, 0, 0, 0, 361, 362, 1, 0, 0, 0, 362, 59, 1, 0, 0, 0,
                363, 361, 1, 0, 0, 0, 364, 365, 6, 30, -1, 0, 365, 366, 3, 58, 29, 0, 366, 372, 1, 0, 0, 0, 367,
                368, 10, 1, 0, 0, 368, 369, 5, 35, 0, 0, 369, 371, 3, 58, 29, 0, 370, 367, 1, 0, 0, 0, 371, 374,
                1, 0, 0, 0, 372, 370, 1, 0, 0, 0, 372, 373, 1, 0, 0, 0, 373, 61, 1, 0, 0, 0, 374, 372, 1, 0, 0,
                0, 375, 376, 3, 52, 26, 0, 376, 63, 1, 0, 0, 0, 37, 66, 68, 75, 84, 98, 111, 114, 117, 125,
                137, 147, 152, 161, 164, 167, 173, 185, 198, 201, 207, 214, 222, 252, 255, 268, 277, 281,
                287, 293, 302, 307, 317, 328, 339, 350, 361, 372
        };
        staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) /
                                                                                         sizeof(serializedATNSegment[0]));

        antlr4::atn::ATNDeserializer deserializer;
        staticData->atn = deserializer.deserialize(staticData->serializedATN);

        const size_t count = staticData->atn->getNumberOfDecisions();
        staticData->decisionToDFA.reserve(count);
        for (size_t i = 0; i < count; i++) {
            staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
        }
        sysyParserStaticData = staticData.release();
    }

}

SysyParser::SysyParser(TokenStream *input) : SysyParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

SysyParser::SysyParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
    SysyParser::initialize();
    _interpreter = new atn::ParserATNSimulator(this, *sysyParserStaticData->atn, sysyParserStaticData->decisionToDFA,
                                               sysyParserStaticData->sharedContextCache, options);
}

SysyParser::~SysyParser() {
    delete _interpreter;
}

const atn::ATN &SysyParser::getATN() const {
    return *sysyParserStaticData->atn;
}

std::string SysyParser::getGrammarFileName() const {
    return "Sysy.g4";
}

const std::vector<std::string> &SysyParser::getRuleNames() const {
    return sysyParserStaticData->ruleNames;
}

const dfa::Vocabulary &SysyParser::getVocabulary() const {
    return sysyParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SysyParser::getSerializedATN() const {
    return sysyParserStaticData->serializedATN;
}


//----------------- CompUnitContext ------------------------------------------------------------------

SysyParser::CompUnitContext::CompUnitContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysyParser::CompUnitContext::EOF() {
    return getToken(SysyParser::EOF, 0);
}

std::vector<SysyParser::DeclContext *> SysyParser::CompUnitContext::decl() {
    return getRuleContexts<SysyParser::DeclContext>();
}

SysyParser::DeclContext *SysyParser::CompUnitContext::decl(size_t i) {
    return getRuleContext<SysyParser::DeclContext>(i);
}

std::vector<SysyParser::FuncDefContext *> SysyParser::CompUnitContext::funcDef() {
    return getRuleContexts<SysyParser::FuncDefContext>();
}

SysyParser::FuncDefContext *SysyParser::CompUnitContext::funcDef(size_t i) {
    return getRuleContext<SysyParser::FuncDefContext>(i);
}


size_t SysyParser::CompUnitContext::getRuleIndex() const {
    return SysyParser::RuleCompUnit;
}


std::any SysyParser::CompUnitContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitCompUnit(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::CompUnitContext *SysyParser::compUnit() {
    CompUnitContext *_localctx = _tracker.createInstance<CompUnitContext>(_ctx, getState());
    enterRule(_localctx, 0, SysyParser::RuleCompUnit);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(68);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while ((((_la & ~0x3fULL) == 0) &&
                ((1ULL << _la) & 480) != 0)) {
            setState(66);
            _errHandler->sync(this);
            switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
                case 1: {
                    setState(64);
                    decl();
                    break;
                }

                case 2: {
                    setState(65);
                    funcDef();
                    break;
                }

                default:
                    break;
            }
            setState(70);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }
        setState(71);
        match(SysyParser::EOF);

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- DeclContext ------------------------------------------------------------------

SysyParser::DeclContext::DeclContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

SysyParser::ConstDeclContext *SysyParser::DeclContext::constDecl() {
    return getRuleContext<SysyParser::ConstDeclContext>(0);
}

SysyParser::VarDeclContext *SysyParser::DeclContext::varDecl() {
    return getRuleContext<SysyParser::VarDeclContext>(0);
}


size_t SysyParser::DeclContext::getRuleIndex() const {
    return SysyParser::RuleDecl;
}


std::any SysyParser::DeclContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitDecl(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::DeclContext *SysyParser::decl() {
    DeclContext *_localctx = _tracker.createInstance<DeclContext>(_ctx, getState());
    enterRule(_localctx, 2, SysyParser::RuleDecl);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(75);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case SysyParser::KW_Const: {
                enterOuterAlt(_localctx, 1);
                setState(73);
                constDecl();
                break;
            }

            case SysyParser::KW_Int:
            case SysyParser::KW_Float: {
                enterOuterAlt(_localctx, 2);
                setState(74);
                varDecl();
                break;
            }

            default:
                throw NoViableAltException(this);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ConstDeclContext ------------------------------------------------------------------

SysyParser::ConstDeclContext::ConstDeclContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysyParser::ConstDeclContext::KW_Const() {
    return getToken(SysyParser::KW_Const, 0);
}

SysyParser::BTypeContext *SysyParser::ConstDeclContext::bType() {
    return getRuleContext<SysyParser::BTypeContext>(0);
}

std::vector<SysyParser::ConstDefContext *> SysyParser::ConstDeclContext::constDef() {
    return getRuleContexts<SysyParser::ConstDefContext>();
}

SysyParser::ConstDefContext *SysyParser::ConstDeclContext::constDef(size_t i) {
    return getRuleContext<SysyParser::ConstDefContext>(i);
}

tree::TerminalNode *SysyParser::ConstDeclContext::Semi() {
    return getToken(SysyParser::Semi, 0);
}

std::vector<tree::TerminalNode *> SysyParser::ConstDeclContext::Comma() {
    return getTokens(SysyParser::Comma);
}

tree::TerminalNode *SysyParser::ConstDeclContext::Comma(size_t i) {
    return getToken(SysyParser::Comma, i);
}


size_t SysyParser::ConstDeclContext::getRuleIndex() const {
    return SysyParser::RuleConstDecl;
}


std::any SysyParser::ConstDeclContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitConstDecl(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::ConstDeclContext *SysyParser::constDecl() {
    ConstDeclContext *_localctx = _tracker.createInstance<ConstDeclContext>(_ctx, getState());
    enterRule(_localctx, 4, SysyParser::RuleConstDecl);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(77);
        match(SysyParser::KW_Const);
        setState(78);
        bType();
        setState(79);
        constDef();
        setState(84);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == SysyParser::Comma) {
            setState(80);
            match(SysyParser::Comma);
            setState(81);
            constDef();
            setState(86);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }
        setState(87);
        match(SysyParser::Semi);

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- BTypeContext ------------------------------------------------------------------

SysyParser::BTypeContext::BTypeContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysyParser::BTypeContext::KW_Int() {
    return getToken(SysyParser::KW_Int, 0);
}

tree::TerminalNode *SysyParser::BTypeContext::KW_Float() {
    return getToken(SysyParser::KW_Float, 0);
}


size_t SysyParser::BTypeContext::getRuleIndex() const {
    return SysyParser::RuleBType;
}


std::any SysyParser::BTypeContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitBType(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::BTypeContext *SysyParser::bType() {
    BTypeContext *_localctx = _tracker.createInstance<BTypeContext>(_ctx, getState());
    enterRule(_localctx, 6, SysyParser::RuleBType);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(89);
        _la = _input->LA(1);
        if (!(_la == SysyParser::KW_Int

              || _la == SysyParser::KW_Float)) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ConstDefContext ------------------------------------------------------------------

SysyParser::ConstDefContext::ConstDefContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysyParser::ConstDefContext::Ident() {
    return getToken(SysyParser::Ident, 0);
}

SysyParser::ConstInitValContext *SysyParser::ConstDefContext::constInitVal() {
    return getRuleContext<SysyParser::ConstInitValContext>(0);
}

std::vector<tree::TerminalNode *> SysyParser::ConstDefContext::Lbarck() {
    return getTokens(SysyParser::Lbarck);
}

tree::TerminalNode *SysyParser::ConstDefContext::Lbarck(size_t i) {
    return getToken(SysyParser::Lbarck, i);
}

std::vector<SysyParser::ConstExpContext *> SysyParser::ConstDefContext::constExp() {
    return getRuleContexts<SysyParser::ConstExpContext>();
}

SysyParser::ConstExpContext *SysyParser::ConstDefContext::constExp(size_t i) {
    return getRuleContext<SysyParser::ConstExpContext>(i);
}

std::vector<tree::TerminalNode *> SysyParser::ConstDefContext::Rbarck() {
    return getTokens(SysyParser::Rbarck);
}

tree::TerminalNode *SysyParser::ConstDefContext::Rbarck(size_t i) {
    return getToken(SysyParser::Rbarck, i);
}


size_t SysyParser::ConstDefContext::getRuleIndex() const {
    return SysyParser::RuleConstDef;
}


std::any SysyParser::ConstDefContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitConstDef(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::ConstDefContext *SysyParser::constDef() {
    ConstDefContext *_localctx = _tracker.createInstance<ConstDefContext>(_ctx, getState());
    enterRule(_localctx, 8, SysyParser::RuleConstDef);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(91);
        match(SysyParser::Ident);
        setState(98);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == SysyParser::Lbarck) {
            setState(92);
            match(SysyParser::Lbarck);
            setState(93);
            constExp();
            setState(94);
            match(SysyParser::Rbarck);
            setState(100);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }
        setState(101);
        match(SysyParser::T__0);
        setState(102);
        constInitVal();

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ConstInitValContext ------------------------------------------------------------------

SysyParser::ConstInitValContext::ConstInitValContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::ConstInitValContext::getRuleIndex() const {
    return SysyParser::RuleConstInitVal;
}

void SysyParser::ConstInitValContext::copyFrom(ConstInitValContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- ListConstInitValContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::ListConstInitValContext::Lbrace() {
    return getToken(SysyParser::Lbrace, 0);
}

tree::TerminalNode *SysyParser::ListConstInitValContext::Rbrace() {
    return getToken(SysyParser::Rbrace, 0);
}

std::vector<SysyParser::ConstInitValContext *> SysyParser::ListConstInitValContext::constInitVal() {
    return getRuleContexts<SysyParser::ConstInitValContext>();
}

SysyParser::ConstInitValContext *SysyParser::ListConstInitValContext::constInitVal(size_t i) {
    return getRuleContext<SysyParser::ConstInitValContext>(i);
}

std::vector<tree::TerminalNode *> SysyParser::ListConstInitValContext::Comma() {
    return getTokens(SysyParser::Comma);
}

tree::TerminalNode *SysyParser::ListConstInitValContext::Comma(size_t i) {
    return getToken(SysyParser::Comma, i);
}

SysyParser::ListConstInitValContext::ListConstInitValContext(ConstInitValContext *ctx) { copyFrom(ctx); }


std::any SysyParser::ListConstInitValContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitListConstInitVal(this);
    else
        return visitor->visitChildren(this);
}
//----------------- ScalarConstInitValContext ------------------------------------------------------------------

SysyParser::ConstExpContext *SysyParser::ScalarConstInitValContext::constExp() {
    return getRuleContext<SysyParser::ConstExpContext>(0);
}

SysyParser::ScalarConstInitValContext::ScalarConstInitValContext(ConstInitValContext *ctx) { copyFrom(ctx); }


std::any SysyParser::ScalarConstInitValContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitScalarConstInitVal(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::ConstInitValContext *SysyParser::constInitVal() {
    ConstInitValContext *_localctx = _tracker.createInstance<ConstInitValContext>(_ctx, getState());
    enterRule(_localctx, 10, SysyParser::RuleConstInitVal);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(117);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case SysyParser::Lparen:
            case SysyParser::Minus:
            case SysyParser::Point:
            case SysyParser::Add:
            case SysyParser::Int:
            case SysyParser::Ident:
            case SysyParser::Float: {
                _localctx = _tracker.createInstance<SysyParser::ScalarConstInitValContext>(_localctx);
                enterOuterAlt(_localctx, 1);
                setState(104);
                constExp();
                break;
            }

            case SysyParser::Lbrace: {
                _localctx = _tracker.createInstance<SysyParser::ListConstInitValContext>(_localctx);
                enterOuterAlt(_localctx, 2);
                setState(105);
                match(SysyParser::Lbrace);
                setState(114);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if ((((_la & ~0x3fULL) == 0) &&
                     ((1ULL << _la) & 48379990245376) != 0)) {
                    setState(106);
                    constInitVal();
                    setState(111);
                    _errHandler->sync(this);
                    _la = _input->LA(1);
                    while (_la == SysyParser::Comma) {
                        setState(107);
                        match(SysyParser::Comma);
                        setState(108);
                        constInitVal();
                        setState(113);
                        _errHandler->sync(this);
                        _la = _input->LA(1);
                    }
                }
                setState(116);
                match(SysyParser::Rbrace);
                break;
            }

            default:
                throw NoViableAltException(this);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- VarDeclContext ------------------------------------------------------------------

SysyParser::VarDeclContext::VarDeclContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

SysyParser::BTypeContext *SysyParser::VarDeclContext::bType() {
    return getRuleContext<SysyParser::BTypeContext>(0);
}

std::vector<SysyParser::VarDefContext *> SysyParser::VarDeclContext::varDef() {
    return getRuleContexts<SysyParser::VarDefContext>();
}

SysyParser::VarDefContext *SysyParser::VarDeclContext::varDef(size_t i) {
    return getRuleContext<SysyParser::VarDefContext>(i);
}

tree::TerminalNode *SysyParser::VarDeclContext::Semi() {
    return getToken(SysyParser::Semi, 0);
}

std::vector<tree::TerminalNode *> SysyParser::VarDeclContext::Comma() {
    return getTokens(SysyParser::Comma);
}

tree::TerminalNode *SysyParser::VarDeclContext::Comma(size_t i) {
    return getToken(SysyParser::Comma, i);
}


size_t SysyParser::VarDeclContext::getRuleIndex() const {
    return SysyParser::RuleVarDecl;
}


std::any SysyParser::VarDeclContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitVarDecl(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::VarDeclContext *SysyParser::varDecl() {
    VarDeclContext *_localctx = _tracker.createInstance<VarDeclContext>(_ctx, getState());
    enterRule(_localctx, 12, SysyParser::RuleVarDecl);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(119);
        bType();
        setState(120);
        varDef();
        setState(125);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == SysyParser::Comma) {
            setState(121);
            match(SysyParser::Comma);
            setState(122);
            varDef();
            setState(127);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }
        setState(128);
        match(SysyParser::Semi);

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- VarDefContext ------------------------------------------------------------------

SysyParser::VarDefContext::VarDefContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::VarDefContext::getRuleIndex() const {
    return SysyParser::RuleVarDef;
}

void SysyParser::VarDefContext::copyFrom(VarDefContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- UninitVarDefContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::UninitVarDefContext::Ident() {
    return getToken(SysyParser::Ident, 0);
}

std::vector<tree::TerminalNode *> SysyParser::UninitVarDefContext::Lbarck() {
    return getTokens(SysyParser::Lbarck);
}

tree::TerminalNode *SysyParser::UninitVarDefContext::Lbarck(size_t i) {
    return getToken(SysyParser::Lbarck, i);
}

std::vector<SysyParser::ConstExpContext *> SysyParser::UninitVarDefContext::constExp() {
    return getRuleContexts<SysyParser::ConstExpContext>();
}

SysyParser::ConstExpContext *SysyParser::UninitVarDefContext::constExp(size_t i) {
    return getRuleContext<SysyParser::ConstExpContext>(i);
}

std::vector<tree::TerminalNode *> SysyParser::UninitVarDefContext::Rbarck() {
    return getTokens(SysyParser::Rbarck);
}

tree::TerminalNode *SysyParser::UninitVarDefContext::Rbarck(size_t i) {
    return getToken(SysyParser::Rbarck, i);
}

SysyParser::UninitVarDefContext::UninitVarDefContext(VarDefContext *ctx) { copyFrom(ctx); }


std::any SysyParser::UninitVarDefContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitUninitVarDef(this);
    else
        return visitor->visitChildren(this);
}
//----------------- InitVarDefContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::InitVarDefContext::Ident() {
    return getToken(SysyParser::Ident, 0);
}

SysyParser::InitValContext *SysyParser::InitVarDefContext::initVal() {
    return getRuleContext<SysyParser::InitValContext>(0);
}

std::vector<tree::TerminalNode *> SysyParser::InitVarDefContext::Lbarck() {
    return getTokens(SysyParser::Lbarck);
}

tree::TerminalNode *SysyParser::InitVarDefContext::Lbarck(size_t i) {
    return getToken(SysyParser::Lbarck, i);
}

std::vector<SysyParser::ConstExpContext *> SysyParser::InitVarDefContext::constExp() {
    return getRuleContexts<SysyParser::ConstExpContext>();
}

SysyParser::ConstExpContext *SysyParser::InitVarDefContext::constExp(size_t i) {
    return getRuleContext<SysyParser::ConstExpContext>(i);
}

std::vector<tree::TerminalNode *> SysyParser::InitVarDefContext::Rbarck() {
    return getTokens(SysyParser::Rbarck);
}

tree::TerminalNode *SysyParser::InitVarDefContext::Rbarck(size_t i) {
    return getToken(SysyParser::Rbarck, i);
}

SysyParser::InitVarDefContext::InitVarDefContext(VarDefContext *ctx) { copyFrom(ctx); }


std::any SysyParser::InitVarDefContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitInitVarDef(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::VarDefContext *SysyParser::varDef() {
    VarDefContext *_localctx = _tracker.createInstance<VarDefContext>(_ctx, getState());
    enterRule(_localctx, 14, SysyParser::RuleVarDef);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(152);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx)) {
            case 1: {
                _localctx = _tracker.createInstance<SysyParser::UninitVarDefContext>(_localctx);
                enterOuterAlt(_localctx, 1);
                setState(130);
                match(SysyParser::Ident);
                setState(137);
                _errHandler->sync(this);
                _la = _input->LA(1);
                while (_la == SysyParser::Lbarck) {
                    setState(131);
                    match(SysyParser::Lbarck);
                    setState(132);
                    constExp();
                    setState(133);
                    match(SysyParser::Rbarck);
                    setState(139);
                    _errHandler->sync(this);
                    _la = _input->LA(1);
                }
                break;
            }

            case 2: {
                _localctx = _tracker.createInstance<SysyParser::InitVarDefContext>(_localctx);
                enterOuterAlt(_localctx, 2);
                setState(140);
                match(SysyParser::Ident);
                setState(147);
                _errHandler->sync(this);
                _la = _input->LA(1);
                while (_la == SysyParser::Lbarck) {
                    setState(141);
                    match(SysyParser::Lbarck);
                    setState(142);
                    constExp();
                    setState(143);
                    match(SysyParser::Rbarck);
                    setState(149);
                    _errHandler->sync(this);
                    _la = _input->LA(1);
                }
                setState(150);
                match(SysyParser::T__0);
                setState(151);
                initVal();
                break;
            }

            default:
                break;
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- InitValContext ------------------------------------------------------------------

SysyParser::InitValContext::InitValContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::InitValContext::getRuleIndex() const {
    return SysyParser::RuleInitVal;
}

void SysyParser::InitValContext::copyFrom(InitValContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- ScalarInitValContext ------------------------------------------------------------------

SysyParser::ExpContext *SysyParser::ScalarInitValContext::exp() {
    return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::ScalarInitValContext::ScalarInitValContext(InitValContext *ctx) { copyFrom(ctx); }


std::any SysyParser::ScalarInitValContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitScalarInitVal(this);
    else
        return visitor->visitChildren(this);
}
//----------------- ListInitvalContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::ListInitvalContext::Lbrace() {
    return getToken(SysyParser::Lbrace, 0);
}

tree::TerminalNode *SysyParser::ListInitvalContext::Rbrace() {
    return getToken(SysyParser::Rbrace, 0);
}

std::vector<SysyParser::InitValContext *> SysyParser::ListInitvalContext::initVal() {
    return getRuleContexts<SysyParser::InitValContext>();
}

SysyParser::InitValContext *SysyParser::ListInitvalContext::initVal(size_t i) {
    return getRuleContext<SysyParser::InitValContext>(i);
}

std::vector<tree::TerminalNode *> SysyParser::ListInitvalContext::Comma() {
    return getTokens(SysyParser::Comma);
}

tree::TerminalNode *SysyParser::ListInitvalContext::Comma(size_t i) {
    return getToken(SysyParser::Comma, i);
}

SysyParser::ListInitvalContext::ListInitvalContext(InitValContext *ctx) { copyFrom(ctx); }


std::any SysyParser::ListInitvalContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitListInitval(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::InitValContext *SysyParser::initVal() {
    InitValContext *_localctx = _tracker.createInstance<InitValContext>(_ctx, getState());
    enterRule(_localctx, 16, SysyParser::RuleInitVal);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(167);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case SysyParser::Lparen:
            case SysyParser::Minus:
            case SysyParser::Point:
            case SysyParser::Add:
            case SysyParser::Int:
            case SysyParser::Ident:
            case SysyParser::Float: {
                _localctx = _tracker.createInstance<SysyParser::ScalarInitValContext>(_localctx);
                enterOuterAlt(_localctx, 1);
                setState(154);
                exp();
                break;
            }

            case SysyParser::Lbrace: {
                _localctx = _tracker.createInstance<SysyParser::ListInitvalContext>(_localctx);
                enterOuterAlt(_localctx, 2);
                setState(155);
                match(SysyParser::Lbrace);
                setState(164);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if ((((_la & ~0x3fULL) == 0) &&
                     ((1ULL << _la) & 48379990245376) != 0)) {
                    setState(156);
                    initVal();
                    setState(161);
                    _errHandler->sync(this);
                    _la = _input->LA(1);
                    while (_la == SysyParser::Comma) {
                        setState(157);
                        match(SysyParser::Comma);
                        setState(158);
                        initVal();
                        setState(163);
                        _errHandler->sync(this);
                        _la = _input->LA(1);
                    }
                }
                setState(166);
                match(SysyParser::Rbrace);
                break;
            }

            default:
                throw NoViableAltException(this);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- FuncDefContext ------------------------------------------------------------------

SysyParser::FuncDefContext::FuncDefContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

SysyParser::FuncTypeContext *SysyParser::FuncDefContext::funcType() {
    return getRuleContext<SysyParser::FuncTypeContext>(0);
}

tree::TerminalNode *SysyParser::FuncDefContext::Ident() {
    return getToken(SysyParser::Ident, 0);
}

tree::TerminalNode *SysyParser::FuncDefContext::Lparen() {
    return getToken(SysyParser::Lparen, 0);
}

tree::TerminalNode *SysyParser::FuncDefContext::Rparen() {
    return getToken(SysyParser::Rparen, 0);
}

SysyParser::BlockContext *SysyParser::FuncDefContext::block() {
    return getRuleContext<SysyParser::BlockContext>(0);
}

SysyParser::FuncFParamsContext *SysyParser::FuncDefContext::funcFParams() {
    return getRuleContext<SysyParser::FuncFParamsContext>(0);
}


size_t SysyParser::FuncDefContext::getRuleIndex() const {
    return SysyParser::RuleFuncDef;
}


std::any SysyParser::FuncDefContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitFuncDef(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::FuncDefContext *SysyParser::funcDef() {
    FuncDefContext *_localctx = _tracker.createInstance<FuncDefContext>(_ctx, getState());
    enterRule(_localctx, 18, SysyParser::RuleFuncDef);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(169);
        funcType();
        setState(170);
        match(SysyParser::Ident);
        setState(171);
        match(SysyParser::Lparen);
        setState(173);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SysyParser::KW_Int

            || _la == SysyParser::KW_Float) {
            setState(172);
            funcFParams();
        }
        setState(175);
        match(SysyParser::Rparen);
        setState(176);
        block();

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- FuncTypeContext ------------------------------------------------------------------

SysyParser::FuncTypeContext::FuncTypeContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysyParser::FuncTypeContext::KW_Void() {
    return getToken(SysyParser::KW_Void, 0);
}

tree::TerminalNode *SysyParser::FuncTypeContext::KW_Int() {
    return getToken(SysyParser::KW_Int, 0);
}

tree::TerminalNode *SysyParser::FuncTypeContext::KW_Float() {
    return getToken(SysyParser::KW_Float, 0);
}


size_t SysyParser::FuncTypeContext::getRuleIndex() const {
    return SysyParser::RuleFuncType;
}


std::any SysyParser::FuncTypeContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitFuncType(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::FuncTypeContext *SysyParser::funcType() {
    FuncTypeContext *_localctx = _tracker.createInstance<FuncTypeContext>(_ctx, getState());
    enterRule(_localctx, 20, SysyParser::RuleFuncType);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(178);
        _la = _input->LA(1);
        if (!((((_la & ~0x3fULL) == 0) &&
               ((1ULL << _la) & 224) != 0))) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- FuncFParamsContext ------------------------------------------------------------------

SysyParser::FuncFParamsContext::FuncFParamsContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

std::vector<SysyParser::FuncFParamContext *> SysyParser::FuncFParamsContext::funcFParam() {
    return getRuleContexts<SysyParser::FuncFParamContext>();
}

SysyParser::FuncFParamContext *SysyParser::FuncFParamsContext::funcFParam(size_t i) {
    return getRuleContext<SysyParser::FuncFParamContext>(i);
}

std::vector<tree::TerminalNode *> SysyParser::FuncFParamsContext::Comma() {
    return getTokens(SysyParser::Comma);
}

tree::TerminalNode *SysyParser::FuncFParamsContext::Comma(size_t i) {
    return getToken(SysyParser::Comma, i);
}


size_t SysyParser::FuncFParamsContext::getRuleIndex() const {
    return SysyParser::RuleFuncFParams;
}


std::any SysyParser::FuncFParamsContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitFuncFParams(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::FuncFParamsContext *SysyParser::funcFParams() {
    FuncFParamsContext *_localctx = _tracker.createInstance<FuncFParamsContext>(_ctx, getState());
    enterRule(_localctx, 22, SysyParser::RuleFuncFParams);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(180);
        funcFParam();
        setState(185);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == SysyParser::Comma) {
            setState(181);
            match(SysyParser::Comma);
            setState(182);
            funcFParam();
            setState(187);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- FuncFParamContext ------------------------------------------------------------------

SysyParser::FuncFParamContext::FuncFParamContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

SysyParser::BTypeContext *SysyParser::FuncFParamContext::bType() {
    return getRuleContext<SysyParser::BTypeContext>(0);
}

tree::TerminalNode *SysyParser::FuncFParamContext::Ident() {
    return getToken(SysyParser::Ident, 0);
}

std::vector<tree::TerminalNode *> SysyParser::FuncFParamContext::Lbarck() {
    return getTokens(SysyParser::Lbarck);
}

tree::TerminalNode *SysyParser::FuncFParamContext::Lbarck(size_t i) {
    return getToken(SysyParser::Lbarck, i);
}

std::vector<tree::TerminalNode *> SysyParser::FuncFParamContext::Rbarck() {
    return getTokens(SysyParser::Rbarck);
}

tree::TerminalNode *SysyParser::FuncFParamContext::Rbarck(size_t i) {
    return getToken(SysyParser::Rbarck, i);
}

std::vector<SysyParser::ConstExpContext *> SysyParser::FuncFParamContext::constExp() {
    return getRuleContexts<SysyParser::ConstExpContext>();
}

SysyParser::ConstExpContext *SysyParser::FuncFParamContext::constExp(size_t i) {
    return getRuleContext<SysyParser::ConstExpContext>(i);
}


size_t SysyParser::FuncFParamContext::getRuleIndex() const {
    return SysyParser::RuleFuncFParam;
}


std::any SysyParser::FuncFParamContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitFuncFParam(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::FuncFParamContext *SysyParser::funcFParam() {
    FuncFParamContext *_localctx = _tracker.createInstance<FuncFParamContext>(_ctx, getState());
    enterRule(_localctx, 24, SysyParser::RuleFuncFParam);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(188);
        bType();
        setState(189);
        match(SysyParser::Ident);
        setState(201);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == SysyParser::Lbarck) {
            setState(190);
            match(SysyParser::Lbarck);
            setState(191);
            match(SysyParser::Rbarck);
            setState(198);
            _errHandler->sync(this);
            _la = _input->LA(1);
            while (_la == SysyParser::Lbarck) {
                setState(192);
                match(SysyParser::Lbarck);
                setState(193);
                constExp();
                setState(194);
                match(SysyParser::Rbarck);
                setState(200);
                _errHandler->sync(this);
                _la = _input->LA(1);
            }
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- BlockContext ------------------------------------------------------------------

SysyParser::BlockContext::BlockContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysyParser::BlockContext::Lbrace() {
    return getToken(SysyParser::Lbrace, 0);
}

tree::TerminalNode *SysyParser::BlockContext::Rbrace() {
    return getToken(SysyParser::Rbrace, 0);
}

std::vector<SysyParser::BlockItemContext *> SysyParser::BlockContext::blockItem() {
    return getRuleContexts<SysyParser::BlockItemContext>();
}

SysyParser::BlockItemContext *SysyParser::BlockContext::blockItem(size_t i) {
    return getRuleContext<SysyParser::BlockItemContext>(i);
}


size_t SysyParser::BlockContext::getRuleIndex() const {
    return SysyParser::RuleBlock;
}


std::any SysyParser::BlockContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitBlock(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::BlockContext *SysyParser::block() {
    BlockContext *_localctx = _tracker.createInstance<BlockContext>(_ctx, getState());
    enterRule(_localctx, 26, SysyParser::RuleBlock);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(203);
        match(SysyParser::Lbrace);
        setState(207);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while ((((_la & ~0x3fULL) == 0) &&
                ((1ULL << _la) & 48380007130976) != 0)) {
            setState(204);
            blockItem();
            setState(209);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }
        setState(210);
        match(SysyParser::Rbrace);

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- BlockItemContext ------------------------------------------------------------------

SysyParser::BlockItemContext::BlockItemContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

SysyParser::DeclContext *SysyParser::BlockItemContext::decl() {
    return getRuleContext<SysyParser::DeclContext>(0);
}

SysyParser::StmtContext *SysyParser::BlockItemContext::stmt() {
    return getRuleContext<SysyParser::StmtContext>(0);
}


size_t SysyParser::BlockItemContext::getRuleIndex() const {
    return SysyParser::RuleBlockItem;
}


std::any SysyParser::BlockItemContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitBlockItem(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::BlockItemContext *SysyParser::blockItem() {
    BlockItemContext *_localctx = _tracker.createInstance<BlockItemContext>(_ctx, getState());
    enterRule(_localctx, 28, SysyParser::RuleBlockItem);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(214);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case SysyParser::KW_Int:
            case SysyParser::KW_Float:
            case SysyParser::KW_Const: {
                enterOuterAlt(_localctx, 1);
                setState(212);
                decl();
                break;
            }

            case SysyParser::KW_Return:
            case SysyParser::KW_If:
            case SysyParser::KW_While:
            case SysyParser::KW_Break:
            case SysyParser::KW_Continue:
            case SysyParser::Lparen:
            case SysyParser::Lbrace:
            case SysyParser::Semi:
            case SysyParser::Minus:
            case SysyParser::Point:
            case SysyParser::Add:
            case SysyParser::Int:
            case SysyParser::Ident:
            case SysyParser::Float: {
                enterOuterAlt(_localctx, 2);
                setState(213);
                stmt();
                break;
            }

            default:
                throw NoViableAltException(this);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- StmtContext ------------------------------------------------------------------

SysyParser::StmtContext::StmtContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::StmtContext::getRuleIndex() const {
    return SysyParser::RuleStmt;
}

void SysyParser::StmtContext::copyFrom(StmtContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- WhileStmtContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::WhileStmtContext::KW_While() {
    return getToken(SysyParser::KW_While, 0);
}

tree::TerminalNode *SysyParser::WhileStmtContext::Lparen() {
    return getToken(SysyParser::Lparen, 0);
}

SysyParser::CondContext *SysyParser::WhileStmtContext::cond() {
    return getRuleContext<SysyParser::CondContext>(0);
}

tree::TerminalNode *SysyParser::WhileStmtContext::Rparen() {
    return getToken(SysyParser::Rparen, 0);
}

SysyParser::StmtContext *SysyParser::WhileStmtContext::stmt() {
    return getRuleContext<SysyParser::StmtContext>(0);
}

SysyParser::WhileStmtContext::WhileStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysyParser::WhileStmtContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitWhileStmt(this);
    else
        return visitor->visitChildren(this);
}
//----------------- BlockStmtContext ------------------------------------------------------------------

SysyParser::BlockContext *SysyParser::BlockStmtContext::block() {
    return getRuleContext<SysyParser::BlockContext>(0);
}

SysyParser::BlockStmtContext::BlockStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysyParser::BlockStmtContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitBlockStmt(this);
    else
        return visitor->visitChildren(this);
}
//----------------- AssignmentContext ------------------------------------------------------------------

SysyParser::LValContext *SysyParser::AssignmentContext::lVal() {
    return getRuleContext<SysyParser::LValContext>(0);
}

SysyParser::ExpContext *SysyParser::AssignmentContext::exp() {
    return getRuleContext<SysyParser::ExpContext>(0);
}

tree::TerminalNode *SysyParser::AssignmentContext::Semi() {
    return getToken(SysyParser::Semi, 0);
}

SysyParser::AssignmentContext::AssignmentContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysyParser::AssignmentContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitAssignment(this);
    else
        return visitor->visitChildren(this);
}
//----------------- IfStmt1Context ------------------------------------------------------------------

tree::TerminalNode *SysyParser::IfStmt1Context::KW_If() {
    return getToken(SysyParser::KW_If, 0);
}

tree::TerminalNode *SysyParser::IfStmt1Context::Lparen() {
    return getToken(SysyParser::Lparen, 0);
}

SysyParser::CondContext *SysyParser::IfStmt1Context::cond() {
    return getRuleContext<SysyParser::CondContext>(0);
}

tree::TerminalNode *SysyParser::IfStmt1Context::Rparen() {
    return getToken(SysyParser::Rparen, 0);
}

SysyParser::StmtContext *SysyParser::IfStmt1Context::stmt() {
    return getRuleContext<SysyParser::StmtContext>(0);
}

SysyParser::IfStmt1Context::IfStmt1Context(StmtContext *ctx) { copyFrom(ctx); }


std::any SysyParser::IfStmt1Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitIfStmt1(this);
    else
        return visitor->visitChildren(this);
}
//----------------- BreakStmtContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::BreakStmtContext::KW_Break() {
    return getToken(SysyParser::KW_Break, 0);
}

tree::TerminalNode *SysyParser::BreakStmtContext::Semi() {
    return getToken(SysyParser::Semi, 0);
}

SysyParser::BreakStmtContext::BreakStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysyParser::BreakStmtContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitBreakStmt(this);
    else
        return visitor->visitChildren(this);
}
//----------------- ExpStmtContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::ExpStmtContext::Semi() {
    return getToken(SysyParser::Semi, 0);
}

SysyParser::ExpContext *SysyParser::ExpStmtContext::exp() {
    return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::ExpStmtContext::ExpStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysyParser::ExpStmtContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitExpStmt(this);
    else
        return visitor->visitChildren(this);
}
//----------------- IfStmt2Context ------------------------------------------------------------------

tree::TerminalNode *SysyParser::IfStmt2Context::KW_If() {
    return getToken(SysyParser::KW_If, 0);
}

tree::TerminalNode *SysyParser::IfStmt2Context::Lparen() {
    return getToken(SysyParser::Lparen, 0);
}

SysyParser::CondContext *SysyParser::IfStmt2Context::cond() {
    return getRuleContext<SysyParser::CondContext>(0);
}

tree::TerminalNode *SysyParser::IfStmt2Context::Rparen() {
    return getToken(SysyParser::Rparen, 0);
}

std::vector<SysyParser::StmtContext *> SysyParser::IfStmt2Context::stmt() {
    return getRuleContexts<SysyParser::StmtContext>();
}

SysyParser::StmtContext *SysyParser::IfStmt2Context::stmt(size_t i) {
    return getRuleContext<SysyParser::StmtContext>(i);
}

tree::TerminalNode *SysyParser::IfStmt2Context::KW_Else() {
    return getToken(SysyParser::KW_Else, 0);
}

SysyParser::IfStmt2Context::IfStmt2Context(StmtContext *ctx) { copyFrom(ctx); }


std::any SysyParser::IfStmt2Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitIfStmt2(this);
    else
        return visitor->visitChildren(this);
}
//----------------- ReturnStmtContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::ReturnStmtContext::KW_Return() {
    return getToken(SysyParser::KW_Return, 0);
}

tree::TerminalNode *SysyParser::ReturnStmtContext::Semi() {
    return getToken(SysyParser::Semi, 0);
}

SysyParser::ExpContext *SysyParser::ReturnStmtContext::exp() {
    return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::ReturnStmtContext::ReturnStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysyParser::ReturnStmtContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitReturnStmt(this);
    else
        return visitor->visitChildren(this);
}
//----------------- ContinueStmtContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::ContinueStmtContext::KW_Continue() {
    return getToken(SysyParser::KW_Continue, 0);
}

tree::TerminalNode *SysyParser::ContinueStmtContext::Semi() {
    return getToken(SysyParser::Semi, 0);
}

SysyParser::ContinueStmtContext::ContinueStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysyParser::ContinueStmtContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitContinueStmt(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::StmtContext *SysyParser::stmt() {
    StmtContext *_localctx = _tracker.createInstance<StmtContext>(_ctx, getState());
    enterRule(_localctx, 30, SysyParser::RuleStmt);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(255);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx)) {
            case 1: {
                _localctx = _tracker.createInstance<SysyParser::AssignmentContext>(_localctx);
                enterOuterAlt(_localctx, 1);
                setState(216);
                lVal();
                setState(217);
                match(SysyParser::T__0);
                setState(218);
                exp();
                setState(219);
                match(SysyParser::Semi);
                break;
            }

            case 2: {
                _localctx = _tracker.createInstance<SysyParser::ExpStmtContext>(_localctx);
                enterOuterAlt(_localctx, 2);
                setState(222);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if ((((_la & ~0x3fULL) == 0) &&
                     ((1ULL << _la) & 48379988148224) != 0)) {
                    setState(221);
                    exp();
                }
                setState(224);
                match(SysyParser::Semi);
                break;
            }

            case 3: {
                _localctx = _tracker.createInstance<SysyParser::BlockStmtContext>(_localctx);
                enterOuterAlt(_localctx, 3);
                setState(225);
                block();
                break;
            }

            case 4: {
                _localctx = _tracker.createInstance<SysyParser::IfStmt1Context>(_localctx);
                enterOuterAlt(_localctx, 4);
                setState(226);
                match(SysyParser::KW_If);
                setState(227);
                match(SysyParser::Lparen);
                setState(228);
                cond();
                setState(229);
                match(SysyParser::Rparen);
                setState(230);
                stmt();
                break;
            }

            case 5: {
                _localctx = _tracker.createInstance<SysyParser::IfStmt2Context>(_localctx);
                enterOuterAlt(_localctx, 5);
                setState(232);
                match(SysyParser::KW_If);
                setState(233);
                match(SysyParser::Lparen);
                setState(234);
                cond();
                setState(235);
                match(SysyParser::Rparen);
                setState(236);
                stmt();
                setState(237);
                match(SysyParser::KW_Else);
                setState(238);
                stmt();
                break;
            }

            case 6: {
                _localctx = _tracker.createInstance<SysyParser::WhileStmtContext>(_localctx);
                enterOuterAlt(_localctx, 6);
                setState(240);
                match(SysyParser::KW_While);
                setState(241);
                match(SysyParser::Lparen);
                setState(242);
                cond();
                setState(243);
                match(SysyParser::Rparen);
                setState(244);
                stmt();
                break;
            }

            case 7: {
                _localctx = _tracker.createInstance<SysyParser::BreakStmtContext>(_localctx);
                enterOuterAlt(_localctx, 7);
                setState(246);
                match(SysyParser::KW_Break);
                setState(247);
                match(SysyParser::Semi);
                break;
            }

            case 8: {
                _localctx = _tracker.createInstance<SysyParser::ContinueStmtContext>(_localctx);
                enterOuterAlt(_localctx, 8);
                setState(248);
                match(SysyParser::KW_Continue);
                setState(249);
                match(SysyParser::Semi);
                break;
            }

            case 9: {
                _localctx = _tracker.createInstance<SysyParser::ReturnStmtContext>(_localctx);
                enterOuterAlt(_localctx, 9);
                setState(250);
                match(SysyParser::KW_Return);
                setState(252);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if ((((_la & ~0x3fULL) == 0) &&
                     ((1ULL << _la) & 48379988148224) != 0)) {
                    setState(251);
                    exp();
                }
                setState(254);
                match(SysyParser::Semi);
                break;
            }

            default:
                break;
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- ExpContext ------------------------------------------------------------------

SysyParser::ExpContext::ExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

SysyParser::AddExpContext *SysyParser::ExpContext::addExp() {
    return getRuleContext<SysyParser::AddExpContext>(0);
}


size_t SysyParser::ExpContext::getRuleIndex() const {
    return SysyParser::RuleExp;
}


std::any SysyParser::ExpContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitExp(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::ExpContext *SysyParser::exp() {
    ExpContext *_localctx = _tracker.createInstance<ExpContext>(_ctx, getState());
    enterRule(_localctx, 32, SysyParser::RuleExp);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(257);
        addExp(0);

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- CondContext ------------------------------------------------------------------

SysyParser::CondContext::CondContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

SysyParser::LOrExpContext *SysyParser::CondContext::lOrExp() {
    return getRuleContext<SysyParser::LOrExpContext>(0);
}


size_t SysyParser::CondContext::getRuleIndex() const {
    return SysyParser::RuleCond;
}


std::any SysyParser::CondContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitCond(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::CondContext *SysyParser::cond() {
    CondContext *_localctx = _tracker.createInstance<CondContext>(_ctx, getState());
    enterRule(_localctx, 34, SysyParser::RuleCond);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(259);
        lOrExp(0);

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- LValContext ------------------------------------------------------------------

SysyParser::LValContext::LValContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysyParser::LValContext::Ident() {
    return getToken(SysyParser::Ident, 0);
}

std::vector<tree::TerminalNode *> SysyParser::LValContext::Lbarck() {
    return getTokens(SysyParser::Lbarck);
}

tree::TerminalNode *SysyParser::LValContext::Lbarck(size_t i) {
    return getToken(SysyParser::Lbarck, i);
}

std::vector<SysyParser::ExpContext *> SysyParser::LValContext::exp() {
    return getRuleContexts<SysyParser::ExpContext>();
}

SysyParser::ExpContext *SysyParser::LValContext::exp(size_t i) {
    return getRuleContext<SysyParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> SysyParser::LValContext::Rbarck() {
    return getTokens(SysyParser::Rbarck);
}

tree::TerminalNode *SysyParser::LValContext::Rbarck(size_t i) {
    return getToken(SysyParser::Rbarck, i);
}


size_t SysyParser::LValContext::getRuleIndex() const {
    return SysyParser::RuleLVal;
}


std::any SysyParser::LValContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitLVal(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::LValContext *SysyParser::lVal() {
    LValContext *_localctx = _tracker.createInstance<LValContext>(_ctx, getState());
    enterRule(_localctx, 36, SysyParser::RuleLVal);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        setState(261);
        match(SysyParser::Ident);
        setState(268);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 24, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                setState(262);
                match(SysyParser::Lbarck);
                setState(263);
                exp();
                setState(264);
                match(SysyParser::Rbarck);
            }
            setState(270);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 24, _ctx);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- PrimaryExpContext ------------------------------------------------------------------

SysyParser::PrimaryExpContext::PrimaryExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::PrimaryExpContext::getRuleIndex() const {
    return SysyParser::RulePrimaryExp;
}

void SysyParser::PrimaryExpContext::copyFrom(PrimaryExpContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- PrimaryExp2Context ------------------------------------------------------------------

SysyParser::LValContext *SysyParser::PrimaryExp2Context::lVal() {
    return getRuleContext<SysyParser::LValContext>(0);
}

SysyParser::PrimaryExp2Context::PrimaryExp2Context(PrimaryExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::PrimaryExp2Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitPrimaryExp2(this);
    else
        return visitor->visitChildren(this);
}
//----------------- PrimaryExp1Context ------------------------------------------------------------------

tree::TerminalNode *SysyParser::PrimaryExp1Context::Lparen() {
    return getToken(SysyParser::Lparen, 0);
}

SysyParser::ExpContext *SysyParser::PrimaryExp1Context::exp() {
    return getRuleContext<SysyParser::ExpContext>(0);
}

tree::TerminalNode *SysyParser::PrimaryExp1Context::Rparen() {
    return getToken(SysyParser::Rparen, 0);
}

SysyParser::PrimaryExp1Context::PrimaryExp1Context(PrimaryExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::PrimaryExp1Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitPrimaryExp1(this);
    else
        return visitor->visitChildren(this);
}
//----------------- PrimaryExp3Context ------------------------------------------------------------------

SysyParser::NumberContext *SysyParser::PrimaryExp3Context::number() {
    return getRuleContext<SysyParser::NumberContext>(0);
}

SysyParser::PrimaryExp3Context::PrimaryExp3Context(PrimaryExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::PrimaryExp3Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitPrimaryExp3(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::PrimaryExpContext *SysyParser::primaryExp() {
    PrimaryExpContext *_localctx = _tracker.createInstance<PrimaryExpContext>(_ctx, getState());
    enterRule(_localctx, 38, SysyParser::RulePrimaryExp);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(277);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case SysyParser::Lparen: {
                _localctx = _tracker.createInstance<SysyParser::PrimaryExp1Context>(_localctx);
                enterOuterAlt(_localctx, 1);
                setState(271);
                match(SysyParser::Lparen);
                setState(272);
                exp();
                setState(273);
                match(SysyParser::Rparen);
                break;
            }

            case SysyParser::Ident: {
                _localctx = _tracker.createInstance<SysyParser::PrimaryExp2Context>(_localctx);
                enterOuterAlt(_localctx, 2);
                setState(275);
                lVal();
                break;
            }

            case SysyParser::Int:
            case SysyParser::Float: {
                _localctx = _tracker.createInstance<SysyParser::PrimaryExp3Context>(_localctx);
                enterOuterAlt(_localctx, 3);
                setState(276);
                number();
                break;
            }

            default:
                throw NoViableAltException(this);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- NumberContext ------------------------------------------------------------------

SysyParser::NumberContext::NumberContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::NumberContext::getRuleIndex() const {
    return SysyParser::RuleNumber;
}

void SysyParser::NumberContext::copyFrom(NumberContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- NumberIntContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::NumberIntContext::Int() {
    return getToken(SysyParser::Int, 0);
}

SysyParser::NumberIntContext::NumberIntContext(NumberContext *ctx) { copyFrom(ctx); }


std::any SysyParser::NumberIntContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitNumberInt(this);
    else
        return visitor->visitChildren(this);
}
//----------------- NunmberFloatContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::NunmberFloatContext::Float() {
    return getToken(SysyParser::Float, 0);
}

SysyParser::NunmberFloatContext::NunmberFloatContext(NumberContext *ctx) { copyFrom(ctx); }


std::any SysyParser::NunmberFloatContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitNunmberFloat(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::NumberContext *SysyParser::number() {
    NumberContext *_localctx = _tracker.createInstance<NumberContext>(_ctx, getState());
    enterRule(_localctx, 40, SysyParser::RuleNumber);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(281);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case SysyParser::Float: {
                _localctx = _tracker.createInstance<SysyParser::NunmberFloatContext>(_localctx);
                enterOuterAlt(_localctx, 1);
                setState(279);
                match(SysyParser::Float);
                break;
            }

            case SysyParser::Int: {
                _localctx = _tracker.createInstance<SysyParser::NumberIntContext>(_localctx);
                enterOuterAlt(_localctx, 2);
                setState(280);
                match(SysyParser::Int);
                break;
            }

            default:
                throw NoViableAltException(this);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- UnaryExpContext ------------------------------------------------------------------

SysyParser::UnaryExpContext::UnaryExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::UnaryExpContext::getRuleIndex() const {
    return SysyParser::RuleUnaryExp;
}

void SysyParser::UnaryExpContext::copyFrom(UnaryExpContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- UnaryExp3Context ------------------------------------------------------------------

SysyParser::UnaryOpContext *SysyParser::UnaryExp3Context::unaryOp() {
    return getRuleContext<SysyParser::UnaryOpContext>(0);
}

SysyParser::UnaryExpContext *SysyParser::UnaryExp3Context::unaryExp() {
    return getRuleContext<SysyParser::UnaryExpContext>(0);
}

SysyParser::UnaryExp3Context::UnaryExp3Context(UnaryExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::UnaryExp3Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitUnaryExp3(this);
    else
        return visitor->visitChildren(this);
}
//----------------- UnaryExp2Context ------------------------------------------------------------------

tree::TerminalNode *SysyParser::UnaryExp2Context::Ident() {
    return getToken(SysyParser::Ident, 0);
}

tree::TerminalNode *SysyParser::UnaryExp2Context::Lparen() {
    return getToken(SysyParser::Lparen, 0);
}

tree::TerminalNode *SysyParser::UnaryExp2Context::Rparen() {
    return getToken(SysyParser::Rparen, 0);
}

SysyParser::FuncRParamsContext *SysyParser::UnaryExp2Context::funcRParams() {
    return getRuleContext<SysyParser::FuncRParamsContext>(0);
}

SysyParser::UnaryExp2Context::UnaryExp2Context(UnaryExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::UnaryExp2Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitUnaryExp2(this);
    else
        return visitor->visitChildren(this);
}
//----------------- UnaryExp1Context ------------------------------------------------------------------

SysyParser::PrimaryExpContext *SysyParser::UnaryExp1Context::primaryExp() {
    return getRuleContext<SysyParser::PrimaryExpContext>(0);
}

SysyParser::UnaryExp1Context::UnaryExp1Context(UnaryExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::UnaryExp1Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitUnaryExp1(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::UnaryExpContext *SysyParser::unaryExp() {
    UnaryExpContext *_localctx = _tracker.createInstance<UnaryExpContext>(_ctx, getState());
    enterRule(_localctx, 42, SysyParser::RuleUnaryExp);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(293);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 28, _ctx)) {
            case 1: {
                _localctx = _tracker.createInstance<SysyParser::UnaryExp1Context>(_localctx);
                enterOuterAlt(_localctx, 1);
                setState(283);
                primaryExp();
                break;
            }

            case 2: {
                _localctx = _tracker.createInstance<SysyParser::UnaryExp2Context>(_localctx);
                enterOuterAlt(_localctx, 2);
                setState(284);
                match(SysyParser::Ident);
                setState(285);
                match(SysyParser::Lparen);
                setState(287);
                _errHandler->sync(this);

                _la = _input->LA(1);
                if ((((_la & ~0x3fULL) == 0) &&
                     ((1ULL << _la) & 65972174192640) != 0)) {
                    setState(286);
                    funcRParams();
                }
                setState(289);
                match(SysyParser::Rparen);
                break;
            }

            case 3: {
                _localctx = _tracker.createInstance<SysyParser::UnaryExp3Context>(_localctx);
                enterOuterAlt(_localctx, 3);
                setState(290);
                unaryOp();
                setState(291);
                unaryExp();
                break;
            }

            default:
                break;
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- UnaryOpContext ------------------------------------------------------------------

SysyParser::UnaryOpContext::UnaryOpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysyParser::UnaryOpContext::Add() {
    return getToken(SysyParser::Add, 0);
}

tree::TerminalNode *SysyParser::UnaryOpContext::Minus() {
    return getToken(SysyParser::Minus, 0);
}

tree::TerminalNode *SysyParser::UnaryOpContext::Point() {
    return getToken(SysyParser::Point, 0);
}


size_t SysyParser::UnaryOpContext::getRuleIndex() const {
    return SysyParser::RuleUnaryOp;
}


std::any SysyParser::UnaryOpContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitUnaryOp(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::UnaryOpContext *SysyParser::unaryOp() {
    UnaryOpContext *_localctx = _tracker.createInstance<UnaryOpContext>(_ctx, getState());
    enterRule(_localctx, 44, SysyParser::RuleUnaryOp);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(295);
        _la = _input->LA(1);
        if (!((((_la & ~0x3fULL) == 0) &&
               ((1ULL << _la) & 1476395008) != 0))) {
            _errHandler->recoverInline(this);
        } else {
            _errHandler->reportMatch(this);
            consume();
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- FuncRParamsContext ------------------------------------------------------------------

SysyParser::FuncRParamsContext::FuncRParamsContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

std::vector<SysyParser::FuncRParamContext *> SysyParser::FuncRParamsContext::funcRParam() {
    return getRuleContexts<SysyParser::FuncRParamContext>();
}

SysyParser::FuncRParamContext *SysyParser::FuncRParamsContext::funcRParam(size_t i) {
    return getRuleContext<SysyParser::FuncRParamContext>(i);
}

std::vector<tree::TerminalNode *> SysyParser::FuncRParamsContext::Comma() {
    return getTokens(SysyParser::Comma);
}

tree::TerminalNode *SysyParser::FuncRParamsContext::Comma(size_t i) {
    return getToken(SysyParser::Comma, i);
}


size_t SysyParser::FuncRParamsContext::getRuleIndex() const {
    return SysyParser::RuleFuncRParams;
}


std::any SysyParser::FuncRParamsContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitFuncRParams(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::FuncRParamsContext *SysyParser::funcRParams() {
    FuncRParamsContext *_localctx = _tracker.createInstance<FuncRParamsContext>(_ctx, getState());
    enterRule(_localctx, 46, SysyParser::RuleFuncRParams);
    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(297);
        funcRParam();
        setState(302);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == SysyParser::Comma) {
            setState(298);
            match(SysyParser::Comma);
            setState(299);
            funcRParam();
            setState(304);
            _errHandler->sync(this);
            _la = _input->LA(1);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- FuncRParamContext ------------------------------------------------------------------

SysyParser::FuncRParamContext::FuncRParamContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::FuncRParamContext::getRuleIndex() const {
    return SysyParser::RuleFuncRParam;
}

void SysyParser::FuncRParamContext::copyFrom(FuncRParamContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- StringParamContext ------------------------------------------------------------------

tree::TerminalNode *SysyParser::StringParamContext::STRING() {
    return getToken(SysyParser::STRING, 0);
}

SysyParser::StringParamContext::StringParamContext(FuncRParamContext *ctx) { copyFrom(ctx); }


std::any SysyParser::StringParamContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitStringParam(this);
    else
        return visitor->visitChildren(this);
}
//----------------- ExpParamContext ------------------------------------------------------------------

SysyParser::ExpContext *SysyParser::ExpParamContext::exp() {
    return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::ExpParamContext::ExpParamContext(FuncRParamContext *ctx) { copyFrom(ctx); }


std::any SysyParser::ExpParamContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitExpParam(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::FuncRParamContext *SysyParser::funcRParam() {
    FuncRParamContext *_localctx = _tracker.createInstance<FuncRParamContext>(_ctx, getState());
    enterRule(_localctx, 48, SysyParser::RuleFuncRParam);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        setState(307);
        _errHandler->sync(this);
        switch (_input->LA(1)) {
            case SysyParser::Lparen:
            case SysyParser::Minus:
            case SysyParser::Point:
            case SysyParser::Add:
            case SysyParser::Int:
            case SysyParser::Ident:
            case SysyParser::Float: {
                _localctx = _tracker.createInstance<SysyParser::ExpParamContext>(_localctx);
                enterOuterAlt(_localctx, 1);
                setState(305);
                exp();
                break;
            }

            case SysyParser::STRING: {
                _localctx = _tracker.createInstance<SysyParser::StringParamContext>(_localctx);
                enterOuterAlt(_localctx, 2);
                setState(306);
                match(SysyParser::STRING);
                break;
            }

            default:
                throw NoViableAltException(this);
        }

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

//----------------- MulExpContext ------------------------------------------------------------------

SysyParser::MulExpContext::MulExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::MulExpContext::getRuleIndex() const {
    return SysyParser::RuleMulExp;
}

void SysyParser::MulExpContext::copyFrom(MulExpContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- MulExp2Context ------------------------------------------------------------------

SysyParser::MulExpContext *SysyParser::MulExp2Context::mulExp() {
    return getRuleContext<SysyParser::MulExpContext>(0);
}

SysyParser::UnaryExpContext *SysyParser::MulExp2Context::unaryExp() {
    return getRuleContext<SysyParser::UnaryExpContext>(0);
}

tree::TerminalNode *SysyParser::MulExp2Context::Mul() {
    return getToken(SysyParser::Mul, 0);
}

tree::TerminalNode *SysyParser::MulExp2Context::Div() {
    return getToken(SysyParser::Div, 0);
}

tree::TerminalNode *SysyParser::MulExp2Context::Mode() {
    return getToken(SysyParser::Mode, 0);
}

SysyParser::MulExp2Context::MulExp2Context(MulExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::MulExp2Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitMulExp2(this);
    else
        return visitor->visitChildren(this);
}
//----------------- MulExp1Context ------------------------------------------------------------------

SysyParser::UnaryExpContext *SysyParser::MulExp1Context::unaryExp() {
    return getRuleContext<SysyParser::UnaryExpContext>(0);
}

SysyParser::MulExp1Context::MulExp1Context(MulExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::MulExp1Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitMulExp1(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::MulExpContext *SysyParser::mulExp() {
    return mulExp(0);
}

SysyParser::MulExpContext *SysyParser::mulExp(int precedence) {
    ParserRuleContext *parentContext = _ctx;
    size_t parentState = getState();
    SysyParser::MulExpContext *_localctx = _tracker.createInstance<MulExpContext>(_ctx, parentState);
    SysyParser::MulExpContext *previousContext = _localctx;
    (void) previousContext; // Silence compiler, in case the context is not used by generated code.
    size_t startState = 50;
    enterRecursionRule(_localctx, 50, SysyParser::RuleMulExp, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        unrollRecursionContexts(parentContext);
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        _localctx = _tracker.createInstance<MulExp1Context>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(310);
        unaryExp();
        _ctx->stop = _input->LT(-1);
        setState(317);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 31, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                if (!_parseListeners.empty())
                    triggerExitRuleEvent();
                previousContext = _localctx;
                auto newContext = _tracker.createInstance<MulExp2Context>(
                        _tracker.createInstance<MulExpContext>(parentContext, parentState));
                _localctx = newContext;
                pushNewRecursionContext(newContext, startState, RuleMulExp);
                setState(312);

                if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
                setState(313);
                _la = _input->LA(1);
                if (!((((_la & ~0x3fULL) == 0) &&
                       ((1ULL << _la) & 15032385536) != 0))) {
                    _errHandler->recoverInline(this);
                } else {
                    _errHandler->reportMatch(this);
                    consume();
                }
                setState(314);
                unaryExp();
            }
            setState(319);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 31, _ctx);
        }
    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }
    return _localctx;
}

//----------------- AddExpContext ------------------------------------------------------------------

SysyParser::AddExpContext::AddExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::AddExpContext::getRuleIndex() const {
    return SysyParser::RuleAddExp;
}

void SysyParser::AddExpContext::copyFrom(AddExpContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- AddExp2Context ------------------------------------------------------------------

SysyParser::AddExpContext *SysyParser::AddExp2Context::addExp() {
    return getRuleContext<SysyParser::AddExpContext>(0);
}

SysyParser::MulExpContext *SysyParser::AddExp2Context::mulExp() {
    return getRuleContext<SysyParser::MulExpContext>(0);
}

tree::TerminalNode *SysyParser::AddExp2Context::Add() {
    return getToken(SysyParser::Add, 0);
}

tree::TerminalNode *SysyParser::AddExp2Context::Minus() {
    return getToken(SysyParser::Minus, 0);
}

SysyParser::AddExp2Context::AddExp2Context(AddExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::AddExp2Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitAddExp2(this);
    else
        return visitor->visitChildren(this);
}
//----------------- AddExp1Context ------------------------------------------------------------------

SysyParser::MulExpContext *SysyParser::AddExp1Context::mulExp() {
    return getRuleContext<SysyParser::MulExpContext>(0);
}

SysyParser::AddExp1Context::AddExp1Context(AddExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::AddExp1Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitAddExp1(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::AddExpContext *SysyParser::addExp() {
    return addExp(0);
}

SysyParser::AddExpContext *SysyParser::addExp(int precedence) {
    ParserRuleContext *parentContext = _ctx;
    size_t parentState = getState();
    SysyParser::AddExpContext *_localctx = _tracker.createInstance<AddExpContext>(_ctx, parentState);
    SysyParser::AddExpContext *previousContext = _localctx;
    (void) previousContext; // Silence compiler, in case the context is not used by generated code.
    size_t startState = 52;
    enterRecursionRule(_localctx, 52, SysyParser::RuleAddExp, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        unrollRecursionContexts(parentContext);
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        _localctx = _tracker.createInstance<AddExp1Context>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(321);
        mulExp(0);
        _ctx->stop = _input->LT(-1);
        setState(328);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 32, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                if (!_parseListeners.empty())
                    triggerExitRuleEvent();
                previousContext = _localctx;
                auto newContext = _tracker.createInstance<AddExp2Context>(
                        _tracker.createInstance<AddExpContext>(parentContext, parentState));
                _localctx = newContext;
                pushNewRecursionContext(newContext, startState, RuleAddExp);
                setState(323);

                if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
                setState(324);
                _la = _input->LA(1);
                if (!(_la == SysyParser::Minus

                      || _la == SysyParser::Add)) {
                    _errHandler->recoverInline(this);
                } else {
                    _errHandler->reportMatch(this);
                    consume();
                }
                setState(325);
                mulExp(0);
            }
            setState(330);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 32, _ctx);
        }
    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }
    return _localctx;
}

//----------------- RelExpContext ------------------------------------------------------------------

SysyParser::RelExpContext::RelExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::RelExpContext::getRuleIndex() const {
    return SysyParser::RuleRelExp;
}

void SysyParser::RelExpContext::copyFrom(RelExpContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- RelExp2Context ------------------------------------------------------------------

SysyParser::RelExpContext *SysyParser::RelExp2Context::relExp() {
    return getRuleContext<SysyParser::RelExpContext>(0);
}

SysyParser::AddExpContext *SysyParser::RelExp2Context::addExp() {
    return getRuleContext<SysyParser::AddExpContext>(0);
}

tree::TerminalNode *SysyParser::RelExp2Context::LT() {
    return getToken(SysyParser::LT, 0);
}

tree::TerminalNode *SysyParser::RelExp2Context::GT() {
    return getToken(SysyParser::GT, 0);
}

tree::TerminalNode *SysyParser::RelExp2Context::LE() {
    return getToken(SysyParser::LE, 0);
}

tree::TerminalNode *SysyParser::RelExp2Context::GE() {
    return getToken(SysyParser::GE, 0);
}

SysyParser::RelExp2Context::RelExp2Context(RelExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::RelExp2Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitRelExp2(this);
    else
        return visitor->visitChildren(this);
}
//----------------- RelExp1Context ------------------------------------------------------------------

SysyParser::AddExpContext *SysyParser::RelExp1Context::addExp() {
    return getRuleContext<SysyParser::AddExpContext>(0);
}

SysyParser::RelExp1Context::RelExp1Context(RelExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::RelExp1Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitRelExp1(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::RelExpContext *SysyParser::relExp() {
    return relExp(0);
}

SysyParser::RelExpContext *SysyParser::relExp(int precedence) {
    ParserRuleContext *parentContext = _ctx;
    size_t parentState = getState();
    SysyParser::RelExpContext *_localctx = _tracker.createInstance<RelExpContext>(_ctx, parentState);
    SysyParser::RelExpContext *previousContext = _localctx;
    (void) previousContext; // Silence compiler, in case the context is not used by generated code.
    size_t startState = 54;
    enterRecursionRule(_localctx, 54, SysyParser::RuleRelExp, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        unrollRecursionContexts(parentContext);
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        _localctx = _tracker.createInstance<RelExp1Context>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(332);
        addExp(0);
        _ctx->stop = _input->LT(-1);
        setState(339);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 33, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                if (!_parseListeners.empty())
                    triggerExitRuleEvent();
                previousContext = _localctx;
                auto newContext = _tracker.createInstance<RelExp2Context>(
                        _tracker.createInstance<RelExpContext>(parentContext, parentState));
                _localctx = newContext;
                pushNewRecursionContext(newContext, startState, RuleRelExp);
                setState(334);

                if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
                setState(335);
                _la = _input->LA(1);
                if (!((((_la & ~0x3fULL) == 0) &&
                       ((1ULL << _la) & 4123168604160) != 0))) {
                    _errHandler->recoverInline(this);
                } else {
                    _errHandler->reportMatch(this);
                    consume();
                }
                setState(336);
                addExp(0);
            }
            setState(341);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 33, _ctx);
        }
    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }
    return _localctx;
}

//----------------- EqExpContext ------------------------------------------------------------------

SysyParser::EqExpContext::EqExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::EqExpContext::getRuleIndex() const {
    return SysyParser::RuleEqExp;
}

void SysyParser::EqExpContext::copyFrom(EqExpContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- EqExp2Context ------------------------------------------------------------------

SysyParser::EqExpContext *SysyParser::EqExp2Context::eqExp() {
    return getRuleContext<SysyParser::EqExpContext>(0);
}

SysyParser::RelExpContext *SysyParser::EqExp2Context::relExp() {
    return getRuleContext<SysyParser::RelExpContext>(0);
}

tree::TerminalNode *SysyParser::EqExp2Context::EQ() {
    return getToken(SysyParser::EQ, 0);
}

tree::TerminalNode *SysyParser::EqExp2Context::NEQ() {
    return getToken(SysyParser::NEQ, 0);
}

SysyParser::EqExp2Context::EqExp2Context(EqExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::EqExp2Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitEqExp2(this);
    else
        return visitor->visitChildren(this);
}
//----------------- EqExp1Context ------------------------------------------------------------------

SysyParser::RelExpContext *SysyParser::EqExp1Context::relExp() {
    return getRuleContext<SysyParser::RelExpContext>(0);
}

SysyParser::EqExp1Context::EqExp1Context(EqExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::EqExp1Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitEqExp1(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::EqExpContext *SysyParser::eqExp() {
    return eqExp(0);
}

SysyParser::EqExpContext *SysyParser::eqExp(int precedence) {
    ParserRuleContext *parentContext = _ctx;
    size_t parentState = getState();
    SysyParser::EqExpContext *_localctx = _tracker.createInstance<EqExpContext>(_ctx, parentState);
    SysyParser::EqExpContext *previousContext = _localctx;
    (void) previousContext; // Silence compiler, in case the context is not used by generated code.
    size_t startState = 56;
    enterRecursionRule(_localctx, 56, SysyParser::RuleEqExp, precedence);

    size_t _la = 0;

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        unrollRecursionContexts(parentContext);
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        _localctx = _tracker.createInstance<EqExp1Context>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(343);
        relExp(0);
        _ctx->stop = _input->LT(-1);
        setState(350);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                if (!_parseListeners.empty())
                    triggerExitRuleEvent();
                previousContext = _localctx;
                auto newContext = _tracker.createInstance<EqExp2Context>(
                        _tracker.createInstance<EqExpContext>(parentContext, parentState));
                _localctx = newContext;
                pushNewRecursionContext(newContext, startState, RuleEqExp);
                setState(345);

                if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
                setState(346);
                _la = _input->LA(1);
                if (!(_la == SysyParser::EQ

                      || _la == SysyParser::NEQ)) {
                    _errHandler->recoverInline(this);
                } else {
                    _errHandler->reportMatch(this);
                    consume();
                }
                setState(347);
                relExp(0);
            }
            setState(352);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
        }
    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }
    return _localctx;
}

//----------------- LAndExpContext ------------------------------------------------------------------

SysyParser::LAndExpContext::LAndExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::LAndExpContext::getRuleIndex() const {
    return SysyParser::RuleLAndExp;
}

void SysyParser::LAndExpContext::copyFrom(LAndExpContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- LAndExp1Context ------------------------------------------------------------------

SysyParser::EqExpContext *SysyParser::LAndExp1Context::eqExp() {
    return getRuleContext<SysyParser::EqExpContext>(0);
}

SysyParser::LAndExp1Context::LAndExp1Context(LAndExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::LAndExp1Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitLAndExp1(this);
    else
        return visitor->visitChildren(this);
}
//----------------- LAndExp2Context ------------------------------------------------------------------

SysyParser::LAndExpContext *SysyParser::LAndExp2Context::lAndExp() {
    return getRuleContext<SysyParser::LAndExpContext>(0);
}

tree::TerminalNode *SysyParser::LAndExp2Context::LAND() {
    return getToken(SysyParser::LAND, 0);
}

SysyParser::EqExpContext *SysyParser::LAndExp2Context::eqExp() {
    return getRuleContext<SysyParser::EqExpContext>(0);
}

SysyParser::LAndExp2Context::LAndExp2Context(LAndExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::LAndExp2Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitLAndExp2(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::LAndExpContext *SysyParser::lAndExp() {
    return lAndExp(0);
}

SysyParser::LAndExpContext *SysyParser::lAndExp(int precedence) {
    ParserRuleContext *parentContext = _ctx;
    size_t parentState = getState();
    SysyParser::LAndExpContext *_localctx = _tracker.createInstance<LAndExpContext>(_ctx, parentState);
    SysyParser::LAndExpContext *previousContext = _localctx;
    (void) previousContext; // Silence compiler, in case the context is not used by generated code.
    size_t startState = 58;
    enterRecursionRule(_localctx, 58, SysyParser::RuleLAndExp, precedence);


#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        unrollRecursionContexts(parentContext);
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        _localctx = _tracker.createInstance<LAndExp1Context>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(354);
        eqExp(0);
        _ctx->stop = _input->LT(-1);
        setState(361);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 35, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                if (!_parseListeners.empty())
                    triggerExitRuleEvent();
                previousContext = _localctx;
                auto newContext = _tracker.createInstance<LAndExp2Context>(
                        _tracker.createInstance<LAndExpContext>(parentContext, parentState));
                _localctx = newContext;
                pushNewRecursionContext(newContext, startState, RuleLAndExp);
                setState(356);

                if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
                setState(357);
                match(SysyParser::LAND);
                setState(358);
                eqExp(0);
            }
            setState(363);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 35, _ctx);
        }
    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }
    return _localctx;
}

//----------------- LOrExpContext ------------------------------------------------------------------

SysyParser::LOrExpContext::LOrExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::LOrExpContext::getRuleIndex() const {
    return SysyParser::RuleLOrExp;
}

void SysyParser::LOrExpContext::copyFrom(LOrExpContext *ctx) {
    ParserRuleContext::copyFrom(ctx);
}

//----------------- LOrExp2Context ------------------------------------------------------------------

SysyParser::LOrExpContext *SysyParser::LOrExp2Context::lOrExp() {
    return getRuleContext<SysyParser::LOrExpContext>(0);
}

tree::TerminalNode *SysyParser::LOrExp2Context::LOR() {
    return getToken(SysyParser::LOR, 0);
}

SysyParser::LAndExpContext *SysyParser::LOrExp2Context::lAndExp() {
    return getRuleContext<SysyParser::LAndExpContext>(0);
}

SysyParser::LOrExp2Context::LOrExp2Context(LOrExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::LOrExp2Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitLOrExp2(this);
    else
        return visitor->visitChildren(this);
}
//----------------- LOrExp1Context ------------------------------------------------------------------

SysyParser::LAndExpContext *SysyParser::LOrExp1Context::lAndExp() {
    return getRuleContext<SysyParser::LAndExpContext>(0);
}

SysyParser::LOrExp1Context::LOrExp1Context(LOrExpContext *ctx) { copyFrom(ctx); }


std::any SysyParser::LOrExp1Context::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitLOrExp1(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::LOrExpContext *SysyParser::lOrExp() {
    return lOrExp(0);
}

SysyParser::LOrExpContext *SysyParser::lOrExp(int precedence) {
    ParserRuleContext *parentContext = _ctx;
    size_t parentState = getState();
    SysyParser::LOrExpContext *_localctx = _tracker.createInstance<LOrExpContext>(_ctx, parentState);
    SysyParser::LOrExpContext *previousContext = _localctx;
    (void) previousContext; // Silence compiler, in case the context is not used by generated code.
    size_t startState = 60;
    enterRecursionRule(_localctx, 60, SysyParser::RuleLOrExp, precedence);


#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        unrollRecursionContexts(parentContext);
    });
    try {
        size_t alt;
        enterOuterAlt(_localctx, 1);
        _localctx = _tracker.createInstance<LOrExp1Context>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(365);
        lAndExp(0);
        _ctx->stop = _input->LT(-1);
        setState(372);
        _errHandler->sync(this);
        alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 36, _ctx);
        while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
            if (alt == 1) {
                if (!_parseListeners.empty())
                    triggerExitRuleEvent();
                previousContext = _localctx;
                auto newContext = _tracker.createInstance<LOrExp2Context>(
                        _tracker.createInstance<LOrExpContext>(parentContext, parentState));
                _localctx = newContext;
                pushNewRecursionContext(newContext, startState, RuleLOrExp);
                setState(367);

                if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
                setState(368);
                match(SysyParser::LOR);
                setState(369);
                lAndExp(0);
            }
            setState(374);
            _errHandler->sync(this);
            alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 36, _ctx);
        }
    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }
    return _localctx;
}

//----------------- ConstExpContext ------------------------------------------------------------------

SysyParser::ConstExpContext::ConstExpContext(ParserRuleContext *parent, size_t invokingState)
        : ParserRuleContext(parent, invokingState) {
}

SysyParser::AddExpContext *SysyParser::ConstExpContext::addExp() {
    return getRuleContext<SysyParser::AddExpContext>(0);
}


size_t SysyParser::ConstExpContext::getRuleIndex() const {
    return SysyParser::RuleConstExp;
}


std::any SysyParser::ConstExpContext::accept(tree::ParseTreeVisitor *visitor) {
    if (auto parserVisitor = dynamic_cast<SysyVisitor *>(visitor))
        return parserVisitor->visitConstExp(this);
    else
        return visitor->visitChildren(this);
}

SysyParser::ConstExpContext *SysyParser::constExp() {
    ConstExpContext *_localctx = _tracker.createInstance<ConstExpContext>(_ctx, getState());
    enterRule(_localctx, 62, SysyParser::RuleConstExp);

#if __cplusplus > 201703L
    auto onExit = finally([=, this] {
#else
    auto onExit = finally([=] {
#endif
        exitRule();
    });
    try {
        enterOuterAlt(_localctx, 1);
        setState(375);
        addExp(0);

    }
    catch (RecognitionException &e) {
        _errHandler->reportError(this, e);
        _localctx->exception = std::current_exception();
        _errHandler->recover(this, _localctx->exception);
    }

    return _localctx;
}

bool SysyParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
    switch (ruleIndex) {
        case 25:
            return mulExpSempred(antlrcpp::downCast<MulExpContext *>(context), predicateIndex);
        case 26:
            return addExpSempred(antlrcpp::downCast<AddExpContext *>(context), predicateIndex);
        case 27:
            return relExpSempred(antlrcpp::downCast<RelExpContext *>(context), predicateIndex);
        case 28:
            return eqExpSempred(antlrcpp::downCast<EqExpContext *>(context), predicateIndex);
        case 29:
            return lAndExpSempred(antlrcpp::downCast<LAndExpContext *>(context), predicateIndex);
        case 30:
            return lOrExpSempred(antlrcpp::downCast<LOrExpContext *>(context), predicateIndex);

        default:
            break;
    }
    return true;
}

bool SysyParser::mulExpSempred(MulExpContext *_localctx, size_t predicateIndex) {
    switch (predicateIndex) {
        case 0:
            return precpred(_ctx, 1);

        default:
            break;
    }
    return true;
}

bool SysyParser::addExpSempred(AddExpContext *_localctx, size_t predicateIndex) {
    switch (predicateIndex) {
        case 1:
            return precpred(_ctx, 1);

        default:
            break;
    }
    return true;
}

bool SysyParser::relExpSempred(RelExpContext *_localctx, size_t predicateIndex) {
    switch (predicateIndex) {
        case 2:
            return precpred(_ctx, 1);

        default:
            break;
    }
    return true;
}

bool SysyParser::eqExpSempred(EqExpContext *_localctx, size_t predicateIndex) {
    switch (predicateIndex) {
        case 3:
            return precpred(_ctx, 1);

        default:
            break;
    }
    return true;
}

bool SysyParser::lAndExpSempred(LAndExpContext *_localctx, size_t predicateIndex) {
    switch (predicateIndex) {
        case 4:
            return precpred(_ctx, 1);

        default:
            break;
    }
    return true;
}

bool SysyParser::lOrExpSempred(LOrExpContext *_localctx, size_t predicateIndex) {
    switch (predicateIndex) {
        case 5:
            return precpred(_ctx, 1);

        default:
            break;
    }
    return true;
}

void SysyParser::initialize() {
    ::antlr4::internal::call_once(sysyParserOnceFlag, sysyParserInitialize);
}
