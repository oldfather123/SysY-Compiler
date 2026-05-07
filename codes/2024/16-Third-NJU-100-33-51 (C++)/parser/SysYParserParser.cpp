
// Generated from ./parser/SysYParser.g4 by ANTLR 4.12.0


#include "SysYParserListener.h"
#include "SysYParserVisitor.h"

#include "SysYParserParser.h"


using namespace antlrcpp;

using namespace antlr4;

namespace {

struct SysYParserParserStaticData final {
  SysYParserParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  SysYParserParserStaticData(const SysYParserParserStaticData&) = delete;
  SysYParserParserStaticData(SysYParserParserStaticData&&) = delete;
  SysYParserParserStaticData& operator=(const SysYParserParserStaticData&) = delete;
  SysYParserParserStaticData& operator=(SysYParserParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag sysyparserParserOnceFlag;
SysYParserParserStaticData *sysyparserParserStaticData = nullptr;

void sysyparserParserInitialize() {
  assert(sysyparserParserStaticData == nullptr);
  auto staticData = std::make_unique<SysYParserParserStaticData>(
    std::vector<std::string>{
      "program", "compUnit", "decl", "constDecl", "bType", "constDef", "constInitVal", 
      "varDecl", "varDef", "initLVal", "initVal", "funcDef", "funcType", 
      "funcFParams", "funcFParam", "block", "blockItem", "stmt", "exp", 
      "cond", "lVal", "primaryExp", "number", "unaryExp", "unaryOp", "funcRParams", 
      "funcRParam", "mulExp", "addExp", "relExp", "eqExp", "lAndExp", "lOrExp", 
      "constExp", "intConst", "floatConst"
    },
    std::vector<std::string>{
      "", "'const'", "','", "';'", "'int'", "'float'", "'='", "'{'", "'}'", 
      "'['", "']'", "'('", "')'", "'void'", "'if'", "'else'", "'while'", 
      "'break'", "'continue'", "'return'", "'+'", "'-'", "'!'", "'*'", "'/'", 
      "'%'", "'<'", "'>'", "'<='", "'>='", "'=='", "'!='", "'&&'", "'||'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "Identifier", "DecimalConstant", "OctalConstant", "HexadecimalConstant", 
      "FloatingConstant", "StringLiteral", "Whitespace", "Newline", "BlockComment", 
      "LineComment"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,43,369,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,2,34,7,34,2,35,7,
  	35,1,0,1,0,1,1,1,1,5,1,77,8,1,10,1,12,1,80,9,1,1,1,1,1,1,2,1,2,3,2,86,
  	8,2,1,3,1,3,1,3,1,3,1,3,5,3,93,8,3,10,3,12,3,96,9,3,1,3,1,3,1,4,1,4,1,
  	5,1,5,1,5,1,5,1,5,1,5,1,5,3,5,109,8,5,1,6,1,6,1,6,1,6,1,6,5,6,116,8,6,
  	10,6,12,6,119,9,6,3,6,121,8,6,1,6,3,6,124,8,6,1,7,1,7,1,7,1,7,5,7,130,
  	8,7,10,7,12,7,133,9,7,1,7,1,7,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,1,8,3,8,
  	146,8,8,1,9,1,9,1,9,1,9,1,9,4,9,153,8,9,11,9,12,9,154,1,10,1,10,1,10,
  	1,10,1,10,5,10,162,8,10,10,10,12,10,165,9,10,3,10,167,8,10,1,10,3,10,
  	170,8,10,1,11,1,11,1,11,1,11,3,11,176,8,11,1,11,1,11,1,11,1,12,1,12,1,
  	13,1,13,1,13,5,13,186,8,13,10,13,12,13,189,9,13,1,14,1,14,1,14,1,14,1,
  	14,1,14,1,14,1,14,1,14,1,14,1,14,5,14,202,8,14,10,14,12,14,205,9,14,3,
  	14,207,8,14,1,15,1,15,5,15,211,8,15,10,15,12,15,214,9,15,1,15,1,15,1,
  	16,1,16,3,16,220,8,16,1,17,1,17,1,17,1,17,1,17,1,17,3,17,228,8,17,1,17,
  	1,17,1,17,1,17,1,17,1,17,1,17,1,17,1,17,3,17,239,8,17,1,17,1,17,1,17,
  	1,17,1,17,1,17,1,17,1,17,1,17,1,17,1,17,1,17,3,17,253,8,17,1,17,3,17,
  	256,8,17,1,18,1,18,1,19,1,19,1,20,1,20,1,20,1,20,1,20,1,20,4,20,268,8,
  	20,11,20,12,20,269,3,20,272,8,20,1,21,1,21,1,21,1,21,1,21,1,21,3,21,280,
  	8,21,1,22,1,22,3,22,284,8,22,1,23,1,23,1,23,1,23,3,23,290,8,23,1,23,1,
  	23,1,23,1,23,3,23,296,8,23,1,24,1,24,1,25,1,25,1,25,5,25,303,8,25,10,
  	25,12,25,306,9,25,1,26,1,26,3,26,310,8,26,1,27,1,27,1,27,5,27,315,8,27,
  	10,27,12,27,318,9,27,1,28,1,28,1,28,5,28,323,8,28,10,28,12,28,326,9,28,
  	1,29,1,29,1,29,5,29,331,8,29,10,29,12,29,334,9,29,1,30,1,30,1,30,5,30,
  	339,8,30,10,30,12,30,342,9,30,1,31,1,31,1,31,5,31,347,8,31,10,31,12,31,
  	350,9,31,1,32,1,32,1,32,5,32,355,8,32,10,32,12,32,358,9,32,1,33,1,33,
  	1,34,1,34,1,34,3,34,365,8,34,1,35,1,35,1,35,0,0,36,0,2,4,6,8,10,12,14,
  	16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,
  	62,64,66,68,70,0,7,1,0,4,5,2,0,4,5,13,13,1,0,20,22,1,0,23,25,1,0,20,21,
  	1,0,26,29,1,0,30,31,382,0,72,1,0,0,0,2,78,1,0,0,0,4,85,1,0,0,0,6,87,1,
  	0,0,0,8,99,1,0,0,0,10,108,1,0,0,0,12,123,1,0,0,0,14,125,1,0,0,0,16,145,
  	1,0,0,0,18,147,1,0,0,0,20,169,1,0,0,0,22,171,1,0,0,0,24,180,1,0,0,0,26,
  	182,1,0,0,0,28,206,1,0,0,0,30,208,1,0,0,0,32,219,1,0,0,0,34,255,1,0,0,
  	0,36,257,1,0,0,0,38,259,1,0,0,0,40,271,1,0,0,0,42,279,1,0,0,0,44,283,
  	1,0,0,0,46,295,1,0,0,0,48,297,1,0,0,0,50,299,1,0,0,0,52,309,1,0,0,0,54,
  	311,1,0,0,0,56,319,1,0,0,0,58,327,1,0,0,0,60,335,1,0,0,0,62,343,1,0,0,
  	0,64,351,1,0,0,0,66,359,1,0,0,0,68,364,1,0,0,0,70,366,1,0,0,0,72,73,3,
  	2,1,0,73,1,1,0,0,0,74,77,3,4,2,0,75,77,3,22,11,0,76,74,1,0,0,0,76,75,
  	1,0,0,0,77,80,1,0,0,0,78,76,1,0,0,0,78,79,1,0,0,0,79,81,1,0,0,0,80,78,
  	1,0,0,0,81,82,5,0,0,1,82,3,1,0,0,0,83,86,3,6,3,0,84,86,3,14,7,0,85,83,
  	1,0,0,0,85,84,1,0,0,0,86,5,1,0,0,0,87,88,5,1,0,0,88,89,3,8,4,0,89,94,
  	3,10,5,0,90,91,5,2,0,0,91,93,3,10,5,0,92,90,1,0,0,0,93,96,1,0,0,0,94,
  	92,1,0,0,0,94,95,1,0,0,0,95,97,1,0,0,0,96,94,1,0,0,0,97,98,5,3,0,0,98,
  	7,1,0,0,0,99,100,7,0,0,0,100,9,1,0,0,0,101,102,5,34,0,0,102,103,5,6,0,
  	0,103,109,3,12,6,0,104,105,3,18,9,0,105,106,5,6,0,0,106,107,3,12,6,0,
  	107,109,1,0,0,0,108,101,1,0,0,0,108,104,1,0,0,0,109,11,1,0,0,0,110,124,
  	3,66,33,0,111,120,5,7,0,0,112,117,3,12,6,0,113,114,5,2,0,0,114,116,3,
  	12,6,0,115,113,1,0,0,0,116,119,1,0,0,0,117,115,1,0,0,0,117,118,1,0,0,
  	0,118,121,1,0,0,0,119,117,1,0,0,0,120,112,1,0,0,0,120,121,1,0,0,0,121,
  	122,1,0,0,0,122,124,5,8,0,0,123,110,1,0,0,0,123,111,1,0,0,0,124,13,1,
  	0,0,0,125,126,3,8,4,0,126,131,3,16,8,0,127,128,5,2,0,0,128,130,3,16,8,
  	0,129,127,1,0,0,0,130,133,1,0,0,0,131,129,1,0,0,0,131,132,1,0,0,0,132,
  	134,1,0,0,0,133,131,1,0,0,0,134,135,5,3,0,0,135,15,1,0,0,0,136,146,5,
  	34,0,0,137,146,3,18,9,0,138,139,5,34,0,0,139,140,5,6,0,0,140,146,3,20,
  	10,0,141,142,3,18,9,0,142,143,5,6,0,0,143,144,3,20,10,0,144,146,1,0,0,
  	0,145,136,1,0,0,0,145,137,1,0,0,0,145,138,1,0,0,0,145,141,1,0,0,0,146,
  	17,1,0,0,0,147,152,5,34,0,0,148,149,5,9,0,0,149,150,3,66,33,0,150,151,
  	5,10,0,0,151,153,1,0,0,0,152,148,1,0,0,0,153,154,1,0,0,0,154,152,1,0,
  	0,0,154,155,1,0,0,0,155,19,1,0,0,0,156,170,3,36,18,0,157,166,5,7,0,0,
  	158,163,3,20,10,0,159,160,5,2,0,0,160,162,3,20,10,0,161,159,1,0,0,0,162,
  	165,1,0,0,0,163,161,1,0,0,0,163,164,1,0,0,0,164,167,1,0,0,0,165,163,1,
  	0,0,0,166,158,1,0,0,0,166,167,1,0,0,0,167,168,1,0,0,0,168,170,5,8,0,0,
  	169,156,1,0,0,0,169,157,1,0,0,0,170,21,1,0,0,0,171,172,3,24,12,0,172,
  	173,5,34,0,0,173,175,5,11,0,0,174,176,3,26,13,0,175,174,1,0,0,0,175,176,
  	1,0,0,0,176,177,1,0,0,0,177,178,5,12,0,0,178,179,3,30,15,0,179,23,1,0,
  	0,0,180,181,7,1,0,0,181,25,1,0,0,0,182,187,3,28,14,0,183,184,5,2,0,0,
  	184,186,3,28,14,0,185,183,1,0,0,0,186,189,1,0,0,0,187,185,1,0,0,0,187,
  	188,1,0,0,0,188,27,1,0,0,0,189,187,1,0,0,0,190,191,3,8,4,0,191,192,5,
  	34,0,0,192,207,1,0,0,0,193,194,3,8,4,0,194,195,5,34,0,0,195,196,5,9,0,
  	0,196,203,5,10,0,0,197,198,5,9,0,0,198,199,3,36,18,0,199,200,5,10,0,0,
  	200,202,1,0,0,0,201,197,1,0,0,0,202,205,1,0,0,0,203,201,1,0,0,0,203,204,
  	1,0,0,0,204,207,1,0,0,0,205,203,1,0,0,0,206,190,1,0,0,0,206,193,1,0,0,
  	0,207,29,1,0,0,0,208,212,5,7,0,0,209,211,3,32,16,0,210,209,1,0,0,0,211,
  	214,1,0,0,0,212,210,1,0,0,0,212,213,1,0,0,0,213,215,1,0,0,0,214,212,1,
  	0,0,0,215,216,5,8,0,0,216,31,1,0,0,0,217,220,3,4,2,0,218,220,3,34,17,
  	0,219,217,1,0,0,0,219,218,1,0,0,0,220,33,1,0,0,0,221,222,3,40,20,0,222,
  	223,5,6,0,0,223,224,3,36,18,0,224,225,5,3,0,0,225,256,1,0,0,0,226,228,
  	3,36,18,0,227,226,1,0,0,0,227,228,1,0,0,0,228,229,1,0,0,0,229,256,5,3,
  	0,0,230,256,3,30,15,0,231,232,5,14,0,0,232,233,5,11,0,0,233,234,3,38,
  	19,0,234,235,5,12,0,0,235,238,3,34,17,0,236,237,5,15,0,0,237,239,3,34,
  	17,0,238,236,1,0,0,0,238,239,1,0,0,0,239,256,1,0,0,0,240,241,5,16,0,0,
  	241,242,5,11,0,0,242,243,3,38,19,0,243,244,5,12,0,0,244,245,3,34,17,0,
  	245,256,1,0,0,0,246,247,5,17,0,0,247,256,5,3,0,0,248,249,5,18,0,0,249,
  	256,5,3,0,0,250,252,5,19,0,0,251,253,3,36,18,0,252,251,1,0,0,0,252,253,
  	1,0,0,0,253,254,1,0,0,0,254,256,5,3,0,0,255,221,1,0,0,0,255,227,1,0,0,
  	0,255,230,1,0,0,0,255,231,1,0,0,0,255,240,1,0,0,0,255,246,1,0,0,0,255,
  	248,1,0,0,0,255,250,1,0,0,0,256,35,1,0,0,0,257,258,3,56,28,0,258,37,1,
  	0,0,0,259,260,3,64,32,0,260,39,1,0,0,0,261,272,5,34,0,0,262,267,5,34,
  	0,0,263,264,5,9,0,0,264,265,3,36,18,0,265,266,5,10,0,0,266,268,1,0,0,
  	0,267,263,1,0,0,0,268,269,1,0,0,0,269,267,1,0,0,0,269,270,1,0,0,0,270,
  	272,1,0,0,0,271,261,1,0,0,0,271,262,1,0,0,0,272,41,1,0,0,0,273,274,5,
  	11,0,0,274,275,3,36,18,0,275,276,5,12,0,0,276,280,1,0,0,0,277,280,3,40,
  	20,0,278,280,3,44,22,0,279,273,1,0,0,0,279,277,1,0,0,0,279,278,1,0,0,
  	0,280,43,1,0,0,0,281,284,3,68,34,0,282,284,3,70,35,0,283,281,1,0,0,0,
  	283,282,1,0,0,0,284,45,1,0,0,0,285,296,3,42,21,0,286,287,5,34,0,0,287,
  	289,5,11,0,0,288,290,3,50,25,0,289,288,1,0,0,0,289,290,1,0,0,0,290,291,
  	1,0,0,0,291,296,5,12,0,0,292,293,3,48,24,0,293,294,3,46,23,0,294,296,
  	1,0,0,0,295,285,1,0,0,0,295,286,1,0,0,0,295,292,1,0,0,0,296,47,1,0,0,
  	0,297,298,7,2,0,0,298,49,1,0,0,0,299,304,3,52,26,0,300,301,5,2,0,0,301,
  	303,3,52,26,0,302,300,1,0,0,0,303,306,1,0,0,0,304,302,1,0,0,0,304,305,
  	1,0,0,0,305,51,1,0,0,0,306,304,1,0,0,0,307,310,3,36,18,0,308,310,5,39,
  	0,0,309,307,1,0,0,0,309,308,1,0,0,0,310,53,1,0,0,0,311,316,3,46,23,0,
  	312,313,7,3,0,0,313,315,3,46,23,0,314,312,1,0,0,0,315,318,1,0,0,0,316,
  	314,1,0,0,0,316,317,1,0,0,0,317,55,1,0,0,0,318,316,1,0,0,0,319,324,3,
  	54,27,0,320,321,7,4,0,0,321,323,3,54,27,0,322,320,1,0,0,0,323,326,1,0,
  	0,0,324,322,1,0,0,0,324,325,1,0,0,0,325,57,1,0,0,0,326,324,1,0,0,0,327,
  	332,3,56,28,0,328,329,7,5,0,0,329,331,3,56,28,0,330,328,1,0,0,0,331,334,
  	1,0,0,0,332,330,1,0,0,0,332,333,1,0,0,0,333,59,1,0,0,0,334,332,1,0,0,
  	0,335,340,3,58,29,0,336,337,7,6,0,0,337,339,3,58,29,0,338,336,1,0,0,0,
  	339,342,1,0,0,0,340,338,1,0,0,0,340,341,1,0,0,0,341,61,1,0,0,0,342,340,
  	1,0,0,0,343,348,3,60,30,0,344,345,5,32,0,0,345,347,3,60,30,0,346,344,
  	1,0,0,0,347,350,1,0,0,0,348,346,1,0,0,0,348,349,1,0,0,0,349,63,1,0,0,
  	0,350,348,1,0,0,0,351,356,3,62,31,0,352,353,5,33,0,0,353,355,3,62,31,
  	0,354,352,1,0,0,0,355,358,1,0,0,0,356,354,1,0,0,0,356,357,1,0,0,0,357,
  	65,1,0,0,0,358,356,1,0,0,0,359,360,3,56,28,0,360,67,1,0,0,0,361,365,5,
  	35,0,0,362,365,5,36,0,0,363,365,5,37,0,0,364,361,1,0,0,0,364,362,1,0,
  	0,0,364,363,1,0,0,0,365,69,1,0,0,0,366,367,5,38,0,0,367,71,1,0,0,0,39,
  	76,78,85,94,108,117,120,123,131,145,154,163,166,169,175,187,203,206,212,
  	219,227,238,252,255,269,271,279,283,289,295,304,309,316,324,332,340,348,
  	356,364
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  sysyparserParserStaticData = staticData.release();
}

}

SysYParserParser::SysYParserParser(TokenStream *input) : SysYParserParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

SysYParserParser::SysYParserParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  SysYParserParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *sysyparserParserStaticData->atn, sysyparserParserStaticData->decisionToDFA, sysyparserParserStaticData->sharedContextCache, options);
}

SysYParserParser::~SysYParserParser() {
  delete _interpreter;
}

const atn::ATN& SysYParserParser::getATN() const {
  return *sysyparserParserStaticData->atn;
}

std::string SysYParserParser::getGrammarFileName() const {
  return "SysYParser.g4";
}

const std::vector<std::string>& SysYParserParser::getRuleNames() const {
  return sysyparserParserStaticData->ruleNames;
}

const dfa::Vocabulary& SysYParserParser::getVocabulary() const {
  return sysyparserParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SysYParserParser::getSerializedATN() const {
  return sysyparserParserStaticData->serializedATN;
}


//----------------- ProgramContext ------------------------------------------------------------------

SysYParserParser::ProgramContext::ProgramContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::CompUnitContext* SysYParserParser::ProgramContext::compUnit() {
  return getRuleContext<SysYParserParser::CompUnitContext>(0);
}


size_t SysYParserParser::ProgramContext::getRuleIndex() const {
  return SysYParserParser::RuleProgram;
}

void SysYParserParser::ProgramContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterProgram(this);
}

void SysYParserParser::ProgramContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitProgram(this);
}


std::any SysYParserParser::ProgramContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitProgram(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::ProgramContext* SysYParserParser::program() {
  ProgramContext *_localctx = _tracker.createInstance<ProgramContext>(_ctx, getState());
  enterRule(_localctx, 0, SysYParserParser::RuleProgram);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(72);
    compUnit();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CompUnitContext ------------------------------------------------------------------

SysYParserParser::CompUnitContext::CompUnitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParserParser::CompUnitContext::EOF() {
  return getToken(SysYParserParser::EOF, 0);
}

std::vector<SysYParserParser::DeclContext *> SysYParserParser::CompUnitContext::decl() {
  return getRuleContexts<SysYParserParser::DeclContext>();
}

SysYParserParser::DeclContext* SysYParserParser::CompUnitContext::decl(size_t i) {
  return getRuleContext<SysYParserParser::DeclContext>(i);
}

std::vector<SysYParserParser::FuncDefContext *> SysYParserParser::CompUnitContext::funcDef() {
  return getRuleContexts<SysYParserParser::FuncDefContext>();
}

SysYParserParser::FuncDefContext* SysYParserParser::CompUnitContext::funcDef(size_t i) {
  return getRuleContext<SysYParserParser::FuncDefContext>(i);
}


size_t SysYParserParser::CompUnitContext::getRuleIndex() const {
  return SysYParserParser::RuleCompUnit;
}

void SysYParserParser::CompUnitContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCompUnit(this);
}

void SysYParserParser::CompUnitContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCompUnit(this);
}


std::any SysYParserParser::CompUnitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitCompUnit(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::CompUnitContext* SysYParserParser::compUnit() {
  CompUnitContext *_localctx = _tracker.createInstance<CompUnitContext>(_ctx, getState());
  enterRule(_localctx, 2, SysYParserParser::RuleCompUnit);
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
    setState(78);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 8242) != 0)) {
      setState(76);
      _errHandler->sync(this);
      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
      case 1: {
        setState(74);
        decl();
        break;
      }

      case 2: {
        setState(75);
        funcDef();
        break;
      }

      default:
        break;
      }
      setState(80);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(81);
    match(SysYParserParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- DeclContext ------------------------------------------------------------------

SysYParserParser::DeclContext::DeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::ConstDeclContext* SysYParserParser::DeclContext::constDecl() {
  return getRuleContext<SysYParserParser::ConstDeclContext>(0);
}

SysYParserParser::VarDeclContext* SysYParserParser::DeclContext::varDecl() {
  return getRuleContext<SysYParserParser::VarDeclContext>(0);
}


size_t SysYParserParser::DeclContext::getRuleIndex() const {
  return SysYParserParser::RuleDecl;
}

void SysYParserParser::DeclContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDecl(this);
}

void SysYParserParser::DeclContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDecl(this);
}


std::any SysYParserParser::DeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitDecl(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::DeclContext* SysYParserParser::decl() {
  DeclContext *_localctx = _tracker.createInstance<DeclContext>(_ctx, getState());
  enterRule(_localctx, 4, SysYParserParser::RuleDecl);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(85);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParserParser::T__0: {
        enterOuterAlt(_localctx, 1);
        setState(83);
        constDecl();
        break;
      }

      case SysYParserParser::T__3:
      case SysYParserParser::T__4: {
        enterOuterAlt(_localctx, 2);
        setState(84);
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

SysYParserParser::ConstDeclContext::ConstDeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::BTypeContext* SysYParserParser::ConstDeclContext::bType() {
  return getRuleContext<SysYParserParser::BTypeContext>(0);
}

std::vector<SysYParserParser::ConstDefContext *> SysYParserParser::ConstDeclContext::constDef() {
  return getRuleContexts<SysYParserParser::ConstDefContext>();
}

SysYParserParser::ConstDefContext* SysYParserParser::ConstDeclContext::constDef(size_t i) {
  return getRuleContext<SysYParserParser::ConstDefContext>(i);
}


size_t SysYParserParser::ConstDeclContext::getRuleIndex() const {
  return SysYParserParser::RuleConstDecl;
}

void SysYParserParser::ConstDeclContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstDecl(this);
}

void SysYParserParser::ConstDeclContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstDecl(this);
}


std::any SysYParserParser::ConstDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitConstDecl(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::ConstDeclContext* SysYParserParser::constDecl() {
  ConstDeclContext *_localctx = _tracker.createInstance<ConstDeclContext>(_ctx, getState());
  enterRule(_localctx, 6, SysYParserParser::RuleConstDecl);
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
    setState(87);
    match(SysYParserParser::T__0);
    setState(88);
    bType();
    setState(89);
    constDef();
    setState(94);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParserParser::T__1) {
      setState(90);
      match(SysYParserParser::T__1);
      setState(91);
      constDef();
      setState(96);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(97);
    match(SysYParserParser::T__2);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BTypeContext ------------------------------------------------------------------

SysYParserParser::BTypeContext::BTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::BTypeContext::getRuleIndex() const {
  return SysYParserParser::RuleBType;
}

void SysYParserParser::BTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBType(this);
}

void SysYParserParser::BTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBType(this);
}


std::any SysYParserParser::BTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitBType(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::BTypeContext* SysYParserParser::bType() {
  BTypeContext *_localctx = _tracker.createInstance<BTypeContext>(_ctx, getState());
  enterRule(_localctx, 8, SysYParserParser::RuleBType);
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
    setState(99);
    _la = _input->LA(1);
    if (!(_la == SysYParserParser::T__3

    || _la == SysYParserParser::T__4)) {
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

SysYParserParser::ConstDefContext::ConstDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::ConstDefContext::getRuleIndex() const {
  return SysYParserParser::RuleConstDef;
}

void SysYParserParser::ConstDefContext::copyFrom(ConstDefContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ConstDefSingleContext ------------------------------------------------------------------

tree::TerminalNode* SysYParserParser::ConstDefSingleContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

SysYParserParser::ConstInitValContext* SysYParserParser::ConstDefSingleContext::constInitVal() {
  return getRuleContext<SysYParserParser::ConstInitValContext>(0);
}

SysYParserParser::ConstDefSingleContext::ConstDefSingleContext(ConstDefContext *ctx) { copyFrom(ctx); }

void SysYParserParser::ConstDefSingleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstDefSingle(this);
}
void SysYParserParser::ConstDefSingleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstDefSingle(this);
}

std::any SysYParserParser::ConstDefSingleContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitConstDefSingle(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ConstDefArrayContext ------------------------------------------------------------------

SysYParserParser::InitLValContext* SysYParserParser::ConstDefArrayContext::initLVal() {
  return getRuleContext<SysYParserParser::InitLValContext>(0);
}

SysYParserParser::ConstInitValContext* SysYParserParser::ConstDefArrayContext::constInitVal() {
  return getRuleContext<SysYParserParser::ConstInitValContext>(0);
}

SysYParserParser::ConstDefArrayContext::ConstDefArrayContext(ConstDefContext *ctx) { copyFrom(ctx); }

void SysYParserParser::ConstDefArrayContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstDefArray(this);
}
void SysYParserParser::ConstDefArrayContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstDefArray(this);
}

std::any SysYParserParser::ConstDefArrayContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitConstDefArray(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::ConstDefContext* SysYParserParser::constDef() {
  ConstDefContext *_localctx = _tracker.createInstance<ConstDefContext>(_ctx, getState());
  enterRule(_localctx, 10, SysYParserParser::RuleConstDef);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(108);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 4, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SysYParserParser::ConstDefSingleContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(101);
      match(SysYParserParser::Identifier);
      setState(102);
      match(SysYParserParser::T__5);
      setState(103);
      constInitVal();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysYParserParser::ConstDefArrayContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(104);
      initLVal();
      setState(105);
      match(SysYParserParser::T__5);
      setState(106);
      constInitVal();
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

//----------------- ConstInitValContext ------------------------------------------------------------------

SysYParserParser::ConstInitValContext::ConstInitValContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::ConstInitValContext::getRuleIndex() const {
  return SysYParserParser::RuleConstInitVal;
}

void SysYParserParser::ConstInitValContext::copyFrom(ConstInitValContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- ConstInitValSingleContext ------------------------------------------------------------------

SysYParserParser::ConstExpContext* SysYParserParser::ConstInitValSingleContext::constExp() {
  return getRuleContext<SysYParserParser::ConstExpContext>(0);
}

SysYParserParser::ConstInitValSingleContext::ConstInitValSingleContext(ConstInitValContext *ctx) { copyFrom(ctx); }

void SysYParserParser::ConstInitValSingleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstInitValSingle(this);
}
void SysYParserParser::ConstInitValSingleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstInitValSingle(this);
}

std::any SysYParserParser::ConstInitValSingleContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitConstInitValSingle(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ConstInitValArrayContext ------------------------------------------------------------------

std::vector<SysYParserParser::ConstInitValContext *> SysYParserParser::ConstInitValArrayContext::constInitVal() {
  return getRuleContexts<SysYParserParser::ConstInitValContext>();
}

SysYParserParser::ConstInitValContext* SysYParserParser::ConstInitValArrayContext::constInitVal(size_t i) {
  return getRuleContext<SysYParserParser::ConstInitValContext>(i);
}

SysYParserParser::ConstInitValArrayContext::ConstInitValArrayContext(ConstInitValContext *ctx) { copyFrom(ctx); }

void SysYParserParser::ConstInitValArrayContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstInitValArray(this);
}
void SysYParserParser::ConstInitValArrayContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstInitValArray(this);
}

std::any SysYParserParser::ConstInitValArrayContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitConstInitValArray(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::ConstInitValContext* SysYParserParser::constInitVal() {
  ConstInitValContext *_localctx = _tracker.createInstance<ConstInitValContext>(_ctx, getState());
  enterRule(_localctx, 12, SysYParserParser::RuleConstInitVal);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(123);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParserParser::T__10:
      case SysYParserParser::T__19:
      case SysYParserParser::T__20:
      case SysYParserParser::T__21:
      case SysYParserParser::Identifier:
      case SysYParserParser::DecimalConstant:
      case SysYParserParser::OctalConstant:
      case SysYParserParser::HexadecimalConstant:
      case SysYParserParser::FloatingConstant: {
        _localctx = _tracker.createInstance<SysYParserParser::ConstInitValSingleContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(110);
        constExp();
        break;
      }

      case SysYParserParser::T__6: {
        _localctx = _tracker.createInstance<SysYParserParser::ConstInitValArrayContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(111);
        match(SysYParserParser::T__6);
        setState(120);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 532583286912) != 0)) {
          setState(112);
          constInitVal();
          setState(117);
          _errHandler->sync(this);
          _la = _input->LA(1);
          while (_la == SysYParserParser::T__1) {
            setState(113);
            match(SysYParserParser::T__1);
            setState(114);
            constInitVal();
            setState(119);
            _errHandler->sync(this);
            _la = _input->LA(1);
          }
        }
        setState(122);
        match(SysYParserParser::T__7);
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

SysYParserParser::VarDeclContext::VarDeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::BTypeContext* SysYParserParser::VarDeclContext::bType() {
  return getRuleContext<SysYParserParser::BTypeContext>(0);
}

std::vector<SysYParserParser::VarDefContext *> SysYParserParser::VarDeclContext::varDef() {
  return getRuleContexts<SysYParserParser::VarDefContext>();
}

SysYParserParser::VarDefContext* SysYParserParser::VarDeclContext::varDef(size_t i) {
  return getRuleContext<SysYParserParser::VarDefContext>(i);
}


size_t SysYParserParser::VarDeclContext::getRuleIndex() const {
  return SysYParserParser::RuleVarDecl;
}

void SysYParserParser::VarDeclContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVarDecl(this);
}

void SysYParserParser::VarDeclContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVarDecl(this);
}


std::any SysYParserParser::VarDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitVarDecl(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::VarDeclContext* SysYParserParser::varDecl() {
  VarDeclContext *_localctx = _tracker.createInstance<VarDeclContext>(_ctx, getState());
  enterRule(_localctx, 14, SysYParserParser::RuleVarDecl);
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
    setState(125);
    bType();
    setState(126);
    varDef();
    setState(131);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParserParser::T__1) {
      setState(127);
      match(SysYParserParser::T__1);
      setState(128);
      varDef();
      setState(133);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(134);
    match(SysYParserParser::T__2);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VarDefContext ------------------------------------------------------------------

SysYParserParser::VarDefContext::VarDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::VarDefContext::getRuleIndex() const {
  return SysYParserParser::RuleVarDef;
}

void SysYParserParser::VarDefContext::copyFrom(VarDefContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- VarDefSingleInitValContext ------------------------------------------------------------------

tree::TerminalNode* SysYParserParser::VarDefSingleInitValContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

SysYParserParser::InitValContext* SysYParserParser::VarDefSingleInitValContext::initVal() {
  return getRuleContext<SysYParserParser::InitValContext>(0);
}

SysYParserParser::VarDefSingleInitValContext::VarDefSingleInitValContext(VarDefContext *ctx) { copyFrom(ctx); }

void SysYParserParser::VarDefSingleInitValContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVarDefSingleInitVal(this);
}
void SysYParserParser::VarDefSingleInitValContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVarDefSingleInitVal(this);
}

std::any SysYParserParser::VarDefSingleInitValContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitVarDefSingleInitVal(this);
  else
    return visitor->visitChildren(this);
}
//----------------- VarDefArrayInitValContext ------------------------------------------------------------------

SysYParserParser::InitLValContext* SysYParserParser::VarDefArrayInitValContext::initLVal() {
  return getRuleContext<SysYParserParser::InitLValContext>(0);
}

SysYParserParser::InitValContext* SysYParserParser::VarDefArrayInitValContext::initVal() {
  return getRuleContext<SysYParserParser::InitValContext>(0);
}

SysYParserParser::VarDefArrayInitValContext::VarDefArrayInitValContext(VarDefContext *ctx) { copyFrom(ctx); }

void SysYParserParser::VarDefArrayInitValContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVarDefArrayInitVal(this);
}
void SysYParserParser::VarDefArrayInitValContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVarDefArrayInitVal(this);
}

std::any SysYParserParser::VarDefArrayInitValContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitVarDefArrayInitVal(this);
  else
    return visitor->visitChildren(this);
}
//----------------- VarDefSingleContext ------------------------------------------------------------------

tree::TerminalNode* SysYParserParser::VarDefSingleContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

SysYParserParser::VarDefSingleContext::VarDefSingleContext(VarDefContext *ctx) { copyFrom(ctx); }

void SysYParserParser::VarDefSingleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVarDefSingle(this);
}
void SysYParserParser::VarDefSingleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVarDefSingle(this);
}

std::any SysYParserParser::VarDefSingleContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitVarDefSingle(this);
  else
    return visitor->visitChildren(this);
}
//----------------- VarDefArrayContext ------------------------------------------------------------------

SysYParserParser::InitLValContext* SysYParserParser::VarDefArrayContext::initLVal() {
  return getRuleContext<SysYParserParser::InitLValContext>(0);
}

SysYParserParser::VarDefArrayContext::VarDefArrayContext(VarDefContext *ctx) { copyFrom(ctx); }

void SysYParserParser::VarDefArrayContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVarDefArray(this);
}
void SysYParserParser::VarDefArrayContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVarDefArray(this);
}

std::any SysYParserParser::VarDefArrayContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitVarDefArray(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::VarDefContext* SysYParserParser::varDef() {
  VarDefContext *_localctx = _tracker.createInstance<VarDefContext>(_ctx, getState());
  enterRule(_localctx, 16, SysYParserParser::RuleVarDef);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(145);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 9, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SysYParserParser::VarDefSingleContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(136);
      match(SysYParserParser::Identifier);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysYParserParser::VarDefArrayContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(137);
      initLVal();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SysYParserParser::VarDefSingleInitValContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(138);
      match(SysYParserParser::Identifier);
      setState(139);
      match(SysYParserParser::T__5);
      setState(140);
      initVal();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<SysYParserParser::VarDefArrayInitValContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(141);
      initLVal();
      setState(142);
      match(SysYParserParser::T__5);
      setState(143);
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

//----------------- InitLValContext ------------------------------------------------------------------

SysYParserParser::InitLValContext::InitLValContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParserParser::InitLValContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

std::vector<SysYParserParser::ConstExpContext *> SysYParserParser::InitLValContext::constExp() {
  return getRuleContexts<SysYParserParser::ConstExpContext>();
}

SysYParserParser::ConstExpContext* SysYParserParser::InitLValContext::constExp(size_t i) {
  return getRuleContext<SysYParserParser::ConstExpContext>(i);
}


size_t SysYParserParser::InitLValContext::getRuleIndex() const {
  return SysYParserParser::RuleInitLVal;
}

void SysYParserParser::InitLValContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterInitLVal(this);
}

void SysYParserParser::InitLValContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitInitLVal(this);
}


std::any SysYParserParser::InitLValContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitInitLVal(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::InitLValContext* SysYParserParser::initLVal() {
  InitLValContext *_localctx = _tracker.createInstance<InitLValContext>(_ctx, getState());
  enterRule(_localctx, 18, SysYParserParser::RuleInitLVal);
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
    setState(147);
    match(SysYParserParser::Identifier);
    setState(152); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(148);
      match(SysYParserParser::T__8);
      setState(149);
      constExp();
      setState(150);
      match(SysYParserParser::T__9);
      setState(154); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while (_la == SysYParserParser::T__8);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- InitValContext ------------------------------------------------------------------

SysYParserParser::InitValContext::InitValContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::InitValContext::getRuleIndex() const {
  return SysYParserParser::RuleInitVal;
}

void SysYParserParser::InitValContext::copyFrom(InitValContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- InitValArrayContext ------------------------------------------------------------------

std::vector<SysYParserParser::InitValContext *> SysYParserParser::InitValArrayContext::initVal() {
  return getRuleContexts<SysYParserParser::InitValContext>();
}

SysYParserParser::InitValContext* SysYParserParser::InitValArrayContext::initVal(size_t i) {
  return getRuleContext<SysYParserParser::InitValContext>(i);
}

SysYParserParser::InitValArrayContext::InitValArrayContext(InitValContext *ctx) { copyFrom(ctx); }

void SysYParserParser::InitValArrayContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterInitValArray(this);
}
void SysYParserParser::InitValArrayContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitInitValArray(this);
}

std::any SysYParserParser::InitValArrayContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitInitValArray(this);
  else
    return visitor->visitChildren(this);
}
//----------------- InitValSingleContext ------------------------------------------------------------------

SysYParserParser::ExpContext* SysYParserParser::InitValSingleContext::exp() {
  return getRuleContext<SysYParserParser::ExpContext>(0);
}

SysYParserParser::InitValSingleContext::InitValSingleContext(InitValContext *ctx) { copyFrom(ctx); }

void SysYParserParser::InitValSingleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterInitValSingle(this);
}
void SysYParserParser::InitValSingleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitInitValSingle(this);
}

std::any SysYParserParser::InitValSingleContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitInitValSingle(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::InitValContext* SysYParserParser::initVal() {
  InitValContext *_localctx = _tracker.createInstance<InitValContext>(_ctx, getState());
  enterRule(_localctx, 20, SysYParserParser::RuleInitVal);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(169);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParserParser::T__10:
      case SysYParserParser::T__19:
      case SysYParserParser::T__20:
      case SysYParserParser::T__21:
      case SysYParserParser::Identifier:
      case SysYParserParser::DecimalConstant:
      case SysYParserParser::OctalConstant:
      case SysYParserParser::HexadecimalConstant:
      case SysYParserParser::FloatingConstant: {
        _localctx = _tracker.createInstance<SysYParserParser::InitValSingleContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(156);
        exp();
        break;
      }

      case SysYParserParser::T__6: {
        _localctx = _tracker.createInstance<SysYParserParser::InitValArrayContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(157);
        match(SysYParserParser::T__6);
        setState(166);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 532583286912) != 0)) {
          setState(158);
          initVal();
          setState(163);
          _errHandler->sync(this);
          _la = _input->LA(1);
          while (_la == SysYParserParser::T__1) {
            setState(159);
            match(SysYParserParser::T__1);
            setState(160);
            initVal();
            setState(165);
            _errHandler->sync(this);
            _la = _input->LA(1);
          }
        }
        setState(168);
        match(SysYParserParser::T__7);
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

SysYParserParser::FuncDefContext::FuncDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::FuncTypeContext* SysYParserParser::FuncDefContext::funcType() {
  return getRuleContext<SysYParserParser::FuncTypeContext>(0);
}

tree::TerminalNode* SysYParserParser::FuncDefContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

SysYParserParser::BlockContext* SysYParserParser::FuncDefContext::block() {
  return getRuleContext<SysYParserParser::BlockContext>(0);
}

SysYParserParser::FuncFParamsContext* SysYParserParser::FuncDefContext::funcFParams() {
  return getRuleContext<SysYParserParser::FuncFParamsContext>(0);
}


size_t SysYParserParser::FuncDefContext::getRuleIndex() const {
  return SysYParserParser::RuleFuncDef;
}

void SysYParserParser::FuncDefContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncDef(this);
}

void SysYParserParser::FuncDefContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncDef(this);
}


std::any SysYParserParser::FuncDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitFuncDef(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::FuncDefContext* SysYParserParser::funcDef() {
  FuncDefContext *_localctx = _tracker.createInstance<FuncDefContext>(_ctx, getState());
  enterRule(_localctx, 22, SysYParserParser::RuleFuncDef);
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
    setState(171);
    funcType();
    setState(172);
    match(SysYParserParser::Identifier);
    setState(173);
    match(SysYParserParser::T__10);
    setState(175);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParserParser::T__3

    || _la == SysYParserParser::T__4) {
      setState(174);
      funcFParams();
    }
    setState(177);
    match(SysYParserParser::T__11);
    setState(178);
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

SysYParserParser::FuncTypeContext::FuncTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::FuncTypeContext::getRuleIndex() const {
  return SysYParserParser::RuleFuncType;
}

void SysYParserParser::FuncTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncType(this);
}

void SysYParserParser::FuncTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncType(this);
}


std::any SysYParserParser::FuncTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitFuncType(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::FuncTypeContext* SysYParserParser::funcType() {
  FuncTypeContext *_localctx = _tracker.createInstance<FuncTypeContext>(_ctx, getState());
  enterRule(_localctx, 24, SysYParserParser::RuleFuncType);
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
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 8240) != 0))) {
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

//----------------- FuncFParamsContext ------------------------------------------------------------------

SysYParserParser::FuncFParamsContext::FuncFParamsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParserParser::FuncFParamContext *> SysYParserParser::FuncFParamsContext::funcFParam() {
  return getRuleContexts<SysYParserParser::FuncFParamContext>();
}

SysYParserParser::FuncFParamContext* SysYParserParser::FuncFParamsContext::funcFParam(size_t i) {
  return getRuleContext<SysYParserParser::FuncFParamContext>(i);
}


size_t SysYParserParser::FuncFParamsContext::getRuleIndex() const {
  return SysYParserParser::RuleFuncFParams;
}

void SysYParserParser::FuncFParamsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncFParams(this);
}

void SysYParserParser::FuncFParamsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncFParams(this);
}


std::any SysYParserParser::FuncFParamsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitFuncFParams(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::FuncFParamsContext* SysYParserParser::funcFParams() {
  FuncFParamsContext *_localctx = _tracker.createInstance<FuncFParamsContext>(_ctx, getState());
  enterRule(_localctx, 26, SysYParserParser::RuleFuncFParams);
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
    setState(182);
    funcFParam();
    setState(187);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParserParser::T__1) {
      setState(183);
      match(SysYParserParser::T__1);
      setState(184);
      funcFParam();
      setState(189);
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

SysYParserParser::FuncFParamContext::FuncFParamContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::FuncFParamContext::getRuleIndex() const {
  return SysYParserParser::RuleFuncFParam;
}

void SysYParserParser::FuncFParamContext::copyFrom(FuncFParamContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- FuncFParamArrayContext ------------------------------------------------------------------

SysYParserParser::BTypeContext* SysYParserParser::FuncFParamArrayContext::bType() {
  return getRuleContext<SysYParserParser::BTypeContext>(0);
}

tree::TerminalNode* SysYParserParser::FuncFParamArrayContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

std::vector<SysYParserParser::ExpContext *> SysYParserParser::FuncFParamArrayContext::exp() {
  return getRuleContexts<SysYParserParser::ExpContext>();
}

SysYParserParser::ExpContext* SysYParserParser::FuncFParamArrayContext::exp(size_t i) {
  return getRuleContext<SysYParserParser::ExpContext>(i);
}

SysYParserParser::FuncFParamArrayContext::FuncFParamArrayContext(FuncFParamContext *ctx) { copyFrom(ctx); }

void SysYParserParser::FuncFParamArrayContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncFParamArray(this);
}
void SysYParserParser::FuncFParamArrayContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncFParamArray(this);
}

std::any SysYParserParser::FuncFParamArrayContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitFuncFParamArray(this);
  else
    return visitor->visitChildren(this);
}
//----------------- FuncFParamSingleContext ------------------------------------------------------------------

SysYParserParser::BTypeContext* SysYParserParser::FuncFParamSingleContext::bType() {
  return getRuleContext<SysYParserParser::BTypeContext>(0);
}

tree::TerminalNode* SysYParserParser::FuncFParamSingleContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

SysYParserParser::FuncFParamSingleContext::FuncFParamSingleContext(FuncFParamContext *ctx) { copyFrom(ctx); }

void SysYParserParser::FuncFParamSingleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncFParamSingle(this);
}
void SysYParserParser::FuncFParamSingleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncFParamSingle(this);
}

std::any SysYParserParser::FuncFParamSingleContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitFuncFParamSingle(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::FuncFParamContext* SysYParserParser::funcFParam() {
  FuncFParamContext *_localctx = _tracker.createInstance<FuncFParamContext>(_ctx, getState());
  enterRule(_localctx, 28, SysYParserParser::RuleFuncFParam);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(206);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 17, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SysYParserParser::FuncFParamSingleContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(190);
      bType();
      setState(191);
      match(SysYParserParser::Identifier);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysYParserParser::FuncFParamArrayContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(193);
      bType();
      setState(194);
      match(SysYParserParser::Identifier);
      setState(195);
      match(SysYParserParser::T__8);
      setState(196);
      match(SysYParserParser::T__9);
      setState(203);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == SysYParserParser::T__8) {
        setState(197);
        match(SysYParserParser::T__8);
        setState(198);
        exp();
        setState(199);
        match(SysYParserParser::T__9);
        setState(205);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
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

//----------------- BlockContext ------------------------------------------------------------------

SysYParserParser::BlockContext::BlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParserParser::BlockItemContext *> SysYParserParser::BlockContext::blockItem() {
  return getRuleContexts<SysYParserParser::BlockItemContext>();
}

SysYParserParser::BlockItemContext* SysYParserParser::BlockContext::blockItem(size_t i) {
  return getRuleContext<SysYParserParser::BlockItemContext>(i);
}


size_t SysYParserParser::BlockContext::getRuleIndex() const {
  return SysYParserParser::RuleBlock;
}

void SysYParserParser::BlockContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBlock(this);
}

void SysYParserParser::BlockContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBlock(this);
}


std::any SysYParserParser::BlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitBlock(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::BlockContext* SysYParserParser::block() {
  BlockContext *_localctx = _tracker.createInstance<BlockContext>(_ctx, getState());
  enterRule(_localctx, 30, SysYParserParser::RuleBlock);
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
    match(SysYParserParser::T__6);
    setState(212);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 532584286394) != 0)) {
      setState(209);
      blockItem();
      setState(214);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(215);
    match(SysYParserParser::T__7);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BlockItemContext ------------------------------------------------------------------

SysYParserParser::BlockItemContext::BlockItemContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::DeclContext* SysYParserParser::BlockItemContext::decl() {
  return getRuleContext<SysYParserParser::DeclContext>(0);
}

SysYParserParser::StmtContext* SysYParserParser::BlockItemContext::stmt() {
  return getRuleContext<SysYParserParser::StmtContext>(0);
}


size_t SysYParserParser::BlockItemContext::getRuleIndex() const {
  return SysYParserParser::RuleBlockItem;
}

void SysYParserParser::BlockItemContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBlockItem(this);
}

void SysYParserParser::BlockItemContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBlockItem(this);
}


std::any SysYParserParser::BlockItemContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitBlockItem(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::BlockItemContext* SysYParserParser::blockItem() {
  BlockItemContext *_localctx = _tracker.createInstance<BlockItemContext>(_ctx, getState());
  enterRule(_localctx, 32, SysYParserParser::RuleBlockItem);

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
      case SysYParserParser::T__0:
      case SysYParserParser::T__3:
      case SysYParserParser::T__4: {
        enterOuterAlt(_localctx, 1);
        setState(217);
        decl();
        break;
      }

      case SysYParserParser::T__2:
      case SysYParserParser::T__6:
      case SysYParserParser::T__10:
      case SysYParserParser::T__13:
      case SysYParserParser::T__15:
      case SysYParserParser::T__16:
      case SysYParserParser::T__17:
      case SysYParserParser::T__18:
      case SysYParserParser::T__19:
      case SysYParserParser::T__20:
      case SysYParserParser::T__21:
      case SysYParserParser::Identifier:
      case SysYParserParser::DecimalConstant:
      case SysYParserParser::OctalConstant:
      case SysYParserParser::HexadecimalConstant:
      case SysYParserParser::FloatingConstant: {
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

SysYParserParser::StmtContext::StmtContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::StmtContext::getRuleIndex() const {
  return SysYParserParser::RuleStmt;
}

void SysYParserParser::StmtContext::copyFrom(StmtContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- StmtExpContext ------------------------------------------------------------------

SysYParserParser::ExpContext* SysYParserParser::StmtExpContext::exp() {
  return getRuleContext<SysYParserParser::ExpContext>(0);
}

SysYParserParser::StmtExpContext::StmtExpContext(StmtContext *ctx) { copyFrom(ctx); }

void SysYParserParser::StmtExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStmtExp(this);
}
void SysYParserParser::StmtExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStmtExp(this);
}

std::any SysYParserParser::StmtExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitStmtExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StmtBlockContext ------------------------------------------------------------------

SysYParserParser::BlockContext* SysYParserParser::StmtBlockContext::block() {
  return getRuleContext<SysYParserParser::BlockContext>(0);
}

SysYParserParser::StmtBlockContext::StmtBlockContext(StmtContext *ctx) { copyFrom(ctx); }

void SysYParserParser::StmtBlockContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStmtBlock(this);
}
void SysYParserParser::StmtBlockContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStmtBlock(this);
}

std::any SysYParserParser::StmtBlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitStmtBlock(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StmtBreakContext ------------------------------------------------------------------

SysYParserParser::StmtBreakContext::StmtBreakContext(StmtContext *ctx) { copyFrom(ctx); }

void SysYParserParser::StmtBreakContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStmtBreak(this);
}
void SysYParserParser::StmtBreakContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStmtBreak(this);
}

std::any SysYParserParser::StmtBreakContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitStmtBreak(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StmtWhileContext ------------------------------------------------------------------

SysYParserParser::CondContext* SysYParserParser::StmtWhileContext::cond() {
  return getRuleContext<SysYParserParser::CondContext>(0);
}

SysYParserParser::StmtContext* SysYParserParser::StmtWhileContext::stmt() {
  return getRuleContext<SysYParserParser::StmtContext>(0);
}

SysYParserParser::StmtWhileContext::StmtWhileContext(StmtContext *ctx) { copyFrom(ctx); }

void SysYParserParser::StmtWhileContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStmtWhile(this);
}
void SysYParserParser::StmtWhileContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStmtWhile(this);
}

std::any SysYParserParser::StmtWhileContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitStmtWhile(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StmtCondContext ------------------------------------------------------------------

SysYParserParser::CondContext* SysYParserParser::StmtCondContext::cond() {
  return getRuleContext<SysYParserParser::CondContext>(0);
}

std::vector<SysYParserParser::StmtContext *> SysYParserParser::StmtCondContext::stmt() {
  return getRuleContexts<SysYParserParser::StmtContext>();
}

SysYParserParser::StmtContext* SysYParserParser::StmtCondContext::stmt(size_t i) {
  return getRuleContext<SysYParserParser::StmtContext>(i);
}

SysYParserParser::StmtCondContext::StmtCondContext(StmtContext *ctx) { copyFrom(ctx); }

void SysYParserParser::StmtCondContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStmtCond(this);
}
void SysYParserParser::StmtCondContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStmtCond(this);
}

std::any SysYParserParser::StmtCondContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitStmtCond(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StmtContinueContext ------------------------------------------------------------------

SysYParserParser::StmtContinueContext::StmtContinueContext(StmtContext *ctx) { copyFrom(ctx); }

void SysYParserParser::StmtContinueContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStmtContinue(this);
}
void SysYParserParser::StmtContinueContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStmtContinue(this);
}

std::any SysYParserParser::StmtContinueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitStmtContinue(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StmtReturnContext ------------------------------------------------------------------

SysYParserParser::ExpContext* SysYParserParser::StmtReturnContext::exp() {
  return getRuleContext<SysYParserParser::ExpContext>(0);
}

SysYParserParser::StmtReturnContext::StmtReturnContext(StmtContext *ctx) { copyFrom(ctx); }

void SysYParserParser::StmtReturnContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStmtReturn(this);
}
void SysYParserParser::StmtReturnContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStmtReturn(this);
}

std::any SysYParserParser::StmtReturnContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitStmtReturn(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StmtAssignContext ------------------------------------------------------------------

SysYParserParser::LValContext* SysYParserParser::StmtAssignContext::lVal() {
  return getRuleContext<SysYParserParser::LValContext>(0);
}

SysYParserParser::ExpContext* SysYParserParser::StmtAssignContext::exp() {
  return getRuleContext<SysYParserParser::ExpContext>(0);
}

SysYParserParser::StmtAssignContext::StmtAssignContext(StmtContext *ctx) { copyFrom(ctx); }

void SysYParserParser::StmtAssignContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStmtAssign(this);
}
void SysYParserParser::StmtAssignContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStmtAssign(this);
}

std::any SysYParserParser::StmtAssignContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitStmtAssign(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::StmtContext* SysYParserParser::stmt() {
  StmtContext *_localctx = _tracker.createInstance<StmtContext>(_ctx, getState());
  enterRule(_localctx, 34, SysYParserParser::RuleStmt);
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
      _localctx = _tracker.createInstance<SysYParserParser::StmtAssignContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(221);
      lVal();
      setState(222);
      match(SysYParserParser::T__5);
      setState(223);
      exp();
      setState(224);
      match(SysYParserParser::T__2);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysYParserParser::StmtExpContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(227);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 532583286784) != 0)) {
        setState(226);
        exp();
      }
      setState(229);
      match(SysYParserParser::T__2);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SysYParserParser::StmtBlockContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(230);
      block();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<SysYParserParser::StmtCondContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(231);
      match(SysYParserParser::T__13);
      setState(232);
      match(SysYParserParser::T__10);
      setState(233);
      cond();
      setState(234);
      match(SysYParserParser::T__11);
      setState(235);
      stmt();
      setState(238);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 21, _ctx)) {
      case 1: {
        setState(236);
        match(SysYParserParser::T__14);
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
      _localctx = _tracker.createInstance<SysYParserParser::StmtWhileContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(240);
      match(SysYParserParser::T__15);
      setState(241);
      match(SysYParserParser::T__10);
      setState(242);
      cond();
      setState(243);
      match(SysYParserParser::T__11);
      setState(244);
      stmt();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<SysYParserParser::StmtBreakContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(246);
      match(SysYParserParser::T__16);
      setState(247);
      match(SysYParserParser::T__2);
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<SysYParserParser::StmtContinueContext>(_localctx);
      enterOuterAlt(_localctx, 7);
      setState(248);
      match(SysYParserParser::T__17);
      setState(249);
      match(SysYParserParser::T__2);
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<SysYParserParser::StmtReturnContext>(_localctx);
      enterOuterAlt(_localctx, 8);
      setState(250);
      match(SysYParserParser::T__18);
      setState(252);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 532583286784) != 0)) {
        setState(251);
        exp();
      }
      setState(254);
      match(SysYParserParser::T__2);
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

SysYParserParser::ExpContext::ExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::AddExpContext* SysYParserParser::ExpContext::addExp() {
  return getRuleContext<SysYParserParser::AddExpContext>(0);
}


size_t SysYParserParser::ExpContext::getRuleIndex() const {
  return SysYParserParser::RuleExp;
}

void SysYParserParser::ExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExp(this);
}

void SysYParserParser::ExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExp(this);
}


std::any SysYParserParser::ExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::ExpContext* SysYParserParser::exp() {
  ExpContext *_localctx = _tracker.createInstance<ExpContext>(_ctx, getState());
  enterRule(_localctx, 36, SysYParserParser::RuleExp);

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

SysYParserParser::CondContext::CondContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::LOrExpContext* SysYParserParser::CondContext::lOrExp() {
  return getRuleContext<SysYParserParser::LOrExpContext>(0);
}


size_t SysYParserParser::CondContext::getRuleIndex() const {
  return SysYParserParser::RuleCond;
}

void SysYParserParser::CondContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCond(this);
}

void SysYParserParser::CondContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCond(this);
}


std::any SysYParserParser::CondContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitCond(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::CondContext* SysYParserParser::cond() {
  CondContext *_localctx = _tracker.createInstance<CondContext>(_ctx, getState());
  enterRule(_localctx, 38, SysYParserParser::RuleCond);

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

//----------------- LValContext ------------------------------------------------------------------

SysYParserParser::LValContext::LValContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::LValContext::getRuleIndex() const {
  return SysYParserParser::RuleLVal;
}

void SysYParserParser::LValContext::copyFrom(LValContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- LValArrayContext ------------------------------------------------------------------

tree::TerminalNode* SysYParserParser::LValArrayContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

std::vector<SysYParserParser::ExpContext *> SysYParserParser::LValArrayContext::exp() {
  return getRuleContexts<SysYParserParser::ExpContext>();
}

SysYParserParser::ExpContext* SysYParserParser::LValArrayContext::exp(size_t i) {
  return getRuleContext<SysYParserParser::ExpContext>(i);
}

SysYParserParser::LValArrayContext::LValArrayContext(LValContext *ctx) { copyFrom(ctx); }

void SysYParserParser::LValArrayContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLValArray(this);
}
void SysYParserParser::LValArrayContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLValArray(this);
}

std::any SysYParserParser::LValArrayContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitLValArray(this);
  else
    return visitor->visitChildren(this);
}
//----------------- LValSingleContext ------------------------------------------------------------------

tree::TerminalNode* SysYParserParser::LValSingleContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

SysYParserParser::LValSingleContext::LValSingleContext(LValContext *ctx) { copyFrom(ctx); }

void SysYParserParser::LValSingleContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLValSingle(this);
}
void SysYParserParser::LValSingleContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLValSingle(this);
}

std::any SysYParserParser::LValSingleContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitLValSingle(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::LValContext* SysYParserParser::lVal() {
  LValContext *_localctx = _tracker.createInstance<LValContext>(_ctx, getState());
  enterRule(_localctx, 40, SysYParserParser::RuleLVal);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(271);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 25, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SysYParserParser::LValSingleContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(261);
      match(SysYParserParser::Identifier);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysYParserParser::LValArrayContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(262);
      match(SysYParserParser::Identifier);
      setState(267); 
      _errHandler->sync(this);
      _la = _input->LA(1);
      do {
        setState(263);
        match(SysYParserParser::T__8);
        setState(264);
        exp();
        setState(265);
        match(SysYParserParser::T__9);
        setState(269); 
        _errHandler->sync(this);
        _la = _input->LA(1);
      } while (_la == SysYParserParser::T__8);
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

//----------------- PrimaryExpContext ------------------------------------------------------------------

SysYParserParser::PrimaryExpContext::PrimaryExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::PrimaryExpContext::getRuleIndex() const {
  return SysYParserParser::RulePrimaryExp;
}

void SysYParserParser::PrimaryExpContext::copyFrom(PrimaryExpContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- PrimaryExpParenContext ------------------------------------------------------------------

SysYParserParser::ExpContext* SysYParserParser::PrimaryExpParenContext::exp() {
  return getRuleContext<SysYParserParser::ExpContext>(0);
}

SysYParserParser::PrimaryExpParenContext::PrimaryExpParenContext(PrimaryExpContext *ctx) { copyFrom(ctx); }

void SysYParserParser::PrimaryExpParenContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterPrimaryExpParen(this);
}
void SysYParserParser::PrimaryExpParenContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitPrimaryExpParen(this);
}

std::any SysYParserParser::PrimaryExpParenContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitPrimaryExpParen(this);
  else
    return visitor->visitChildren(this);
}
//----------------- PrimaryExpNumberContext ------------------------------------------------------------------

SysYParserParser::NumberContext* SysYParserParser::PrimaryExpNumberContext::number() {
  return getRuleContext<SysYParserParser::NumberContext>(0);
}

SysYParserParser::PrimaryExpNumberContext::PrimaryExpNumberContext(PrimaryExpContext *ctx) { copyFrom(ctx); }

void SysYParserParser::PrimaryExpNumberContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterPrimaryExpNumber(this);
}
void SysYParserParser::PrimaryExpNumberContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitPrimaryExpNumber(this);
}

std::any SysYParserParser::PrimaryExpNumberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitPrimaryExpNumber(this);
  else
    return visitor->visitChildren(this);
}
//----------------- PrimaryExpLValContext ------------------------------------------------------------------

SysYParserParser::LValContext* SysYParserParser::PrimaryExpLValContext::lVal() {
  return getRuleContext<SysYParserParser::LValContext>(0);
}

SysYParserParser::PrimaryExpLValContext::PrimaryExpLValContext(PrimaryExpContext *ctx) { copyFrom(ctx); }

void SysYParserParser::PrimaryExpLValContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterPrimaryExpLVal(this);
}
void SysYParserParser::PrimaryExpLValContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitPrimaryExpLVal(this);
}

std::any SysYParserParser::PrimaryExpLValContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitPrimaryExpLVal(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::PrimaryExpContext* SysYParserParser::primaryExp() {
  PrimaryExpContext *_localctx = _tracker.createInstance<PrimaryExpContext>(_ctx, getState());
  enterRule(_localctx, 42, SysYParserParser::RulePrimaryExp);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(279);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParserParser::T__10: {
        _localctx = _tracker.createInstance<SysYParserParser::PrimaryExpParenContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(273);
        match(SysYParserParser::T__10);
        setState(274);
        exp();
        setState(275);
        match(SysYParserParser::T__11);
        break;
      }

      case SysYParserParser::Identifier: {
        _localctx = _tracker.createInstance<SysYParserParser::PrimaryExpLValContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(277);
        lVal();
        break;
      }

      case SysYParserParser::DecimalConstant:
      case SysYParserParser::OctalConstant:
      case SysYParserParser::HexadecimalConstant:
      case SysYParserParser::FloatingConstant: {
        _localctx = _tracker.createInstance<SysYParserParser::PrimaryExpNumberContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(278);
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

SysYParserParser::NumberContext::NumberContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::IntConstContext* SysYParserParser::NumberContext::intConst() {
  return getRuleContext<SysYParserParser::IntConstContext>(0);
}

SysYParserParser::FloatConstContext* SysYParserParser::NumberContext::floatConst() {
  return getRuleContext<SysYParserParser::FloatConstContext>(0);
}


size_t SysYParserParser::NumberContext::getRuleIndex() const {
  return SysYParserParser::RuleNumber;
}

void SysYParserParser::NumberContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNumber(this);
}

void SysYParserParser::NumberContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNumber(this);
}


std::any SysYParserParser::NumberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitNumber(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::NumberContext* SysYParserParser::number() {
  NumberContext *_localctx = _tracker.createInstance<NumberContext>(_ctx, getState());
  enterRule(_localctx, 44, SysYParserParser::RuleNumber);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(283);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParserParser::DecimalConstant:
      case SysYParserParser::OctalConstant:
      case SysYParserParser::HexadecimalConstant: {
        enterOuterAlt(_localctx, 1);
        setState(281);
        intConst();
        break;
      }

      case SysYParserParser::FloatingConstant: {
        enterOuterAlt(_localctx, 2);
        setState(282);
        floatConst();
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

SysYParserParser::UnaryExpContext::UnaryExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::UnaryExpContext::getRuleIndex() const {
  return SysYParserParser::RuleUnaryExp;
}

void SysYParserParser::UnaryExpContext::copyFrom(UnaryExpContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- UnaryExpFuncRContext ------------------------------------------------------------------

tree::TerminalNode* SysYParserParser::UnaryExpFuncRContext::Identifier() {
  return getToken(SysYParserParser::Identifier, 0);
}

SysYParserParser::FuncRParamsContext* SysYParserParser::UnaryExpFuncRContext::funcRParams() {
  return getRuleContext<SysYParserParser::FuncRParamsContext>(0);
}

SysYParserParser::UnaryExpFuncRContext::UnaryExpFuncRContext(UnaryExpContext *ctx) { copyFrom(ctx); }

void SysYParserParser::UnaryExpFuncRContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnaryExpFuncR(this);
}
void SysYParserParser::UnaryExpFuncRContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnaryExpFuncR(this);
}

std::any SysYParserParser::UnaryExpFuncRContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitUnaryExpFuncR(this);
  else
    return visitor->visitChildren(this);
}
//----------------- UnaryExpPrimaryExpContext ------------------------------------------------------------------

SysYParserParser::PrimaryExpContext* SysYParserParser::UnaryExpPrimaryExpContext::primaryExp() {
  return getRuleContext<SysYParserParser::PrimaryExpContext>(0);
}

SysYParserParser::UnaryExpPrimaryExpContext::UnaryExpPrimaryExpContext(UnaryExpContext *ctx) { copyFrom(ctx); }

void SysYParserParser::UnaryExpPrimaryExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnaryExpPrimaryExp(this);
}
void SysYParserParser::UnaryExpPrimaryExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnaryExpPrimaryExp(this);
}

std::any SysYParserParser::UnaryExpPrimaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitUnaryExpPrimaryExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- UnaryExpUnaryContext ------------------------------------------------------------------

SysYParserParser::UnaryOpContext* SysYParserParser::UnaryExpUnaryContext::unaryOp() {
  return getRuleContext<SysYParserParser::UnaryOpContext>(0);
}

SysYParserParser::UnaryExpContext* SysYParserParser::UnaryExpUnaryContext::unaryExp() {
  return getRuleContext<SysYParserParser::UnaryExpContext>(0);
}

SysYParserParser::UnaryExpUnaryContext::UnaryExpUnaryContext(UnaryExpContext *ctx) { copyFrom(ctx); }

void SysYParserParser::UnaryExpUnaryContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnaryExpUnary(this);
}
void SysYParserParser::UnaryExpUnaryContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnaryExpUnary(this);
}

std::any SysYParserParser::UnaryExpUnaryContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitUnaryExpUnary(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::UnaryExpContext* SysYParserParser::unaryExp() {
  UnaryExpContext *_localctx = _tracker.createInstance<UnaryExpContext>(_ctx, getState());
  enterRule(_localctx, 46, SysYParserParser::RuleUnaryExp);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(295);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SysYParserParser::UnaryExpPrimaryExpContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(285);
      primaryExp();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysYParserParser::UnaryExpFuncRContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(286);
      match(SysYParserParser::Identifier);
      setState(287);
      match(SysYParserParser::T__10);
      setState(289);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1082339100672) != 0)) {
        setState(288);
        funcRParams();
      }
      setState(291);
      match(SysYParserParser::T__11);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SysYParserParser::UnaryExpUnaryContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(292);
      unaryOp();
      setState(293);
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

SysYParserParser::UnaryOpContext::UnaryOpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::UnaryOpContext::getRuleIndex() const {
  return SysYParserParser::RuleUnaryOp;
}

void SysYParserParser::UnaryOpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnaryOp(this);
}

void SysYParserParser::UnaryOpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnaryOp(this);
}


std::any SysYParserParser::UnaryOpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitUnaryOp(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::UnaryOpContext* SysYParserParser::unaryOp() {
  UnaryOpContext *_localctx = _tracker.createInstance<UnaryOpContext>(_ctx, getState());
  enterRule(_localctx, 48, SysYParserParser::RuleUnaryOp);
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
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 7340032) != 0))) {
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

SysYParserParser::FuncRParamsContext::FuncRParamsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParserParser::FuncRParamContext *> SysYParserParser::FuncRParamsContext::funcRParam() {
  return getRuleContexts<SysYParserParser::FuncRParamContext>();
}

SysYParserParser::FuncRParamContext* SysYParserParser::FuncRParamsContext::funcRParam(size_t i) {
  return getRuleContext<SysYParserParser::FuncRParamContext>(i);
}


size_t SysYParserParser::FuncRParamsContext::getRuleIndex() const {
  return SysYParserParser::RuleFuncRParams;
}

void SysYParserParser::FuncRParamsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncRParams(this);
}

void SysYParserParser::FuncRParamsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncRParams(this);
}


std::any SysYParserParser::FuncRParamsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitFuncRParams(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::FuncRParamsContext* SysYParserParser::funcRParams() {
  FuncRParamsContext *_localctx = _tracker.createInstance<FuncRParamsContext>(_ctx, getState());
  enterRule(_localctx, 50, SysYParserParser::RuleFuncRParams);
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
    setState(299);
    funcRParam();
    setState(304);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParserParser::T__1) {
      setState(300);
      match(SysYParserParser::T__1);
      setState(301);
      funcRParam();
      setState(306);
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

SysYParserParser::FuncRParamContext::FuncRParamContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::ExpContext* SysYParserParser::FuncRParamContext::exp() {
  return getRuleContext<SysYParserParser::ExpContext>(0);
}

tree::TerminalNode* SysYParserParser::FuncRParamContext::StringLiteral() {
  return getToken(SysYParserParser::StringLiteral, 0);
}


size_t SysYParserParser::FuncRParamContext::getRuleIndex() const {
  return SysYParserParser::RuleFuncRParam;
}

void SysYParserParser::FuncRParamContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncRParam(this);
}

void SysYParserParser::FuncRParamContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncRParam(this);
}


std::any SysYParserParser::FuncRParamContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitFuncRParam(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::FuncRParamContext* SysYParserParser::funcRParam() {
  FuncRParamContext *_localctx = _tracker.createInstance<FuncRParamContext>(_ctx, getState());
  enterRule(_localctx, 52, SysYParserParser::RuleFuncRParam);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(309);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParserParser::T__10:
      case SysYParserParser::T__19:
      case SysYParserParser::T__20:
      case SysYParserParser::T__21:
      case SysYParserParser::Identifier:
      case SysYParserParser::DecimalConstant:
      case SysYParserParser::OctalConstant:
      case SysYParserParser::HexadecimalConstant:
      case SysYParserParser::FloatingConstant: {
        enterOuterAlt(_localctx, 1);
        setState(307);
        exp();
        break;
      }

      case SysYParserParser::StringLiteral: {
        enterOuterAlt(_localctx, 2);
        setState(308);
        match(SysYParserParser::StringLiteral);
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

SysYParserParser::MulExpContext::MulExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParserParser::UnaryExpContext *> SysYParserParser::MulExpContext::unaryExp() {
  return getRuleContexts<SysYParserParser::UnaryExpContext>();
}

SysYParserParser::UnaryExpContext* SysYParserParser::MulExpContext::unaryExp(size_t i) {
  return getRuleContext<SysYParserParser::UnaryExpContext>(i);
}


size_t SysYParserParser::MulExpContext::getRuleIndex() const {
  return SysYParserParser::RuleMulExp;
}

void SysYParserParser::MulExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMulExp(this);
}

void SysYParserParser::MulExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMulExp(this);
}


std::any SysYParserParser::MulExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitMulExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::MulExpContext* SysYParserParser::mulExp() {
  MulExpContext *_localctx = _tracker.createInstance<MulExpContext>(_ctx, getState());
  enterRule(_localctx, 54, SysYParserParser::RuleMulExp);
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
    setState(311);
    antlrcpp::downCast<MulExpContext *>(_localctx)->left = unaryExp();
    setState(316);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 58720256) != 0)) {
      setState(312);
      antlrcpp::downCast<MulExpContext *>(_localctx)->_tset661 = _input->LT(1);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 58720256) != 0))) {
        antlrcpp::downCast<MulExpContext *>(_localctx)->_tset661 = _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      antlrcpp::downCast<MulExpContext *>(_localctx)->op.push_back(antlrcpp::downCast<MulExpContext *>(_localctx)->_tset661);
      setState(313);
      antlrcpp::downCast<MulExpContext *>(_localctx)->unaryExpContext = unaryExp();
      antlrcpp::downCast<MulExpContext *>(_localctx)->right.push_back(antlrcpp::downCast<MulExpContext *>(_localctx)->unaryExpContext);
      setState(318);
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

SysYParserParser::AddExpContext::AddExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParserParser::MulExpContext *> SysYParserParser::AddExpContext::mulExp() {
  return getRuleContexts<SysYParserParser::MulExpContext>();
}

SysYParserParser::MulExpContext* SysYParserParser::AddExpContext::mulExp(size_t i) {
  return getRuleContext<SysYParserParser::MulExpContext>(i);
}


size_t SysYParserParser::AddExpContext::getRuleIndex() const {
  return SysYParserParser::RuleAddExp;
}

void SysYParserParser::AddExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAddExp(this);
}

void SysYParserParser::AddExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAddExp(this);
}


std::any SysYParserParser::AddExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitAddExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::AddExpContext* SysYParserParser::addExp() {
  AddExpContext *_localctx = _tracker.createInstance<AddExpContext>(_ctx, getState());
  enterRule(_localctx, 56, SysYParserParser::RuleAddExp);
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
    setState(319);
    antlrcpp::downCast<AddExpContext *>(_localctx)->left = mulExp();
    setState(324);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParserParser::T__19

    || _la == SysYParserParser::T__20) {
      setState(320);
      antlrcpp::downCast<AddExpContext *>(_localctx)->_tset694 = _input->LT(1);
      _la = _input->LA(1);
      if (!(_la == SysYParserParser::T__19

      || _la == SysYParserParser::T__20)) {
        antlrcpp::downCast<AddExpContext *>(_localctx)->_tset694 = _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      antlrcpp::downCast<AddExpContext *>(_localctx)->op.push_back(antlrcpp::downCast<AddExpContext *>(_localctx)->_tset694);
      setState(321);
      antlrcpp::downCast<AddExpContext *>(_localctx)->mulExpContext = mulExp();
      antlrcpp::downCast<AddExpContext *>(_localctx)->right.push_back(antlrcpp::downCast<AddExpContext *>(_localctx)->mulExpContext);
      setState(326);
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

SysYParserParser::RelExpContext::RelExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParserParser::AddExpContext *> SysYParserParser::RelExpContext::addExp() {
  return getRuleContexts<SysYParserParser::AddExpContext>();
}

SysYParserParser::AddExpContext* SysYParserParser::RelExpContext::addExp(size_t i) {
  return getRuleContext<SysYParserParser::AddExpContext>(i);
}


size_t SysYParserParser::RelExpContext::getRuleIndex() const {
  return SysYParserParser::RuleRelExp;
}

void SysYParserParser::RelExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRelExp(this);
}

void SysYParserParser::RelExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRelExp(this);
}


std::any SysYParserParser::RelExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitRelExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::RelExpContext* SysYParserParser::relExp() {
  RelExpContext *_localctx = _tracker.createInstance<RelExpContext>(_ctx, getState());
  enterRule(_localctx, 58, SysYParserParser::RuleRelExp);
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
    setState(327);
    antlrcpp::downCast<RelExpContext *>(_localctx)->left = addExp();
    setState(332);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1006632960) != 0)) {
      setState(328);
      antlrcpp::downCast<RelExpContext *>(_localctx)->_tset722 = _input->LT(1);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 1006632960) != 0))) {
        antlrcpp::downCast<RelExpContext *>(_localctx)->_tset722 = _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      antlrcpp::downCast<RelExpContext *>(_localctx)->op.push_back(antlrcpp::downCast<RelExpContext *>(_localctx)->_tset722);
      setState(329);
      antlrcpp::downCast<RelExpContext *>(_localctx)->addExpContext = addExp();
      antlrcpp::downCast<RelExpContext *>(_localctx)->right.push_back(antlrcpp::downCast<RelExpContext *>(_localctx)->addExpContext);
      setState(334);
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

SysYParserParser::EqExpContext::EqExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParserParser::RelExpContext *> SysYParserParser::EqExpContext::relExp() {
  return getRuleContexts<SysYParserParser::RelExpContext>();
}

SysYParserParser::RelExpContext* SysYParserParser::EqExpContext::relExp(size_t i) {
  return getRuleContext<SysYParserParser::RelExpContext>(i);
}


size_t SysYParserParser::EqExpContext::getRuleIndex() const {
  return SysYParserParser::RuleEqExp;
}

void SysYParserParser::EqExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterEqExp(this);
}

void SysYParserParser::EqExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitEqExp(this);
}


std::any SysYParserParser::EqExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitEqExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::EqExpContext* SysYParserParser::eqExp() {
  EqExpContext *_localctx = _tracker.createInstance<EqExpContext>(_ctx, getState());
  enterRule(_localctx, 60, SysYParserParser::RuleEqExp);
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
    setState(335);
    antlrcpp::downCast<EqExpContext *>(_localctx)->left = relExp();
    setState(340);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParserParser::T__29

    || _la == SysYParserParser::T__30) {
      setState(336);
      antlrcpp::downCast<EqExpContext *>(_localctx)->_tset757 = _input->LT(1);
      _la = _input->LA(1);
      if (!(_la == SysYParserParser::T__29

      || _la == SysYParserParser::T__30)) {
        antlrcpp::downCast<EqExpContext *>(_localctx)->_tset757 = _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      antlrcpp::downCast<EqExpContext *>(_localctx)->op.push_back(antlrcpp::downCast<EqExpContext *>(_localctx)->_tset757);
      setState(337);
      antlrcpp::downCast<EqExpContext *>(_localctx)->relExpContext = relExp();
      antlrcpp::downCast<EqExpContext *>(_localctx)->right.push_back(antlrcpp::downCast<EqExpContext *>(_localctx)->relExpContext);
      setState(342);
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

SysYParserParser::LAndExpContext::LAndExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParserParser::EqExpContext *> SysYParserParser::LAndExpContext::eqExp() {
  return getRuleContexts<SysYParserParser::EqExpContext>();
}

SysYParserParser::EqExpContext* SysYParserParser::LAndExpContext::eqExp(size_t i) {
  return getRuleContext<SysYParserParser::EqExpContext>(i);
}


size_t SysYParserParser::LAndExpContext::getRuleIndex() const {
  return SysYParserParser::RuleLAndExp;
}

void SysYParserParser::LAndExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLAndExp(this);
}

void SysYParserParser::LAndExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLAndExp(this);
}


std::any SysYParserParser::LAndExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitLAndExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::LAndExpContext* SysYParserParser::lAndExp() {
  LAndExpContext *_localctx = _tracker.createInstance<LAndExpContext>(_ctx, getState());
  enterRule(_localctx, 62, SysYParserParser::RuleLAndExp);
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
    setState(343);
    antlrcpp::downCast<LAndExpContext *>(_localctx)->left = eqExp();
    setState(348);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParserParser::T__31) {
      setState(344);
      antlrcpp::downCast<LAndExpContext *>(_localctx)->s32 = match(SysYParserParser::T__31);
      antlrcpp::downCast<LAndExpContext *>(_localctx)->op.push_back(antlrcpp::downCast<LAndExpContext *>(_localctx)->s32);
      setState(345);
      antlrcpp::downCast<LAndExpContext *>(_localctx)->eqExpContext = eqExp();
      antlrcpp::downCast<LAndExpContext *>(_localctx)->right.push_back(antlrcpp::downCast<LAndExpContext *>(_localctx)->eqExpContext);
      setState(350);
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

SysYParserParser::LOrExpContext::LOrExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysYParserParser::LAndExpContext *> SysYParserParser::LOrExpContext::lAndExp() {
  return getRuleContexts<SysYParserParser::LAndExpContext>();
}

SysYParserParser::LAndExpContext* SysYParserParser::LOrExpContext::lAndExp(size_t i) {
  return getRuleContext<SysYParserParser::LAndExpContext>(i);
}


size_t SysYParserParser::LOrExpContext::getRuleIndex() const {
  return SysYParserParser::RuleLOrExp;
}

void SysYParserParser::LOrExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLOrExp(this);
}

void SysYParserParser::LOrExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLOrExp(this);
}


std::any SysYParserParser::LOrExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitLOrExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::LOrExpContext* SysYParserParser::lOrExp() {
  LOrExpContext *_localctx = _tracker.createInstance<LOrExpContext>(_ctx, getState());
  enterRule(_localctx, 64, SysYParserParser::RuleLOrExp);
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
    setState(351);
    antlrcpp::downCast<LOrExpContext *>(_localctx)->left = lAndExp();
    setState(356);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParserParser::T__32) {
      setState(352);
      antlrcpp::downCast<LOrExpContext *>(_localctx)->s33 = match(SysYParserParser::T__32);
      antlrcpp::downCast<LOrExpContext *>(_localctx)->op.push_back(antlrcpp::downCast<LOrExpContext *>(_localctx)->s33);
      setState(353);
      antlrcpp::downCast<LOrExpContext *>(_localctx)->lAndExpContext = lAndExp();
      antlrcpp::downCast<LOrExpContext *>(_localctx)->right.push_back(antlrcpp::downCast<LOrExpContext *>(_localctx)->lAndExpContext);
      setState(358);
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

SysYParserParser::ConstExpContext::ConstExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParserParser::AddExpContext* SysYParserParser::ConstExpContext::addExp() {
  return getRuleContext<SysYParserParser::AddExpContext>(0);
}


size_t SysYParserParser::ConstExpContext::getRuleIndex() const {
  return SysYParserParser::RuleConstExp;
}

void SysYParserParser::ConstExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstExp(this);
}

void SysYParserParser::ConstExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstExp(this);
}


std::any SysYParserParser::ConstExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitConstExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::ConstExpContext* SysYParserParser::constExp() {
  ConstExpContext *_localctx = _tracker.createInstance<ConstExpContext>(_ctx, getState());
  enterRule(_localctx, 66, SysYParserParser::RuleConstExp);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(359);
    addExp();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IntConstContext ------------------------------------------------------------------

SysYParserParser::IntConstContext::IntConstContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysYParserParser::IntConstContext::getRuleIndex() const {
  return SysYParserParser::RuleIntConst;
}

void SysYParserParser::IntConstContext::copyFrom(IntConstContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- IntHexConstContext ------------------------------------------------------------------

tree::TerminalNode* SysYParserParser::IntHexConstContext::HexadecimalConstant() {
  return getToken(SysYParserParser::HexadecimalConstant, 0);
}

SysYParserParser::IntHexConstContext::IntHexConstContext(IntConstContext *ctx) { copyFrom(ctx); }

void SysYParserParser::IntHexConstContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIntHexConst(this);
}
void SysYParserParser::IntHexConstContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIntHexConst(this);
}

std::any SysYParserParser::IntHexConstContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitIntHexConst(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IntDecConstContext ------------------------------------------------------------------

tree::TerminalNode* SysYParserParser::IntDecConstContext::DecimalConstant() {
  return getToken(SysYParserParser::DecimalConstant, 0);
}

SysYParserParser::IntDecConstContext::IntDecConstContext(IntConstContext *ctx) { copyFrom(ctx); }

void SysYParserParser::IntDecConstContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIntDecConst(this);
}
void SysYParserParser::IntDecConstContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIntDecConst(this);
}

std::any SysYParserParser::IntDecConstContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitIntDecConst(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IntOctConstContext ------------------------------------------------------------------

tree::TerminalNode* SysYParserParser::IntOctConstContext::OctalConstant() {
  return getToken(SysYParserParser::OctalConstant, 0);
}

SysYParserParser::IntOctConstContext::IntOctConstContext(IntConstContext *ctx) { copyFrom(ctx); }

void SysYParserParser::IntOctConstContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIntOctConst(this);
}
void SysYParserParser::IntOctConstContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIntOctConst(this);
}

std::any SysYParserParser::IntOctConstContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitIntOctConst(this);
  else
    return visitor->visitChildren(this);
}
SysYParserParser::IntConstContext* SysYParserParser::intConst() {
  IntConstContext *_localctx = _tracker.createInstance<IntConstContext>(_ctx, getState());
  enterRule(_localctx, 68, SysYParserParser::RuleIntConst);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(364);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParserParser::DecimalConstant: {
        _localctx = _tracker.createInstance<SysYParserParser::IntDecConstContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(361);
        match(SysYParserParser::DecimalConstant);
        break;
      }

      case SysYParserParser::OctalConstant: {
        _localctx = _tracker.createInstance<SysYParserParser::IntOctConstContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(362);
        match(SysYParserParser::OctalConstant);
        break;
      }

      case SysYParserParser::HexadecimalConstant: {
        _localctx = _tracker.createInstance<SysYParserParser::IntHexConstContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(363);
        match(SysYParserParser::HexadecimalConstant);
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

//----------------- FloatConstContext ------------------------------------------------------------------

SysYParserParser::FloatConstContext::FloatConstContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParserParser::FloatConstContext::FloatingConstant() {
  return getToken(SysYParserParser::FloatingConstant, 0);
}


size_t SysYParserParser::FloatConstContext::getRuleIndex() const {
  return SysYParserParser::RuleFloatConst;
}

void SysYParserParser::FloatConstContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFloatConst(this);
}

void SysYParserParser::FloatConstContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysYParserListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFloatConst(this);
}


std::any SysYParserParser::FloatConstContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYParserVisitor*>(visitor))
    return parserVisitor->visitFloatConst(this);
  else
    return visitor->visitChildren(this);
}

SysYParserParser::FloatConstContext* SysYParserParser::floatConst() {
  FloatConstContext *_localctx = _tracker.createInstance<FloatConstContext>(_ctx, getState());
  enterRule(_localctx, 70, SysYParserParser::RuleFloatConst);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(366);
    match(SysYParserParser::FloatingConstant);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void SysYParserParser::initialize() {
  ::antlr4::internal::call_once(sysyparserParserOnceFlag, sysyparserParserInitialize);
}
