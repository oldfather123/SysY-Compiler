#define NDEBUG

// Generated from SysY.g4 by ANTLR 4.12.0


#include "../include/SysYVisitor.h"

#include "../include/SysYParser.h"


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
SysYParserStaticData *sysyParserStaticData = nullptr;

void sysyParserInitialize() {
  assert(sysyParserStaticData == nullptr);
  auto staticData = std::make_unique<SysYParserStaticData>(
    std::vector<std::string>{
      "compUnit", "decl", "btype", "varDef", "initValue", "funcDecl", "funcDef", 
      "funcType", "funcFParams", "funcFParam", "blockStmt", "blockItem", 
      "stmt", "assignStmt", "expStmt", "ifStmt", "whileStmt", "breakStmt", 
      "continueStmt", "returnStmt", "emptyStmt", "exp", "call", "var", "lValue", 
      "number", "string", "funcRParams"
    },
    std::vector<std::string>{
      "", "'const'", "'int'", "'float'", "'void'", "'if'", "'else'", "'while'", 
      "'break'", "'continue'", "'return'", "'='", "'+'", "'-'", "'*'", "'/'", 
      "'%'", "'<'", "'>'", "'<='", "'>='", "'=='", "'!='", "'&&'", "'||'", 
      "'!'", "'('", "')'", "'['", "']'", "'{'", "'}'", "','", "';'"
    },
    std::vector<std::string>{
      "", "CONST", "INT", "FLOAT", "VOID", "IF", "ELSE", "WHILE", "BREAK", 
      "CONTINUE", "RETURN", "ASSIGN", "ADD", "SUB", "MUL", "DIV", "MODULO", 
      "LT", "GT", "LE", "GE", "EQ", "NE", "AND", "OR", "NOT", "LPAREN", 
      "RPAREN", "LBRACKET", "RBRACKET", "LBRACE", "RBRACE", "COMMA", "SEMICOLON", 
      "ID", "ILITERAL", "FLITERAL", "STRING", "WS", "LINECOMMENT", "BLOCKCOMMENT"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,40,279,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,1,0,1,
  	0,1,0,4,0,60,8,0,11,0,12,0,61,1,1,3,1,65,8,1,1,1,1,1,1,1,1,1,5,1,71,8,
  	1,10,1,12,1,74,9,1,1,1,1,1,1,2,1,2,1,3,1,3,1,3,3,3,83,8,3,1,4,1,4,1,4,
  	1,4,1,4,5,4,90,8,4,10,4,12,4,93,9,4,3,4,95,8,4,1,4,3,4,98,8,4,1,5,1,5,
  	1,5,1,5,3,5,104,8,5,1,5,1,5,1,5,1,6,1,6,1,6,1,6,3,6,113,8,6,1,6,1,6,1,
  	6,1,7,1,7,1,8,1,8,1,8,5,8,123,8,8,10,8,12,8,126,9,8,1,9,1,9,1,9,1,9,1,
  	9,1,9,1,9,1,9,5,9,136,8,9,10,9,12,9,139,9,9,3,9,141,8,9,1,10,1,10,5,10,
  	145,8,10,10,10,12,10,148,9,10,1,10,1,10,1,11,1,11,3,11,154,8,11,1,12,
  	1,12,1,12,1,12,1,12,1,12,1,12,1,12,1,12,3,12,165,8,12,1,13,1,13,1,13,
  	1,13,1,13,1,14,1,14,1,14,1,15,1,15,1,15,1,15,1,15,1,15,1,15,3,15,182,
  	8,15,1,16,1,16,1,16,1,16,1,16,1,16,1,17,1,17,1,17,1,18,1,18,1,18,1,19,
  	1,19,3,19,198,8,19,1,19,1,19,1,20,1,20,1,21,1,21,1,21,1,21,1,21,1,21,
  	1,21,1,21,1,21,1,21,1,21,3,21,215,8,21,1,21,1,21,1,21,1,21,1,21,1,21,
  	1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,21,5,21,235,
  	8,21,10,21,12,21,238,9,21,1,22,1,22,1,22,3,22,243,8,22,1,22,1,22,1,23,
  	1,23,1,23,1,23,1,23,5,23,252,8,23,10,23,12,23,255,9,23,1,24,1,24,1,24,
  	1,24,1,24,5,24,262,8,24,10,24,12,24,265,9,24,1,25,1,25,1,26,1,26,1,27,
  	1,27,1,27,5,27,274,8,27,10,27,12,27,277,9,27,1,27,0,1,42,28,0,2,4,6,8,
  	10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,
  	0,8,1,0,2,3,1,0,2,4,2,0,12,13,25,25,1,0,14,16,1,0,12,13,1,0,17,20,1,0,
  	21,22,1,0,35,36,291,0,59,1,0,0,0,2,64,1,0,0,0,4,77,1,0,0,0,6,79,1,0,0,
  	0,8,97,1,0,0,0,10,99,1,0,0,0,12,108,1,0,0,0,14,117,1,0,0,0,16,119,1,0,
  	0,0,18,127,1,0,0,0,20,142,1,0,0,0,22,153,1,0,0,0,24,164,1,0,0,0,26,166,
  	1,0,0,0,28,171,1,0,0,0,30,174,1,0,0,0,32,183,1,0,0,0,34,189,1,0,0,0,36,
  	192,1,0,0,0,38,195,1,0,0,0,40,201,1,0,0,0,42,214,1,0,0,0,44,239,1,0,0,
  	0,46,246,1,0,0,0,48,256,1,0,0,0,50,266,1,0,0,0,52,268,1,0,0,0,54,270,
  	1,0,0,0,56,60,3,2,1,0,57,60,3,10,5,0,58,60,3,12,6,0,59,56,1,0,0,0,59,
  	57,1,0,0,0,59,58,1,0,0,0,60,61,1,0,0,0,61,59,1,0,0,0,61,62,1,0,0,0,62,
  	1,1,0,0,0,63,65,5,1,0,0,64,63,1,0,0,0,64,65,1,0,0,0,65,66,1,0,0,0,66,
  	67,3,4,2,0,67,72,3,6,3,0,68,69,5,32,0,0,69,71,3,6,3,0,70,68,1,0,0,0,71,
  	74,1,0,0,0,72,70,1,0,0,0,72,73,1,0,0,0,73,75,1,0,0,0,74,72,1,0,0,0,75,
  	76,5,33,0,0,76,3,1,0,0,0,77,78,7,0,0,0,78,5,1,0,0,0,79,82,3,48,24,0,80,
  	81,5,11,0,0,81,83,3,8,4,0,82,80,1,0,0,0,82,83,1,0,0,0,83,7,1,0,0,0,84,
  	98,3,42,21,0,85,94,5,30,0,0,86,91,3,8,4,0,87,88,5,32,0,0,88,90,3,8,4,
  	0,89,87,1,0,0,0,90,93,1,0,0,0,91,89,1,0,0,0,91,92,1,0,0,0,92,95,1,0,0,
  	0,93,91,1,0,0,0,94,86,1,0,0,0,94,95,1,0,0,0,95,96,1,0,0,0,96,98,5,31,
  	0,0,97,84,1,0,0,0,97,85,1,0,0,0,98,9,1,0,0,0,99,100,3,14,7,0,100,101,
  	5,34,0,0,101,103,5,26,0,0,102,104,3,16,8,0,103,102,1,0,0,0,103,104,1,
  	0,0,0,104,105,1,0,0,0,105,106,5,27,0,0,106,107,5,33,0,0,107,11,1,0,0,
  	0,108,109,3,14,7,0,109,110,5,34,0,0,110,112,5,26,0,0,111,113,3,16,8,0,
  	112,111,1,0,0,0,112,113,1,0,0,0,113,114,1,0,0,0,114,115,5,27,0,0,115,
  	116,3,20,10,0,116,13,1,0,0,0,117,118,7,1,0,0,118,15,1,0,0,0,119,124,3,
  	18,9,0,120,121,5,32,0,0,121,123,3,18,9,0,122,120,1,0,0,0,123,126,1,0,
  	0,0,124,122,1,0,0,0,124,125,1,0,0,0,125,17,1,0,0,0,126,124,1,0,0,0,127,
  	128,3,4,2,0,128,140,5,34,0,0,129,130,5,28,0,0,130,137,5,29,0,0,131,132,
  	5,28,0,0,132,133,3,42,21,0,133,134,5,29,0,0,134,136,1,0,0,0,135,131,1,
  	0,0,0,136,139,1,0,0,0,137,135,1,0,0,0,137,138,1,0,0,0,138,141,1,0,0,0,
  	139,137,1,0,0,0,140,129,1,0,0,0,140,141,1,0,0,0,141,19,1,0,0,0,142,146,
  	5,30,0,0,143,145,3,22,11,0,144,143,1,0,0,0,145,148,1,0,0,0,146,144,1,
  	0,0,0,146,147,1,0,0,0,147,149,1,0,0,0,148,146,1,0,0,0,149,150,5,31,0,
  	0,150,21,1,0,0,0,151,154,3,2,1,0,152,154,3,24,12,0,153,151,1,0,0,0,153,
  	152,1,0,0,0,154,23,1,0,0,0,155,165,3,26,13,0,156,165,3,28,14,0,157,165,
  	3,30,15,0,158,165,3,32,16,0,159,165,3,34,17,0,160,165,3,36,18,0,161,165,
  	3,38,19,0,162,165,3,20,10,0,163,165,3,40,20,0,164,155,1,0,0,0,164,156,
  	1,0,0,0,164,157,1,0,0,0,164,158,1,0,0,0,164,159,1,0,0,0,164,160,1,0,0,
  	0,164,161,1,0,0,0,164,162,1,0,0,0,164,163,1,0,0,0,165,25,1,0,0,0,166,
  	167,3,48,24,0,167,168,5,11,0,0,168,169,3,42,21,0,169,170,5,33,0,0,170,
  	27,1,0,0,0,171,172,3,42,21,0,172,173,5,33,0,0,173,29,1,0,0,0,174,175,
  	5,5,0,0,175,176,5,26,0,0,176,177,3,42,21,0,177,178,5,27,0,0,178,181,3,
  	24,12,0,179,180,5,6,0,0,180,182,3,24,12,0,181,179,1,0,0,0,181,182,1,0,
  	0,0,182,31,1,0,0,0,183,184,5,7,0,0,184,185,5,26,0,0,185,186,3,42,21,0,
  	186,187,5,27,0,0,187,188,3,24,12,0,188,33,1,0,0,0,189,190,5,8,0,0,190,
  	191,5,33,0,0,191,35,1,0,0,0,192,193,5,9,0,0,193,194,5,33,0,0,194,37,1,
  	0,0,0,195,197,5,10,0,0,196,198,3,42,21,0,197,196,1,0,0,0,197,198,1,0,
  	0,0,198,199,1,0,0,0,199,200,5,33,0,0,200,39,1,0,0,0,201,202,5,33,0,0,
  	202,41,1,0,0,0,203,204,6,21,-1,0,204,205,5,26,0,0,205,206,3,42,21,0,206,
  	207,5,27,0,0,207,215,1,0,0,0,208,215,3,46,23,0,209,215,3,50,25,0,210,
  	215,3,52,26,0,211,215,3,44,22,0,212,213,7,2,0,0,213,215,3,42,21,7,214,
  	203,1,0,0,0,214,208,1,0,0,0,214,209,1,0,0,0,214,210,1,0,0,0,214,211,1,
  	0,0,0,214,212,1,0,0,0,215,236,1,0,0,0,216,217,10,6,0,0,217,218,7,3,0,
  	0,218,235,3,42,21,7,219,220,10,5,0,0,220,221,7,4,0,0,221,235,3,42,21,
  	6,222,223,10,4,0,0,223,224,7,5,0,0,224,235,3,42,21,5,225,226,10,3,0,0,
  	226,227,7,6,0,0,227,235,3,42,21,4,228,229,10,2,0,0,229,230,5,23,0,0,230,
  	235,3,42,21,3,231,232,10,1,0,0,232,233,5,24,0,0,233,235,3,42,21,2,234,
  	216,1,0,0,0,234,219,1,0,0,0,234,222,1,0,0,0,234,225,1,0,0,0,234,228,1,
  	0,0,0,234,231,1,0,0,0,235,238,1,0,0,0,236,234,1,0,0,0,236,237,1,0,0,0,
  	237,43,1,0,0,0,238,236,1,0,0,0,239,240,5,34,0,0,240,242,5,26,0,0,241,
  	243,3,54,27,0,242,241,1,0,0,0,242,243,1,0,0,0,243,244,1,0,0,0,244,245,
  	5,27,0,0,245,45,1,0,0,0,246,253,5,34,0,0,247,248,5,28,0,0,248,249,3,42,
  	21,0,249,250,5,29,0,0,250,252,1,0,0,0,251,247,1,0,0,0,252,255,1,0,0,0,
  	253,251,1,0,0,0,253,254,1,0,0,0,254,47,1,0,0,0,255,253,1,0,0,0,256,263,
  	5,34,0,0,257,258,5,28,0,0,258,259,3,42,21,0,259,260,5,29,0,0,260,262,
  	1,0,0,0,261,257,1,0,0,0,262,265,1,0,0,0,263,261,1,0,0,0,263,264,1,0,0,
  	0,264,49,1,0,0,0,265,263,1,0,0,0,266,267,7,7,0,0,267,51,1,0,0,0,268,269,
  	5,37,0,0,269,53,1,0,0,0,270,275,3,42,21,0,271,272,5,32,0,0,272,274,3,
  	42,21,0,273,271,1,0,0,0,274,277,1,0,0,0,275,273,1,0,0,0,275,276,1,0,0,
  	0,276,55,1,0,0,0,277,275,1,0,0,0,25,59,61,64,72,82,91,94,97,103,112,124,
  	137,140,146,153,164,181,197,214,234,236,242,253,263,275
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

std::vector<SysYParser::DeclContext *> SysYParser::CompUnitContext::decl() {
  return getRuleContexts<SysYParser::DeclContext>();
}

SysYParser::DeclContext* SysYParser::CompUnitContext::decl(size_t i) {
  return getRuleContext<SysYParser::DeclContext>(i);
}

std::vector<SysYParser::FuncDeclContext *> SysYParser::CompUnitContext::funcDecl() {
  return getRuleContexts<SysYParser::FuncDeclContext>();
}

SysYParser::FuncDeclContext* SysYParser::CompUnitContext::funcDecl(size_t i) {
  return getRuleContext<SysYParser::FuncDeclContext>(i);
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
    setState(59); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(59);
      _errHandler->sync(this);
      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 0, _ctx)) {
      case 1: {
        setState(56);
        decl();
        break;
      }

      case 2: {
        setState(57);
        funcDecl();
        break;
      }

      case 3: {
        setState(58);
        funcDef();
        break;
      }

      default:
        break;
      }
      setState(61); 
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

//----------------- DeclContext ------------------------------------------------------------------

SysYParser::DeclContext::DeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::BtypeContext* SysYParser::DeclContext::btype() {
  return getRuleContext<SysYParser::BtypeContext>(0);
}

std::vector<SysYParser::VarDefContext *> SysYParser::DeclContext::varDef() {
  return getRuleContexts<SysYParser::VarDefContext>();
}

SysYParser::VarDefContext* SysYParser::DeclContext::varDef(size_t i) {
  return getRuleContext<SysYParser::VarDefContext>(i);
}

tree::TerminalNode* SysYParser::DeclContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

tree::TerminalNode* SysYParser::DeclContext::CONST() {
  return getToken(SysYParser::CONST, 0);
}

std::vector<tree::TerminalNode *> SysYParser::DeclContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode* SysYParser::DeclContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
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
    setState(64);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::CONST) {
      setState(63);
      match(SysYParser::CONST);
    }
    setState(66);
    btype();
    setState(67);
    varDef();
    setState(72);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(68);
      match(SysYParser::COMMA);
      setState(69);
      varDef();
      setState(74);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(75);
    match(SysYParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BtypeContext ------------------------------------------------------------------

SysYParser::BtypeContext::BtypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::BtypeContext::INT() {
  return getToken(SysYParser::INT, 0);
}

tree::TerminalNode* SysYParser::BtypeContext::FLOAT() {
  return getToken(SysYParser::FLOAT, 0);
}


size_t SysYParser::BtypeContext::getRuleIndex() const {
  return SysYParser::RuleBtype;
}


std::any SysYParser::BtypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitBtype(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::BtypeContext* SysYParser::btype() {
  BtypeContext *_localctx = _tracker.createInstance<BtypeContext>(_ctx, getState());
  enterRule(_localctx, 4, SysYParser::RuleBtype);
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

//----------------- VarDefContext ------------------------------------------------------------------

SysYParser::VarDefContext::VarDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::LValueContext* SysYParser::VarDefContext::lValue() {
  return getRuleContext<SysYParser::LValueContext>(0);
}

tree::TerminalNode* SysYParser::VarDefContext::ASSIGN() {
  return getToken(SysYParser::ASSIGN, 0);
}

SysYParser::InitValueContext* SysYParser::VarDefContext::initValue() {
  return getRuleContext<SysYParser::InitValueContext>(0);
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
  enterRule(_localctx, 6, SysYParser::RuleVarDef);
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
    setState(79);
    lValue();
    setState(82);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::ASSIGN) {
      setState(80);
      match(SysYParser::ASSIGN);
      setState(81);
      initValue();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- InitValueContext ------------------------------------------------------------------

SysYParser::InitValueContext::InitValueContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::ExpContext* SysYParser::InitValueContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode* SysYParser::InitValueContext::LBRACE() {
  return getToken(SysYParser::LBRACE, 0);
}

tree::TerminalNode* SysYParser::InitValueContext::RBRACE() {
  return getToken(SysYParser::RBRACE, 0);
}

std::vector<SysYParser::InitValueContext *> SysYParser::InitValueContext::initValue() {
  return getRuleContexts<SysYParser::InitValueContext>();
}

SysYParser::InitValueContext* SysYParser::InitValueContext::initValue(size_t i) {
  return getRuleContext<SysYParser::InitValueContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::InitValueContext::COMMA() {
  return getTokens(SysYParser::COMMA);
}

tree::TerminalNode* SysYParser::InitValueContext::COMMA(size_t i) {
  return getToken(SysYParser::COMMA, i);
}


size_t SysYParser::InitValueContext::getRuleIndex() const {
  return SysYParser::RuleInitValue;
}


std::any SysYParser::InitValueContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitInitValue(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::InitValueContext* SysYParser::initValue() {
  InitValueContext *_localctx = _tracker.createInstance<InitValueContext>(_ctx, getState());
  enterRule(_localctx, 8, SysYParser::RuleInitValue);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(97);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParser::ADD:
      case SysYParser::SUB:
      case SysYParser::NOT:
      case SysYParser::LPAREN:
      case SysYParser::ID:
      case SysYParser::ILITERAL:
      case SysYParser::FLITERAL:
      case SysYParser::STRING: {
        enterOuterAlt(_localctx, 1);
        setState(84);
        exp(0);
        break;
      }

      case SysYParser::LBRACE: {
        enterOuterAlt(_localctx, 2);
        setState(85);
        match(SysYParser::LBRACE);
        setState(94);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 258872455168) != 0)) {
          setState(86);
          initValue();
          setState(91);
          _errHandler->sync(this);
          _la = _input->LA(1);
          while (_la == SysYParser::COMMA) {
            setState(87);
            match(SysYParser::COMMA);
            setState(88);
            initValue();
            setState(93);
            _errHandler->sync(this);
            _la = _input->LA(1);
          }
        }
        setState(96);
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

//----------------- FuncDeclContext ------------------------------------------------------------------

SysYParser::FuncDeclContext::FuncDeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

SysYParser::FuncTypeContext* SysYParser::FuncDeclContext::funcType() {
  return getRuleContext<SysYParser::FuncTypeContext>(0);
}

tree::TerminalNode* SysYParser::FuncDeclContext::ID() {
  return getToken(SysYParser::ID, 0);
}

tree::TerminalNode* SysYParser::FuncDeclContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

tree::TerminalNode* SysYParser::FuncDeclContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

tree::TerminalNode* SysYParser::FuncDeclContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

SysYParser::FuncFParamsContext* SysYParser::FuncDeclContext::funcFParams() {
  return getRuleContext<SysYParser::FuncFParamsContext>(0);
}


size_t SysYParser::FuncDeclContext::getRuleIndex() const {
  return SysYParser::RuleFuncDecl;
}


std::any SysYParser::FuncDeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitFuncDecl(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::FuncDeclContext* SysYParser::funcDecl() {
  FuncDeclContext *_localctx = _tracker.createInstance<FuncDeclContext>(_ctx, getState());
  enterRule(_localctx, 10, SysYParser::RuleFuncDecl);
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
    funcType();
    setState(100);
    match(SysYParser::ID);
    setState(101);
    match(SysYParser::LPAREN);
    setState(103);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::INT

    || _la == SysYParser::FLOAT) {
      setState(102);
      funcFParams();
    }
    setState(105);
    match(SysYParser::RPAREN);
    setState(106);
    match(SysYParser::SEMICOLON);
   
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

tree::TerminalNode* SysYParser::FuncDefContext::ID() {
  return getToken(SysYParser::ID, 0);
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
  enterRule(_localctx, 12, SysYParser::RuleFuncDef);
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
    setState(108);
    funcType();
    setState(109);
    match(SysYParser::ID);
    setState(110);
    match(SysYParser::LPAREN);
    setState(112);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::INT

    || _la == SysYParser::FLOAT) {
      setState(111);
      funcFParams();
    }
    setState(114);
    match(SysYParser::RPAREN);
    setState(115);
    blockStmt();
   
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
  enterRule(_localctx, 14, SysYParser::RuleFuncType);
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
  enterRule(_localctx, 16, SysYParser::RuleFuncFParams);
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
    funcFParam();
    setState(124);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(120);
      match(SysYParser::COMMA);
      setState(121);
      funcFParam();
      setState(126);
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

SysYParser::BtypeContext* SysYParser::FuncFParamContext::btype() {
  return getRuleContext<SysYParser::BtypeContext>(0);
}

tree::TerminalNode* SysYParser::FuncFParamContext::ID() {
  return getToken(SysYParser::ID, 0);
}

std::vector<tree::TerminalNode *> SysYParser::FuncFParamContext::LBRACKET() {
  return getTokens(SysYParser::LBRACKET);
}

tree::TerminalNode* SysYParser::FuncFParamContext::LBRACKET(size_t i) {
  return getToken(SysYParser::LBRACKET, i);
}

std::vector<tree::TerminalNode *> SysYParser::FuncFParamContext::RBRACKET() {
  return getTokens(SysYParser::RBRACKET);
}

tree::TerminalNode* SysYParser::FuncFParamContext::RBRACKET(size_t i) {
  return getToken(SysYParser::RBRACKET, i);
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
  enterRule(_localctx, 18, SysYParser::RuleFuncFParam);
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
    setState(127);
    btype();
    setState(128);
    match(SysYParser::ID);
    setState(140);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == SysYParser::LBRACKET) {
      setState(129);
      match(SysYParser::LBRACKET);
      setState(130);
      match(SysYParser::RBRACKET);
      setState(137);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == SysYParser::LBRACKET) {
        setState(131);
        match(SysYParser::LBRACKET);
        setState(132);
        exp(0);
        setState(133);
        match(SysYParser::RBRACKET);
        setState(139);
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
  enterRule(_localctx, 20, SysYParser::RuleBlockStmt);
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
    setState(142);
    match(SysYParser::LBRACE);
    setState(146);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 267462391726) != 0)) {
      setState(143);
      blockItem();
      setState(148);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(149);
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
  enterRule(_localctx, 22, SysYParser::RuleBlockItem);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(153);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case SysYParser::CONST:
      case SysYParser::INT:
      case SysYParser::FLOAT: {
        enterOuterAlt(_localctx, 1);
        setState(151);
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
      case SysYParser::LPAREN:
      case SysYParser::LBRACE:
      case SysYParser::SEMICOLON:
      case SysYParser::ID:
      case SysYParser::ILITERAL:
      case SysYParser::FLITERAL:
      case SysYParser::STRING: {
        enterOuterAlt(_localctx, 2);
        setState(152);
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

SysYParser::AssignStmtContext* SysYParser::StmtContext::assignStmt() {
  return getRuleContext<SysYParser::AssignStmtContext>(0);
}

SysYParser::ExpStmtContext* SysYParser::StmtContext::expStmt() {
  return getRuleContext<SysYParser::ExpStmtContext>(0);
}

SysYParser::IfStmtContext* SysYParser::StmtContext::ifStmt() {
  return getRuleContext<SysYParser::IfStmtContext>(0);
}

SysYParser::WhileStmtContext* SysYParser::StmtContext::whileStmt() {
  return getRuleContext<SysYParser::WhileStmtContext>(0);
}

SysYParser::BreakStmtContext* SysYParser::StmtContext::breakStmt() {
  return getRuleContext<SysYParser::BreakStmtContext>(0);
}

SysYParser::ContinueStmtContext* SysYParser::StmtContext::continueStmt() {
  return getRuleContext<SysYParser::ContinueStmtContext>(0);
}

SysYParser::ReturnStmtContext* SysYParser::StmtContext::returnStmt() {
  return getRuleContext<SysYParser::ReturnStmtContext>(0);
}

SysYParser::BlockStmtContext* SysYParser::StmtContext::blockStmt() {
  return getRuleContext<SysYParser::BlockStmtContext>(0);
}

SysYParser::EmptyStmtContext* SysYParser::StmtContext::emptyStmt() {
  return getRuleContext<SysYParser::EmptyStmtContext>(0);
}


size_t SysYParser::StmtContext::getRuleIndex() const {
  return SysYParser::RuleStmt;
}


std::any SysYParser::StmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::StmtContext* SysYParser::stmt() {
  StmtContext *_localctx = _tracker.createInstance<StmtContext>(_ctx, getState());
  enterRule(_localctx, 24, SysYParser::RuleStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(164);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(155);
      assignStmt();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(156);
      expStmt();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(157);
      ifStmt();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(158);
      whileStmt();
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(159);
      breakStmt();
      break;
    }

    case 6: {
      enterOuterAlt(_localctx, 6);
      setState(160);
      continueStmt();
      break;
    }

    case 7: {
      enterOuterAlt(_localctx, 7);
      setState(161);
      returnStmt();
      break;
    }

    case 8: {
      enterOuterAlt(_localctx, 8);
      setState(162);
      blockStmt();
      break;
    }

    case 9: {
      enterOuterAlt(_localctx, 9);
      setState(163);
      emptyStmt();
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

//----------------- AssignStmtContext ------------------------------------------------------------------

SysYParser::AssignStmtContext::AssignStmtContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

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


size_t SysYParser::AssignStmtContext::getRuleIndex() const {
  return SysYParser::RuleAssignStmt;
}


std::any SysYParser::AssignStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitAssignStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::AssignStmtContext* SysYParser::assignStmt() {
  AssignStmtContext *_localctx = _tracker.createInstance<AssignStmtContext>(_ctx, getState());
  enterRule(_localctx, 26, SysYParser::RuleAssignStmt);

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
    lValue();
    setState(167);
    match(SysYParser::ASSIGN);
    setState(168);
    exp(0);
    setState(169);
    match(SysYParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
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

SysYParser::ExpContext* SysYParser::ExpStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode* SysYParser::ExpStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}


size_t SysYParser::ExpStmtContext::getRuleIndex() const {
  return SysYParser::RuleExpStmt;
}


std::any SysYParser::ExpStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitExpStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ExpStmtContext* SysYParser::expStmt() {
  ExpStmtContext *_localctx = _tracker.createInstance<ExpStmtContext>(_ctx, getState());
  enterRule(_localctx, 28, SysYParser::RuleExpStmt);

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
    exp(0);
    setState(172);
    match(SysYParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
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

tree::TerminalNode* SysYParser::IfStmtContext::IF() {
  return getToken(SysYParser::IF, 0);
}

tree::TerminalNode* SysYParser::IfStmtContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

SysYParser::ExpContext* SysYParser::IfStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
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


size_t SysYParser::IfStmtContext::getRuleIndex() const {
  return SysYParser::RuleIfStmt;
}


std::any SysYParser::IfStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitIfStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::IfStmtContext* SysYParser::ifStmt() {
  IfStmtContext *_localctx = _tracker.createInstance<IfStmtContext>(_ctx, getState());
  enterRule(_localctx, 30, SysYParser::RuleIfStmt);

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
    match(SysYParser::IF);
    setState(175);
    match(SysYParser::LPAREN);
    setState(176);
    exp(0);
    setState(177);
    match(SysYParser::RPAREN);
    setState(178);
    stmt();
    setState(181);
    _errHandler->sync(this);

    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 16, _ctx)) {
    case 1: {
      setState(179);
      match(SysYParser::ELSE);
      setState(180);
      stmt();
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

//----------------- WhileStmtContext ------------------------------------------------------------------

SysYParser::WhileStmtContext::WhileStmtContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::WhileStmtContext::WHILE() {
  return getToken(SysYParser::WHILE, 0);
}

tree::TerminalNode* SysYParser::WhileStmtContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

SysYParser::ExpContext* SysYParser::WhileStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode* SysYParser::WhileStmtContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::StmtContext* SysYParser::WhileStmtContext::stmt() {
  return getRuleContext<SysYParser::StmtContext>(0);
}


size_t SysYParser::WhileStmtContext::getRuleIndex() const {
  return SysYParser::RuleWhileStmt;
}


std::any SysYParser::WhileStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitWhileStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::WhileStmtContext* SysYParser::whileStmt() {
  WhileStmtContext *_localctx = _tracker.createInstance<WhileStmtContext>(_ctx, getState());
  enterRule(_localctx, 32, SysYParser::RuleWhileStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(183);
    match(SysYParser::WHILE);
    setState(184);
    match(SysYParser::LPAREN);
    setState(185);
    exp(0);
    setState(186);
    match(SysYParser::RPAREN);
    setState(187);
    stmt();
   
  }
  catch (RecognitionException &e) {
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

tree::TerminalNode* SysYParser::BreakStmtContext::BREAK() {
  return getToken(SysYParser::BREAK, 0);
}

tree::TerminalNode* SysYParser::BreakStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}


size_t SysYParser::BreakStmtContext::getRuleIndex() const {
  return SysYParser::RuleBreakStmt;
}


std::any SysYParser::BreakStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitBreakStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::BreakStmtContext* SysYParser::breakStmt() {
  BreakStmtContext *_localctx = _tracker.createInstance<BreakStmtContext>(_ctx, getState());
  enterRule(_localctx, 34, SysYParser::RuleBreakStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(189);
    match(SysYParser::BREAK);
    setState(190);
    match(SysYParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
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

tree::TerminalNode* SysYParser::ContinueStmtContext::CONTINUE() {
  return getToken(SysYParser::CONTINUE, 0);
}

tree::TerminalNode* SysYParser::ContinueStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}


size_t SysYParser::ContinueStmtContext::getRuleIndex() const {
  return SysYParser::RuleContinueStmt;
}


std::any SysYParser::ContinueStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitContinueStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ContinueStmtContext* SysYParser::continueStmt() {
  ContinueStmtContext *_localctx = _tracker.createInstance<ContinueStmtContext>(_ctx, getState());
  enterRule(_localctx, 36, SysYParser::RuleContinueStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(192);
    match(SysYParser::CONTINUE);
    setState(193);
    match(SysYParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
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

tree::TerminalNode* SysYParser::ReturnStmtContext::RETURN() {
  return getToken(SysYParser::RETURN, 0);
}

tree::TerminalNode* SysYParser::ReturnStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}

SysYParser::ExpContext* SysYParser::ReturnStmtContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}


size_t SysYParser::ReturnStmtContext::getRuleIndex() const {
  return SysYParser::RuleReturnStmt;
}


std::any SysYParser::ReturnStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitReturnStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ReturnStmtContext* SysYParser::returnStmt() {
  ReturnStmtContext *_localctx = _tracker.createInstance<ReturnStmtContext>(_ctx, getState());
  enterRule(_localctx, 38, SysYParser::RuleReturnStmt);
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
    setState(195);
    match(SysYParser::RETURN);
    setState(197);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 257798713344) != 0)) {
      setState(196);
      exp(0);
    }
    setState(199);
    match(SysYParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
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

tree::TerminalNode* SysYParser::EmptyStmtContext::SEMICOLON() {
  return getToken(SysYParser::SEMICOLON, 0);
}


size_t SysYParser::EmptyStmtContext::getRuleIndex() const {
  return SysYParser::RuleEmptyStmt;
}


std::any SysYParser::EmptyStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitEmptyStmt(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::EmptyStmtContext* SysYParser::emptyStmt() {
  EmptyStmtContext *_localctx = _tracker.createInstance<EmptyStmtContext>(_ctx, getState());
  enterRule(_localctx, 40, SysYParser::RuleEmptyStmt);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(201);
    match(SysYParser::SEMICOLON);
   
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


size_t SysYParser::ExpContext::getRuleIndex() const {
  return SysYParser::RuleExp;
}

void SysYParser::ExpContext::copyFrom(ExpContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- RelationExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::RelationExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::RelationExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode* SysYParser::RelationExpContext::LT() {
  return getToken(SysYParser::LT, 0);
}

tree::TerminalNode* SysYParser::RelationExpContext::GT() {
  return getToken(SysYParser::GT, 0);
}

tree::TerminalNode* SysYParser::RelationExpContext::LE() {
  return getToken(SysYParser::LE, 0);
}

tree::TerminalNode* SysYParser::RelationExpContext::GE() {
  return getToken(SysYParser::GE, 0);
}

SysYParser::RelationExpContext::RelationExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::RelationExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitRelationExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- VarExpContext ------------------------------------------------------------------

SysYParser::VarContext* SysYParser::VarExpContext::var() {
  return getRuleContext<SysYParser::VarContext>(0);
}

SysYParser::VarExpContext::VarExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::VarExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitVarExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- MultiplicativeExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::MultiplicativeExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::MultiplicativeExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode* SysYParser::MultiplicativeExpContext::MUL() {
  return getToken(SysYParser::MUL, 0);
}

tree::TerminalNode* SysYParser::MultiplicativeExpContext::DIV() {
  return getToken(SysYParser::DIV, 0);
}

tree::TerminalNode* SysYParser::MultiplicativeExpContext::MODULO() {
  return getToken(SysYParser::MODULO, 0);
}

SysYParser::MultiplicativeExpContext::MultiplicativeExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::MultiplicativeExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitMultiplicativeExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- NumberExpContext ------------------------------------------------------------------

SysYParser::NumberContext* SysYParser::NumberExpContext::number() {
  return getRuleContext<SysYParser::NumberContext>(0);
}

SysYParser::NumberExpContext::NumberExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::NumberExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitNumberExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AndExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::AndExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::AndExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode* SysYParser::AndExpContext::AND() {
  return getToken(SysYParser::AND, 0);
}

SysYParser::AndExpContext::AndExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::AndExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitAndExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- UnaryExpContext ------------------------------------------------------------------

SysYParser::ExpContext* SysYParser::UnaryExpContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode* SysYParser::UnaryExpContext::ADD() {
  return getToken(SysYParser::ADD, 0);
}

tree::TerminalNode* SysYParser::UnaryExpContext::SUB() {
  return getToken(SysYParser::SUB, 0);
}

tree::TerminalNode* SysYParser::UnaryExpContext::NOT() {
  return getToken(SysYParser::NOT, 0);
}

SysYParser::UnaryExpContext::UnaryExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::UnaryExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitUnaryExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- ParenExpContext ------------------------------------------------------------------

tree::TerminalNode* SysYParser::ParenExpContext::LPAREN() {
  return getToken(SysYParser::LPAREN, 0);
}

SysYParser::ExpContext* SysYParser::ParenExpContext::exp() {
  return getRuleContext<SysYParser::ExpContext>(0);
}

tree::TerminalNode* SysYParser::ParenExpContext::RPAREN() {
  return getToken(SysYParser::RPAREN, 0);
}

SysYParser::ParenExpContext::ParenExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::ParenExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitParenExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- StringExpContext ------------------------------------------------------------------

SysYParser::StringContext* SysYParser::StringExpContext::string() {
  return getRuleContext<SysYParser::StringContext>(0);
}

SysYParser::StringExpContext::StringExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::StringExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitStringExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- OrExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::OrExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::OrExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode* SysYParser::OrExpContext::OR() {
  return getToken(SysYParser::OR, 0);
}

SysYParser::OrExpContext::OrExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::OrExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitOrExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- CallExpContext ------------------------------------------------------------------

SysYParser::CallContext* SysYParser::CallExpContext::call() {
  return getRuleContext<SysYParser::CallContext>(0);
}

SysYParser::CallExpContext::CallExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::CallExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitCallExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- AdditiveExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::AdditiveExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::AdditiveExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode* SysYParser::AdditiveExpContext::ADD() {
  return getToken(SysYParser::ADD, 0);
}

tree::TerminalNode* SysYParser::AdditiveExpContext::SUB() {
  return getToken(SysYParser::SUB, 0);
}

SysYParser::AdditiveExpContext::AdditiveExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::AdditiveExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitAdditiveExp(this);
  else
    return visitor->visitChildren(this);
}
//----------------- EqualExpContext ------------------------------------------------------------------

std::vector<SysYParser::ExpContext *> SysYParser::EqualExpContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::EqualExpContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

tree::TerminalNode* SysYParser::EqualExpContext::EQ() {
  return getToken(SysYParser::EQ, 0);
}

tree::TerminalNode* SysYParser::EqualExpContext::NE() {
  return getToken(SysYParser::NE, 0);
}

SysYParser::EqualExpContext::EqualExpContext(ExpContext *ctx) { copyFrom(ctx); }


std::any SysYParser::EqualExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitEqualExp(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::ExpContext* SysYParser::exp() {
   return exp(0);
}

SysYParser::ExpContext* SysYParser::exp(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  SysYParser::ExpContext *_localctx = _tracker.createInstance<ExpContext>(_ctx, parentState);
  SysYParser::ExpContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 42;
  enterRecursionRule(_localctx, 42, SysYParser::RuleExp, precedence);

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
    setState(214);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx)) {
    case 1: {
      _localctx = _tracker.createInstance<ParenExpContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;

      setState(204);
      match(SysYParser::LPAREN);
      setState(205);
      exp(0);
      setState(206);
      match(SysYParser::RPAREN);
      break;
    }

    case 2: {
      _localctx = _tracker.createInstance<VarExpContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(208);
      var();
      break;
    }

    case 3: {
      _localctx = _tracker.createInstance<NumberExpContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(209);
      number();
      break;
    }

    case 4: {
      _localctx = _tracker.createInstance<StringExpContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(210);
      string();
      break;
    }

    case 5: {
      _localctx = _tracker.createInstance<CallExpContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(211);
      call();
      break;
    }

    case 6: {
      _localctx = _tracker.createInstance<UnaryExpContext>(_localctx);
      _ctx = _localctx;
      previousContext = _localctx;
      setState(212);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 33566720) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(213);
      exp(7);
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(236);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 20, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(234);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 19, _ctx)) {
        case 1: {
          auto newContext = _tracker.createInstance<MultiplicativeExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExp);
          setState(216);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(217);
          _la = _input->LA(1);
          if (!((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & 114688) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(218);
          exp(7);
          break;
        }

        case 2: {
          auto newContext = _tracker.createInstance<AdditiveExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExp);
          setState(219);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(220);
          _la = _input->LA(1);
          if (!(_la == SysYParser::ADD

          || _la == SysYParser::SUB)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(221);
          exp(6);
          break;
        }

        case 3: {
          auto newContext = _tracker.createInstance<RelationExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExp);
          setState(222);

          if (!(precpred(_ctx, 4))) throw FailedPredicateException(this, "precpred(_ctx, 4)");
          setState(223);
          _la = _input->LA(1);
          if (!((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & 1966080) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(224);
          exp(5);
          break;
        }

        case 4: {
          auto newContext = _tracker.createInstance<EqualExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExp);
          setState(225);

          if (!(precpred(_ctx, 3))) throw FailedPredicateException(this, "precpred(_ctx, 3)");
          setState(226);
          _la = _input->LA(1);
          if (!(_la == SysYParser::EQ

          || _la == SysYParser::NE)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(227);
          exp(4);
          break;
        }

        case 5: {
          auto newContext = _tracker.createInstance<AndExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExp);
          setState(228);

          if (!(precpred(_ctx, 2))) throw FailedPredicateException(this, "precpred(_ctx, 2)");
          setState(229);
          match(SysYParser::AND);
          setState(230);
          exp(3);
          break;
        }

        case 6: {
          auto newContext = _tracker.createInstance<OrExpContext>(_tracker.createInstance<ExpContext>(parentContext, parentState));
          _localctx = newContext;
          pushNewRecursionContext(newContext, startState, RuleExp);
          setState(231);

          if (!(precpred(_ctx, 1))) throw FailedPredicateException(this, "precpred(_ctx, 1)");
          setState(232);
          match(SysYParser::OR);
          setState(233);
          exp(2);
          break;
        }

        default:
          break;
        } 
      }
      setState(238);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 20, _ctx);
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

tree::TerminalNode* SysYParser::CallContext::ID() {
  return getToken(SysYParser::ID, 0);
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
    setState(239);
    match(SysYParser::ID);
    setState(240);
    match(SysYParser::LPAREN);
    setState(242);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 257798713344) != 0)) {
      setState(241);
      funcRParams();
    }
    setState(244);
    match(SysYParser::RPAREN);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VarContext ------------------------------------------------------------------

SysYParser::VarContext::VarContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* SysYParser::VarContext::ID() {
  return getToken(SysYParser::ID, 0);
}

std::vector<tree::TerminalNode *> SysYParser::VarContext::LBRACKET() {
  return getTokens(SysYParser::LBRACKET);
}

tree::TerminalNode* SysYParser::VarContext::LBRACKET(size_t i) {
  return getToken(SysYParser::LBRACKET, i);
}

std::vector<SysYParser::ExpContext *> SysYParser::VarContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::VarContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::VarContext::RBRACKET() {
  return getTokens(SysYParser::RBRACKET);
}

tree::TerminalNode* SysYParser::VarContext::RBRACKET(size_t i) {
  return getToken(SysYParser::RBRACKET, i);
}


size_t SysYParser::VarContext::getRuleIndex() const {
  return SysYParser::RuleVar;
}


std::any SysYParser::VarContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<SysYVisitor*>(visitor))
    return parserVisitor->visitVar(this);
  else
    return visitor->visitChildren(this);
}

SysYParser::VarContext* SysYParser::var() {
  VarContext *_localctx = _tracker.createInstance<VarContext>(_ctx, getState());
  enterRule(_localctx, 46, SysYParser::RuleVar);

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
    setState(246);
    match(SysYParser::ID);
    setState(253);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 22, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(247);
        match(SysYParser::LBRACKET);
        setState(248);
        exp(0);
        setState(249);
        match(SysYParser::RBRACKET); 
      }
      setState(255);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 22, _ctx);
    }
   
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

tree::TerminalNode* SysYParser::LValueContext::ID() {
  return getToken(SysYParser::ID, 0);
}

std::vector<tree::TerminalNode *> SysYParser::LValueContext::LBRACKET() {
  return getTokens(SysYParser::LBRACKET);
}

tree::TerminalNode* SysYParser::LValueContext::LBRACKET(size_t i) {
  return getToken(SysYParser::LBRACKET, i);
}

std::vector<SysYParser::ExpContext *> SysYParser::LValueContext::exp() {
  return getRuleContexts<SysYParser::ExpContext>();
}

SysYParser::ExpContext* SysYParser::LValueContext::exp(size_t i) {
  return getRuleContext<SysYParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> SysYParser::LValueContext::RBRACKET() {
  return getTokens(SysYParser::RBRACKET);
}

tree::TerminalNode* SysYParser::LValueContext::RBRACKET(size_t i) {
  return getToken(SysYParser::RBRACKET, i);
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
  enterRule(_localctx, 48, SysYParser::RuleLValue);
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
    setState(256);
    match(SysYParser::ID);
    setState(263);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::LBRACKET) {
      setState(257);
      match(SysYParser::LBRACKET);
      setState(258);
      exp(0);
      setState(259);
      match(SysYParser::RBRACKET);
      setState(265);
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
  enterRule(_localctx, 50, SysYParser::RuleNumber);
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
    setState(266);
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
    setState(268);
    match(SysYParser::STRING);
   
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
  enterRule(_localctx, 54, SysYParser::RuleFuncRParams);
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
    setState(270);
    exp(0);
    setState(275);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == SysYParser::COMMA) {
      setState(271);
      match(SysYParser::COMMA);
      setState(272);
      exp(0);
      setState(277);
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

bool SysYParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 21: return expSempred(antlrcpp::downCast<ExpContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool SysYParser::expSempred(ExpContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 6);
    case 1: return precpred(_ctx, 5);
    case 2: return precpred(_ctx, 4);
    case 3: return precpred(_ctx, 3);
    case 4: return precpred(_ctx, 2);
    case 5: return precpred(_ctx, 1);

  default:
    break;
  }
  return true;
}

void SysYParser::initialize() {
  ::antlr4::internal::call_once(sysyParserOnceFlag, sysyParserInitialize);
}
