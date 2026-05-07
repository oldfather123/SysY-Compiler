
// Generated from Sysy.g4 by ANTLR 4.12.0


#include "SysyListener.h"
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

  SysyParserStaticData(const SysyParserStaticData&) = delete;
  SysyParserStaticData(SysyParserStaticData&&) = delete;
  SysyParserStaticData& operator=(const SysyParserStaticData&) = delete;
  SysyParserStaticData& operator=(SysyParserStaticData&&) = delete;

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
      "primaryExp", "number", "unaryExp", "unaryOp", "funcRParams", "mulExp", 
      "addExp", "relExp", "eqExp", "lAndExp", "lOrExp", "constExp", "epsilon"
    },
    std::vector<std::string>{
      "", "'const'", "','", "';'", "'int'", "'float'", "'['", "']'", "'='", 
      "'{'", "'}'", "'('", "')'", "'void'", "'if'", "'else'", "'while'", 
      "'break'", "'continue'", "'return'", "'+'", "'-'", "'!'", "'*'", "'/'", 
      "'%'", "'<'", "'>'", "'<='", "'>='", "'=='", "'!='", "'&&'", "'||'"
    },
    std::vector<std::string>{
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "", 
      "Ident", "IntConst", "FloatConst", "Whitespace", "LineComment", "BlockComment"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,39,373,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,2,31,7,31,1,0,1,0,5,0,67,8,0,10,0,12,0,70,9,0,
  	1,1,1,1,3,1,74,8,1,1,2,1,2,1,2,1,2,1,2,5,2,81,8,2,10,2,12,2,84,9,2,1,
  	2,1,2,1,3,1,3,1,4,1,4,1,4,1,4,1,4,5,4,95,8,4,10,4,12,4,98,9,4,1,4,1,4,
  	1,4,1,5,1,5,1,5,1,5,1,5,5,5,108,8,5,10,5,12,5,111,9,5,3,5,113,8,5,1,5,
  	3,5,116,8,5,1,6,1,6,1,6,1,6,5,6,122,8,6,10,6,12,6,125,9,6,1,6,1,6,1,7,
  	1,7,1,7,1,7,1,7,5,7,134,8,7,10,7,12,7,137,9,7,1,7,1,7,1,7,1,7,1,7,5,7,
  	144,8,7,10,7,12,7,147,9,7,1,7,1,7,3,7,151,8,7,1,8,1,8,1,8,1,8,1,8,5,8,
  	158,8,8,10,8,12,8,161,9,8,3,8,163,8,8,1,8,3,8,166,8,8,1,9,1,9,1,9,1,9,
  	3,9,172,8,9,1,9,1,9,1,9,1,10,1,10,1,11,1,11,1,11,5,11,182,8,11,10,11,
  	12,11,185,9,11,1,12,1,12,1,12,1,12,1,12,1,12,1,12,1,12,1,12,5,12,196,
  	8,12,10,12,12,12,199,9,12,3,12,201,8,12,1,13,1,13,5,13,205,8,13,10,13,
  	12,13,208,9,13,1,13,1,13,1,14,1,14,3,14,214,8,14,1,15,1,15,1,15,1,15,
  	1,15,1,15,3,15,222,8,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,
  	1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,1,15,
  	1,15,1,15,1,15,1,15,1,15,3,15,252,8,15,1,15,3,15,255,8,15,1,16,1,16,1,
  	17,1,17,1,18,1,18,1,18,1,18,1,18,5,18,266,8,18,10,18,12,18,269,9,18,1,
  	19,1,19,1,19,1,19,1,19,1,19,3,19,277,8,19,1,20,1,20,1,21,1,21,1,21,1,
  	21,3,21,285,8,21,1,21,1,21,1,21,1,21,3,21,291,8,21,1,22,1,22,1,23,1,23,
  	1,23,5,23,298,8,23,10,23,12,23,301,9,23,1,24,1,24,1,24,1,24,1,24,1,24,
  	5,24,309,8,24,10,24,12,24,312,9,24,1,25,1,25,1,25,1,25,1,25,1,25,5,25,
  	320,8,25,10,25,12,25,323,9,25,1,26,1,26,1,26,1,26,1,26,1,26,5,26,331,
  	8,26,10,26,12,26,334,9,26,1,27,1,27,1,27,1,27,1,27,1,27,5,27,342,8,27,
  	10,27,12,27,345,9,27,1,28,1,28,1,28,1,28,1,28,1,28,5,28,353,8,28,10,28,
  	12,28,356,9,28,1,29,1,29,1,29,1,29,1,29,1,29,5,29,364,8,29,10,29,12,29,
  	367,9,29,1,30,1,30,1,31,1,31,1,31,0,6,48,50,52,54,56,58,32,0,2,4,6,8,
  	10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,
  	56,58,60,62,0,8,1,0,4,5,2,0,4,5,13,13,1,0,35,36,1,0,20,22,1,0,23,25,1,
  	0,20,21,1,0,26,29,1,0,30,31,384,0,68,1,0,0,0,2,73,1,0,0,0,4,75,1,0,0,
  	0,6,87,1,0,0,0,8,89,1,0,0,0,10,115,1,0,0,0,12,117,1,0,0,0,14,150,1,0,
  	0,0,16,165,1,0,0,0,18,167,1,0,0,0,20,176,1,0,0,0,22,178,1,0,0,0,24,186,
  	1,0,0,0,26,202,1,0,0,0,28,213,1,0,0,0,30,254,1,0,0,0,32,256,1,0,0,0,34,
  	258,1,0,0,0,36,260,1,0,0,0,38,276,1,0,0,0,40,278,1,0,0,0,42,290,1,0,0,
  	0,44,292,1,0,0,0,46,294,1,0,0,0,48,302,1,0,0,0,50,313,1,0,0,0,52,324,
  	1,0,0,0,54,335,1,0,0,0,56,346,1,0,0,0,58,357,1,0,0,0,60,368,1,0,0,0,62,
  	370,1,0,0,0,64,67,3,2,1,0,65,67,3,18,9,0,66,64,1,0,0,0,66,65,1,0,0,0,
  	67,70,1,0,0,0,68,66,1,0,0,0,68,69,1,0,0,0,69,1,1,0,0,0,70,68,1,0,0,0,
  	71,74,3,4,2,0,72,74,3,12,6,0,73,71,1,0,0,0,73,72,1,0,0,0,74,3,1,0,0,0,
  	75,76,5,1,0,0,76,77,3,6,3,0,77,82,3,8,4,0,78,79,5,2,0,0,79,81,3,8,4,0,
  	80,78,1,0,0,0,81,84,1,0,0,0,82,80,1,0,0,0,82,83,1,0,0,0,83,85,1,0,0,0,
  	84,82,1,0,0,0,85,86,5,3,0,0,86,5,1,0,0,0,87,88,7,0,0,0,88,7,1,0,0,0,89,
  	96,5,34,0,0,90,91,5,6,0,0,91,92,3,60,30,0,92,93,5,7,0,0,93,95,1,0,0,0,
  	94,90,1,0,0,0,95,98,1,0,0,0,96,94,1,0,0,0,96,97,1,0,0,0,97,99,1,0,0,0,
  	98,96,1,0,0,0,99,100,5,8,0,0,100,101,3,10,5,0,101,9,1,0,0,0,102,116,3,
  	60,30,0,103,112,5,9,0,0,104,109,3,10,5,0,105,106,5,2,0,0,106,108,3,10,
  	5,0,107,105,1,0,0,0,108,111,1,0,0,0,109,107,1,0,0,0,109,110,1,0,0,0,110,
  	113,1,0,0,0,111,109,1,0,0,0,112,104,1,0,0,0,112,113,1,0,0,0,113,114,1,
  	0,0,0,114,116,5,10,0,0,115,102,1,0,0,0,115,103,1,0,0,0,116,11,1,0,0,0,
  	117,118,3,6,3,0,118,123,3,14,7,0,119,120,5,2,0,0,120,122,3,14,7,0,121,
  	119,1,0,0,0,122,125,1,0,0,0,123,121,1,0,0,0,123,124,1,0,0,0,124,126,1,
  	0,0,0,125,123,1,0,0,0,126,127,5,3,0,0,127,13,1,0,0,0,128,135,5,34,0,0,
  	129,130,5,6,0,0,130,131,3,60,30,0,131,132,5,7,0,0,132,134,1,0,0,0,133,
  	129,1,0,0,0,134,137,1,0,0,0,135,133,1,0,0,0,135,136,1,0,0,0,136,151,1,
  	0,0,0,137,135,1,0,0,0,138,145,5,34,0,0,139,140,5,6,0,0,140,141,3,60,30,
  	0,141,142,5,7,0,0,142,144,1,0,0,0,143,139,1,0,0,0,144,147,1,0,0,0,145,
  	143,1,0,0,0,145,146,1,0,0,0,146,148,1,0,0,0,147,145,1,0,0,0,148,149,5,
  	8,0,0,149,151,3,16,8,0,150,128,1,0,0,0,150,138,1,0,0,0,151,15,1,0,0,0,
  	152,166,3,32,16,0,153,162,5,9,0,0,154,159,3,16,8,0,155,156,5,2,0,0,156,
  	158,3,16,8,0,157,155,1,0,0,0,158,161,1,0,0,0,159,157,1,0,0,0,159,160,
  	1,0,0,0,160,163,1,0,0,0,161,159,1,0,0,0,162,154,1,0,0,0,162,163,1,0,0,
  	0,163,164,1,0,0,0,164,166,5,10,0,0,165,152,1,0,0,0,165,153,1,0,0,0,166,
  	17,1,0,0,0,167,168,3,20,10,0,168,169,5,34,0,0,169,171,5,11,0,0,170,172,
  	3,22,11,0,171,170,1,0,0,0,171,172,1,0,0,0,172,173,1,0,0,0,173,174,5,12,
  	0,0,174,175,3,26,13,0,175,19,1,0,0,0,176,177,7,1,0,0,177,21,1,0,0,0,178,
  	183,3,24,12,0,179,180,5,2,0,0,180,182,3,24,12,0,181,179,1,0,0,0,182,185,
  	1,0,0,0,183,181,1,0,0,0,183,184,1,0,0,0,184,23,1,0,0,0,185,183,1,0,0,
  	0,186,187,3,6,3,0,187,200,5,34,0,0,188,189,5,6,0,0,189,190,3,62,31,0,
  	190,197,5,7,0,0,191,192,5,6,0,0,192,193,3,32,16,0,193,194,5,7,0,0,194,
  	196,1,0,0,0,195,191,1,0,0,0,196,199,1,0,0,0,197,195,1,0,0,0,197,198,1,
  	0,0,0,198,201,1,0,0,0,199,197,1,0,0,0,200,188,1,0,0,0,200,201,1,0,0,0,
  	201,25,1,0,0,0,202,206,5,9,0,0,203,205,3,28,14,0,204,203,1,0,0,0,205,
  	208,1,0,0,0,206,204,1,0,0,0,206,207,1,0,0,0,207,209,1,0,0,0,208,206,1,
  	0,0,0,209,210,5,10,0,0,210,27,1,0,0,0,211,214,3,2,1,0,212,214,3,30,15,
  	0,213,211,1,0,0,0,213,212,1,0,0,0,214,29,1,0,0,0,215,216,3,36,18,0,216,
  	217,5,8,0,0,217,218,3,32,16,0,218,219,5,3,0,0,219,255,1,0,0,0,220,222,
  	3,32,16,0,221,220,1,0,0,0,221,222,1,0,0,0,222,223,1,0,0,0,223,255,5,3,
  	0,0,224,255,3,26,13,0,225,226,5,14,0,0,226,227,5,11,0,0,227,228,3,32,
  	16,0,228,229,5,12,0,0,229,230,3,30,15,0,230,255,1,0,0,0,231,232,5,14,
  	0,0,232,233,5,11,0,0,233,234,3,32,16,0,234,235,5,12,0,0,235,236,3,30,
  	15,0,236,237,5,15,0,0,237,238,3,30,15,0,238,255,1,0,0,0,239,240,5,16,
  	0,0,240,241,5,11,0,0,241,242,3,32,16,0,242,243,5,12,0,0,243,244,3,30,
  	15,0,244,255,1,0,0,0,245,246,5,17,0,0,246,255,5,3,0,0,247,248,5,18,0,
  	0,248,255,5,3,0,0,249,251,5,19,0,0,250,252,3,32,16,0,251,250,1,0,0,0,
  	251,252,1,0,0,0,252,253,1,0,0,0,253,255,5,3,0,0,254,215,1,0,0,0,254,221,
  	1,0,0,0,254,224,1,0,0,0,254,225,1,0,0,0,254,231,1,0,0,0,254,239,1,0,0,
  	0,254,245,1,0,0,0,254,247,1,0,0,0,254,249,1,0,0,0,255,31,1,0,0,0,256,
  	257,3,34,17,0,257,33,1,0,0,0,258,259,3,58,29,0,259,35,1,0,0,0,260,267,
  	5,34,0,0,261,262,5,6,0,0,262,263,3,32,16,0,263,264,5,7,0,0,264,266,1,
  	0,0,0,265,261,1,0,0,0,266,269,1,0,0,0,267,265,1,0,0,0,267,268,1,0,0,0,
  	268,37,1,0,0,0,269,267,1,0,0,0,270,271,5,11,0,0,271,272,3,32,16,0,272,
  	273,5,12,0,0,273,277,1,0,0,0,274,277,3,36,18,0,275,277,3,40,20,0,276,
  	270,1,0,0,0,276,274,1,0,0,0,276,275,1,0,0,0,277,39,1,0,0,0,278,279,7,
  	2,0,0,279,41,1,0,0,0,280,291,3,38,19,0,281,282,5,34,0,0,282,284,5,11,
  	0,0,283,285,3,46,23,0,284,283,1,0,0,0,284,285,1,0,0,0,285,286,1,0,0,0,
  	286,291,5,12,0,0,287,288,3,44,22,0,288,289,3,42,21,0,289,291,1,0,0,0,
  	290,280,1,0,0,0,290,281,1,0,0,0,290,287,1,0,0,0,291,43,1,0,0,0,292,293,
  	7,3,0,0,293,45,1,0,0,0,294,299,3,32,16,0,295,296,5,2,0,0,296,298,3,32,
  	16,0,297,295,1,0,0,0,298,301,1,0,0,0,299,297,1,0,0,0,299,300,1,0,0,0,
  	300,47,1,0,0,0,301,299,1,0,0,0,302,303,6,24,-1,0,303,304,3,42,21,0,304,
  	310,1,0,0,0,305,306,10,1,0,0,306,307,7,4,0,0,307,309,3,42,21,0,308,305,
  	1,0,0,0,309,312,1,0,0,0,310,308,1,0,0,0,310,311,1,0,0,0,311,49,1,0,0,
  	0,312,310,1,0,0,0,313,314,6,25,-1,0,314,315,3,48,24,0,315,321,1,0,0,0,
  	316,317,10,1,0,0,317,318,7,5,0,0,318,320,3,48,24,0,319,316,1,0,0,0,320,
  	323,1,0,0,0,321,319,1,0,0,0,321,322,1,0,0,0,322,51,1,0,0,0,323,321,1,
  	0,0,0,324,325,6,26,-1,0,325,326,3,50,25,0,326,332,1,0,0,0,327,328,10,
  	1,0,0,328,329,7,6,0,0,329,331,3,50,25,0,330,327,1,0,0,0,331,334,1,0,0,
  	0,332,330,1,0,0,0,332,333,1,0,0,0,333,53,1,0,0,0,334,332,1,0,0,0,335,
  	336,6,27,-1,0,336,337,3,52,26,0,337,343,1,0,0,0,338,339,10,1,0,0,339,
  	340,7,7,0,0,340,342,3,52,26,0,341,338,1,0,0,0,342,345,1,0,0,0,343,341,
  	1,0,0,0,343,344,1,0,0,0,344,55,1,0,0,0,345,343,1,0,0,0,346,347,6,28,-1,
  	0,347,348,3,54,27,0,348,354,1,0,0,0,349,350,10,1,0,0,350,351,5,32,0,0,
  	351,353,3,54,27,0,352,349,1,0,0,0,353,356,1,0,0,0,354,352,1,0,0,0,354,
  	355,1,0,0,0,355,57,1,0,0,0,356,354,1,0,0,0,357,358,6,29,-1,0,358,359,
  	3,56,28,0,359,365,1,0,0,0,360,361,10,1,0,0,361,362,5,33,0,0,362,364,3,
  	56,28,0,363,360,1,0,0,0,364,367,1,0,0,0,365,363,1,0,0,0,365,366,1,0,0,
  	0,366,59,1,0,0,0,367,365,1,0,0,0,368,369,3,50,25,0,369,61,1,0,0,0,370,
  	371,1,0,0,0,371,63,1,0,0,0,35,66,68,73,82,96,109,112,115,123,135,145,
  	150,159,162,165,171,183,197,200,206,213,221,251,254,267,276,284,290,299,
  	310,321,332,343,354,365
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

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
  _interpreter = new atn::ParserATNSimulator(this, *sysyParserStaticData->atn, sysyParserStaticData->decisionToDFA, sysyParserStaticData->sharedContextCache, options);
}

SysyParser::~SysyParser() {
  delete _interpreter;
}

const atn::ATN& SysyParser::getATN() const {
  return *sysyParserStaticData->atn;
}

std::string SysyParser::getGrammarFileName() const {
  return "Sysy.g4";
}

const std::vector<std::string>& SysyParser::getRuleNames() const {
  return sysyParserStaticData->ruleNames;
}

const dfa::Vocabulary& SysyParser::getVocabulary() const {
  return sysyParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView SysyParser::getSerializedATN() const {
  return sysyParserStaticData->serializedATN;
}


//----------------- CompUnitContext ------------------------------------------------------------------

SysyParser::CompUnitContext::CompUnitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysyParser::DeclContext *> SysyParser::CompUnitContext::decl() {
  return getRuleContexts<SysyParser::DeclContext>();
}

SysyParser::DeclContext* SysyParser::CompUnitContext::decl(size_t i) {
  return getRuleContext<SysyParser::DeclContext>(i);
}

std::vector<SysyParser::FuncDefContext *> SysyParser::CompUnitContext::funcDef() {
  return getRuleContexts<SysyParser::FuncDefContext>();
}

SysyParser::FuncDefContext* SysyParser::CompUnitContext::funcDef(size_t i) {
  return getRuleContext<SysyParser::FuncDefContext>(i);
}


size_t SysyParser::CompUnitContext::getRuleIndex() const {
  return SysyParser::RuleCompUnit;
}

void SysyParser::CompUnitContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCompUnit(this);
}

void SysyParser::CompUnitContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCompUnit(this);
}


std::any SysyParser::CompUnitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitCompUnit(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::CompUnitContext* SysyParser::compUnit() {
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
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 8242) != 0)) {
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

SysyParser::ConstDeclContext* SysyParser::DeclContext::constDecl() {
  return getRuleContext<SysyParser::ConstDeclContext>(0);
}

SysyParser::VarDeclContext* SysyParser::DeclContext::varDecl() {
  return getRuleContext<SysyParser::VarDeclContext>(0);
}


size_t SysyParser::DeclContext::getRuleIndex() const {
  return SysyParser::RuleDecl;
}

void SysyParser::DeclContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDecl(this);
}

void SysyParser::DeclContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDecl(this);
}


std::any SysyParser::DeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitDecl(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::DeclContext* SysyParser::decl() {
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
    setState(73);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysyParser::T__0: {
        enterOuterAlt(_localctx, 1);
        setState(71);
        constDecl();
        break;
      }

      case SysyParser::T__3:
      case SysyParser::T__4: {
        enterOuterAlt(_localctx, 2);
        setState(72);
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

SysyParser::BTypeContext* SysyParser::ConstDeclContext::bType() {
  return getRuleContext<SysyParser::BTypeContext>(0);
}

std::vector<SysyParser::ConstDefContext *> SysyParser::ConstDeclContext::constDef() {
  return getRuleContexts<SysyParser::ConstDefContext>();
}

SysyParser::ConstDefContext* SysyParser::ConstDeclContext::constDef(size_t i) {
  return getRuleContext<SysyParser::ConstDefContext>(i);
}


size_t SysyParser::ConstDeclContext::getRuleIndex() const {
  return SysyParser::RuleConstDecl;
}

void SysyParser::ConstDeclContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstDecl(this);
}

void SysyParser::ConstDeclContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstDecl(this);
}


std::any SysyParser::ConstDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitConstDecl(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::ConstDeclContext* SysyParser::constDecl() {
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
    setState(75);
    match(SysyParser::T__0);
    setState(76);
    bType();
    setState(77);
    constDef();
    setState(82);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysyParser::T__1) {
      setState(78);
      match(SysyParser::T__1);
      setState(79);
      constDef();
      setState(84);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(85);
    match(SysyParser::T__2);
   
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


size_t SysyParser::BTypeContext::getRuleIndex() const {
  return SysyParser::RuleBType;
}

void SysyParser::BTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBType(this);
}

void SysyParser::BTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBType(this);
}


std::any SysyParser::BTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitBType(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::BTypeContext* SysyParser::bType() {
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
    setState(87);
    _la = _input->LA(1);
    if (!(_la == SysyParser::T__3

    || _la == SysyParser::T__4)) {
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

SysyParser::ConstDefContext::ConstDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysyParser::ConstDefContext::Ident() {
  return getToken(SysyParser::Ident, 0);
}

SysyParser::ConstInitValContext* SysyParser::ConstDefContext::constInitVal() {
  return getRuleContext<SysyParser::ConstInitValContext>(0);
}

std::vector<SysyParser::ConstExpContext *> SysyParser::ConstDefContext::constExp() {
  return getRuleContexts<SysyParser::ConstExpContext>();
}

SysyParser::ConstExpContext* SysyParser::ConstDefContext::constExp(size_t i) {
  return getRuleContext<SysyParser::ConstExpContext>(i);
}


size_t SysyParser::ConstDefContext::getRuleIndex() const {
  return SysyParser::RuleConstDef;
}

void SysyParser::ConstDefContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstDef(this);
}

void SysyParser::ConstDefContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstDef(this);
}


std::any SysyParser::ConstDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitConstDef(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::ConstDefContext* SysyParser::constDef() {
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
    setState(89);
    match(SysyParser::Ident);
    setState(96);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysyParser::T__5) {
      setState(90);
      match(SysyParser::T__5);
      setState(91);
      constExp();
      setState(92);
      match(SysyParser::T__6);
      setState(98);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(99);
    match(SysyParser::T__7);
    setState(100);
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

//----------------- SingleConstInitValContext ------------------------------------------------------------------

SysyParser::ConstExpContext* SysyParser::SingleConstInitValContext::constExp() {
  return getRuleContext<SysyParser::ConstExpContext>(0);
}

SysyParser::SingleConstInitValContext::SingleConstInitValContext(ConstInitValContext *ctx) { copyFrom(ctx); }

void SysyParser::SingleConstInitValContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSingleConstInitVal(this);
}
void SysyParser::SingleConstInitValContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSingleConstInitVal(this);
}

std::any SysyParser::SingleConstInitValContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitSingleConstInitVal(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MultiConstInitValContext ------------------------------------------------------------------

std::vector<SysyParser::ConstInitValContext *> SysyParser::MultiConstInitValContext::constInitVal() {
  return getRuleContexts<SysyParser::ConstInitValContext>();
}

SysyParser::ConstInitValContext* SysyParser::MultiConstInitValContext::constInitVal(size_t i) {
  return getRuleContext<SysyParser::ConstInitValContext>(i);
}

SysyParser::MultiConstInitValContext::MultiConstInitValContext(ConstInitValContext *ctx) { copyFrom(ctx); }

void SysyParser::MultiConstInitValContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMultiConstInitVal(this);
}
void SysyParser::MultiConstInitValContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMultiConstInitVal(this);
}

std::any SysyParser::MultiConstInitValContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitMultiConstInitVal(this);
  else
    return visitor->visitChildren(this);
}
SysyParser::ConstInitValContext* SysyParser::constInitVal() {
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
    setState(115);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysyParser::T__10:
      case SysyParser::T__19:
      case SysyParser::T__20:
      case SysyParser::T__21:
      case SysyParser::Ident:
      case SysyParser::IntConst:
      case SysyParser::FloatConst: {
        _localctx = _tracker.createInstance<SysyParser::SingleConstInitValContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(102);
        constExp();
        break;
      }

      case SysyParser::T__8: {
        _localctx = _tracker.createInstance<SysyParser::MultiConstInitValContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(103);
        match(SysyParser::T__8);
        setState(112);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 120266426880) != 0)) {
          setState(104);
          constInitVal();
          setState(109);
          _errHandler->sync(this);
          _la = _input->LA(1);
          while (_la == SysyParser::T__1) {
            setState(105);
            match(SysyParser::T__1);
            setState(106);
            constInitVal();
            setState(111);
            _errHandler->sync(this);
            _la = _input->LA(1);
          }
        }
        setState(114);
        match(SysyParser::T__9);
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

SysyParser::BTypeContext* SysyParser::VarDeclContext::bType() {
  return getRuleContext<SysyParser::BTypeContext>(0);
}

std::vector<SysyParser::VarDefContext *> SysyParser::VarDeclContext::varDef() {
  return getRuleContexts<SysyParser::VarDefContext>();
}

SysyParser::VarDefContext* SysyParser::VarDeclContext::varDef(size_t i) {
  return getRuleContext<SysyParser::VarDefContext>(i);
}


size_t SysyParser::VarDeclContext::getRuleIndex() const {
  return SysyParser::RuleVarDecl;
}

void SysyParser::VarDeclContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVarDecl(this);
}

void SysyParser::VarDeclContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVarDecl(this);
}


std::any SysyParser::VarDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitVarDecl(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::VarDeclContext* SysyParser::varDecl() {
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
    setState(117);
    bType();
    setState(118);
    varDef();
    setState(123);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysyParser::T__1) {
      setState(119);
      match(SysyParser::T__1);
      setState(120);
      varDef();
      setState(125);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(126);
    match(SysyParser::T__2);
   
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

//----------------- NoinitVarDefContext ------------------------------------------------------------------

tree::TerminalNode* SysyParser::NoinitVarDefContext::Ident() {
  return getToken(SysyParser::Ident, 0);
}

std::vector<SysyParser::ConstExpContext *> SysyParser::NoinitVarDefContext::constExp() {
  return getRuleContexts<SysyParser::ConstExpContext>();
}

SysyParser::ConstExpContext* SysyParser::NoinitVarDefContext::constExp(size_t i) {
  return getRuleContext<SysyParser::ConstExpContext>(i);
}

SysyParser::NoinitVarDefContext::NoinitVarDefContext(VarDefContext *ctx) { copyFrom(ctx); }

void SysyParser::NoinitVarDefContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNoinitVarDef(this);
}
void SysyParser::NoinitVarDefContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNoinitVarDef(this);
}

std::any SysyParser::NoinitVarDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitNoinitVarDef(this);
  else
    return visitor->visitChildren(this);
}
//----------------- InitVarDefContext ------------------------------------------------------------------

tree::TerminalNode* SysyParser::InitVarDefContext::Ident() {
  return getToken(SysyParser::Ident, 0);
}

SysyParser::InitValContext* SysyParser::InitVarDefContext::initVal() {
  return getRuleContext<SysyParser::InitValContext>(0);
}

std::vector<SysyParser::ConstExpContext *> SysyParser::InitVarDefContext::constExp() {
  return getRuleContexts<SysyParser::ConstExpContext>();
}

SysyParser::ConstExpContext* SysyParser::InitVarDefContext::constExp(size_t i) {
  return getRuleContext<SysyParser::ConstExpContext>(i);
}

SysyParser::InitVarDefContext::InitVarDefContext(VarDefContext *ctx) { copyFrom(ctx); }

void SysyParser::InitVarDefContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterInitVarDef(this);
}
void SysyParser::InitVarDefContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitInitVarDef(this);
}

std::any SysyParser::InitVarDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitInitVarDef(this);
  else
    return visitor->visitChildren(this);
}
SysyParser::VarDefContext* SysyParser::varDef() {
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
    setState(150);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 11, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SysyParser::NoinitVarDefContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(128);
      match(SysyParser::Ident);
      setState(135);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == SysyParser::T__5) {
        setState(129);
        match(SysyParser::T__5);
        setState(130);
        constExp();
        setState(131);
        match(SysyParser::T__6);
        setState(137);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysyParser::InitVarDefContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(138);
      match(SysyParser::Ident);
      setState(145);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == SysyParser::T__5) {
        setState(139);
        match(SysyParser::T__5);
        setState(140);
        constExp();
        setState(141);
        match(SysyParser::T__6);
        setState(147);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      setState(148);
      match(SysyParser::T__7);
      setState(149);
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

//----------------- SingleInitValContext ------------------------------------------------------------------

SysyParser::ExpContext* SysyParser::SingleInitValContext::exp() {
  return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::SingleInitValContext::SingleInitValContext(InitValContext *ctx) { copyFrom(ctx); }

void SysyParser::SingleInitValContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterSingleInitVal(this);
}
void SysyParser::SingleInitValContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitSingleInitVal(this);
}

std::any SysyParser::SingleInitValContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitSingleInitVal(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MultiInitValContext ------------------------------------------------------------------

std::vector<SysyParser::InitValContext *> SysyParser::MultiInitValContext::initVal() {
  return getRuleContexts<SysyParser::InitValContext>();
}

SysyParser::InitValContext* SysyParser::MultiInitValContext::initVal(size_t i) {
  return getRuleContext<SysyParser::InitValContext>(i);
}

SysyParser::MultiInitValContext::MultiInitValContext(InitValContext *ctx) { copyFrom(ctx); }

void SysyParser::MultiInitValContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMultiInitVal(this);
}
void SysyParser::MultiInitValContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMultiInitVal(this);
}

std::any SysyParser::MultiInitValContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitMultiInitVal(this);
  else
    return visitor->visitChildren(this);
}
SysyParser::InitValContext* SysyParser::initVal() {
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
    setState(165);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysyParser::T__10:
      case SysyParser::T__19:
      case SysyParser::T__20:
      case SysyParser::T__21:
      case SysyParser::Ident:
      case SysyParser::IntConst:
      case SysyParser::FloatConst: {
        _localctx = _tracker.createInstance<SysyParser::SingleInitValContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(152);
        exp();
        break;
      }

      case SysyParser::T__8: {
        _localctx = _tracker.createInstance<SysyParser::MultiInitValContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(153);
        match(SysyParser::T__8);
        setState(162);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 120266426880) != 0)) {
          setState(154);
          initVal();
          setState(159);
          _errHandler->sync(this);
          _la = _input->LA(1);
          while (_la == SysyParser::T__1) {
            setState(155);
            match(SysyParser::T__1);
            setState(156);
            initVal();
            setState(161);
            _errHandler->sync(this);
            _la = _input->LA(1);
          }
        }
        setState(164);
        match(SysyParser::T__9);
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

SysyParser::FuncTypeContext* SysyParser::FuncDefContext::funcType() {
  return getRuleContext<SysyParser::FuncTypeContext>(0);
}

tree::TerminalNode* SysyParser::FuncDefContext::Ident() {
  return getToken(SysyParser::Ident, 0);
}

SysyParser::BlockContext* SysyParser::FuncDefContext::block() {
  return getRuleContext<SysyParser::BlockContext>(0);
}

SysyParser::FuncFParamsContext* SysyParser::FuncDefContext::funcFParams() {
  return getRuleContext<SysyParser::FuncFParamsContext>(0);
}


size_t SysyParser::FuncDefContext::getRuleIndex() const {
  return SysyParser::RuleFuncDef;
}

void SysyParser::FuncDefContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncDef(this);
}

void SysyParser::FuncDefContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncDef(this);
}


std::any SysyParser::FuncDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitFuncDef(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::FuncDefContext* SysyParser::funcDef() {
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
    setState(167);
    funcType();
    setState(168);
    match(SysyParser::Ident);
    setState(169);
    match(SysyParser::T__10);
    setState(171);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysyParser::T__3

    || _la == SysyParser::T__4) {
      setState(170);
      funcFParams();
    }
    setState(173);
    match(SysyParser::T__11);
    setState(174);
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


size_t SysyParser::FuncTypeContext::getRuleIndex() const {
  return SysyParser::RuleFuncType;
}

void SysyParser::FuncTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncType(this);
}

void SysyParser::FuncTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncType(this);
}


std::any SysyParser::FuncTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitFuncType(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::FuncTypeContext* SysyParser::funcType() {
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
    setState(176);
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

SysyParser::FuncFParamsContext::FuncFParamsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysyParser::FuncFParamContext *> SysyParser::FuncFParamsContext::funcFParam() {
  return getRuleContexts<SysyParser::FuncFParamContext>();
}

SysyParser::FuncFParamContext* SysyParser::FuncFParamsContext::funcFParam(size_t i) {
  return getRuleContext<SysyParser::FuncFParamContext>(i);
}


size_t SysyParser::FuncFParamsContext::getRuleIndex() const {
  return SysyParser::RuleFuncFParams;
}

void SysyParser::FuncFParamsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncFParams(this);
}

void SysyParser::FuncFParamsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncFParams(this);
}


std::any SysyParser::FuncFParamsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitFuncFParams(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::FuncFParamsContext* SysyParser::funcFParams() {
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
    setState(178);
    funcFParam();
    setState(183);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysyParser::T__1) {
      setState(179);
      match(SysyParser::T__1);
      setState(180);
      funcFParam();
      setState(185);
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

SysyParser::BTypeContext* SysyParser::FuncFParamContext::bType() {
  return getRuleContext<SysyParser::BTypeContext>(0);
}

tree::TerminalNode* SysyParser::FuncFParamContext::Ident() {
  return getToken(SysyParser::Ident, 0);
}

SysyParser::EpsilonContext* SysyParser::FuncFParamContext::epsilon() {
  return getRuleContext<SysyParser::EpsilonContext>(0);
}

std::vector<SysyParser::ExpContext *> SysyParser::FuncFParamContext::exp() {
  return getRuleContexts<SysyParser::ExpContext>();
}

SysyParser::ExpContext* SysyParser::FuncFParamContext::exp(size_t i) {
  return getRuleContext<SysyParser::ExpContext>(i);
}


size_t SysyParser::FuncFParamContext::getRuleIndex() const {
  return SysyParser::RuleFuncFParam;
}

void SysyParser::FuncFParamContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncFParam(this);
}

void SysyParser::FuncFParamContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncFParam(this);
}


std::any SysyParser::FuncFParamContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitFuncFParam(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::FuncFParamContext* SysyParser::funcFParam() {
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
    setState(186);
    bType();
    setState(187);
    match(SysyParser::Ident);
    setState(200);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysyParser::T__5) {
      setState(188);
      match(SysyParser::T__5);
      setState(189);
      epsilon();
      setState(190);
      match(SysyParser::T__6);
      setState(197);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == SysyParser::T__5) {
        setState(191);
        match(SysyParser::T__5);
        setState(192);
        exp();
        setState(193);
        match(SysyParser::T__6);
        setState(199);
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

std::vector<SysyParser::BlockItemContext *> SysyParser::BlockContext::blockItem() {
  return getRuleContexts<SysyParser::BlockItemContext>();
}

SysyParser::BlockItemContext* SysyParser::BlockContext::blockItem(size_t i) {
  return getRuleContext<SysyParser::BlockItemContext>(i);
}


size_t SysyParser::BlockContext::getRuleIndex() const {
  return SysyParser::RuleBlock;
}

void SysyParser::BlockContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBlock(this);
}

void SysyParser::BlockContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBlock(this);
}


std::any SysyParser::BlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitBlock(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::BlockContext* SysyParser::block() {
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
    setState(202);
    match(SysyParser::T__8);
    setState(206);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 120267426362) != 0)) {
      setState(203);
      blockItem();
      setState(208);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(209);
    match(SysyParser::T__9);
   
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

SysyParser::DeclContext* SysyParser::BlockItemContext::decl() {
  return getRuleContext<SysyParser::DeclContext>(0);
}

SysyParser::StmtContext* SysyParser::BlockItemContext::stmt() {
  return getRuleContext<SysyParser::StmtContext>(0);
}


size_t SysyParser::BlockItemContext::getRuleIndex() const {
  return SysyParser::RuleBlockItem;
}

void SysyParser::BlockItemContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBlockItem(this);
}

void SysyParser::BlockItemContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBlockItem(this);
}


std::any SysyParser::BlockItemContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitBlockItem(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::BlockItemContext* SysyParser::blockItem() {
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
    setState(213);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysyParser::T__0:
      case SysyParser::T__3:
      case SysyParser::T__4: {
        enterOuterAlt(_localctx, 1);
        setState(211);
        decl();
        break;
      }

      case SysyParser::T__2:
      case SysyParser::T__8:
      case SysyParser::T__10:
      case SysyParser::T__13:
      case SysyParser::T__15:
      case SysyParser::T__16:
      case SysyParser::T__17:
      case SysyParser::T__18:
      case SysyParser::T__19:
      case SysyParser::T__20:
      case SysyParser::T__21:
      case SysyParser::Ident:
      case SysyParser::IntConst:
      case SysyParser::FloatConst: {
        enterOuterAlt(_localctx, 2);
        setState(212);
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

SysyParser::ExpContext* SysyParser::WhileStmtContext::exp() {
  return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::StmtContext* SysyParser::WhileStmtContext::stmt() {
  return getRuleContext<SysyParser::StmtContext>(0);
}

SysyParser::WhileStmtContext::WhileStmtContext(StmtContext *ctx) { copyFrom(ctx); }

void SysyParser::WhileStmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterWhileStmt(this);
}
void SysyParser::WhileStmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitWhileStmt(this);
}

std::any SysyParser::WhileStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitWhileStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IfStmtContext ------------------------------------------------------------------

SysyParser::ExpContext* SysyParser::IfStmtContext::exp() {
  return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::StmtContext* SysyParser::IfStmtContext::stmt() {
  return getRuleContext<SysyParser::StmtContext>(0);
}

SysyParser::IfStmtContext::IfStmtContext(StmtContext *ctx) { copyFrom(ctx); }

void SysyParser::IfStmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIfStmt(this);
}
void SysyParser::IfStmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIfStmt(this);
}

std::any SysyParser::IfStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitIfStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BlockStmtContext ------------------------------------------------------------------

SysyParser::BlockContext* SysyParser::BlockStmtContext::block() {
  return getRuleContext<SysyParser::BlockContext>(0);
}

SysyParser::BlockStmtContext::BlockStmtContext(StmtContext *ctx) { copyFrom(ctx); }

void SysyParser::BlockStmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBlockStmt(this);
}
void SysyParser::BlockStmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBlockStmt(this);
}

std::any SysyParser::BlockStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitBlockStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- IfElseStmtContext ------------------------------------------------------------------

SysyParser::ExpContext* SysyParser::IfElseStmtContext::exp() {
  return getRuleContext<SysyParser::ExpContext>(0);
}

std::vector<SysyParser::StmtContext *> SysyParser::IfElseStmtContext::stmt() {
  return getRuleContexts<SysyParser::StmtContext>();
}

SysyParser::StmtContext* SysyParser::IfElseStmtContext::stmt(size_t i) {
  return getRuleContext<SysyParser::StmtContext>(i);
}

SysyParser::IfElseStmtContext::IfElseStmtContext(StmtContext *ctx) { copyFrom(ctx); }

void SysyParser::IfElseStmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIfElseStmt(this);
}
void SysyParser::IfElseStmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIfElseStmt(this);
}

std::any SysyParser::IfElseStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitIfElseStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AssignStmtContext ------------------------------------------------------------------

SysyParser::LValContext* SysyParser::AssignStmtContext::lVal() {
  return getRuleContext<SysyParser::LValContext>(0);
}

SysyParser::ExpContext* SysyParser::AssignStmtContext::exp() {
  return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::AssignStmtContext::AssignStmtContext(StmtContext *ctx) { copyFrom(ctx); }

void SysyParser::AssignStmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAssignStmt(this);
}
void SysyParser::AssignStmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAssignStmt(this);
}

std::any SysyParser::AssignStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitAssignStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BreakStmtContext ------------------------------------------------------------------

SysyParser::BreakStmtContext::BreakStmtContext(StmtContext *ctx) { copyFrom(ctx); }

void SysyParser::BreakStmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBreakStmt(this);
}
void SysyParser::BreakStmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBreakStmt(this);
}

std::any SysyParser::BreakStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitBreakStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ExpStmtContext ------------------------------------------------------------------

SysyParser::ExpContext* SysyParser::ExpStmtContext::exp() {
  return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::ExpStmtContext::ExpStmtContext(StmtContext *ctx) { copyFrom(ctx); }

void SysyParser::ExpStmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpStmt(this);
}
void SysyParser::ExpStmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpStmt(this);
}

std::any SysyParser::ExpStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitExpStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ReturnStmtContext ------------------------------------------------------------------

SysyParser::ExpContext* SysyParser::ReturnStmtContext::exp() {
  return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::ReturnStmtContext::ReturnStmtContext(StmtContext *ctx) { copyFrom(ctx); }

void SysyParser::ReturnStmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterReturnStmt(this);
}
void SysyParser::ReturnStmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitReturnStmt(this);
}

std::any SysyParser::ReturnStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitReturnStmt(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ContinueStmtContext ------------------------------------------------------------------

SysyParser::ContinueStmtContext::ContinueStmtContext(StmtContext *ctx) { copyFrom(ctx); }

void SysyParser::ContinueStmtContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterContinueStmt(this);
}
void SysyParser::ContinueStmtContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitContinueStmt(this);
}

std::any SysyParser::ContinueStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitContinueStmt(this);
  else
    return visitor->visitChildren(this);
}
SysyParser::StmtContext* SysyParser::stmt() {
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
    setState(254);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SysyParser::AssignStmtContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(215);
      lVal();
      setState(216);
      match(SysyParser::T__7);
      setState(217);
      exp();
      setState(218);
      match(SysyParser::T__2);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysyParser::ExpStmtContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(221);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 120266426368) != 0)) {
        setState(220);
        exp();
      }
      setState(223);
      match(SysyParser::T__2);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SysyParser::BlockStmtContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(224);
      block();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<SysyParser::IfStmtContext>(_localctx);
      enterOuterAlt(_localctx, 4);
      setState(225);
      match(SysyParser::T__13);
      setState(226);
      match(SysyParser::T__10);
      setState(227);
      exp();
      setState(228);
      match(SysyParser::T__11);
      setState(229);
      stmt();
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<SysyParser::IfElseStmtContext>(_localctx);
      enterOuterAlt(_localctx, 5);
      setState(231);
      match(SysyParser::T__13);
      setState(232);
      match(SysyParser::T__10);
      setState(233);
      exp();
      setState(234);
      match(SysyParser::T__11);
      setState(235);
      stmt();

      setState(236);
      match(SysyParser::T__14);
      setState(237);
      stmt();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<SysyParser::WhileStmtContext>(_localctx);
      enterOuterAlt(_localctx, 6);
      setState(239);
      match(SysyParser::T__15);
      setState(240);
      match(SysyParser::T__10);
      setState(241);
      exp();
      setState(242);
      match(SysyParser::T__11);
      setState(243);
      stmt();
      break;
    }

    case 7: {
      _localctx = _tracker.createInstance<SysyParser::BreakStmtContext>(_localctx);
      enterOuterAlt(_localctx, 7);
      setState(245);
      match(SysyParser::T__16);
      setState(246);
      match(SysyParser::T__2);
      break;
    }

    case 8: {
      _localctx = _tracker.createInstance<SysyParser::ContinueStmtContext>(_localctx);
      enterOuterAlt(_localctx, 8);
      setState(247);
      match(SysyParser::T__17);
      setState(248);
      match(SysyParser::T__2);
      break;
    }

    case 9: {
      _localctx = _tracker.createInstance<SysyParser::ReturnStmtContext>(_localctx);
      enterOuterAlt(_localctx, 9);
      setState(249);
      match(SysyParser::T__18);
      setState(251);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 120266426368) != 0)) {
        setState(250);
        exp();
      }
      setState(253);
      match(SysyParser::T__2);
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

SysyParser::CondContext* SysyParser::ExpContext::cond() {
  return getRuleContext<SysyParser::CondContext>(0);
}


size_t SysyParser::ExpContext::getRuleIndex() const {
  return SysyParser::RuleExp;
}

void SysyParser::ExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExp(this);
}

void SysyParser::ExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExp(this);
}


std::any SysyParser::ExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitExp(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::ExpContext* SysyParser::exp() {
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
    setState(256);
    cond();
   
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

SysyParser::LOrExpContext* SysyParser::CondContext::lOrExp() {
  return getRuleContext<SysyParser::LOrExpContext>(0);
}


size_t SysyParser::CondContext::getRuleIndex() const {
  return SysyParser::RuleCond;
}

void SysyParser::CondContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCond(this);
}

void SysyParser::CondContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCond(this);
}


std::any SysyParser::CondContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitCond(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::CondContext* SysyParser::cond() {
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
    setState(258);
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

tree::TerminalNode* SysyParser::LValContext::Ident() {
  return getToken(SysyParser::Ident, 0);
}

std::vector<SysyParser::ExpContext *> SysyParser::LValContext::exp() {
  return getRuleContexts<SysyParser::ExpContext>();
}

SysyParser::ExpContext* SysyParser::LValContext::exp(size_t i) {
  return getRuleContext<SysyParser::ExpContext>(i);
}


size_t SysyParser::LValContext::getRuleIndex() const {
  return SysyParser::RuleLVal;
}

void SysyParser::LValContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLVal(this);
}

void SysyParser::LValContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLVal(this);
}


std::any SysyParser::LValContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitLVal(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::LValContext* SysyParser::lVal() {
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
    setState(260);
    match(SysyParser::Ident);
    setState(267);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 24, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(261);
        match(SysyParser::T__5);
        setState(262);
        exp();
        setState(263);
        match(SysyParser::T__6); 
      }
      setState(269);
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

//----------------- LValPrimaryExpContext ------------------------------------------------------------------

SysyParser::LValContext* SysyParser::LValPrimaryExpContext::lVal() {
  return getRuleContext<SysyParser::LValContext>(0);
}

SysyParser::LValPrimaryExpContext::LValPrimaryExpContext(PrimaryExpContext *ctx) { copyFrom(ctx); }

void SysyParser::LValPrimaryExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLValPrimaryExp(this);
}
void SysyParser::LValPrimaryExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLValPrimaryExp(this);
}

std::any SysyParser::LValPrimaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitLValPrimaryExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NumberPrimaryExpContext ------------------------------------------------------------------

SysyParser::NumberContext* SysyParser::NumberPrimaryExpContext::number() {
  return getRuleContext<SysyParser::NumberContext>(0);
}

SysyParser::NumberPrimaryExpContext::NumberPrimaryExpContext(PrimaryExpContext *ctx) { copyFrom(ctx); }

void SysyParser::NumberPrimaryExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNumberPrimaryExp(this);
}
void SysyParser::NumberPrimaryExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNumberPrimaryExp(this);
}

std::any SysyParser::NumberPrimaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitNumberPrimaryExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ParensPrimaryExpContext ------------------------------------------------------------------

SysyParser::ExpContext* SysyParser::ParensPrimaryExpContext::exp() {
  return getRuleContext<SysyParser::ExpContext>(0);
}

SysyParser::ParensPrimaryExpContext::ParensPrimaryExpContext(PrimaryExpContext *ctx) { copyFrom(ctx); }

void SysyParser::ParensPrimaryExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterParensPrimaryExp(this);
}
void SysyParser::ParensPrimaryExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitParensPrimaryExp(this);
}

std::any SysyParser::ParensPrimaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitParensPrimaryExp(this);
  else
    return visitor->visitChildren(this);
}
SysyParser::PrimaryExpContext* SysyParser::primaryExp() {
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
    setState(276);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysyParser::T__10: {
        _localctx = _tracker.createInstance<SysyParser::ParensPrimaryExpContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(270);
        match(SysyParser::T__10);
        setState(271);
        exp();
        setState(272);
        match(SysyParser::T__11);
        break;
      }

      case SysyParser::Ident: {
        _localctx = _tracker.createInstance<SysyParser::LValPrimaryExpContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(274);
        lVal();
        break;
      }

      case SysyParser::IntConst:
      case SysyParser::FloatConst: {
        _localctx = _tracker.createInstance<SysyParser::NumberPrimaryExpContext>(_localctx);
        enterOuterAlt(_localctx, 3);
        setState(275);
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

tree::TerminalNode* SysyParser::NumberContext::IntConst() {
  return getToken(SysyParser::IntConst, 0);
}

tree::TerminalNode* SysyParser::NumberContext::FloatConst() {
  return getToken(SysyParser::FloatConst, 0);
}


size_t SysyParser::NumberContext::getRuleIndex() const {
  return SysyParser::RuleNumber;
}

void SysyParser::NumberContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNumber(this);
}

void SysyParser::NumberContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNumber(this);
}


std::any SysyParser::NumberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitNumber(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::NumberContext* SysyParser::number() {
  NumberContext *_localctx = _tracker.createInstance<NumberContext>(_ctx, getState());
  enterRule(_localctx, 40, SysyParser::RuleNumber);
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
    setState(278);
    _la = _input->LA(1);
    if (!(_la == SysyParser::IntConst

    || _la == SysyParser::FloatConst)) {
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

//----------------- UnaryOpUnaryExpContext ------------------------------------------------------------------

SysyParser::UnaryOpContext* SysyParser::UnaryOpUnaryExpContext::unaryOp() {
  return getRuleContext<SysyParser::UnaryOpContext>(0);
}

SysyParser::UnaryExpContext* SysyParser::UnaryOpUnaryExpContext::unaryExp() {
  return getRuleContext<SysyParser::UnaryExpContext>(0);
}

SysyParser::UnaryOpUnaryExpContext::UnaryOpUnaryExpContext(UnaryExpContext *ctx) { copyFrom(ctx); }

void SysyParser::UnaryOpUnaryExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnaryOpUnaryExp(this);
}
void SysyParser::UnaryOpUnaryExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnaryOpUnaryExp(this);
}

std::any SysyParser::UnaryOpUnaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitUnaryOpUnaryExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- FuncCallUnaryExpContext ------------------------------------------------------------------

tree::TerminalNode* SysyParser::FuncCallUnaryExpContext::Ident() {
  return getToken(SysyParser::Ident, 0);
}

SysyParser::FuncRParamsContext* SysyParser::FuncCallUnaryExpContext::funcRParams() {
  return getRuleContext<SysyParser::FuncRParamsContext>(0);
}

SysyParser::FuncCallUnaryExpContext::FuncCallUnaryExpContext(UnaryExpContext *ctx) { copyFrom(ctx); }

void SysyParser::FuncCallUnaryExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncCallUnaryExp(this);
}
void SysyParser::FuncCallUnaryExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncCallUnaryExp(this);
}

std::any SysyParser::FuncCallUnaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitFuncCallUnaryExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- PrimaryUnaryExpContext ------------------------------------------------------------------

SysyParser::PrimaryExpContext* SysyParser::PrimaryUnaryExpContext::primaryExp() {
  return getRuleContext<SysyParser::PrimaryExpContext>(0);
}

SysyParser::PrimaryUnaryExpContext::PrimaryUnaryExpContext(UnaryExpContext *ctx) { copyFrom(ctx); }

void SysyParser::PrimaryUnaryExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterPrimaryUnaryExp(this);
}
void SysyParser::PrimaryUnaryExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitPrimaryUnaryExp(this);
}

std::any SysyParser::PrimaryUnaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitPrimaryUnaryExp(this);
  else
    return visitor->visitChildren(this);
}
SysyParser::UnaryExpContext* SysyParser::unaryExp() {
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
    setState(290);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 27, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<SysyParser::PrimaryUnaryExpContext>(_localctx);
      enterOuterAlt(_localctx, 1);
      setState(280);
      primaryExp();
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<SysyParser::FuncCallUnaryExpContext>(_localctx);
      enterOuterAlt(_localctx, 2);
      setState(281);
      match(SysyParser::Ident);
      setState(282);
      match(SysyParser::T__10);
      setState(284);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 120266426368) != 0)) {
        setState(283);
        funcRParams();
      }
      setState(286);
      match(SysyParser::T__11);
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<SysyParser::UnaryOpUnaryExpContext>(_localctx);
      enterOuterAlt(_localctx, 3);
      setState(287);
      unaryOp();
      setState(288);
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


size_t SysyParser::UnaryOpContext::getRuleIndex() const {
  return SysyParser::RuleUnaryOp;
}

void SysyParser::UnaryOpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnaryOp(this);
}

void SysyParser::UnaryOpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnaryOp(this);
}


std::any SysyParser::UnaryOpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitUnaryOp(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::UnaryOpContext* SysyParser::unaryOp() {
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
    setState(292);
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

SysyParser::FuncRParamsContext::FuncRParamsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<SysyParser::ExpContext *> SysyParser::FuncRParamsContext::exp() {
  return getRuleContexts<SysyParser::ExpContext>();
}

SysyParser::ExpContext* SysyParser::FuncRParamsContext::exp(size_t i) {
  return getRuleContext<SysyParser::ExpContext>(i);
}


size_t SysyParser::FuncRParamsContext::getRuleIndex() const {
  return SysyParser::RuleFuncRParams;
}

void SysyParser::FuncRParamsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFuncRParams(this);
}

void SysyParser::FuncRParamsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFuncRParams(this);
}


std::any SysyParser::FuncRParamsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitFuncRParams(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::FuncRParamsContext* SysyParser::funcRParams() {
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
    setState(294);
    exp();
    setState(299);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysyParser::T__1) {
      setState(295);
      match(SysyParser::T__1);
      setState(296);
      exp();
      setState(301);
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

//----------------- MulMulExpContext ------------------------------------------------------------------

SysyParser::MulExpContext* SysyParser::MulMulExpContext::mulExp() {
  return getRuleContext<SysyParser::MulExpContext>(0);
}

SysyParser::UnaryExpContext* SysyParser::MulMulExpContext::unaryExp() {
  return getRuleContext<SysyParser::UnaryExpContext>(0);
}

SysyParser::MulMulExpContext::MulMulExpContext(MulExpContext *ctx) { copyFrom(ctx); }

void SysyParser::MulMulExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMulMulExp(this);
}
void SysyParser::MulMulExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMulMulExp(this);
}

std::any SysyParser::MulMulExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitMulMulExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- UnaryMulExpContext ------------------------------------------------------------------

SysyParser::UnaryExpContext* SysyParser::UnaryMulExpContext::unaryExp() {
  return getRuleContext<SysyParser::UnaryExpContext>(0);
}

SysyParser::UnaryMulExpContext::UnaryMulExpContext(MulExpContext *ctx) { copyFrom(ctx); }

void SysyParser::UnaryMulExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnaryMulExp(this);
}
void SysyParser::UnaryMulExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnaryMulExp(this);
}

std::any SysyParser::UnaryMulExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitUnaryMulExp(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::MulExpContext* SysyParser::mulExp() {
   return mulExp(0);
}

SysyParser::MulExpContext* SysyParser::mulExp(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  SysyParser::MulExpContext *_localctx = _tracker.createInstance<MulExpContext>(_ctx, parentState);
  SysyParser::MulExpContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 48;
  enterRecursionRule(_localctx, 48, SysyParser::RuleMulExp, precedence);

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
    _localctx = _tracker.createInstance<UnaryMulExpContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(303);
    unaryExp();
    _ctx->stop = _input->LT(-1);
    setState(310);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        auto newContext = _tracker.createInstance<MulMulExpContext>(_tracker.createInstance<MulExpContext>(parentContext, parentState));
        _localctx = newContext;
        pushNewRecursionContext(newContext, startState, RuleMulExp);
        setState(305);

        if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
        setState(306);
        _la = _input->LA(1);
        if (!((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 58720256) != 0))) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(307);
        unaryExp(); 
      }
      setState(312);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 29, _ctx);
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

//----------------- AddAddExpContext ------------------------------------------------------------------

SysyParser::AddExpContext* SysyParser::AddAddExpContext::addExp() {
  return getRuleContext<SysyParser::AddExpContext>(0);
}

SysyParser::MulExpContext* SysyParser::AddAddExpContext::mulExp() {
  return getRuleContext<SysyParser::MulExpContext>(0);
}

SysyParser::AddAddExpContext::AddAddExpContext(AddExpContext *ctx) { copyFrom(ctx); }

void SysyParser::AddAddExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAddAddExp(this);
}
void SysyParser::AddAddExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAddAddExp(this);
}

std::any SysyParser::AddAddExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitAddAddExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MulAddExpContext ------------------------------------------------------------------

SysyParser::MulExpContext* SysyParser::MulAddExpContext::mulExp() {
  return getRuleContext<SysyParser::MulExpContext>(0);
}

SysyParser::MulAddExpContext::MulAddExpContext(AddExpContext *ctx) { copyFrom(ctx); }

void SysyParser::MulAddExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMulAddExp(this);
}
void SysyParser::MulAddExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMulAddExp(this);
}

std::any SysyParser::MulAddExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitMulAddExp(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::AddExpContext* SysyParser::addExp() {
   return addExp(0);
}

SysyParser::AddExpContext* SysyParser::addExp(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  SysyParser::AddExpContext *_localctx = _tracker.createInstance<AddExpContext>(_ctx, parentState);
  SysyParser::AddExpContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 50;
  enterRecursionRule(_localctx, 50, SysyParser::RuleAddExp, precedence);

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
    _localctx = _tracker.createInstance<MulAddExpContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(314);
    mulExp(0);
    _ctx->stop = _input->LT(-1);
    setState(321);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 30, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        auto newContext = _tracker.createInstance<AddAddExpContext>(_tracker.createInstance<AddExpContext>(parentContext, parentState));
        _localctx = newContext;
        pushNewRecursionContext(newContext, startState, RuleAddExp);
        setState(316);

        if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
        setState(317);
        _la = _input->LA(1);
        if (!(_la == SysyParser::T__19

        || _la == SysyParser::T__20)) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(318);
        mulExp(0); 
      }
      setState(323);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 30, _ctx);
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

//----------------- RelRelExpContext ------------------------------------------------------------------

SysyParser::RelExpContext* SysyParser::RelRelExpContext::relExp() {
  return getRuleContext<SysyParser::RelExpContext>(0);
}

SysyParser::AddExpContext* SysyParser::RelRelExpContext::addExp() {
  return getRuleContext<SysyParser::AddExpContext>(0);
}

SysyParser::RelRelExpContext::RelRelExpContext(RelExpContext *ctx) { copyFrom(ctx); }

void SysyParser::RelRelExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRelRelExp(this);
}
void SysyParser::RelRelExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRelRelExp(this);
}

std::any SysyParser::RelRelExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitRelRelExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AddRelExpContext ------------------------------------------------------------------

SysyParser::AddExpContext* SysyParser::AddRelExpContext::addExp() {
  return getRuleContext<SysyParser::AddExpContext>(0);
}

SysyParser::AddRelExpContext::AddRelExpContext(RelExpContext *ctx) { copyFrom(ctx); }

void SysyParser::AddRelExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAddRelExp(this);
}
void SysyParser::AddRelExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAddRelExp(this);
}

std::any SysyParser::AddRelExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitAddRelExp(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::RelExpContext* SysyParser::relExp() {
   return relExp(0);
}

SysyParser::RelExpContext* SysyParser::relExp(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  SysyParser::RelExpContext *_localctx = _tracker.createInstance<RelExpContext>(_ctx, parentState);
  SysyParser::RelExpContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 52;
  enterRecursionRule(_localctx, 52, SysyParser::RuleRelExp, precedence);

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
    _localctx = _tracker.createInstance<AddRelExpContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(325);
    addExp(0);
    _ctx->stop = _input->LT(-1);
    setState(332);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 31, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        auto newContext = _tracker.createInstance<RelRelExpContext>(_tracker.createInstance<RelExpContext>(parentContext, parentState));
        _localctx = newContext;
        pushNewRecursionContext(newContext, startState, RuleRelExp);
        setState(327);

        if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
        setState(328);
        _la = _input->LA(1);
        if (!((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 1006632960) != 0))) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(329);
        addExp(0); 
      }
      setState(334);
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

//----------------- EqEqExpContext ------------------------------------------------------------------

SysyParser::EqExpContext* SysyParser::EqEqExpContext::eqExp() {
  return getRuleContext<SysyParser::EqExpContext>(0);
}

SysyParser::RelExpContext* SysyParser::EqEqExpContext::relExp() {
  return getRuleContext<SysyParser::RelExpContext>(0);
}

SysyParser::EqEqExpContext::EqEqExpContext(EqExpContext *ctx) { copyFrom(ctx); }

void SysyParser::EqEqExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterEqEqExp(this);
}
void SysyParser::EqEqExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitEqEqExp(this);
}

std::any SysyParser::EqEqExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitEqEqExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- RelEqExpContext ------------------------------------------------------------------

SysyParser::RelExpContext* SysyParser::RelEqExpContext::relExp() {
  return getRuleContext<SysyParser::RelExpContext>(0);
}

SysyParser::RelEqExpContext::RelEqExpContext(EqExpContext *ctx) { copyFrom(ctx); }

void SysyParser::RelEqExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRelEqExp(this);
}
void SysyParser::RelEqExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRelEqExp(this);
}

std::any SysyParser::RelEqExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitRelEqExp(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::EqExpContext* SysyParser::eqExp() {
   return eqExp(0);
}

SysyParser::EqExpContext* SysyParser::eqExp(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  SysyParser::EqExpContext *_localctx = _tracker.createInstance<EqExpContext>(_ctx, parentState);
  SysyParser::EqExpContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 54;
  enterRecursionRule(_localctx, 54, SysyParser::RuleEqExp, precedence);

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
    _localctx = _tracker.createInstance<RelEqExpContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(336);
    relExp(0);
    _ctx->stop = _input->LT(-1);
    setState(343);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 32, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        auto newContext = _tracker.createInstance<EqEqExpContext>(_tracker.createInstance<EqExpContext>(parentContext, parentState));
        _localctx = newContext;
        pushNewRecursionContext(newContext, startState, RuleEqExp);
        setState(338);

        if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
        setState(339);
        _la = _input->LA(1);
        if (!(_la == SysyParser::T__29

        || _la == SysyParser::T__30)) {
        _errHandler->recoverInline(this);
        }
        else {
          _errHandler->reportMatch(this);
          consume();
        }
        setState(340);
        relExp(0); 
      }
      setState(345);
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

//----------------- EqLAndExpContext ------------------------------------------------------------------

SysyParser::EqExpContext* SysyParser::EqLAndExpContext::eqExp() {
  return getRuleContext<SysyParser::EqExpContext>(0);
}

SysyParser::EqLAndExpContext::EqLAndExpContext(LAndExpContext *ctx) { copyFrom(ctx); }

void SysyParser::EqLAndExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterEqLAndExp(this);
}
void SysyParser::EqLAndExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitEqLAndExp(this);
}

std::any SysyParser::EqLAndExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitEqLAndExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- LAndLAndExpContext ------------------------------------------------------------------

SysyParser::LAndExpContext* SysyParser::LAndLAndExpContext::lAndExp() {
  return getRuleContext<SysyParser::LAndExpContext>(0);
}

SysyParser::EqExpContext* SysyParser::LAndLAndExpContext::eqExp() {
  return getRuleContext<SysyParser::EqExpContext>(0);
}

SysyParser::LAndLAndExpContext::LAndLAndExpContext(LAndExpContext *ctx) { copyFrom(ctx); }

void SysyParser::LAndLAndExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLAndLAndExp(this);
}
void SysyParser::LAndLAndExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLAndLAndExp(this);
}

std::any SysyParser::LAndLAndExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitLAndLAndExp(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::LAndExpContext* SysyParser::lAndExp() {
   return lAndExp(0);
}

SysyParser::LAndExpContext* SysyParser::lAndExp(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  SysyParser::LAndExpContext *_localctx = _tracker.createInstance<LAndExpContext>(_ctx, parentState);
  SysyParser::LAndExpContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 56;
  enterRecursionRule(_localctx, 56, SysyParser::RuleLAndExp, precedence);

    

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
    _localctx = _tracker.createInstance<EqLAndExpContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(347);
    eqExp(0);
    _ctx->stop = _input->LT(-1);
    setState(354);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 33, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        auto newContext = _tracker.createInstance<LAndLAndExpContext>(_tracker.createInstance<LAndExpContext>(parentContext, parentState));
        _localctx = newContext;
        pushNewRecursionContext(newContext, startState, RuleLAndExp);
        setState(349);

        if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
        setState(350);
        match(SysyParser::T__31);
        setState(351);
        eqExp(0); 
      }
      setState(356);
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

//----------------- LAndLOrExpContext ------------------------------------------------------------------

SysyParser::LAndExpContext* SysyParser::LAndLOrExpContext::lAndExp() {
  return getRuleContext<SysyParser::LAndExpContext>(0);
}

SysyParser::LAndLOrExpContext::LAndLOrExpContext(LOrExpContext *ctx) { copyFrom(ctx); }

void SysyParser::LAndLOrExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLAndLOrExp(this);
}
void SysyParser::LAndLOrExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLAndLOrExp(this);
}

std::any SysyParser::LAndLOrExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitLAndLOrExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- LOrLOrExpContext ------------------------------------------------------------------

SysyParser::LOrExpContext* SysyParser::LOrLOrExpContext::lOrExp() {
  return getRuleContext<SysyParser::LOrExpContext>(0);
}

SysyParser::LAndExpContext* SysyParser::LOrLOrExpContext::lAndExp() {
  return getRuleContext<SysyParser::LAndExpContext>(0);
}

SysyParser::LOrLOrExpContext::LOrLOrExpContext(LOrExpContext *ctx) { copyFrom(ctx); }

void SysyParser::LOrLOrExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLOrLOrExp(this);
}
void SysyParser::LOrLOrExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLOrLOrExp(this);
}

std::any SysyParser::LOrLOrExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitLOrLOrExp(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::LOrExpContext* SysyParser::lOrExp() {
   return lOrExp(0);
}

SysyParser::LOrExpContext* SysyParser::lOrExp(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  SysyParser::LOrExpContext *_localctx = _tracker.createInstance<LOrExpContext>(_ctx, parentState);
  SysyParser::LOrExpContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 58;
  enterRecursionRule(_localctx, 58, SysyParser::RuleLOrExp, precedence);

    

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
    _localctx = _tracker.createInstance<LAndLOrExpContext>(_localctx);
    _ctx = _localctx;
    previousContext = _localctx;

    setState(358);
    lAndExp(0);
    _ctx->stop = _input->LT(-1);
    setState(365);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 34, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        auto newContext = _tracker.createInstance<LOrLOrExpContext>(_tracker.createInstance<LOrExpContext>(parentContext, parentState));
        _localctx = newContext;
        pushNewRecursionContext(newContext, startState, RuleLOrExp);
        setState(360);

        if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
        setState(361);
        match(SysyParser::T__32);
        setState(362);
        lAndExp(0); 
      }
      setState(367);
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

//----------------- ConstExpContext ------------------------------------------------------------------

SysyParser::ConstExpContext::ConstExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysyParser::AddExpContext* SysyParser::ConstExpContext::addExp() {
  return getRuleContext<SysyParser::AddExpContext>(0);
}


size_t SysyParser::ConstExpContext::getRuleIndex() const {
  return SysyParser::RuleConstExp;
}

void SysyParser::ConstExpContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstExp(this);
}

void SysyParser::ConstExpContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstExp(this);
}


std::any SysyParser::ConstExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitConstExp(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::ConstExpContext* SysyParser::constExp() {
  ConstExpContext *_localctx = _tracker.createInstance<ConstExpContext>(_ctx, getState());
  enterRule(_localctx, 60, SysyParser::RuleConstExp);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(368);
    addExp(0);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- EpsilonContext ------------------------------------------------------------------

SysyParser::EpsilonContext::EpsilonContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t SysyParser::EpsilonContext::getRuleIndex() const {
  return SysyParser::RuleEpsilon;
}

void SysyParser::EpsilonContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterEpsilon(this);
}

void SysyParser::EpsilonContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<SysyListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitEpsilon(this);
}


std::any SysyParser::EpsilonContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysyVisitor*>(visitor))
    return parserVisitor->visitEpsilon(this);
  else
    return visitor->visitChildren(this);
}

SysyParser::EpsilonContext* SysyParser::epsilon() {
  EpsilonContext *_localctx = _tracker.createInstance<EpsilonContext>(_ctx, getState());
  enterRule(_localctx, 62, SysyParser::RuleEpsilon);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);

   
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
    case 24: return mulExpSempred(antlrcpp::downCast<MulExpContext *>(context), predicateIndex);
    case 25: return addExpSempred(antlrcpp::downCast<AddExpContext *>(context), predicateIndex);
    case 26: return relExpSempred(antlrcpp::downCast<RelExpContext *>(context), predicateIndex);
    case 27: return eqExpSempred(antlrcpp::downCast<EqExpContext *>(context), predicateIndex);
    case 28: return lAndExpSempred(antlrcpp::downCast<LAndExpContext *>(context), predicateIndex);
    case 29: return lOrExpSempred(antlrcpp::downCast<LOrExpContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool SysyParser::mulExpSempred(MulExpContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

bool SysyParser::addExpSempred(AddExpContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 1: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

bool SysyParser::relExpSempred(RelExpContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 2: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

bool SysyParser::eqExpSempred(EqExpContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 3: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

bool SysyParser::lAndExpSempred(LAndExpContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 4: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

bool SysyParser::lOrExpSempred(LOrExpContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 5: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

void SysyParser::initialize() {
  ::antlr4::internal::call_once(sysyParserOnceFlag, sysyParserInitialize);
}
