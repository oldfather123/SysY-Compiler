
// Generated from ./src/antlr4/SysY.g4 by ANTLR 4.12.0

#include "SysYParser.h"

#include "SysYVisitor.h"

using namespace antlrcpp;

using namespace antlr4;

namespace {

struct SysYParserStaticData final {
  SysYParserStaticData(std::vector<std::string> ruleNames, std::vector<std::string> literalNames,
                       std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)),
        literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {
  }

  SysYParserStaticData(const SysYParserStaticData &) = delete;
  SysYParserStaticData(SysYParserStaticData &&) = delete;
  SysYParserStaticData &operator=(const SysYParserStaticData &) = delete;
  SysYParserStaticData &operator=(SysYParserStaticData &&) = delete;

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
SysYParserStaticData *sysyParserStaticData = nullptr;

void sysyParserInitialize() {
  assert(sysyParserStaticData == nullptr);
  auto staticData = std::make_unique<SysYParserStaticData>(
      std::vector<std::string>{"comp",       "decl",         "varDef",      "init",      "funcDef",   "funcFParams",
                               "funcFParam", "stmt",         "assignStmt",  "expStmt",   "ifStmt",    "whileStmt",
                               "breakStmt",  "continueStmt", "returnStmt",  "blockStmt", "blockItem", "emptyStmt",
                               "exp",        "call",         "funcRParams", "lValue",    "number",    "string",
                               "returnType", "bType"},
      std::vector<std::string>{"",     "'int'",  "'float'",    "'void'",  "'const'",  "'for'", "'do'", "'while'",
                               "'if'", "'else'", "'continue'", "'break'", "'return'", "'('",   "')'",  "'['",
                               "']'",  "'{'",    "'}'",        "','",     "';'",      "'='",   "'+'",  "'-'",
                               "'*'",  "'/'",    "'%'",        "'<'",     "'>'",      "'<='",  "'>='", "'=='",
                               "'!='", "'&&'",   "'||'",       "'!'"},
      std::vector<std::string>{
          "",         "INT",         "FLOAT",       "VOID",  "CONST",      "FOR",          "DO",         "WHILE",
          "IF",       "ELSE",        "CONTINUE",    "BREAK", "RETURN",     "LPAREN",       "RPAREN",     "LBRACKET",
          "RBRACKET", "LBRACES",     "RBRACES",     "COMMA", "SEMICOLON",  "ASSIGN",       "ADD",        "SUB",
          "MUL",      "DIV",         "MOD",         "LT",    "GT",         "LE",           "GE",         "EQ",
          "NE",       "AND",         "OR",          "NOT",   "INTLITERAL", "FLOATLITERAL", "IDENTIFIER", "STRING",
          "SPACE",    "LINECOMMENT", "BLOCKCOMMENT"});
  static const int32_t serializedATNSegment[] = {
      4,   1,   42,  260, 2,   0,   7,   0,   2,   1,   7,   1,   2,   2,   7,   2,   2,   3,   7,   3,   2,   4,   7,
      4,   2,   5,   7,   5,   2,   6,   7,   6,   2,   7,   7,   7,   2,   8,   7,   8,   2,   9,   7,   9,   2,   10,
      7,   10,  2,   11,  7,   11,  2,   12,  7,   12,  2,   13,  7,   13,  2,   14,  7,   14,  2,   15,  7,   15,  2,
      16,  7,   16,  2,   17,  7,   17,  2,   18,  7,   18,  2,   19,  7,   19,  2,   20,  7,   20,  2,   21,  7,   21,
      2,   22,  7,   22,  2,   23,  7,   23,  2,   24,  7,   24,  2,   25,  7,   25,  1,   0,   1,   0,   5,   0,   55,
      8,   0,   10,  0,   12,  0,   58,  9,   0,   1,   0,   1,   0,   1,   1,   3,   1,   63,  8,   1,   1,   1,   1,
      1,   1,   1,   1,   1,   5,   1,   69,  8,   1,   10,  1,   12,  1,   72,  9,   1,   1,   1,   1,   1,   1,   2,
      1,   2,   1,   2,   1,   2,   1,   2,   3,   2,   81,  8,   2,   1,   3,   1,   3,   1,   3,   1,   3,   1,   3,
      5,   3,   88,  8,   3,   10,  3,   12,  3,   91,  9,   3,   3,   3,   93,  8,   3,   1,   3,   3,   3,   96,  8,
      3,   1,   4,   1,   4,   1,   4,   1,   4,   3,   4,   102, 8,   4,   1,   4,   1,   4,   1,   4,   1,   5,   1,
      5,   1,   5,   5,   5,   110, 8,   5,   10,  5,   12,  5,   113, 9,   5,   1,   6,   1,   6,   1,   6,   1,   6,
      1,   6,   1,   6,   1,   6,   1,   6,   5,   6,   123, 8,   6,   10,  6,   12,  6,   126, 9,   6,   3,   6,   128,
      8,   6,   1,   7,   1,   7,   1,   7,   1,   7,   1,   7,   1,   7,   1,   7,   1,   7,   1,   7,   3,   7,   139,
      8,   7,   1,   8,   1,   8,   1,   8,   1,   8,   1,   8,   1,   9,   1,   9,   1,   9,   1,   10,  1,   10,  1,
      10,  1,   10,  1,   10,  1,   10,  1,   10,  3,   10,  156, 8,   10,  1,   11,  1,   11,  1,   11,  1,   11,  1,
      11,  1,   11,  1,   12,  1,   12,  1,   12,  1,   13,  1,   13,  1,   13,  1,   14,  1,   14,  3,   14,  172, 8,
      14,  1,   14,  1,   14,  1,   15,  1,   15,  5,   15,  178, 8,   15,  10,  15,  12,  15,  181, 9,   15,  1,   15,
      1,   15,  1,   16,  1,   16,  3,   16,  187, 8,   16,  1,   17,  1,   17,  1,   18,  1,   18,  1,   18,  1,   18,
      1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  3,   18,  202, 8,   18,  1,   18,  1,   18,
      1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,   18,  1,
      18,  1,   18,  1,   18,  1,   18,  1,   18,  5,   18,  222, 8,   18,  10,  18,  12,  18,  225, 9,   18,  1,   19,
      1,   19,  1,   19,  3,   19,  230, 8,   19,  1,   19,  1,   19,  1,   20,  1,   20,  1,   20,  5,   20,  237, 8,
      20,  10,  20,  12,  20,  240, 9,   20,  1,   21,  1,   21,  1,   21,  1,   21,  1,   21,  5,   21,  247, 8,   21,
      10,  21,  12,  21,  250, 9,   21,  1,   22,  1,   22,  1,   23,  1,   23,  1,   24,  1,   24,  1,   25,  1,   25,
      1,   25,  0,   1,   36,  26,  0,   2,   4,   6,   8,   10,  12,  14,  16,  18,  20,  22,  24,  26,  28,  30,  32,
      34,  36,  38,  40,  42,  44,  46,  48,  50,  0,   8,   2,   0,   22,  23,  35,  35,  1,   0,   24,  26,  1,   0,
      22,  23,  1,   0,   27,  30,  1,   0,   31,  32,  1,   0,   36,  37,  1,   0,   1,   3,   1,   0,   1,   2,   271,
      0,   56,  1,   0,   0,   0,   2,   62,  1,   0,   0,   0,   4,   80,  1,   0,   0,   0,   6,   95,  1,   0,   0,
      0,   8,   97,  1,   0,   0,   0,   10,  106, 1,   0,   0,   0,   12,  114, 1,   0,   0,   0,   14,  138, 1,   0,
      0,   0,   16,  140, 1,   0,   0,   0,   18,  145, 1,   0,   0,   0,   20,  148, 1,   0,   0,   0,   22,  157, 1,
      0,   0,   0,   24,  163, 1,   0,   0,   0,   26,  166, 1,   0,   0,   0,   28,  169, 1,   0,   0,   0,   30,  175,
      1,   0,   0,   0,   32,  186, 1,   0,   0,   0,   34,  188, 1,   0,   0,   0,   36,  201, 1,   0,   0,   0,   38,
      226, 1,   0,   0,   0,   40,  233, 1,   0,   0,   0,   42,  241, 1,   0,   0,   0,   44,  251, 1,   0,   0,   0,
      46,  253, 1,   0,   0,   0,   48,  255, 1,   0,   0,   0,   50,  257, 1,   0,   0,   0,   52,  55,  3,   2,   1,
      0,   53,  55,  3,   8,   4,   0,   54,  52,  1,   0,   0,   0,   54,  53,  1,   0,   0,   0,   55,  58,  1,   0,
      0,   0,   56,  54,  1,   0,   0,   0,   56,  57,  1,   0,   0,   0,   57,  59,  1,   0,   0,   0,   58,  56,  1,
      0,   0,   0,   59,  60,  5,   0,   0,   1,   60,  1,   1,   0,   0,   0,   61,  63,  5,   4,   0,   0,   62,  61,
      1,   0,   0,   0,   62,  63,  1,   0,   0,   0,   63,  64,  1,   0,   0,   0,   64,  65,  3,   50,  25,  0,   65,
      70,  3,   4,   2,   0,   66,  67,  5,   19,  0,   0,   67,  69,  3,   4,   2,   0,   68,  66,  1,   0,   0,   0,
      69,  72,  1,   0,   0,   0,   70,  68,  1,   0,   0,   0,   70,  71,  1,   0,   0,   0,   71,  73,  1,   0,   0,
      0,   72,  70,  1,   0,   0,   0,   73,  74,  5,   20,  0,   0,   74,  3,   1,   0,   0,   0,   75,  81,  3,   42,
      21,  0,   76,  77,  3,   42,  21,  0,   77,  78,  5,   21,  0,   0,   78,  79,  3,   6,   3,   0,   79,  81,  1,
      0,   0,   0,   80,  75,  1,   0,   0,   0,   80,  76,  1,   0,   0,   0,   81,  5,   1,   0,   0,   0,   82,  96,
      3,   36,  18,  0,   83,  92,  5,   17,  0,   0,   84,  89,  3,   6,   3,   0,   85,  86,  5,   19,  0,   0,   86,
      88,  3,   6,   3,   0,   87,  85,  1,   0,   0,   0,   88,  91,  1,   0,   0,   0,   89,  87,  1,   0,   0,   0,
      89,  90,  1,   0,   0,   0,   90,  93,  1,   0,   0,   0,   91,  89,  1,   0,   0,   0,   92,  84,  1,   0,   0,
      0,   92,  93,  1,   0,   0,   0,   93,  94,  1,   0,   0,   0,   94,  96,  5,   18,  0,   0,   95,  82,  1,   0,
      0,   0,   95,  83,  1,   0,   0,   0,   96,  7,   1,   0,   0,   0,   97,  98,  3,   48,  24,  0,   98,  99,  5,
      38,  0,   0,   99,  101, 5,   13,  0,   0,   100, 102, 3,   10,  5,   0,   101, 100, 1,   0,   0,   0,   101, 102,
      1,   0,   0,   0,   102, 103, 1,   0,   0,   0,   103, 104, 5,   14,  0,   0,   104, 105, 3,   30,  15,  0,   105,
      9,   1,   0,   0,   0,   106, 111, 3,   12,  6,   0,   107, 108, 5,   19,  0,   0,   108, 110, 3,   12,  6,   0,
      109, 107, 1,   0,   0,   0,   110, 113, 1,   0,   0,   0,   111, 109, 1,   0,   0,   0,   111, 112, 1,   0,   0,
      0,   112, 11,  1,   0,   0,   0,   113, 111, 1,   0,   0,   0,   114, 115, 3,   50,  25,  0,   115, 127, 5,   38,
      0,   0,   116, 117, 5,   15,  0,   0,   117, 124, 5,   16,  0,   0,   118, 119, 5,   15,  0,   0,   119, 120, 3,
      36,  18,  0,   120, 121, 5,   16,  0,   0,   121, 123, 1,   0,   0,   0,   122, 118, 1,   0,   0,   0,   123, 126,
      1,   0,   0,   0,   124, 122, 1,   0,   0,   0,   124, 125, 1,   0,   0,   0,   125, 128, 1,   0,   0,   0,   126,
      124, 1,   0,   0,   0,   127, 116, 1,   0,   0,   0,   127, 128, 1,   0,   0,   0,   128, 13,  1,   0,   0,   0,
      129, 139, 3,   16,  8,   0,   130, 139, 3,   18,  9,   0,   131, 139, 3,   20,  10,  0,   132, 139, 3,   22,  11,
      0,   133, 139, 3,   24,  12,  0,   134, 139, 3,   26,  13,  0,   135, 139, 3,   28,  14,  0,   136, 139, 3,   30,
      15,  0,   137, 139, 3,   34,  17,  0,   138, 129, 1,   0,   0,   0,   138, 130, 1,   0,   0,   0,   138, 131, 1,
      0,   0,   0,   138, 132, 1,   0,   0,   0,   138, 133, 1,   0,   0,   0,   138, 134, 1,   0,   0,   0,   138, 135,
      1,   0,   0,   0,   138, 136, 1,   0,   0,   0,   138, 137, 1,   0,   0,   0,   139, 15,  1,   0,   0,   0,   140,
      141, 3,   42,  21,  0,   141, 142, 5,   21,  0,   0,   142, 143, 3,   36,  18,  0,   143, 144, 5,   20,  0,   0,
      144, 17,  1,   0,   0,   0,   145, 146, 3,   36,  18,  0,   146, 147, 5,   20,  0,   0,   147, 19,  1,   0,   0,
      0,   148, 149, 5,   8,   0,   0,   149, 150, 5,   13,  0,   0,   150, 151, 3,   36,  18,  0,   151, 152, 5,   14,
      0,   0,   152, 155, 3,   14,  7,   0,   153, 154, 5,   9,   0,   0,   154, 156, 3,   14,  7,   0,   155, 153, 1,
      0,   0,   0,   155, 156, 1,   0,   0,   0,   156, 21,  1,   0,   0,   0,   157, 158, 5,   7,   0,   0,   158, 159,
      5,   13,  0,   0,   159, 160, 3,   36,  18,  0,   160, 161, 5,   14,  0,   0,   161, 162, 3,   14,  7,   0,   162,
      23,  1,   0,   0,   0,   163, 164, 5,   11,  0,   0,   164, 165, 5,   20,  0,   0,   165, 25,  1,   0,   0,   0,
      166, 167, 5,   10,  0,   0,   167, 168, 5,   20,  0,   0,   168, 27,  1,   0,   0,   0,   169, 171, 5,   12,  0,
      0,   170, 172, 3,   36,  18,  0,   171, 170, 1,   0,   0,   0,   171, 172, 1,   0,   0,   0,   172, 173, 1,   0,
      0,   0,   173, 174, 5,   20,  0,   0,   174, 29,  1,   0,   0,   0,   175, 179, 5,   17,  0,   0,   176, 178, 3,
      32,  16,  0,   177, 176, 1,   0,   0,   0,   178, 181, 1,   0,   0,   0,   179, 177, 1,   0,   0,   0,   179, 180,
      1,   0,   0,   0,   180, 182, 1,   0,   0,   0,   181, 179, 1,   0,   0,   0,   182, 183, 5,   18,  0,   0,   183,
      31,  1,   0,   0,   0,   184, 187, 3,   2,   1,   0,   185, 187, 3,   14,  7,   0,   186, 184, 1,   0,   0,   0,
      186, 185, 1,   0,   0,   0,   187, 33,  1,   0,   0,   0,   188, 189, 5,   20,  0,   0,   189, 35,  1,   0,   0,
      0,   190, 191, 6,   18,  -1,  0,   191, 192, 5,   13,  0,   0,   192, 193, 3,   36,  18,  0,   193, 194, 5,   14,
      0,   0,   194, 202, 1,   0,   0,   0,   195, 202, 3,   42,  21,  0,   196, 202, 3,   44,  22,  0,   197, 202, 3,
      46,  23,  0,   198, 202, 3,   38,  19,  0,   199, 200, 7,   0,   0,   0,   200, 202, 3,   36,  18,  7,   201, 190,
      1,   0,   0,   0,   201, 195, 1,   0,   0,   0,   201, 196, 1,   0,   0,   0,   201, 197, 1,   0,   0,   0,   201,
      198, 1,   0,   0,   0,   201, 199, 1,   0,   0,   0,   202, 223, 1,   0,   0,   0,   203, 204, 10,  6,   0,   0,
      204, 205, 7,   1,   0,   0,   205, 222, 3,   36,  18,  7,   206, 207, 10,  5,   0,   0,   207, 208, 7,   2,   0,
      0,   208, 222, 3,   36,  18,  6,   209, 210, 10,  4,   0,   0,   210, 211, 7,   3,   0,   0,   211, 222, 3,   36,
      18,  5,   212, 213, 10,  3,   0,   0,   213, 214, 7,   4,   0,   0,   214, 222, 3,   36,  18,  4,   215, 216, 10,
      2,   0,   0,   216, 217, 5,   33,  0,   0,   217, 222, 3,   36,  18,  3,   218, 219, 10,  1,   0,   0,   219, 220,
      5,   34,  0,   0,   220, 222, 3,   36,  18,  2,   221, 203, 1,   0,   0,   0,   221, 206, 1,   0,   0,   0,   221,
      209, 1,   0,   0,   0,   221, 212, 1,   0,   0,   0,   221, 215, 1,   0,   0,   0,   221, 218, 1,   0,   0,   0,
      222, 225, 1,   0,   0,   0,   223, 221, 1,   0,   0,   0,   223, 224, 1,   0,   0,   0,   224, 37,  1,   0,   0,
      0,   225, 223, 1,   0,   0,   0,   226, 227, 5,   38,  0,   0,   227, 229, 5,   13,  0,   0,   228, 230, 3,   40,
      20,  0,   229, 228, 1,   0,   0,   0,   229, 230, 1,   0,   0,   0,   230, 231, 1,   0,   0,   0,   231, 232, 5,
      14,  0,   0,   232, 39,  1,   0,   0,   0,   233, 238, 3,   36,  18,  0,   234, 235, 5,   19,  0,   0,   235, 237,
      3,   36,  18,  0,   236, 234, 1,   0,   0,   0,   237, 240, 1,   0,   0,   0,   238, 236, 1,   0,   0,   0,   238,
      239, 1,   0,   0,   0,   239, 41,  1,   0,   0,   0,   240, 238, 1,   0,   0,   0,   241, 248, 5,   38,  0,   0,
      242, 243, 5,   15,  0,   0,   243, 244, 3,   36,  18,  0,   244, 245, 5,   16,  0,   0,   245, 247, 1,   0,   0,
      0,   246, 242, 1,   0,   0,   0,   247, 250, 1,   0,   0,   0,   248, 246, 1,   0,   0,   0,   248, 249, 1,   0,
      0,   0,   249, 43,  1,   0,   0,   0,   250, 248, 1,   0,   0,   0,   251, 252, 7,   5,   0,   0,   252, 45,  1,
      0,   0,   0,   253, 254, 5,   39,  0,   0,   254, 47,  1,   0,   0,   0,   255, 256, 7,   6,   0,   0,   256, 49,
      1,   0,   0,   0,   257, 258, 7,   7,   0,   0,   258, 51,  1,   0,   0,   0,   23,  54,  56,  62,  70,  80,  89,
      92,  95,  101, 111, 124, 127, 138, 155, 171, 179, 186, 201, 221, 223, 229, 238, 248};
  staticData->serializedATN = antlr4::atn::SerializedATNView(
      serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) {
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  sysyParserStaticData = staticData.release();
}

}  // namespace

SysYParser::SysYParser(TokenStream *input) : SysYParser(input, antlr4::atn::ParserATNSimulatorOptions()) {
}

SysYParser::SysYParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  SysYParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *sysyParserStaticData->atn, sysyParserStaticData->decisionToDFA,
                                             sysyParserStaticData->sharedContextCache, options);
}

SysYParser::~SysYParser() {
  delete _interpreter;
}

const atn::ATN &SysYParser::getATN() const {
  return *sysyParserStaticData->atn;
}

std::string SysYParser::getGrammarFileName() const {
  return "SysY.g4";
}

const std::vector<std::string> &SysYParser::getRuleNames() const {
  return sysyParserStaticData->ruleNames;
}

const dfa::Vocabulary &SysYParser::getVocabulary() const {
  return sysyParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SysYParser::getSerializedATN() const {
  return sysyParserStaticData->serializedATN;
}

//----------------- CompContext ------------------------------------------------------------------

SysYParser::CompContext::CompContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::CompContext::EOF() {
  return getToken(SysYParser::EOF, 0);
}

std::vector<SysYParser::DeclContext *> SysYParser::CompContext::decl() {
  return getRuleContexts<SysYParser::DeclContext>();
}

SysYParser::DeclContext *SysYParser::CompContext::decl(size_t i) {
  return getRuleContext<SysYParser::DeclContext>(i);
}

std::vector<SysYParser::FuncDefContext *> SysYParser::CompContext::funcDef() {
  return getRuleContexts<SysYParser::FuncDefContext>();
}

SysYParser::FuncDefContext *SysYParser::CompContext::funcDef(size_t i) {
  return getRuleContext<SysYParser::FuncDefContext>(i);
}

size_t SysYParser::CompContext::getRuleIndex() const {
  return SysYParser::RuleComp;
}

std::any SysYParser::CompContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitComp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::CompContext *SysYParser::comp() {
  CompContext *_localctx = _tracker.createInstance<CompContext>(_ctx, getState());
  enterRule(_localctx, 0, SysYParser::RuleComp);
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
    setState(56);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 30) != 0)) {
      setState(54);
      _errHandler->sync(this);
      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
        case 1: {
          setState(52);
          decl();
          break;
        }

        case 2: {
          setState(53);
          funcDef();
          break;
        }

        default:
          break;
      }
      setState(58);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(59);
    match(SysYParser::EOF);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- DeclContext ------------------------------------------------------------------

SysYParser::DeclContext::DeclContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

SysYParser::BTypeContext *SysYParser::DeclContext::bType() {
  return getRuleContext<SysYParser::BTypeContext>(0);
}

std::vector<SysYParser::VarDefContext *> SysYParser::DeclContext::varDef() {
  return getRuleContexts<SysYParser::VarDefContext>();
}

SysYParser::VarDefContext *SysYParser::DeclContext::varDef(size_t i) {
  return getRuleContext<SysYParser::VarDefContext>(i);
}

tree::TerminalNode *SysYParser::DeclContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

tree::TerminalNode *SysYParser::DeclContext::CONST() {
  return getToken(SysYParser::CONST, 0);
}

std::vector<tree::TerminalNode *> SysYParser::DeclContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode *SysYParser::DeclContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}

size_t SysYParser::DeclContext::getRuleIndex() const {
  return SysYParser::RuleDecl;
}

std::any SysYParser::DeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitDecl(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::DeclContext *SysYParser::decl() {
  DeclContext *_localctx = _tracker.createInstance<DeclContext>(_ctx, getState());
  enterRule(_localctx, 2, SysYParser::RuleDecl);
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
    setState(62);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::CONST) {
      setState(61);
      match(SysYParser::CONST);
    }
    setState(64);
    bType();
    setState(65);
    varDef();
    setState(70);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(66);
      match(SysYParser::COMMA);
      setState(67);
      varDef();
      setState(72);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(73);
    match(SysYParser::SEMICOLON);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VarDefContext ------------------------------------------------------------------

SysYParser::VarDefContext::VarDefContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

SysYParser::LValueContext *SysYParser::VarDefContext::lValue() {
  return getRuleContext<SysYParser::LValueContext>(0);
}

tree::TerminalNode *SysYParser::VarDefContext::ASSIGN() {
  return getToken(SysYParser::ASSIGN, 0);
}

SysYParser::InitContext *SysYParser::VarDefContext::init() {
  return getRuleContext<SysYParser::InitContext>(0);
}

size_t SysYParser::VarDefContext::getRuleIndex() const {
  return SysYParser::RuleVarDef;
}

std::any SysYParser::VarDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitVarDef(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::VarDefContext *SysYParser::varDef() {
  VarDefContext *_localctx = _tracker.createInstance<VarDefContext>(_ctx, getState());
  enterRule(_localctx, 4, SysYParser::RuleVarDef);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(80);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 4, _ctx)) {
      case 1: {
        enterOuterAlt(_localctx, 1);
        setState(75);
        lValue();
        break;
      }

      case 2: {
        enterOuterAlt(_localctx, 2);
        setState(76);
        lValue();
        setState(77);
        match(SysYParser::ASSIGN);
        setState(78);
        init();
        break;
      }

      default:
        break;
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- InitContext ------------------------------------------------------------------

SysYParser::InitContext::InitContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

size_t SysYParser::InitContext::getRuleIndex() const {
  return SysYParser::RuleInit;
}

void SysYParser::InitContext::copyFrom(InitContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ArrayInitContext ------------------------------------------------------------------

tree::TerminalNode *SysYParser::ArrayInitContext::LBRACES() {
  return getToken(SysYParser::LBRACES, 0);
}

tree::TerminalNode *SysYParser::ArrayInitContext::RBRACES() {
  return getToken(SysYParser::RBRACES, 0);
}

std::vector<SysYParser::InitContext *> SysYParser::ArrayInitContext::init() {
  return getRuleContexts<SysYParser::InitContext>();
}

SysYParser::InitContext *SysYParser::ArrayInitContext::init(size_t i) {
  return getRuleContext<SysYParser::InitContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::ArrayInitContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode *SysYParser::ArrayInitContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}

SysYParser::ArrayInitContext::ArrayInitContext(InitContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::ArrayInitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitArrayInit(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ScalarInitContext ------------------------------------------------------------------

SysYParser::ExpContext *SysYParser::ScalarInitContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

SysYParser::ScalarInitContext::ScalarInitContext(InitContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::ScalarInitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitScalarInit(this);
  else
    return visitor->visitChildren(this);
}
SysYParser::InitContext *SysYParser::init() {
  InitContext *_localctx = _tracker.createInstance<InitContext>(_ctx, getState());
  enterRule(_localctx, 6, SysYParser::RuleInit);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(95);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParser::LPAREN:
      case SysYParser::ADD:
      case SysYParser::SUB:
      case SysYParser::NOT:
      case SysYParser::INTLITERAL:
      case SysYParser::FLOATLITERAL:
      case SysYParser::IDENTIFIER:
      case SysYParser::STRING: {
        _localctx = _tracker.createInstance<SysYParser::ScalarInitContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(82);
        exp(0);
        break;
      }

      case SysYParser::LBRACES: {
        _localctx = _tracker.createInstance<SysYParser::ArrayInitContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(83);
        match(SysYParser::LBRACES);
        setState(92);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 1065164611584) != 0)) {
          setState(84);
          init();
          setState(89);
          _errHandler->sync(this);
          _la = _input->LA(1);
          while (_la == SysYParser::COMMA) {
            setState(85);
            match(SysYParser::COMMA);
            setState(86);
            init();
            setState(91);
            _errHandler->sync(this);
            _la = _input->LA(1);
          }
        }
        setState(94);
        match(SysYParser::RBRACES);
        break;
      }

      default:
        throw NoViableAltException(this);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FuncDefContext ------------------------------------------------------------------

SysYParser::FuncDefContext::FuncDefContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

SysYParser::ReturnTypeContext *SysYParser::FuncDefContext::returnType() {
  return getRuleContext<SysYParser::ReturnTypeContext>(0);
}

tree::TerminalNode *SysYParser::FuncDefContext::IDENTIFIER() {
  return getToken(SysYParser::IDENTIFIER, 0);
}

tree::TerminalNode *SysYParser::FuncDefContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

tree::TerminalNode *SysYParser::FuncDefContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::BlockStmtContext *SysYParser::FuncDefContext::blockStmt() {
  return getRuleContext<SysYParser::BlockStmtContext>(0);
}

SysYParser::FuncFParamsContext *SysYParser::FuncDefContext::funcFParams() {
  return getRuleContext<SysYParser::FuncFParamsContext>(0);
}

size_t SysYParser::FuncDefContext::getRuleIndex() const {
  return SysYParser::RuleFuncDef;
}

std::any SysYParser::FuncDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitFuncDef(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncDefContext *SysYParser::funcDef() {
  FuncDefContext *_localctx = _tracker.createInstance<FuncDefContext>(_ctx, getState());
  enterRule(_localctx, 8, SysYParser::RuleFuncDef);
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
    setState(97);
    returnType();
    setState(98);
    match(SysYParser::IDENTIFIER);
    setState(99);
    match(SysYParser::LPAREN);
    setState(101);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::INT

        || _la == SysYParser::FLOAT) {
      setState(100);
      funcFParams();
    }
    setState(103);
    match(SysYParser::RPAREN);
    setState(104);
    blockStmt();

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FuncFParamsContext ------------------------------------------------------------------

SysYParser::FuncFParamsContext::FuncFParamsContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::FuncFParamContext *> SysYParser::FuncFParamsContext::funcFParam() {
  return getRuleContexts<SysYParser::FuncFParamContext>();
}

SysYParser::FuncFParamContext *SysYParser::FuncFParamsContext::funcFParam(size_t i) {
  return getRuleContext<SysYParser::FuncFParamContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::FuncFParamsContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode *SysYParser::FuncFParamsContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}

size_t SysYParser::FuncFParamsContext::getRuleIndex() const {
  return SysYParser::RuleFuncFParams;
}

std::any SysYParser::FuncFParamsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitFuncFParams(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncFParamsContext *SysYParser::funcFParams() {
  FuncFParamsContext *_localctx = _tracker.createInstance<FuncFParamsContext>(_ctx, getState());
  enterRule(_localctx, 10, SysYParser::RuleFuncFParams);
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
    setState(106);
    funcFParam();
    setState(111);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(107);
      match(SysYParser::COMMA);
      setState(108);
      funcFParam();
      setState(113);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FuncFParamContext ------------------------------------------------------------------

SysYParser::FuncFParamContext::FuncFParamContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

SysYParser::BTypeContext *SysYParser::FuncFParamContext::bType() {
  return getRuleContext<SysYParser::BTypeContext>(0);
}

tree::TerminalNode *SysYParser::FuncFParamContext::IDENTIFIER() {
  return getToken(SysYParser::IDENTIFIER, 0);
}

std::vector<tree::TerminalNode *> SysYParser::FuncFParamContext::LBRACKET() {
  return getTokens(SysYParser::LBRACKET);
}

tree::TerminalNode *SysYParser::FuncFParamContext::LBRACKET(size_t i) {
  return getToken(SysYParser::LBRACKET, i);
}

std::vector<tree::TerminalNode *> SysYParser::FuncFParamContext::RBRACKET() {
  return getTokens(SysYParser::RBRACKET);
}

tree::TerminalNode *SysYParser::FuncFParamContext::RBRACKET(size_t i) {
  return getToken(SysYParser::RBRACKET, i);
}

std::vector<SysYParser::ExpContext *> SysYParser::FuncFParamContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext *SysYParser::FuncFParamContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

size_t SysYParser::FuncFParamContext::getRuleIndex() const {
  return SysYParser::RuleFuncFParam;
}

std::any SysYParser::FuncFParamContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitFuncFParam(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncFParamContext *SysYParser::funcFParam() {
  FuncFParamContext *_localctx = _tracker.createInstance<FuncFParamContext>(_ctx, getState());
  enterRule(_localctx, 12, SysYParser::RuleFuncFParam);
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
    setState(114);
    bType();
    setState(115);
    match(SysYParser::IDENTIFIER);
    setState(127);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::LBRACKET) {
      setState(116);
      match(SysYParser::LBRACKET);
      setState(117);
      match(SysYParser::RBRACKET);
      setState(124);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == SysYParser::LBRACKET) {
        setState(118);
        match(SysYParser::LBRACKET);
        setState(119);
        exp(0);
        setState(120);
        match(SysYParser::RBRACKET);
        setState(126);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StmtContext ------------------------------------------------------------------

SysYParser::StmtContext::StmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

SysYParser::AssignStmtContext *SysYParser::StmtContext::assignStmt() {
  return getRuleContext<SysYParser::AssignStmtContext>(0);
}

SysYParser::ExpStmtContext *SysYParser::StmtContext::expStmt() {
  return getRuleContext<SysYParser::ExpStmtContext>(0);
}

SysYParser::IfStmtContext *SysYParser::StmtContext::ifStmt() {
  return getRuleContext<SysYParser::IfStmtContext>(0);
}

SysYParser::WhileStmtContext *SysYParser::StmtContext::whileStmt() {
  return getRuleContext<SysYParser::WhileStmtContext>(0);
}

SysYParser::BreakStmtContext *SysYParser::StmtContext::breakStmt() {
  return getRuleContext<SysYParser::BreakStmtContext>(0);
}

SysYParser::ContinueStmtContext *SysYParser::StmtContext::continueStmt() {
  return getRuleContext<SysYParser::ContinueStmtContext>(0);
}

SysYParser::ReturnStmtContext *SysYParser::StmtContext::returnStmt() {
  return getRuleContext<SysYParser::ReturnStmtContext>(0);
}

SysYParser::BlockStmtContext *SysYParser::StmtContext::blockStmt() {
  return getRuleContext<SysYParser::BlockStmtContext>(0);
}

SysYParser::EmptyStmtContext *SysYParser::StmtContext::emptyStmt() {
  return getRuleContext<SysYParser::EmptyStmtContext>(0);
}

size_t SysYParser::StmtContext::getRuleIndex() const {
  return SysYParser::RuleStmt;
}

std::any SysYParser::StmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::StmtContext *SysYParser::stmt() {
  StmtContext *_localctx = _tracker.createInstance<StmtContext>(_ctx, getState());
  enterRule(_localctx, 14, SysYParser::RuleStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(138);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
      case 1: {
        enterOuterAlt(_localctx, 1);
        setState(129);
        assignStmt();
        break;
      }

      case 2: {
        enterOuterAlt(_localctx, 2);
        setState(130);
        expStmt();
        break;
      }

      case 3: {
        enterOuterAlt(_localctx, 3);
        setState(131);
        ifStmt();
        break;
      }

      case 4: {
        enterOuterAlt(_localctx, 4);
        setState(132);
        whileStmt();
        break;
      }

      case 5: {
        enterOuterAlt(_localctx, 5);
        setState(133);
        breakStmt();
        break;
      }

      case 6: {
        enterOuterAlt(_localctx, 6);
        setState(134);
        continueStmt();
        break;
      }

      case 7: {
        enterOuterAlt(_localctx, 7);
        setState(135);
        returnStmt();
        break;
      }

      case 8: {
        enterOuterAlt(_localctx, 8);
        setState(136);
        blockStmt();
        break;
      }

      case 9: {
        enterOuterAlt(_localctx, 9);
        setState(137);
        emptyStmt();
        break;
      }

      default:
        break;
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AssignStmtContext ------------------------------------------------------------------

SysYParser::AssignStmtContext::AssignStmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

SysYParser::LValueContext *SysYParser::AssignStmtContext::lValue() {
  return getRuleContext<SysYParser::LValueContext>(0);
}

tree::TerminalNode *SysYParser::AssignStmtContext::ASSIGN() {
  return getToken(SysYParser::ASSIGN, 0);
}

SysYParser::ExpContext *SysYParser::AssignStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode *SysYParser::AssignStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

size_t SysYParser::AssignStmtContext::getRuleIndex() const {
  return SysYParser::RuleAssignStmt;
}

std::any SysYParser::AssignStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitAssignStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::AssignStmtContext *SysYParser::assignStmt() {
  AssignStmtContext *_localctx = _tracker.createInstance<AssignStmtContext>(_ctx, getState());
  enterRule(_localctx, 16, SysYParser::RuleAssignStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(140);
    lValue();
    setState(141);
    match(SysYParser::ASSIGN);
    setState(142);
    exp(0);
    setState(143);
    match(SysYParser::SEMICOLON);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpStmtContext ------------------------------------------------------------------

SysYParser::ExpStmtContext::ExpStmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

SysYParser::ExpContext *SysYParser::ExpStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode *SysYParser::ExpStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

size_t SysYParser::ExpStmtContext::getRuleIndex() const {
  return SysYParser::RuleExpStmt;
}

std::any SysYParser::ExpStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitExpStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ExpStmtContext *SysYParser::expStmt() {
  ExpStmtContext *_localctx = _tracker.createInstance<ExpStmtContext>(_ctx, getState());
  enterRule(_localctx, 18, SysYParser::RuleExpStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(145);
    exp(0);
    setState(146);
    match(SysYParser::SEMICOLON);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IfStmtContext ------------------------------------------------------------------

SysYParser::IfStmtContext::IfStmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::IfStmtContext::IF() {
  return getToken(SysYParser::IF, 0);
}

tree::TerminalNode *SysYParser::IfStmtContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

SysYParser::ExpContext *SysYParser::IfStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode *SysYParser::IfStmtContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

std::vector<SysYParser::StmtContext *> SysYParser::IfStmtContext::stmt() {
  return getRuleContexts<SysYParser::StmtContext>();
}

SysYParser::StmtContext *SysYParser::IfStmtContext::stmt(size_t i) {
  return getRuleContext<SysYParser::StmtContext>(i);
}

tree::TerminalNode *SysYParser::IfStmtContext::ELSE() {
  return getToken(SysYParser::ELSE, 0);
}

size_t SysYParser::IfStmtContext::getRuleIndex() const {
  return SysYParser::RuleIfStmt;
}

std::any SysYParser::IfStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitIfStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::IfStmtContext *SysYParser::ifStmt() {
  IfStmtContext *_localctx = _tracker.createInstance<IfStmtContext>(_ctx, getState());
  enterRule(_localctx, 20, SysYParser::RuleIfStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(148);
    match(SysYParser::IF);
    setState(149);
    match(SysYParser::LPAREN);
    setState(150);
    exp(0);
    setState(151);
    match(SysYParser::RPAREN);
    setState(152);
    stmt();
    setState(155);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 13, _ctx)) {
      case 1: {
        setState(153);
        match(SysYParser::ELSE);
        setState(154);
        stmt();
        break;
      }

      default:
        break;
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- WhileStmtContext ------------------------------------------------------------------

SysYParser::WhileStmtContext::WhileStmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::WhileStmtContext::WHILE() {
  return getToken(SysYParser::WHILE, 0);
}

tree::TerminalNode *SysYParser::WhileStmtContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

SysYParser::ExpContext *SysYParser::WhileStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode *SysYParser::WhileStmtContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::StmtContext *SysYParser::WhileStmtContext::stmt() {
  return getRuleContext<SysYParser::StmtContext>(0);
}

size_t SysYParser::WhileStmtContext::getRuleIndex() const {
  return SysYParser::RuleWhileStmt;
}

std::any SysYParser::WhileStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitWhileStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::WhileStmtContext *SysYParser::whileStmt() {
  WhileStmtContext *_localctx = _tracker.createInstance<WhileStmtContext>(_ctx, getState());
  enterRule(_localctx, 22, SysYParser::RuleWhileStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(157);
    match(SysYParser::WHILE);
    setState(158);
    match(SysYParser::LPAREN);
    setState(159);
    exp(0);
    setState(160);
    match(SysYParser::RPAREN);
    setState(161);
    stmt();

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BreakStmtContext ------------------------------------------------------------------

SysYParser::BreakStmtContext::BreakStmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::BreakStmtContext::BREAK() {
  return getToken(SysYParser::BREAK, 0);
}

tree::TerminalNode *SysYParser::BreakStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

size_t SysYParser::BreakStmtContext::getRuleIndex() const {
  return SysYParser::RuleBreakStmt;
}

std::any SysYParser::BreakStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitBreakStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::BreakStmtContext *SysYParser::breakStmt() {
  BreakStmtContext *_localctx = _tracker.createInstance<BreakStmtContext>(_ctx, getState());
  enterRule(_localctx, 24, SysYParser::RuleBreakStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(163);
    match(SysYParser::BREAK);
    setState(164);
    match(SysYParser::SEMICOLON);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ContinueStmtContext ------------------------------------------------------------------

SysYParser::ContinueStmtContext::ContinueStmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::ContinueStmtContext::CONTINUE() {
  return getToken(SysYParser::CONTINUE, 0);
}

tree::TerminalNode *SysYParser::ContinueStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

size_t SysYParser::ContinueStmtContext::getRuleIndex() const {
  return SysYParser::RuleContinueStmt;
}

std::any SysYParser::ContinueStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitContinueStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ContinueStmtContext *SysYParser::continueStmt() {
  ContinueStmtContext *_localctx = _tracker.createInstance<ContinueStmtContext>(_ctx, getState());
  enterRule(_localctx, 26, SysYParser::RuleContinueStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(166);
    match(SysYParser::CONTINUE);
    setState(167);
    match(SysYParser::SEMICOLON);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ReturnStmtContext ------------------------------------------------------------------

SysYParser::ReturnStmtContext::ReturnStmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::ReturnStmtContext::RETURN() {
  return getToken(SysYParser::RETURN, 0);
}

tree::TerminalNode *SysYParser::ReturnStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

SysYParser::ExpContext *SysYParser::ReturnStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

size_t SysYParser::ReturnStmtContext::getRuleIndex() const {
  return SysYParser::RuleReturnStmt;
}

std::any SysYParser::ReturnStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitReturnStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ReturnStmtContext *SysYParser::returnStmt() {
  ReturnStmtContext *_localctx = _tracker.createInstance<ReturnStmtContext>(_ctx, getState());
  enterRule(_localctx, 28, SysYParser::RuleReturnStmt);
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
    match(SysYParser::RETURN);
    setState(171);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 1065164480512) != 0)) {
      setState(170);
      exp(0);
    }
    setState(173);
    match(SysYParser::SEMICOLON);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BlockStmtContext ------------------------------------------------------------------

SysYParser::BlockStmtContext::BlockStmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::BlockStmtContext::LBRACES() {
  return getToken(SysYParser::LBRACES, 0);
}

tree::TerminalNode *SysYParser::BlockStmtContext::RBRACES() {
  return getToken(SysYParser::RBRACES, 0);
}

std::vector<SysYParser::BlockItemContext *> SysYParser::BlockStmtContext::blockItem() {
  return getRuleContexts<SysYParser::BlockItemContext>();
}

SysYParser::BlockItemContext *SysYParser::BlockStmtContext::blockItem(size_t i) {
  return getRuleContext<SysYParser::BlockItemContext>(i);
}

size_t SysYParser::BlockStmtContext::getRuleIndex() const {
  return SysYParser::RuleBlockStmt;
}

std::any SysYParser::BlockStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitBlockStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::BlockStmtContext *SysYParser::blockStmt() {
  BlockStmtContext *_localctx = _tracker.createInstance<BlockStmtContext>(_ctx, getState());
  enterRule(_localctx, 30, SysYParser::RuleBlockStmt);
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
    setState(175);
    match(SysYParser::LBRACES);
    setState(179);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 1065165667734) != 0)) {
      setState(176);
      blockItem();
      setState(181);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(182);
    match(SysYParser::RBRACES);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BlockItemContext ------------------------------------------------------------------

SysYParser::BlockItemContext::BlockItemContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

SysYParser::DeclContext *SysYParser::BlockItemContext::decl() {
  return getRuleContext<SysYParser::DeclContext>(0);
}

SysYParser::StmtContext *SysYParser::BlockItemContext::stmt() {
  return getRuleContext<SysYParser::StmtContext>(0);
}

size_t SysYParser::BlockItemContext::getRuleIndex() const {
  return SysYParser::RuleBlockItem;
}

std::any SysYParser::BlockItemContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitBlockItem(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::BlockItemContext *SysYParser::blockItem() {
  BlockItemContext *_localctx = _tracker.createInstance<BlockItemContext>(_ctx, getState());
  enterRule(_localctx, 32, SysYParser::RuleBlockItem);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(186);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParser::INT:
      case SysYParser::FLOAT:
      case SysYParser::CONST: {
        enterOuterAlt(_localctx, 1);
        setState(184);
        decl();
        break;
      }

      case SysYParser::WHILE:
      case SysYParser::IF:
      case SysYParser::CONTINUE:
      case SysYParser::BREAK:
      case SysYParser::RETURN:
      case SysYParser::LPAREN:
      case SysYParser::LBRACES:
      case SysYParser::SEMICOLON:
      case SysYParser::ADD:
      case SysYParser::SUB:
      case SysYParser::NOT:
      case SysYParser::INTLITERAL:
      case SysYParser::FLOATLITERAL:
      case SysYParser::IDENTIFIER:
      case SysYParser::STRING: {
        enterOuterAlt(_localctx, 2);
        setState(185);
        stmt();
        break;
      }

      default:
        throw NoViableAltException(this);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EmptyStmtContext ------------------------------------------------------------------

SysYParser::EmptyStmtContext::EmptyStmtContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::EmptyStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

size_t SysYParser::EmptyStmtContext::getRuleIndex() const {
  return SysYParser::RuleEmptyStmt;
}

std::any SysYParser::EmptyStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitEmptyStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::EmptyStmtContext *SysYParser::emptyStmt() {
  EmptyStmtContext *_localctx = _tracker.createInstance<EmptyStmtContext>(_ctx, getState());
  enterRule(_localctx, 34, SysYParser::RuleEmptyStmt);

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
    match(SysYParser::SEMICOLON);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpContext ------------------------------------------------------------------

SysYParser::ExpContext::ExpContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

size_t SysYParser::ExpContext::getRuleIndex() const {
  return SysYParser::RuleExp;
}

void SysYParser::ExpContext::copyFrom(ExpContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- EqExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::EqExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext *SysYParser::EqExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode *SysYParser::EqExpContext::EQ() {
  return getToken(SysYParser::EQ, 0);
}

tree::TerminalNode *SysYParser::EqExpContext::NE() {
  return getToken(SysYParser::NE, 0);
}

SysYParser::EqExpContext::EqExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::EqExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitEqExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- LValueExpContext ------------------------------------------------------------------

SysYParser::LValueContext *SysYParser::LValueExpContext::lValue() {
  return getRuleContext<SysYParser::LValueContext>(0);
}

SysYParser::LValueExpContext::LValueExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::LValueExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitLValueExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NumberExpContext ------------------------------------------------------------------

SysYParser::NumberContext *SysYParser::NumberExpContext::number() {
  return getRuleContext<SysYParser::NumberContext>(0);
}

SysYParser::NumberExpContext::NumberExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::NumberExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitNumberExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AndExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::AndExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext *SysYParser::AndExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode *SysYParser::AndExpContext::AND() {
  return getToken(SysYParser::AND, 0);
}

SysYParser::AndExpContext::AndExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::AndExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitAndExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- UnaryExpContext ------------------------------------------------------------------

SysYParser::ExpContext *SysYParser::UnaryExpContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode *SysYParser::UnaryExpContext::ADD() {
  return getToken(SysYParser::ADD, 0);
}

tree::TerminalNode *SysYParser::UnaryExpContext::SUB() {
  return getToken(SysYParser::SUB, 0);
}

tree::TerminalNode *SysYParser::UnaryExpContext::NOT() {
  return getToken(SysYParser::NOT, 0);
}

SysYParser::UnaryExpContext::UnaryExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::UnaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitUnaryExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ParenExpContext ------------------------------------------------------------------

tree::TerminalNode *SysYParser::ParenExpContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

SysYParser::ExpContext *SysYParser::ParenExpContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode *SysYParser::ParenExpContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::ParenExpContext::ParenExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::ParenExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitParenExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AddExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::AddExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext *SysYParser::AddExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode *SysYParser::AddExpContext::ADD() {
  return getToken(SysYParser::ADD, 0);
}

tree::TerminalNode *SysYParser::AddExpContext::SUB() {
  return getToken(SysYParser::SUB, 0);
}

SysYParser::AddExpContext::AddExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::AddExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitAddExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MulExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::MulExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext *SysYParser::MulExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode *SysYParser::MulExpContext::MUL() {
  return getToken(SysYParser::MUL, 0);
}

tree::TerminalNode *SysYParser::MulExpContext::DIV() {
  return getToken(SysYParser::DIV, 0);
}

tree::TerminalNode *SysYParser::MulExpContext::MOD() {
  return getToken(SysYParser::MOD, 0);
}

SysYParser::MulExpContext::MulExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::MulExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitMulExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StringExpContext ------------------------------------------------------------------

SysYParser::StringContext *SysYParser::StringExpContext::string() {
  return getRuleContext<SysYParser::StringContext>(0);
}

SysYParser::StringExpContext::StringExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::StringExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitStringExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- OrExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::OrExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext *SysYParser::OrExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode *SysYParser::OrExpContext::OR() {
  return getToken(SysYParser::OR, 0);
}

SysYParser::OrExpContext::OrExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::OrExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitOrExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- CallExpContext ------------------------------------------------------------------

SysYParser::CallContext *SysYParser::CallExpContext::call() {
  return getRuleContext<SysYParser::CallContext>(0);
}

SysYParser::CallExpContext::CallExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::CallExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitCallExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- RelExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::RelExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext *SysYParser::RelExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode *SysYParser::RelExpContext::LT() {
  return getToken(SysYParser::LT, 0);
}

tree::TerminalNode *SysYParser::RelExpContext::GT() {
  return getToken(SysYParser::GT, 0);
}

tree::TerminalNode *SysYParser::RelExpContext::LE() {
  return getToken(SysYParser::LE, 0);
}

tree::TerminalNode *SysYParser::RelExpContext::GE() {
  return getToken(SysYParser::GE, 0);
}

SysYParser::RelExpContext::RelExpContext(ExpContext *ctx) {
  copyFrom(ctx);
}

std::any SysYParser::RelExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitRelExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ExpContext *SysYParser::exp() {
  return exp(0);
}

SysYParser::ExpContext *SysYParser::exp(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  SysYParser::ExpContext *_localctx = _tracker.createInstance<ExpContext>(_ctx, parentState);
  SysYParser::ExpContext *previousContext = _localctx;
  (void)previousContext;  // Silence compiler, in case the context is not used by generated code.
  size_t startState = 36;
  enterRecursionRule(_localctx, 36, SysYParser::RuleExp, precedence);

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
    setState(201);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 17, _ctx)) {
      case 1: {
        _localctx = _tracker.createInstance<ParenExpContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;

        setState(191);
        match(SysYParser::LPAREN);
        setState(192);
        exp(0);
        setState(193);
        match(SysYParser::RPAREN);
        break;
      }

      case 2: {
        _localctx = _tracker.createInstance<LValueExpContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(195);
        lValue();
        break;
      }

      case 3: {
        _localctx = _tracker.createInstance<NumberExpContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(196);
        number();
        break;
      }

      case 4: {
        _localctx = _tracker.createInstance<StringExpContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(197);
        string();
        break;
      }

      case 5: {
        _localctx = _tracker.createInstance<CallExpContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(198);
        call();
        break;
      }

      case 6: {
        _localctx = _tracker.createInstance<UnaryExpContext>(_localctx);
        _ctx = _localctx;
        previousContext = _localctx;
        setState(199);
        _la = _input->LA(1);
        if (!((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 34372321280) != 0))) {
          _errHandler->recoverInline(this);
        } else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(200);
        exp(7);
        break;
      }

      default:
        break;
    }
    _ctx->stop = _input->LT(-1);
    setState(223);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty()) triggerExitRuleEvent();
        previousContext = _localctx;
        setState(221);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx)) {
          case 1: {
            auto newContext =
                _tracker.createInstance<MulExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleExp);
            setState(203);

            if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
            setState(204);
            _la = _input->LA(1);
            if (!((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 117440512) != 0))) {
              _errHandler->recoverInline(this);
            } else {
              _errHandler->reportMatch(this);
              consume();
            }
            setState(205);
            exp(7);
            break;
          }

          case 2: {
            auto newContext =
                _tracker.createInstance<AddExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleExp);
            setState(206);

            if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
            setState(207);
            _la = _input->LA(1);
            if (!(_la == SysYParser::ADD

                  || _la == SysYParser::SUB)) {
              _errHandler->recoverInline(this);
            } else {
              _errHandler->reportMatch(this);
              consume();
            }
            setState(208);
            exp(6);
            break;
          }

          case 3: {
            auto newContext =
                _tracker.createInstance<RelExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleExp);
            setState(209);

            if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
            setState(210);
            _la = _input->LA(1);
            if (!((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 2013265920) != 0))) {
              _errHandler->recoverInline(this);
            } else {
              _errHandler->reportMatch(this);
              consume();
            }
            setState(211);
            exp(5);
            break;
          }

          case 4: {
            auto newContext =
                _tracker.createInstance<EqExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleExp);
            setState(212);

            if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
            setState(213);
            _la = _input->LA(1);
            if (!(_la == SysYParser::EQ

                  || _la == SysYParser::NE)) {
              _errHandler->recoverInline(this);
            } else {
              _errHandler->reportMatch(this);
              consume();
            }
            setState(214);
            exp(4);
            break;
          }

          case 5: {
            auto newContext =
                _tracker.createInstance<AndExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleExp);
            setState(215);

            if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
            setState(216);
            match(SysYParser::AND);
            setState(217);
            exp(3);
            break;
          }

          case 6: {
            auto newContext =
                _tracker.createInstance<OrExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
            _localctx = newContext;
            pushNewRecursionContext(newContext, startState, RuleExp);
            setState(218);

            if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
            setState(219);
            match(SysYParser::OR);
            setState(220);
            exp(2);
            break;
          }

          default:
            break;
        }
      }
      setState(225);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx);
    }
  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }
  return _localctx;
}

//----------------- CallContext ------------------------------------------------------------------

SysYParser::CallContext::CallContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::CallContext::IDENTIFIER() {
  return getToken(SysYParser::IDENTIFIER, 0);
}

tree::TerminalNode *SysYParser::CallContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

tree::TerminalNode *SysYParser::CallContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::FuncRParamsContext *SysYParser::CallContext::funcRParams() {
  return getRuleContext<SysYParser::FuncRParamsContext>(0);
}

size_t SysYParser::CallContext::getRuleIndex() const {
  return SysYParser::RuleCall;
}

std::any SysYParser::CallContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitCall(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::CallContext *SysYParser::call() {
  CallContext *_localctx = _tracker.createInstance<CallContext>(_ctx, getState());
  enterRule(_localctx, 38, SysYParser::RuleCall);
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
    setState(226);
    match(SysYParser::IDENTIFIER);
    setState(227);
    match(SysYParser::LPAREN);
    setState(229);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 1065164480512) != 0)) {
      setState(228);
      funcRParams();
    }
    setState(231);
    match(SysYParser::RPAREN);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FuncRParamsContext ------------------------------------------------------------------

SysYParser::FuncRParamsContext::FuncRParamsContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::ExpContext *> SysYParser::FuncRParamsContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext *SysYParser::FuncRParamsContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::FuncRParamsContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode *SysYParser::FuncRParamsContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}

size_t SysYParser::FuncRParamsContext::getRuleIndex() const {
  return SysYParser::RuleFuncRParams;
}

std::any SysYParser::FuncRParamsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitFuncRParams(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncRParamsContext *SysYParser::funcRParams() {
  FuncRParamsContext *_localctx = _tracker.createInstance<FuncRParamsContext>(_ctx, getState());
  enterRule(_localctx, 40, SysYParser::RuleFuncRParams);
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
    setState(233);
    exp(0);
    setState(238);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(234);
      match(SysYParser::COMMA);
      setState(235);
      exp(0);
      setState(240);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LValueContext ------------------------------------------------------------------

SysYParser::LValueContext::LValueContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::LValueContext::IDENTIFIER() {
  return getToken(SysYParser::IDENTIFIER, 0);
}

std::vector<tree::TerminalNode *> SysYParser::LValueContext::LBRACKET() {
  return getTokens(SysYParser::LBRACKET);
}

tree::TerminalNode *SysYParser::LValueContext::LBRACKET(size_t i) {
  return getToken(SysYParser::LBRACKET, i);
}

std::vector<SysYParser::ExpContext *> SysYParser::LValueContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext *SysYParser::LValueContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::LValueContext::RBRACKET() {
  return getTokens(SysYParser::RBRACKET);
}

tree::TerminalNode *SysYParser::LValueContext::RBRACKET(size_t i) {
  return getToken(SysYParser::RBRACKET, i);
}

size_t SysYParser::LValueContext::getRuleIndex() const {
  return SysYParser::RuleLValue;
}

std::any SysYParser::LValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitLValue(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::LValueContext *SysYParser::lValue() {
  LValueContext *_localctx = _tracker.createInstance<LValueContext>(_ctx, getState());
  enterRule(_localctx, 42, SysYParser::RuleLValue);

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
    setState(241);
    match(SysYParser::IDENTIFIER);
    setState(248);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 22, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(242);
        match(SysYParser::LBRACKET);
        setState(243);
        exp(0);
        setState(244);
        match(SysYParser::RBRACKET);
      }
      setState(250);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 22, _ctx);
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NumberContext ------------------------------------------------------------------

SysYParser::NumberContext::NumberContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::NumberContext::INTLITERAL() {
  return getToken(SysYParser::INTLITERAL, 0);
}

tree::TerminalNode *SysYParser::NumberContext::FLOATLITERAL() {
  return getToken(SysYParser::FLOATLITERAL, 0);
}

size_t SysYParser::NumberContext::getRuleIndex() const {
  return SysYParser::RuleNumber;
}

std::any SysYParser::NumberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitNumber(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::NumberContext *SysYParser::number() {
  NumberContext *_localctx = _tracker.createInstance<NumberContext>(_ctx, getState());
  enterRule(_localctx, 44, SysYParser::RuleNumber);
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
    setState(251);
    _la = _input->LA(1);
    if (!(_la == SysYParser::INTLITERAL

          || _la == SysYParser::FLOATLITERAL)) {
      _errHandler->recoverInline(this);
    } else {
      _errHandler->reportMatch(this);
      consume();
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StringContext ------------------------------------------------------------------

SysYParser::StringContext::StringContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::StringContext::STRING() {
  return getToken(SysYParser::STRING, 0);
}

size_t SysYParser::StringContext::getRuleIndex() const {
  return SysYParser::RuleString;
}

std::any SysYParser::StringContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitString(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::StringContext *SysYParser::string() {
  StringContext *_localctx = _tracker.createInstance<StringContext>(_ctx, getState());
  enterRule(_localctx, 46, SysYParser::RuleString);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(253);
    match(SysYParser::STRING);

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ReturnTypeContext ------------------------------------------------------------------

SysYParser::ReturnTypeContext::ReturnTypeContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::ReturnTypeContext::VOID() {
  return getToken(SysYParser::VOID, 0);
}

tree::TerminalNode *SysYParser::ReturnTypeContext::INT() {
  return getToken(SysYParser::INT, 0);
}

tree::TerminalNode *SysYParser::ReturnTypeContext::FLOAT() {
  return getToken(SysYParser::FLOAT, 0);
}

size_t SysYParser::ReturnTypeContext::getRuleIndex() const {
  return SysYParser::RuleReturnType;
}

std::any SysYParser::ReturnTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitReturnType(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ReturnTypeContext *SysYParser::returnType() {
  ReturnTypeContext *_localctx = _tracker.createInstance<ReturnTypeContext>(_ctx, getState());
  enterRule(_localctx, 48, SysYParser::RuleReturnType);
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
    setState(255);
    _la = _input->LA(1);
    if (!((((_la & ~0x3fULL) == 0) && ((1ULL << _la) & 14) != 0))) {
      _errHandler->recoverInline(this);
    } else {
      _errHandler->reportMatch(this);
      consume();
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BTypeContext ------------------------------------------------------------------

SysYParser::BTypeContext::BTypeContext(ParserRuleContext *parent, size_t invokingState)
    : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode *SysYParser::BTypeContext::INT() {
  return getToken(SysYParser::INT, 0);
}

tree::TerminalNode *SysYParser::BTypeContext::FLOAT() {
  return getToken(SysYParser::FLOAT, 0);
}

size_t SysYParser::BTypeContext::getRuleIndex() const {
  return SysYParser::RuleBType;
}

std::any SysYParser::BTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor *>(visitor))
    return parserVisitor->visitBType(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::BTypeContext *SysYParser::bType() {
  BTypeContext *_localctx = _tracker.createInstance<BTypeContext>(_ctx, getState());
  enterRule(_localctx, 50, SysYParser::RuleBType);
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
    setState(257);
    _la = _input->LA(1);
    if (!(_la == SysYParser::INT

          || _la == SysYParser::FLOAT)) {
      _errHandler->recoverInline(this);
    } else {
      _errHandler->reportMatch(this);
      consume();
    }

  } catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

bool SysYParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 18:
      return expSempred(antlrcpp::downCast<ExpContext *>(context), predicateIndex);

    default:
      break;
  }
  return true;
}

bool SysYParser::expSempred(ExpContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0:
      return precpred(_ctx, 6);
    case 1:
      return precpred(_ctx, 5);
    case 2:
      return precpred(_ctx, 4);
    case 3:
      return precpred(_ctx, 3);
    case 4:
      return precpred(_ctx, 2);
    case 5:
      return precpred(_ctx, 1);

    default:
      break;
  }
  return true;
}

void SysYParser::initialize() {
  ::antlr4::internal::call_once(sysyParserOnceFlag, sysyParserInitialize);
}
