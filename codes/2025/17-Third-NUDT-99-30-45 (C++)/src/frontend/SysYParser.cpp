
// Generated from SysY.g4 by ANTLR 4.13.2


#include "SysYVisitor.h"

#include "SysYParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct SysYParserStaticData final {
  SysYParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  SysYParserStaticData(const SysYParserStaticData&) = delete;
  SysYParserStaticData(SysYParserStaticData&&) = delete;
  SysYParserStaticData& operator=(const SysYParserStaticData&) = delete;
  SysYParserStaticData& operator=(SysYParserStaticData&&) = delete;

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
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<SysYParserStaticData> sysyParserStaticData = nullptr;

void sysyParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (sysyParserStaticData != nullptr) {
    return;
  }
#else
  assert(sysyParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<SysYParserStaticData>(
    std::vector<std::string>{
      "compUnit", "globalDecl", "decl", "constDecl", "bType", "constDef", 
      "constInitVal", "varDecl", "varDef", "initVal", "funcType", "funcDef", 
      "funcFParams", "funcFParam", "blockStmt", "blockItem", "stmt", "exp", 
      "cond", "lValue", "primaryExp", "number", "call", "unaryExp", "unaryOp", 
      "funcRParams", "string", "mulExp", "addExp", "relExp", "eqExp", "lAndExp", 
      "lOrExp", "constExp"
    },
    std::vector<std::string>{
      "", "'const'", "'int'", "'float'", "'void'", "'if'", "'else'", "'while'", 
      "'break'", "'continue'", "'return'", "'+'", "'-'", "'*'", "'/'", "'%'", 
      "'=='", "'!='", "'<'", "'<='", "'>'", "'>='", "'&&'", "'||'", "'!'", 
      "'='", "','", "';'", "'('", "')'", "'{'", "'}'", "'['", "']'"
    },
    std::vector<std::string>{
      "", "CONST", "INT", "FLOAT", "VOID", "IF", "ELSE", "WHILE", "BREAK", 
      "CONTINUE", "RETURN", "ADD", "SUB", "MUL", "DIV", "MOD", "EQ", "NE", 
      "LT", "LE", "GT", "GE", "AND", "OR", "NOT", "ASSIGN", "COMMA", "SEMICOLON", 
      "LPAREN", "RPAREN", "LBRACE", "RBRACE", "LBRACK", "RBRACK", "Ident", 
      "ILITERAL", "FLITERAL", "STRING", "WS", "LINECOMMENT", "BLOCKCOMMENT"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,40,359,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,1,0,1,0,4,0,71,8,
  	0,11,0,12,0,72,1,1,1,1,3,1,77,8,1,1,2,1,2,3,2,81,8,2,1,3,1,3,1,3,1,3,
  	1,3,5,3,88,8,3,10,3,12,3,91,9,3,1,3,1,3,1,4,1,4,1,5,1,5,1,5,1,5,1,5,5,
  	5,102,8,5,10,5,12,5,105,9,5,1,5,1,5,1,5,1,6,1,6,1,6,1,6,1,6,5,6,115,8,
  	6,10,6,12,6,118,9,6,3,6,120,8,6,1,6,3,6,123,8,6,1,7,1,7,1,7,1,7,5,7,129,
  	8,7,10,7,12,7,132,9,7,1,7,1,7,1,8,1,8,1,8,1,8,1,8,5,8,141,8,8,10,8,12,
  	8,144,9,8,1,8,1,8,1,8,1,8,1,8,5,8,151,8,8,10,8,12,8,154,9,8,1,8,1,8,3,
  	8,158,8,8,1,9,1,9,1,9,1,9,1,9,5,9,165,8,9,10,9,12,9,168,9,9,3,9,170,8,
  	9,1,9,3,9,173,8,9,1,10,1,10,1,11,1,11,1,11,1,11,3,11,181,8,11,1,11,1,
  	11,1,11,1,12,1,12,1,12,5,12,189,8,12,10,12,12,12,192,9,12,1,13,1,13,1,
  	13,1,13,1,13,1,13,1,13,1,13,5,13,202,8,13,10,13,12,13,205,9,13,3,13,207,
  	8,13,1,14,1,14,5,14,211,8,14,10,14,12,14,214,9,14,1,14,1,14,1,15,1,15,
  	3,15,220,8,15,1,16,1,16,1,16,1,16,1,16,1,16,3,16,228,8,16,1,16,1,16,1,
  	16,1,16,1,16,1,16,1,16,1,16,1,16,3,16,239,8,16,1,16,1,16,1,16,1,16,1,
  	16,1,16,1,16,1,16,1,16,1,16,1,16,1,16,3,16,253,8,16,1,16,3,16,256,8,16,
  	1,17,1,17,1,18,1,18,1,19,1,19,1,19,1,19,1,19,5,19,267,8,19,10,19,12,19,
  	270,9,19,1,20,1,20,1,20,1,20,1,20,1,20,1,20,3,20,279,8,20,1,21,1,21,1,
  	22,1,22,1,22,3,22,286,8,22,1,22,1,22,1,23,1,23,1,23,1,23,1,23,3,23,295,
  	8,23,1,24,1,24,1,25,1,25,1,25,5,25,302,8,25,10,25,12,25,305,9,25,1,26,
  	1,26,1,27,1,27,1,27,5,27,312,8,27,10,27,12,27,315,9,27,1,28,1,28,1,28,
  	5,28,320,8,28,10,28,12,28,323,9,28,1,29,1,29,1,29,5,29,328,8,29,10,29,
  	12,29,331,9,29,1,30,1,30,1,30,5,30,336,8,30,10,30,12,30,339,9,30,1,31,
  	1,31,1,31,5,31,344,8,31,10,31,12,31,347,9,31,1,32,1,32,1,32,5,32,352,
  	8,32,10,32,12,32,355,9,32,1,33,1,33,1,33,0,0,34,0,2,4,6,8,10,12,14,16,
  	18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,
  	64,66,0,8,1,0,2,3,1,0,2,4,1,0,35,36,2,0,11,12,24,24,1,0,13,15,1,0,11,
  	12,1,0,18,21,1,0,16,17,370,0,70,1,0,0,0,2,76,1,0,0,0,4,80,1,0,0,0,6,82,
  	1,0,0,0,8,94,1,0,0,0,10,96,1,0,0,0,12,122,1,0,0,0,14,124,1,0,0,0,16,157,
  	1,0,0,0,18,172,1,0,0,0,20,174,1,0,0,0,22,176,1,0,0,0,24,185,1,0,0,0,26,
  	193,1,0,0,0,28,208,1,0,0,0,30,219,1,0,0,0,32,255,1,0,0,0,34,257,1,0,0,
  	0,36,259,1,0,0,0,38,261,1,0,0,0,40,278,1,0,0,0,42,280,1,0,0,0,44,282,
  	1,0,0,0,46,294,1,0,0,0,48,296,1,0,0,0,50,298,1,0,0,0,52,306,1,0,0,0,54,
  	308,1,0,0,0,56,316,1,0,0,0,58,324,1,0,0,0,60,332,1,0,0,0,62,340,1,0,0,
  	0,64,348,1,0,0,0,66,356,1,0,0,0,68,71,3,2,1,0,69,71,3,22,11,0,70,68,1,
  	0,0,0,70,69,1,0,0,0,71,72,1,0,0,0,72,70,1,0,0,0,72,73,1,0,0,0,73,1,1,
  	0,0,0,74,77,3,6,3,0,75,77,3,14,7,0,76,74,1,0,0,0,76,75,1,0,0,0,77,3,1,
  	0,0,0,78,81,3,6,3,0,79,81,3,14,7,0,80,78,1,0,0,0,80,79,1,0,0,0,81,5,1,
  	0,0,0,82,83,5,1,0,0,83,84,3,8,4,0,84,89,3,10,5,0,85,86,5,26,0,0,86,88,
  	3,10,5,0,87,85,1,0,0,0,88,91,1,0,0,0,89,87,1,0,0,0,89,90,1,0,0,0,90,92,
  	1,0,0,0,91,89,1,0,0,0,92,93,5,27,0,0,93,7,1,0,0,0,94,95,7,0,0,0,95,9,
  	1,0,0,0,96,103,5,34,0,0,97,98,5,32,0,0,98,99,3,66,33,0,99,100,5,33,0,
  	0,100,102,1,0,0,0,101,97,1,0,0,0,102,105,1,0,0,0,103,101,1,0,0,0,103,
  	104,1,0,0,0,104,106,1,0,0,0,105,103,1,0,0,0,106,107,5,25,0,0,107,108,
  	3,12,6,0,108,11,1,0,0,0,109,123,3,66,33,0,110,119,5,30,0,0,111,116,3,
  	12,6,0,112,113,5,26,0,0,113,115,3,12,6,0,114,112,1,0,0,0,115,118,1,0,
  	0,0,116,114,1,0,0,0,116,117,1,0,0,0,117,120,1,0,0,0,118,116,1,0,0,0,119,
  	111,1,0,0,0,119,120,1,0,0,0,120,121,1,0,0,0,121,123,5,31,0,0,122,109,
  	1,0,0,0,122,110,1,0,0,0,123,13,1,0,0,0,124,125,3,8,4,0,125,130,3,16,8,
  	0,126,127,5,26,0,0,127,129,3,16,8,0,128,126,1,0,0,0,129,132,1,0,0,0,130,
  	128,1,0,0,0,130,131,1,0,0,0,131,133,1,0,0,0,132,130,1,0,0,0,133,134,5,
  	27,0,0,134,15,1,0,0,0,135,142,5,34,0,0,136,137,5,32,0,0,137,138,3,66,
  	33,0,138,139,5,33,0,0,139,141,1,0,0,0,140,136,1,0,0,0,141,144,1,0,0,0,
  	142,140,1,0,0,0,142,143,1,0,0,0,143,158,1,0,0,0,144,142,1,0,0,0,145,152,
  	5,34,0,0,146,147,5,32,0,0,147,148,3,66,33,0,148,149,5,33,0,0,149,151,
  	1,0,0,0,150,146,1,0,0,0,151,154,1,0,0,0,152,150,1,0,0,0,152,153,1,0,0,
  	0,153,155,1,0,0,0,154,152,1,0,0,0,155,156,5,25,0,0,156,158,3,18,9,0,157,
  	135,1,0,0,0,157,145,1,0,0,0,158,17,1,0,0,0,159,173,3,34,17,0,160,169,
  	5,30,0,0,161,166,3,18,9,0,162,163,5,26,0,0,163,165,3,18,9,0,164,162,1,
  	0,0,0,165,168,1,0,0,0,166,164,1,0,0,0,166,167,1,0,0,0,167,170,1,0,0,0,
  	168,166,1,0,0,0,169,161,1,0,0,0,169,170,1,0,0,0,170,171,1,0,0,0,171,173,
  	5,31,0,0,172,159,1,0,0,0,172,160,1,0,0,0,173,19,1,0,0,0,174,175,7,1,0,
  	0,175,21,1,0,0,0,176,177,3,20,10,0,177,178,5,34,0,0,178,180,5,28,0,0,
  	179,181,3,24,12,0,180,179,1,0,0,0,180,181,1,0,0,0,181,182,1,0,0,0,182,
  	183,5,29,0,0,183,184,3,28,14,0,184,23,1,0,0,0,185,190,3,26,13,0,186,187,
  	5,26,0,0,187,189,3,26,13,0,188,186,1,0,0,0,189,192,1,0,0,0,190,188,1,
  	0,0,0,190,191,1,0,0,0,191,25,1,0,0,0,192,190,1,0,0,0,193,194,3,8,4,0,
  	194,206,5,34,0,0,195,196,5,32,0,0,196,203,5,33,0,0,197,198,5,32,0,0,198,
  	199,3,34,17,0,199,200,5,33,0,0,200,202,1,0,0,0,201,197,1,0,0,0,202,205,
  	1,0,0,0,203,201,1,0,0,0,203,204,1,0,0,0,204,207,1,0,0,0,205,203,1,0,0,
  	0,206,195,1,0,0,0,206,207,1,0,0,0,207,27,1,0,0,0,208,212,5,30,0,0,209,
  	211,3,30,15,0,210,209,1,0,0,0,211,214,1,0,0,0,212,210,1,0,0,0,212,213,
  	1,0,0,0,213,215,1,0,0,0,214,212,1,0,0,0,215,216,5,31,0,0,216,29,1,0,0,
  	0,217,220,3,4,2,0,218,220,3,32,16,0,219,217,1,0,0,0,219,218,1,0,0,0,220,
  	31,1,0,0,0,221,222,3,38,19,0,222,223,5,25,0,0,223,224,3,34,17,0,224,225,
  	5,27,0,0,225,256,1,0,0,0,226,228,3,34,17,0,227,226,1,0,0,0,227,228,1,
  	0,0,0,228,229,1,0,0,0,229,256,5,27,0,0,230,256,3,28,14,0,231,232,5,5,
  	0,0,232,233,5,28,0,0,233,234,3,36,18,0,234,235,5,29,0,0,235,238,3,32,
  	16,0,236,237,5,6,0,0,237,239,3,32,16,0,238,236,1,0,0,0,238,239,1,0,0,
  	0,239,256,1,0,0,0,240,241,5,7,0,0,241,242,5,28,0,0,242,243,3,36,18,0,
  	243,244,5,29,0,0,244,245,3,32,16,0,245,256,1,0,0,0,246,247,5,8,0,0,247,
  	256,5,27,0,0,248,249,5,9,0,0,249,256,5,27,0,0,250,252,5,10,0,0,251,253,
  	3,34,17,0,252,251,1,0,0,0,252,253,1,0,0,0,253,254,1,0,0,0,254,256,5,27,
  	0,0,255,221,1,0,0,0,255,227,1,0,0,0,255,230,1,0,0,0,255,231,1,0,0,0,255,
  	240,1,0,0,0,255,246,1,0,0,0,255,248,1,0,0,0,255,250,1,0,0,0,256,33,1,
  	0,0,0,257,258,3,56,28,0,258,35,1,0,0,0,259,260,3,64,32,0,260,37,1,0,0,
  	0,261,268,5,34,0,0,262,263,5,32,0,0,263,264,3,34,17,0,264,265,5,33,0,
  	0,265,267,1,0,0,0,266,262,1,0,0,0,267,270,1,0,0,0,268,266,1,0,0,0,268,
  	269,1,0,0,0,269,39,1,0,0,0,270,268,1,0,0,0,271,272,5,28,0,0,272,273,3,
  	34,17,0,273,274,5,29,0,0,274,279,1,0,0,0,275,279,3,38,19,0,276,279,3,
  	42,21,0,277,279,3,52,26,0,278,271,1,0,0,0,278,275,1,0,0,0,278,276,1,0,
  	0,0,278,277,1,0,0,0,279,41,1,0,0,0,280,281,7,2,0,0,281,43,1,0,0,0,282,
  	283,5,34,0,0,283,285,5,28,0,0,284,286,3,50,25,0,285,284,1,0,0,0,285,286,
  	1,0,0,0,286,287,1,0,0,0,287,288,5,29,0,0,288,45,1,0,0,0,289,295,3,40,
  	20,0,290,295,3,44,22,0,291,292,3,48,24,0,292,293,3,46,23,0,293,295,1,
  	0,0,0,294,289,1,0,0,0,294,290,1,0,0,0,294,291,1,0,0,0,295,47,1,0,0,0,
  	296,297,7,3,0,0,297,49,1,0,0,0,298,303,3,34,17,0,299,300,5,26,0,0,300,
  	302,3,34,17,0,301,299,1,0,0,0,302,305,1,0,0,0,303,301,1,0,0,0,303,304,
  	1,0,0,0,304,51,1,0,0,0,305,303,1,0,0,0,306,307,5,37,0,0,307,53,1,0,0,
  	0,308,313,3,46,23,0,309,310,7,4,0,0,310,312,3,46,23,0,311,309,1,0,0,0,
  	312,315,1,0,0,0,313,311,1,0,0,0,313,314,1,0,0,0,314,55,1,0,0,0,315,313,
  	1,0,0,0,316,321,3,54,27,0,317,318,7,5,0,0,318,320,3,54,27,0,319,317,1,
  	0,0,0,320,323,1,0,0,0,321,319,1,0,0,0,321,322,1,0,0,0,322,57,1,0,0,0,
  	323,321,1,0,0,0,324,329,3,56,28,0,325,326,7,6,0,0,326,328,3,56,28,0,327,
  	325,1,0,0,0,328,331,1,0,0,0,329,327,1,0,0,0,329,330,1,0,0,0,330,59,1,
  	0,0,0,331,329,1,0,0,0,332,337,3,58,29,0,333,334,7,7,0,0,334,336,3,58,
  	29,0,335,333,1,0,0,0,336,339,1,0,0,0,337,335,1,0,0,0,337,338,1,0,0,0,
  	338,61,1,0,0,0,339,337,1,0,0,0,340,345,3,60,30,0,341,342,5,22,0,0,342,
  	344,3,60,30,0,343,341,1,0,0,0,344,347,1,0,0,0,345,343,1,0,0,0,345,346,
  	1,0,0,0,346,63,1,0,0,0,347,345,1,0,0,0,348,353,3,62,31,0,349,350,5,23,
  	0,0,350,352,3,62,31,0,351,349,1,0,0,0,352,355,1,0,0,0,353,351,1,0,0,0,
  	353,354,1,0,0,0,354,65,1,0,0,0,355,353,1,0,0,0,356,357,3,56,28,0,357,
  	67,1,0,0,0,37,70,72,76,80,89,103,116,119,122,130,142,152,157,166,169,
  	172,180,190,203,206,212,219,227,238,252,255,268,278,285,294,303,313,321,
  	329,337,345,353
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  sysyParserStaticData = std::move(staticData);
}

}

SysYParser::SysYParser(TokenStream *input) : SysYParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

SysYParser::SysYParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  SysYParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *sysyParserStaticData->atn, sysyParserStaticData->decisionToDFA, sysyParserStaticData->sharedContextCache, options);
}

SysYParser::~SysYParser() {
  delete _interpreter;
}

const atn::ATN& SysYParser::getATN() const {
  return *sysyParserStaticData->atn;
}

std::string SysYParser::getGrammarFileName() const {
  return "SysY.g4";
}

const std::vector<std::string>& SysYParser::getRuleNames() const {
  return sysyParserStaticData->ruleNames;
}

const dfa::Vocabulary& SysYParser::getVocabulary() const {
  return sysyParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SysYParser::getSerializedATN() const {
  return sysyParserStaticData->serializedATN;
}


//----------------- CompUnitContext ------------------------------------------------------------------

SysYParser::CompUnitContext::CompUnitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::GlobalDeclContext *> SysYParser::CompUnitContext::globalDecl() {
  return getRuleContexts<SysYParser::GlobalDeclContext>();
}

SysYParser::GlobalDeclContext* SysYParser::CompUnitContext::globalDecl(size_t i) {
  return getRuleContext<SysYParser::GlobalDeclContext>(i);
}

std::vector<SysYParser::FuncDefContext *> SysYParser::CompUnitContext::funcDef() {
  return getRuleContexts<SysYParser::FuncDefContext>();
}

SysYParser::FuncDefContext* SysYParser::CompUnitContext::funcDef(size_t i) {
  return getRuleContext<SysYParser::FuncDefContext>(i);
}


size_t SysYParser::CompUnitContext::getRuleIndex() const {
  return SysYParser::RuleCompUnit;
}


std::any SysYParser::CompUnitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitCompUnit(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::CompUnitContext* SysYParser::compUnit() {
  CompUnitContext *_localctx = _tracker.createInstance<CompUnitContext>(_ctx, getState());
  enterRule(_localctx, 0, SysYParser::RuleCompUnit);
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
    setState(70); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(70);
      _errHandler->sync(this);
      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
      case 1: {
        setState(68);
        globalDecl();
        break;
      }

      case 2: {
        setState(69);
        funcDef();
        break;
      }

      default:
        break;
      }
      setState(72); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 30) != 0));
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GlobalDeclContext ------------------------------------------------------------------

SysYParser::GlobalDeclContext::GlobalDeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParser::GlobalDeclContext::getRuleIndex() const {
  return SysYParser::RuleGlobalDecl;
}

void SysYParser::GlobalDeclContext::copyFrom(GlobalDeclContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- GlobalConstDeclContext ------------------------------------------------------------------

SysYParser::ConstDeclContext* SysYParser::GlobalConstDeclContext::constDecl() {
  return getRuleContext<SysYParser::ConstDeclContext>(0);
}

SysYParser::GlobalConstDeclContext::GlobalConstDeclContext(GlobalDeclContext *ctx) { copyFrom(ctx); }


std::any SysYParser::GlobalConstDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitGlobalConstDecl(this);
  else
    return visitor->visitChildren(this);
}
//----------------- GlobalVarDeclContext ------------------------------------------------------------------

SysYParser::VarDeclContext* SysYParser::GlobalVarDeclContext::varDecl() {
  return getRuleContext<SysYParser::VarDeclContext>(0);
}

SysYParser::GlobalVarDeclContext::GlobalVarDeclContext(GlobalDeclContext *ctx) { copyFrom(ctx); }


std::any SysYParser::GlobalVarDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitGlobalVarDecl(this);
  else
    return visitor->visitChildren(this);
}
SysYParser::GlobalDeclContext* SysYParser::globalDecl() {
  GlobalDeclContext *_localctx = _tracker.createInstance<GlobalDeclContext>(_ctx, getState());
  enterRule(_localctx, 2, SysYParser::RuleGlobalDecl);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(76);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParser::CONST: {
        _localctx = _tracker.createInstance<SysYParser::GlobalConstDeclContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(74);
        constDecl();
        break;
      }

      case SysYParser::INT:
      case SysYParser::FLOAT: {
        _localctx = _tracker.createInstance<SysYParser::GlobalVarDeclContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(75);
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

//----------------- DeclContext ------------------------------------------------------------------

SysYParser::DeclContext::DeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::ConstDeclContext* SysYParser::DeclContext::constDecl() {
  return getRuleContext<SysYParser::ConstDeclContext>(0);
}

SysYParser::VarDeclContext* SysYParser::DeclContext::varDecl() {
  return getRuleContext<SysYParser::VarDeclContext>(0);
}


size_t SysYParser::DeclContext::getRuleIndex() const {
  return SysYParser::RuleDecl;
}


std::any SysYParser::DeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitDecl(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::DeclContext* SysYParser::decl() {
  DeclContext *_localctx = _tracker.createInstance<DeclContext>(_ctx, getState());
  enterRule(_localctx, 4, SysYParser::RuleDecl);

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
    switch (_input->LA(1)) {
      case SysYParser::CONST: {
        enterOuterAlt(_localctx, 1);
        setState(78);
        constDecl();
        break;
      }

      case SysYParser::INT:
      case SysYParser::FLOAT: {
        enterOuterAlt(_localctx, 2);
        setState(79);
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

SysYParser::ConstDeclContext::ConstDeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::ConstDeclContext::CONST() {
  return getToken(SysYParser::CONST, 0);
}

SysYParser::BTypeContext* SysYParser::ConstDeclContext::bType() {
  return getRuleContext<SysYParser::BTypeContext>(0);
}

std::vector<SysYParser::ConstDefContext *> SysYParser::ConstDeclContext::constDef() {
  return getRuleContexts<SysYParser::ConstDefContext>();
}

SysYParser::ConstDefContext* SysYParser::ConstDeclContext::constDef(size_t i) {
  return getRuleContext<SysYParser::ConstDefContext>(i);
}

tree::TerminalNode* SysYParser::ConstDeclContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

std::vector<tree::TerminalNode *> SysYParser::ConstDeclContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode* SysYParser::ConstDeclContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}


size_t SysYParser::ConstDeclContext::getRuleIndex() const {
  return SysYParser::RuleConstDecl;
}


std::any SysYParser::ConstDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitConstDecl(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ConstDeclContext* SysYParser::constDecl() {
  ConstDeclContext *_localctx = _tracker.createInstance<ConstDeclContext>(_ctx, getState());
  enterRule(_localctx, 6, SysYParser::RuleConstDecl);
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
    setState(82);
    match(SysYParser::CONST);
    setState(83);
    bType();
    setState(84);
    constDef();
    setState(89);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(85);
      match(SysYParser::COMMA);
      setState(86);
      constDef();
      setState(91);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(92);
    match(SysYParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
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

tree::TerminalNode* SysYParser::BTypeContext::INT() {
  return getToken(SysYParser::INT, 0);
}

tree::TerminalNode* SysYParser::BTypeContext::FLOAT() {
  return getToken(SysYParser::FLOAT, 0);
}


size_t SysYParser::BTypeContext::getRuleIndex() const {
  return SysYParser::RuleBType;
}


std::any SysYParser::BTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitBType(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::BTypeContext* SysYParser::bType() {
  BTypeContext *_localctx = _tracker.createInstance<BTypeContext>(_ctx, getState());
  enterRule(_localctx, 8, SysYParser::RuleBType);
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
    setState(94);
    _la = _input->LA(1);
    if (!(_la == SysYParser::INT

    || _la == SysYParser::FLOAT)) {
    _errHandler->recoverInline(this);
    }
    else {
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

SysYParser::ConstDefContext::ConstDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::ConstDefContext::Ident() {
  return getToken(SysYParser::Ident, 0);
}

tree::TerminalNode* SysYParser::ConstDefContext::ASSIGN() {
  return getToken(SysYParser::ASSIGN, 0);
}

SysYParser::ConstInitValContext* SysYParser::ConstDefContext::constInitVal() {
  return getRuleContext<SysYParser::ConstInitValContext>(0);
}

std::vector<tree::TerminalNode *> SysYParser::ConstDefContext::LBRACK() {
  return getTokens(SysYParser::LBRACK);
}

tree::TerminalNode* SysYParser::ConstDefContext::LBRACK(size_t i) {
  return getToken(SysYParser::LBRACK, i);
}

std::vector<SysYParser::ConstExpContext *> SysYParser::ConstDefContext::constExp() {
  return getRuleContexts<SysYParser::ConstExpContext>();
}

SysYParser::ConstExpContext* SysYParser::ConstDefContext::constExp(size_t i) {
  return getRuleContext<SysYParser::ConstExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::ConstDefContext::RBRACK() {
  return getTokens(SysYParser::RBRACK);
}

tree::TerminalNode* SysYParser::ConstDefContext::RBRACK(size_t i) {
  return getToken(SysYParser::RBRACK, i);
}


size_t SysYParser::ConstDefContext::getRuleIndex() const {
  return SysYParser::RuleConstDef;
}


std::any SysYParser::ConstDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitConstDef(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ConstDefContext* SysYParser::constDef() {
  ConstDefContext *_localctx = _tracker.createInstance<ConstDefContext>(_ctx, getState());
  enterRule(_localctx, 10, SysYParser::RuleConstDef);
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
    setState(96);
    match(SysYParser::Ident);
    setState(103);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::LBRACK) {
      setState(97);
      match(SysYParser::LBRACK);
      setState(98);
      constExp();
      setState(99);
      match(SysYParser::RBRACK);
      setState(105);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(106);
    match(SysYParser::ASSIGN);
    setState(107);
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

SysYParser::ConstInitValContext::ConstInitValContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParser::ConstInitValContext::getRuleIndex() const {
  return SysYParser::RuleConstInitVal;
}

void SysYParser::ConstInitValContext::copyFrom(ConstInitValContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ConstScalarInitValueContext ------------------------------------------------------------------

SysYParser::ConstExpContext* SysYParser::ConstScalarInitValueContext::constExp() {
  return getRuleContext<SysYParser::ConstExpContext>(0);
}

SysYParser::ConstScalarInitValueContext::ConstScalarInitValueContext(ConstInitValContext *ctx) { copyFrom(ctx); }


std::any SysYParser::ConstScalarInitValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitConstScalarInitValue(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ConstArrayInitValueContext ------------------------------------------------------------------

tree::TerminalNode* SysYParser::ConstArrayInitValueContext::LBRACE() {
  return getToken(SysYParser::LBRACE, 0);
}

tree::TerminalNode* SysYParser::ConstArrayInitValueContext::RBRACE() {
  return getToken(SysYParser::RBRACE, 0);
}

std::vector<SysYParser::ConstInitValContext *> SysYParser::ConstArrayInitValueContext::constInitVal() {
  return getRuleContexts<SysYParser::ConstInitValContext>();
}

SysYParser::ConstInitValContext* SysYParser::ConstArrayInitValueContext::constInitVal(size_t i) {
  return getRuleContext<SysYParser::ConstInitValContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::ConstArrayInitValueContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode* SysYParser::ConstArrayInitValueContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}

SysYParser::ConstArrayInitValueContext::ConstArrayInitValueContext(ConstInitValContext *ctx) { copyFrom(ctx); }


std::any SysYParser::ConstArrayInitValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitConstArrayInitValue(this);
  else
    return visitor->visitChildren(this);
}
SysYParser::ConstInitValContext* SysYParser::constInitVal() {
  ConstInitValContext *_localctx = _tracker.createInstance<ConstInitValContext>(_ctx, getState());
  enterRule(_localctx, 12, SysYParser::RuleConstInitVal);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(122);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParser::ADD:
      case SysYParser::SUB:
      case SysYParser::NOT:
      case SysYParser::LPAREN:
      case SysYParser::Ident:
      case SysYParser::ILITERAL:
      case SysYParser::FLITERAL:
      case SysYParser::STRING: {
        _localctx = _tracker.createInstance<SysYParser::ConstScalarInitValueContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(109);
        constExp();
        break;
      }

      case SysYParser::LBRACE: {
        _localctx = _tracker.createInstance<SysYParser::ConstArrayInitValueContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(110);
        match(SysYParser::LBRACE);
        setState(119);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 259056998400) != 0)) {
          setState(111);
          constInitVal();
          setState(116);
          _errHandler->sync(this);
          _la = _input->LA(1);
          while (_la == SysYParser::COMMA) {
            setState(112);
            match(SysYParser::COMMA);
            setState(113);
            constInitVal();
            setState(118);
            _errHandler->sync(this);
            _la = _input->LA(1);
          }
        }
        setState(121);
        match(SysYParser::RBRACE);
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

SysYParser::VarDeclContext::VarDeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::BTypeContext* SysYParser::VarDeclContext::bType() {
  return getRuleContext<SysYParser::BTypeContext>(0);
}

std::vector<SysYParser::VarDefContext *> SysYParser::VarDeclContext::varDef() {
  return getRuleContexts<SysYParser::VarDefContext>();
}

SysYParser::VarDefContext* SysYParser::VarDeclContext::varDef(size_t i) {
  return getRuleContext<SysYParser::VarDefContext>(i);
}

tree::TerminalNode* SysYParser::VarDeclContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

std::vector<tree::TerminalNode *> SysYParser::VarDeclContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode* SysYParser::VarDeclContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}


size_t SysYParser::VarDeclContext::getRuleIndex() const {
  return SysYParser::RuleVarDecl;
}


std::any SysYParser::VarDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitVarDecl(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::VarDeclContext* SysYParser::varDecl() {
  VarDeclContext *_localctx = _tracker.createInstance<VarDeclContext>(_ctx, getState());
  enterRule(_localctx, 14, SysYParser::RuleVarDecl);
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
    setState(124);
    bType();
    setState(125);
    varDef();
    setState(130);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(126);
      match(SysYParser::COMMA);
      setState(127);
      varDef();
      setState(132);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(133);
    match(SysYParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
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

tree::TerminalNode* SysYParser::VarDefContext::Ident() {
  return getToken(SysYParser::Ident, 0);
}

std::vector<tree::TerminalNode *> SysYParser::VarDefContext::LBRACK() {
  return getTokens(SysYParser::LBRACK);
}

tree::TerminalNode* SysYParser::VarDefContext::LBRACK(size_t i) {
  return getToken(SysYParser::LBRACK, i);
}

std::vector<SysYParser::ConstExpContext *> SysYParser::VarDefContext::constExp() {
  return getRuleContexts<SysYParser::ConstExpContext>();
}

SysYParser::ConstExpContext* SysYParser::VarDefContext::constExp(size_t i) {
  return getRuleContext<SysYParser::ConstExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::VarDefContext::RBRACK() {
  return getTokens(SysYParser::RBRACK);
}

tree::TerminalNode* SysYParser::VarDefContext::RBRACK(size_t i) {
  return getToken(SysYParser::RBRACK, i);
}

tree::TerminalNode* SysYParser::VarDefContext::ASSIGN() {
  return getToken(SysYParser::ASSIGN, 0);
}

SysYParser::InitValContext* SysYParser::VarDefContext::initVal() {
  return getRuleContext<SysYParser::InitValContext>(0);
}


size_t SysYParser::VarDefContext::getRuleIndex() const {
  return SysYParser::RuleVarDef;
}


std::any SysYParser::VarDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitVarDef(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::VarDefContext* SysYParser::varDef() {
  VarDefContext *_localctx = _tracker.createInstance<VarDefContext>(_ctx, getState());
  enterRule(_localctx, 16, SysYParser::RuleVarDef);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(157);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 12, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(135);
      match(SysYParser::Ident);
      setState(142);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == SysYParser::LBRACK) {
        setState(136);
        match(SysYParser::LBRACK);
        setState(137);
        constExp();
        setState(138);
        match(SysYParser::RBRACK);
        setState(144);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(145);
      match(SysYParser::Ident);
      setState(152);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == SysYParser::LBRACK) {
        setState(146);
        match(SysYParser::LBRACK);
        setState(147);
        constExp();
        setState(148);
        match(SysYParser::RBRACK);
        setState(154);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      setState(155);
      match(SysYParser::ASSIGN);
      setState(156);
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

SysYParser::InitValContext::InitValContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParser::InitValContext::getRuleIndex() const {
  return SysYParser::RuleInitVal;
}

void SysYParser::InitValContext::copyFrom(InitValContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ArrayInitValueContext ------------------------------------------------------------------

tree::TerminalNode* SysYParser::ArrayInitValueContext::LBRACE() {
  return getToken(SysYParser::LBRACE, 0);
}

tree::TerminalNode* SysYParser::ArrayInitValueContext::RBRACE() {
  return getToken(SysYParser::RBRACE, 0);
}

std::vector<SysYParser::InitValContext *> SysYParser::ArrayInitValueContext::initVal() {
  return getRuleContexts<SysYParser::InitValContext>();
}

SysYParser::InitValContext* SysYParser::ArrayInitValueContext::initVal(size_t i) {
  return getRuleContext<SysYParser::InitValContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::ArrayInitValueContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode* SysYParser::ArrayInitValueContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}

SysYParser::ArrayInitValueContext::ArrayInitValueContext(InitValContext *ctx) { copyFrom(ctx); }


std::any SysYParser::ArrayInitValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitArrayInitValue(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ScalarInitValueContext ------------------------------------------------------------------

SysYParser::ExpContext* SysYParser::ScalarInitValueContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

SysYParser::ScalarInitValueContext::ScalarInitValueContext(InitValContext *ctx) { copyFrom(ctx); }


std::any SysYParser::ScalarInitValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitScalarInitValue(this);
  else
    return visitor->visitChildren(this);
}
SysYParser::InitValContext* SysYParser::initVal() {
  InitValContext *_localctx = _tracker.createInstance<InitValContext>(_ctx, getState());
  enterRule(_localctx, 18, SysYParser::RuleInitVal);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(172);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParser::ADD:
      case SysYParser::SUB:
      case SysYParser::NOT:
      case SysYParser::LPAREN:
      case SysYParser::Ident:
      case SysYParser::ILITERAL:
      case SysYParser::FLITERAL:
      case SysYParser::STRING: {
        _localctx = _tracker.createInstance<SysYParser::ScalarInitValueContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(159);
        exp();
        break;
      }

      case SysYParser::LBRACE: {
        _localctx = _tracker.createInstance<SysYParser::ArrayInitValueContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(160);
        match(SysYParser::LBRACE);
        setState(169);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 259056998400) != 0)) {
          setState(161);
          initVal();
          setState(166);
          _errHandler->sync(this);
          _la = _input->LA(1);
          while (_la == SysYParser::COMMA) {
            setState(162);
            match(SysYParser::COMMA);
            setState(163);
            initVal();
            setState(168);
            _errHandler->sync(this);
            _la = _input->LA(1);
          }
        }
        setState(171);
        match(SysYParser::RBRACE);
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

//----------------- FuncTypeContext ------------------------------------------------------------------

SysYParser::FuncTypeContext::FuncTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::FuncTypeContext::VOID() {
  return getToken(SysYParser::VOID, 0);
}

tree::TerminalNode* SysYParser::FuncTypeContext::INT() {
  return getToken(SysYParser::INT, 0);
}

tree::TerminalNode* SysYParser::FuncTypeContext::FLOAT() {
  return getToken(SysYParser::FLOAT, 0);
}


size_t SysYParser::FuncTypeContext::getRuleIndex() const {
  return SysYParser::RuleFuncType;
}


std::any SysYParser::FuncTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitFuncType(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncTypeContext* SysYParser::funcType() {
  FuncTypeContext *_localctx = _tracker.createInstance<FuncTypeContext>(_ctx, getState());
  enterRule(_localctx, 20, SysYParser::RuleFuncType);
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
    setState(174);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 28) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
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

//----------------- FuncDefContext ------------------------------------------------------------------

SysYParser::FuncDefContext::FuncDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::FuncTypeContext* SysYParser::FuncDefContext::funcType() {
  return getRuleContext<SysYParser::FuncTypeContext>(0);
}

tree::TerminalNode* SysYParser::FuncDefContext::Ident() {
  return getToken(SysYParser::Ident, 0);
}

tree::TerminalNode* SysYParser::FuncDefContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

tree::TerminalNode* SysYParser::FuncDefContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::BlockStmtContext* SysYParser::FuncDefContext::blockStmt() {
  return getRuleContext<SysYParser::BlockStmtContext>(0);
}

SysYParser::FuncFParamsContext* SysYParser::FuncDefContext::funcFParams() {
  return getRuleContext<SysYParser::FuncFParamsContext>(0);
}


size_t SysYParser::FuncDefContext::getRuleIndex() const {
  return SysYParser::RuleFuncDef;
}


std::any SysYParser::FuncDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitFuncDef(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncDefContext* SysYParser::funcDef() {
  FuncDefContext *_localctx = _tracker.createInstance<FuncDefContext>(_ctx, getState());
  enterRule(_localctx, 22, SysYParser::RuleFuncDef);
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
    setState(176);
    funcType();
    setState(177);
    match(SysYParser::Ident);
    setState(178);
    match(SysYParser::LPAREN);
    setState(180);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::INT

    || _la == SysYParser::FLOAT) {
      setState(179);
      funcFParams();
    }
    setState(182);
    match(SysYParser::RPAREN);
    setState(183);
    blockStmt();
   
  }
  catch (RecognitionException &e) {
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

SysYParser::FuncFParamContext* SysYParser::FuncFParamsContext::funcFParam(size_t i) {
  return getRuleContext<SysYParser::FuncFParamContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::FuncFParamsContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode* SysYParser::FuncFParamsContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}


size_t SysYParser::FuncFParamsContext::getRuleIndex() const {
  return SysYParser::RuleFuncFParams;
}


std::any SysYParser::FuncFParamsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitFuncFParams(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncFParamsContext* SysYParser::funcFParams() {
  FuncFParamsContext *_localctx = _tracker.createInstance<FuncFParamsContext>(_ctx, getState());
  enterRule(_localctx, 24, SysYParser::RuleFuncFParams);
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
    setState(185);
    funcFParam();
    setState(190);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(186);
      match(SysYParser::COMMA);
      setState(187);
      funcFParam();
      setState(192);
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

SysYParser::FuncFParamContext::FuncFParamContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::BTypeContext* SysYParser::FuncFParamContext::bType() {
  return getRuleContext<SysYParser::BTypeContext>(0);
}

tree::TerminalNode* SysYParser::FuncFParamContext::Ident() {
  return getToken(SysYParser::Ident, 0);
}

std::vector<tree::TerminalNode *> SysYParser::FuncFParamContext::LBRACK() {
  return getTokens(SysYParser::LBRACK);
}

tree::TerminalNode* SysYParser::FuncFParamContext::LBRACK(size_t i) {
  return getToken(SysYParser::LBRACK, i);
}

std::vector<tree::TerminalNode *> SysYParser::FuncFParamContext::RBRACK() {
  return getTokens(SysYParser::RBRACK);
}

tree::TerminalNode* SysYParser::FuncFParamContext::RBRACK(size_t i) {
  return getToken(SysYParser::RBRACK, i);
}

std::vector<SysYParser::ExpContext *> SysYParser::FuncFParamContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::FuncFParamContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}


size_t SysYParser::FuncFParamContext::getRuleIndex() const {
  return SysYParser::RuleFuncFParam;
}


std::any SysYParser::FuncFParamContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitFuncFParam(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncFParamContext* SysYParser::funcFParam() {
  FuncFParamContext *_localctx = _tracker.createInstance<FuncFParamContext>(_ctx, getState());
  enterRule(_localctx, 26, SysYParser::RuleFuncFParam);
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
    setState(193);
    bType();
    setState(194);
    match(SysYParser::Ident);
    setState(206);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::LBRACK) {
      setState(195);
      match(SysYParser::LBRACK);
      setState(196);
      match(SysYParser::RBRACK);
      setState(203);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == SysYParser::LBRACK) {
        setState(197);
        match(SysYParser::LBRACK);
        setState(198);
        exp();
        setState(199);
        match(SysYParser::RBRACK);
        setState(205);
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

//----------------- BlockStmtContext ------------------------------------------------------------------

SysYParser::BlockStmtContext::BlockStmtContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::BlockStmtContext::LBRACE() {
  return getToken(SysYParser::LBRACE, 0);
}

tree::TerminalNode* SysYParser::BlockStmtContext::RBRACE() {
  return getToken(SysYParser::RBRACE, 0);
}

std::vector<SysYParser::BlockItemContext *> SysYParser::BlockStmtContext::blockItem() {
  return getRuleContexts<SysYParser::BlockItemContext>();
}

SysYParser::BlockItemContext* SysYParser::BlockStmtContext::blockItem(size_t i) {
  return getRuleContext<SysYParser::BlockItemContext>(i);
}


size_t SysYParser::BlockStmtContext::getRuleIndex() const {
  return SysYParser::RuleBlockStmt;
}


std::any SysYParser::BlockStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitBlockStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::BlockStmtContext* SysYParser::blockStmt() {
  BlockStmtContext *_localctx = _tracker.createInstance<BlockStmtContext>(_ctx, getState());
  enterRule(_localctx, 28, SysYParser::RuleBlockStmt);
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
    setState(208);
    match(SysYParser::LBRACE);
    setState(212);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 259191218094) != 0)) {
      setState(209);
      blockItem();
      setState(214);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(215);
    match(SysYParser::RBRACE);
   
  }
  catch (RecognitionException &e) {
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

SysYParser::DeclContext* SysYParser::BlockItemContext::decl() {
  return getRuleContext<SysYParser::DeclContext>(0);
}

SysYParser::StmtContext* SysYParser::BlockItemContext::stmt() {
  return getRuleContext<SysYParser::StmtContext>(0);
}


size_t SysYParser::BlockItemContext::getRuleIndex() const {
  return SysYParser::RuleBlockItem;
}


std::any SysYParser::BlockItemContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitBlockItem(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::BlockItemContext* SysYParser::blockItem() {
  BlockItemContext *_localctx = _tracker.createInstance<BlockItemContext>(_ctx, getState());
  enterRule(_localctx, 30, SysYParser::RuleBlockItem);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(219);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParser::CONST:
      case SysYParser::INT:
      case SysYParser::FLOAT: {
        enterOuterAlt(_localctx, 1);
        setState(217);
        decl();
        break;
      }

      case SysYParser::IF:
      case SysYParser::WHILE:
      case SysYParser::BREAK:
      case SysYParser::CONTINUE:
      case SysYParser::RETURN:
      case SysYParser::ADD:
      case SysYParser::SUB:
      case SysYParser::NOT:
      case SysYParser::SEMICOLON:
      case SysYParser::LPAREN:
      case SysYParser::LBRACE:
      case SysYParser::Ident:
      case SysYParser::ILITERAL:
      case SysYParser::FLITERAL:
      case SysYParser::STRING: {
        enterOuterAlt(_localctx, 2);
        setState(218);
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

SysYParser::StmtContext::StmtContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParser::StmtContext::getRuleIndex() const {
  return SysYParser::RuleStmt;
}

void SysYParser::StmtContext::copyFrom(StmtContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- BlkStmtContext ------------------------------------------------------------------

SysYParser::BlockStmtContext* SysYParser::BlkStmtContext::blockStmt() {
  return getRuleContext<SysYParser::BlockStmtContext>(0);
}

SysYParser::BlkStmtContext::BlkStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysYParser::BlkStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitBlkStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- WhileStmtContext ------------------------------------------------------------------

tree::TerminalNode* SysYParser::WhileStmtContext::WHILE() {
  return getToken(SysYParser::WHILE, 0);
}

tree::TerminalNode* SysYParser::WhileStmtContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

SysYParser::CondContext* SysYParser::WhileStmtContext::cond() {
  return getRuleContext<SysYParser::CondContext>(0);
}

tree::TerminalNode* SysYParser::WhileStmtContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::StmtContext* SysYParser::WhileStmtContext::stmt() {
  return getRuleContext<SysYParser::StmtContext>(0);
}

SysYParser::WhileStmtContext::WhileStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysYParser::WhileStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitWhileStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IfStmtContext ------------------------------------------------------------------

tree::TerminalNode* SysYParser::IfStmtContext::IF() {
  return getToken(SysYParser::IF, 0);
}

tree::TerminalNode* SysYParser::IfStmtContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

SysYParser::CondContext* SysYParser::IfStmtContext::cond() {
  return getRuleContext<SysYParser::CondContext>(0);
}

tree::TerminalNode* SysYParser::IfStmtContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

std::vector<SysYParser::StmtContext *> SysYParser::IfStmtContext::stmt() {
  return getRuleContexts<SysYParser::StmtContext>();
}

SysYParser::StmtContext* SysYParser::IfStmtContext::stmt(size_t i) {
  return getRuleContext<SysYParser::StmtContext>(i);
}

tree::TerminalNode* SysYParser::IfStmtContext::ELSE() {
  return getToken(SysYParser::ELSE, 0);
}

SysYParser::IfStmtContext::IfStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysYParser::IfStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitIfStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AssignStmtContext ------------------------------------------------------------------

SysYParser::LValueContext* SysYParser::AssignStmtContext::lValue() {
  return getRuleContext<SysYParser::LValueContext>(0);
}

tree::TerminalNode* SysYParser::AssignStmtContext::ASSIGN() {
  return getToken(SysYParser::ASSIGN, 0);
}

SysYParser::ExpContext* SysYParser::AssignStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode* SysYParser::AssignStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

SysYParser::AssignStmtContext::AssignStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysYParser::AssignStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitAssignStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BreakStmtContext ------------------------------------------------------------------

tree::TerminalNode* SysYParser::BreakStmtContext::BREAK() {
  return getToken(SysYParser::BREAK, 0);
}

tree::TerminalNode* SysYParser::BreakStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

SysYParser::BreakStmtContext::BreakStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysYParser::BreakStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitBreakStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ExpStmtContext ------------------------------------------------------------------

tree::TerminalNode* SysYParser::ExpStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

SysYParser::ExpContext* SysYParser::ExpStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

SysYParser::ExpStmtContext::ExpStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysYParser::ExpStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitExpStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ReturnStmtContext ------------------------------------------------------------------

tree::TerminalNode* SysYParser::ReturnStmtContext::RETURN() {
  return getToken(SysYParser::RETURN, 0);
}

tree::TerminalNode* SysYParser::ReturnStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

SysYParser::ExpContext* SysYParser::ReturnStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

SysYParser::ReturnStmtContext::ReturnStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysYParser::ReturnStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitReturnStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ContinueStmtContext ------------------------------------------------------------------

tree::TerminalNode* SysYParser::ContinueStmtContext::CONTINUE() {
  return getToken(SysYParser::CONTINUE, 0);
}

tree::TerminalNode* SysYParser::ContinueStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

SysYParser::ContinueStmtContext::ContinueStmtContext(StmtContext *ctx) { copyFrom(ctx); }


std::any SysYParser::ContinueStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitContinueStmt(this);
  else
    return visitor->visitChildren(this);
}
SysYParser::StmtContext* SysYParser::stmt() {
  StmtContext *_localctx = _tracker.createInstance<StmtContext>(_ctx, getState());
  enterRule(_localctx, 32, SysYParser::RuleStmt);
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
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 25, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SysYParser::AssignStmtContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(221);
      lValue();
      setState(222);
      match(SysYParser::ASSIGN);
      setState(223);
      exp();
      setState(224);
      match(SysYParser::SEMICOLON);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysYParser::ExpStmtContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(227);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 257983256576) != 0)) {
        setState(226);
        exp();
      }
      setState(229);
      match(SysYParser::SEMICOLON);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SysYParser::BlkStmtContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(230);
      blockStmt();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<SysYParser::IfStmtContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(231);
      match(SysYParser::IF);
      setState(232);
      match(SysYParser::LPAREN);
      setState(233);
      cond();
      setState(234);
      match(SysYParser::RPAREN);
      setState(235);
      stmt();
      setState(238);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx)) {
      case 1: {
        setState(236);
        match(SysYParser::ELSE);
        setState(237);
        stmt();
        break;
      }

      default:
        break;
      }
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<SysYParser::WhileStmtContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(240);
      match(SysYParser::WHILE);
      setState(241);
      match(SysYParser::LPAREN);
      setState(242);
      cond();
      setState(243);
      match(SysYParser::RPAREN);
      setState(244);
      stmt();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<SysYParser::BreakStmtContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(246);
      match(SysYParser::BREAK);
      setState(247);
      match(SysYParser::SEMICOLON);
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<SysYParser::ContinueStmtContext>(_localctx);
      enterOuterAlt(_localctx, 7);
      setState(248);
      match(SysYParser::CONTINUE);
      setState(249);
      match(SysYParser::SEMICOLON);
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<SysYParser::ReturnStmtContext>(_localctx);
      enterOuterAlt(_localctx, 8);
      setState(250);
      match(SysYParser::RETURN);
      setState(252);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 257983256576) != 0)) {
        setState(251);
        exp();
      }
      setState(254);
      match(SysYParser::SEMICOLON);
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

SysYParser::ExpContext::ExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::AddExpContext* SysYParser::ExpContext::addExp() {
  return getRuleContext<SysYParser::AddExpContext>(0);
}


size_t SysYParser::ExpContext::getRuleIndex() const {
  return SysYParser::RuleExp;
}


std::any SysYParser::ExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ExpContext* SysYParser::exp() {
  ExpContext *_localctx = _tracker.createInstance<ExpContext>(_ctx, getState());
  enterRule(_localctx, 34, SysYParser::RuleExp);

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
    addExp();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CondContext ------------------------------------------------------------------

SysYParser::CondContext::CondContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::LOrExpContext* SysYParser::CondContext::lOrExp() {
  return getRuleContext<SysYParser::LOrExpContext>(0);
}


size_t SysYParser::CondContext::getRuleIndex() const {
  return SysYParser::RuleCond;
}


std::any SysYParser::CondContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitCond(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::CondContext* SysYParser::cond() {
  CondContext *_localctx = _tracker.createInstance<CondContext>(_ctx, getState());
  enterRule(_localctx, 36, SysYParser::RuleCond);

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
    lOrExp();
   
  }
  catch (RecognitionException &e) {
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

tree::TerminalNode* SysYParser::LValueContext::Ident() {
  return getToken(SysYParser::Ident, 0);
}

std::vector<tree::TerminalNode *> SysYParser::LValueContext::LBRACK() {
  return getTokens(SysYParser::LBRACK);
}

tree::TerminalNode* SysYParser::LValueContext::LBRACK(size_t i) {
  return getToken(SysYParser::LBRACK, i);
}

std::vector<SysYParser::ExpContext *> SysYParser::LValueContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::LValueContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::LValueContext::RBRACK() {
  return getTokens(SysYParser::RBRACK);
}

tree::TerminalNode* SysYParser::LValueContext::RBRACK(size_t i) {
  return getToken(SysYParser::RBRACK, i);
}


size_t SysYParser::LValueContext::getRuleIndex() const {
  return SysYParser::RuleLValue;
}


std::any SysYParser::LValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitLValue(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::LValueContext* SysYParser::lValue() {
  LValueContext *_localctx = _tracker.createInstance<LValueContext>(_ctx, getState());
  enterRule(_localctx, 38, SysYParser::RuleLValue);
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
    setState(261);
    match(SysYParser::Ident);
    setState(268);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::LBRACK) {
      setState(262);
      match(SysYParser::LBRACK);
      setState(263);
      exp();
      setState(264);
      match(SysYParser::RBRACK);
      setState(270);
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

//----------------- PrimaryExpContext ------------------------------------------------------------------

SysYParser::PrimaryExpContext::PrimaryExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::PrimaryExpContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

SysYParser::ExpContext* SysYParser::PrimaryExpContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode* SysYParser::PrimaryExpContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::LValueContext* SysYParser::PrimaryExpContext::lValue() {
  return getRuleContext<SysYParser::LValueContext>(0);
}

SysYParser::NumberContext* SysYParser::PrimaryExpContext::number() {
  return getRuleContext<SysYParser::NumberContext>(0);
}

SysYParser::StringContext* SysYParser::PrimaryExpContext::string() {
  return getRuleContext<SysYParser::StringContext>(0);
}


size_t SysYParser::PrimaryExpContext::getRuleIndex() const {
  return SysYParser::RulePrimaryExp;
}


std::any SysYParser::PrimaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitPrimaryExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::PrimaryExpContext* SysYParser::primaryExp() {
  PrimaryExpContext *_localctx = _tracker.createInstance<PrimaryExpContext>(_ctx, getState());
  enterRule(_localctx, 40, SysYParser::RulePrimaryExp);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(278);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParser::LPAREN: {
        enterOuterAlt(_localctx, 1);
        setState(271);
        match(SysYParser::LPAREN);
        setState(272);
        exp();
        setState(273);
        match(SysYParser::RPAREN);
        break;
      }

      case SysYParser::Ident: {
        enterOuterAlt(_localctx, 2);
        setState(275);
        lValue();
        break;
      }

      case SysYParser::ILITERAL:
      case SysYParser::FLITERAL: {
        enterOuterAlt(_localctx, 3);
        setState(276);
        number();
        break;
      }

      case SysYParser::STRING: {
        enterOuterAlt(_localctx, 4);
        setState(277);
        string();
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

SysYParser::NumberContext::NumberContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::NumberContext::ILITERAL() {
  return getToken(SysYParser::ILITERAL, 0);
}

tree::TerminalNode* SysYParser::NumberContext::FLITERAL() {
  return getToken(SysYParser::FLITERAL, 0);
}


size_t SysYParser::NumberContext::getRuleIndex() const {
  return SysYParser::RuleNumber;
}


std::any SysYParser::NumberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitNumber(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::NumberContext* SysYParser::number() {
  NumberContext *_localctx = _tracker.createInstance<NumberContext>(_ctx, getState());
  enterRule(_localctx, 42, SysYParser::RuleNumber);
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
    setState(280);
    _la = _input->LA(1);
    if (!(_la == SysYParser::ILITERAL

    || _la == SysYParser::FLITERAL)) {
    _errHandler->recoverInline(this);
    }
    else {
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

//----------------- CallContext ------------------------------------------------------------------

SysYParser::CallContext::CallContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::CallContext::Ident() {
  return getToken(SysYParser::Ident, 0);
}

tree::TerminalNode* SysYParser::CallContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

tree::TerminalNode* SysYParser::CallContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::FuncRParamsContext* SysYParser::CallContext::funcRParams() {
  return getRuleContext<SysYParser::FuncRParamsContext>(0);
}


size_t SysYParser::CallContext::getRuleIndex() const {
  return SysYParser::RuleCall;
}


std::any SysYParser::CallContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitCall(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::CallContext* SysYParser::call() {
  CallContext *_localctx = _tracker.createInstance<CallContext>(_ctx, getState());
  enterRule(_localctx, 44, SysYParser::RuleCall);
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
    setState(282);
    match(SysYParser::Ident);
    setState(283);
    match(SysYParser::LPAREN);
    setState(285);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 257983256576) != 0)) {
      setState(284);
      funcRParams();
    }
    setState(287);
    match(SysYParser::RPAREN);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- UnaryExpContext ------------------------------------------------------------------

SysYParser::UnaryExpContext::UnaryExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::PrimaryExpContext* SysYParser::UnaryExpContext::primaryExp() {
  return getRuleContext<SysYParser::PrimaryExpContext>(0);
}

SysYParser::CallContext* SysYParser::UnaryExpContext::call() {
  return getRuleContext<SysYParser::CallContext>(0);
}

SysYParser::UnaryOpContext* SysYParser::UnaryExpContext::unaryOp() {
  return getRuleContext<SysYParser::UnaryOpContext>(0);
}

SysYParser::UnaryExpContext* SysYParser::UnaryExpContext::unaryExp() {
  return getRuleContext<SysYParser::UnaryExpContext>(0);
}


size_t SysYParser::UnaryExpContext::getRuleIndex() const {
  return SysYParser::RuleUnaryExp;
}


std::any SysYParser::UnaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitUnaryExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::UnaryExpContext* SysYParser::unaryExp() {
  UnaryExpContext *_localctx = _tracker.createInstance<UnaryExpContext>(_ctx, getState());
  enterRule(_localctx, 46, SysYParser::RuleUnaryExp);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(294);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(289);
      primaryExp();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(290);
      call();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(291);
      unaryOp();
      setState(292);
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

SysYParser::UnaryOpContext::UnaryOpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::UnaryOpContext::ADD() {
  return getToken(SysYParser::ADD, 0);
}

tree::TerminalNode* SysYParser::UnaryOpContext::SUB() {
  return getToken(SysYParser::SUB, 0);
}

tree::TerminalNode* SysYParser::UnaryOpContext::NOT() {
  return getToken(SysYParser::NOT, 0);
}


size_t SysYParser::UnaryOpContext::getRuleIndex() const {
  return SysYParser::RuleUnaryOp;
}


std::any SysYParser::UnaryOpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitUnaryOp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::UnaryOpContext* SysYParser::unaryOp() {
  UnaryOpContext *_localctx = _tracker.createInstance<UnaryOpContext>(_ctx, getState());
  enterRule(_localctx, 48, SysYParser::RuleUnaryOp);
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
    setState(296);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 16783360) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
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

SysYParser::FuncRParamsContext::FuncRParamsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::ExpContext *> SysYParser::FuncRParamsContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::FuncRParamsContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::FuncRParamsContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode* SysYParser::FuncRParamsContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}


size_t SysYParser::FuncRParamsContext::getRuleIndex() const {
  return SysYParser::RuleFuncRParams;
}


std::any SysYParser::FuncRParamsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitFuncRParams(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncRParamsContext* SysYParser::funcRParams() {
  FuncRParamsContext *_localctx = _tracker.createInstance<FuncRParamsContext>(_ctx, getState());
  enterRule(_localctx, 50, SysYParser::RuleFuncRParams);
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
    setState(298);
    exp();
    setState(303);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(299);
      match(SysYParser::COMMA);
      setState(300);
      exp();
      setState(305);
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

//----------------- StringContext ------------------------------------------------------------------

SysYParser::StringContext::StringContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::StringContext::STRING() {
  return getToken(SysYParser::STRING, 0);
}


size_t SysYParser::StringContext::getRuleIndex() const {
  return SysYParser::RuleString;
}


std::any SysYParser::StringContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitString(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::StringContext* SysYParser::string() {
  StringContext *_localctx = _tracker.createInstance<StringContext>(_ctx, getState());
  enterRule(_localctx, 52, SysYParser::RuleString);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(306);
    match(SysYParser::STRING);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MulExpContext ------------------------------------------------------------------

SysYParser::MulExpContext::MulExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::UnaryExpContext *> SysYParser::MulExpContext::unaryExp() {
  return getRuleContexts<SysYParser::UnaryExpContext>();
}

SysYParser::UnaryExpContext* SysYParser::MulExpContext::unaryExp(size_t i) {
  return getRuleContext<SysYParser::UnaryExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::MulExpContext::MUL() {
  return getTokens(SysYParser::MUL);
}

tree::TerminalNode* SysYParser::MulExpContext::MUL(size_t i) {
  return getToken(SysYParser::MUL, i);
}

std::vector<tree::TerminalNode *> SysYParser::MulExpContext::DIV() {
  return getTokens(SysYParser::DIV);
}

tree::TerminalNode* SysYParser::MulExpContext::DIV(size_t i) {
  return getToken(SysYParser::DIV, i);
}

std::vector<tree::TerminalNode *> SysYParser::MulExpContext::MOD() {
  return getTokens(SysYParser::MOD);
}

tree::TerminalNode* SysYParser::MulExpContext::MOD(size_t i) {
  return getToken(SysYParser::MOD, i);
}


size_t SysYParser::MulExpContext::getRuleIndex() const {
  return SysYParser::RuleMulExp;
}


std::any SysYParser::MulExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitMulExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::MulExpContext* SysYParser::mulExp() {
  MulExpContext *_localctx = _tracker.createInstance<MulExpContext>(_ctx, getState());
  enterRule(_localctx, 54, SysYParser::RuleMulExp);
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
    setState(308);
    unaryExp();
    setState(313);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 57344) != 0)) {
      setState(309);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 57344) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(310);
      unaryExp();
      setState(315);
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

//----------------- AddExpContext ------------------------------------------------------------------

SysYParser::AddExpContext::AddExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::MulExpContext *> SysYParser::AddExpContext::mulExp() {
  return getRuleContexts<SysYParser::MulExpContext>();
}

SysYParser::MulExpContext* SysYParser::AddExpContext::mulExp(size_t i) {
  return getRuleContext<SysYParser::MulExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::AddExpContext::ADD() {
  return getTokens(SysYParser::ADD);
}

tree::TerminalNode* SysYParser::AddExpContext::ADD(size_t i) {
  return getToken(SysYParser::ADD, i);
}

std::vector<tree::TerminalNode *> SysYParser::AddExpContext::SUB() {
  return getTokens(SysYParser::SUB);
}

tree::TerminalNode* SysYParser::AddExpContext::SUB(size_t i) {
  return getToken(SysYParser::SUB, i);
}


size_t SysYParser::AddExpContext::getRuleIndex() const {
  return SysYParser::RuleAddExp;
}


std::any SysYParser::AddExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitAddExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::AddExpContext* SysYParser::addExp() {
  AddExpContext *_localctx = _tracker.createInstance<AddExpContext>(_ctx, getState());
  enterRule(_localctx, 56, SysYParser::RuleAddExp);
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
    setState(316);
    mulExp();
    setState(321);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::ADD

    || _la == SysYParser::SUB) {
      setState(317);
      _la = _input->LA(1);
      if (!(_la == SysYParser::ADD

      || _la == SysYParser::SUB)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(318);
      mulExp();
      setState(323);
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

//----------------- RelExpContext ------------------------------------------------------------------

SysYParser::RelExpContext::RelExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::AddExpContext *> SysYParser::RelExpContext::addExp() {
  return getRuleContexts<SysYParser::AddExpContext>();
}

SysYParser::AddExpContext* SysYParser::RelExpContext::addExp(size_t i) {
  return getRuleContext<SysYParser::AddExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::RelExpContext::LT() {
  return getTokens(SysYParser::LT);
}

tree::TerminalNode* SysYParser::RelExpContext::LT(size_t i) {
  return getToken(SysYParser::LT, i);
}

std::vector<tree::TerminalNode *> SysYParser::RelExpContext::GT() {
  return getTokens(SysYParser::GT);
}

tree::TerminalNode* SysYParser::RelExpContext::GT(size_t i) {
  return getToken(SysYParser::GT, i);
}

std::vector<tree::TerminalNode *> SysYParser::RelExpContext::LE() {
  return getTokens(SysYParser::LE);
}

tree::TerminalNode* SysYParser::RelExpContext::LE(size_t i) {
  return getToken(SysYParser::LE, i);
}

std::vector<tree::TerminalNode *> SysYParser::RelExpContext::GE() {
  return getTokens(SysYParser::GE);
}

tree::TerminalNode* SysYParser::RelExpContext::GE(size_t i) {
  return getToken(SysYParser::GE, i);
}


size_t SysYParser::RelExpContext::getRuleIndex() const {
  return SysYParser::RuleRelExp;
}


std::any SysYParser::RelExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitRelExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::RelExpContext* SysYParser::relExp() {
  RelExpContext *_localctx = _tracker.createInstance<RelExpContext>(_ctx, getState());
  enterRule(_localctx, 58, SysYParser::RuleRelExp);
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
    setState(324);
    addExp();
    setState(329);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 3932160) != 0)) {
      setState(325);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 3932160) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(326);
      addExp();
      setState(331);
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

//----------------- EqExpContext ------------------------------------------------------------------

SysYParser::EqExpContext::EqExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::RelExpContext *> SysYParser::EqExpContext::relExp() {
  return getRuleContexts<SysYParser::RelExpContext>();
}

SysYParser::RelExpContext* SysYParser::EqExpContext::relExp(size_t i) {
  return getRuleContext<SysYParser::RelExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::EqExpContext::EQ() {
  return getTokens(SysYParser::EQ);
}

tree::TerminalNode* SysYParser::EqExpContext::EQ(size_t i) {
  return getToken(SysYParser::EQ, i);
}

std::vector<tree::TerminalNode *> SysYParser::EqExpContext::NE() {
  return getTokens(SysYParser::NE);
}

tree::TerminalNode* SysYParser::EqExpContext::NE(size_t i) {
  return getToken(SysYParser::NE, i);
}


size_t SysYParser::EqExpContext::getRuleIndex() const {
  return SysYParser::RuleEqExp;
}


std::any SysYParser::EqExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitEqExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::EqExpContext* SysYParser::eqExp() {
  EqExpContext *_localctx = _tracker.createInstance<EqExpContext>(_ctx, getState());
  enterRule(_localctx, 60, SysYParser::RuleEqExp);
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
    setState(332);
    relExp();
    setState(337);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::EQ

    || _la == SysYParser::NE) {
      setState(333);
      _la = _input->LA(1);
      if (!(_la == SysYParser::EQ

      || _la == SysYParser::NE)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(334);
      relExp();
      setState(339);
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

//----------------- LAndExpContext ------------------------------------------------------------------

SysYParser::LAndExpContext::LAndExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::EqExpContext *> SysYParser::LAndExpContext::eqExp() {
  return getRuleContexts<SysYParser::EqExpContext>();
}

SysYParser::EqExpContext* SysYParser::LAndExpContext::eqExp(size_t i) {
  return getRuleContext<SysYParser::EqExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::LAndExpContext::AND() {
  return getTokens(SysYParser::AND);
}

tree::TerminalNode* SysYParser::LAndExpContext::AND(size_t i) {
  return getToken(SysYParser::AND, i);
}


size_t SysYParser::LAndExpContext::getRuleIndex() const {
  return SysYParser::RuleLAndExp;
}


std::any SysYParser::LAndExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitLAndExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::LAndExpContext* SysYParser::lAndExp() {
  LAndExpContext *_localctx = _tracker.createInstance<LAndExpContext>(_ctx, getState());
  enterRule(_localctx, 62, SysYParser::RuleLAndExp);
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
    setState(340);
    eqExp();
    setState(345);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::AND) {
      setState(341);
      match(SysYParser::AND);
      setState(342);
      eqExp();
      setState(347);
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

//----------------- LOrExpContext ------------------------------------------------------------------

SysYParser::LOrExpContext::LOrExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParser::LAndExpContext *> SysYParser::LOrExpContext::lAndExp() {
  return getRuleContexts<SysYParser::LAndExpContext>();
}

SysYParser::LAndExpContext* SysYParser::LOrExpContext::lAndExp(size_t i) {
  return getRuleContext<SysYParser::LAndExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::LOrExpContext::OR() {
  return getTokens(SysYParser::OR);
}

tree::TerminalNode* SysYParser::LOrExpContext::OR(size_t i) {
  return getToken(SysYParser::OR, i);
}


size_t SysYParser::LOrExpContext::getRuleIndex() const {
  return SysYParser::RuleLOrExp;
}


std::any SysYParser::LOrExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitLOrExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::LOrExpContext* SysYParser::lOrExp() {
  LOrExpContext *_localctx = _tracker.createInstance<LOrExpContext>(_ctx, getState());
  enterRule(_localctx, 64, SysYParser::RuleLOrExp);
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
    setState(348);
    lAndExp();
    setState(353);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::OR) {
      setState(349);
      match(SysYParser::OR);
      setState(350);
      lAndExp();
      setState(355);
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

//----------------- ConstExpContext ------------------------------------------------------------------

SysYParser::ConstExpContext::ConstExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::AddExpContext* SysYParser::ConstExpContext::addExp() {
  return getRuleContext<SysYParser::AddExpContext>(0);
}


size_t SysYParser::ConstExpContext::getRuleIndex() const {
  return SysYParser::RuleConstExp;
}


std::any SysYParser::ConstExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitConstExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ConstExpContext* SysYParser::constExp() {
  ConstExpContext *_localctx = _tracker.createInstance<ConstExpContext>(_ctx, getState());
  enterRule(_localctx, 66, SysYParser::RuleConstExp);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(356);
    addExp();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void SysYParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  sysyParserInitialize();
#else
  ::antlr4::internal::call_once(sysyParserOnceFlag, sysyParserInitialize);
#endif
}
