
// Generated from /home/jpy794/actions-runner/_work/sysy-compiler/sysy-compiler/src/ast/grammer/sysy.g4 by ANTLR 4.13.0


#include "sysyVisitor.h"

#include "sysyParser.h"


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
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
SysyParserStaticData *sysyParserStaticData = nullptr;

void sysyParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (sysyParserStaticData != nullptr) {
    return;
  }
#else
  assert(sysyParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<SysyParserStaticData>(
    std::vector<std::string>{
      "compUnit", "globalDef", "vardecl", "vardef", "varInit", "funcdef", 
      "funcparam", "block", "expStmt", "stmt", "lval", "funcCall", "parenExp", 
      "exp", "number"
    },
    std::vector<std::string>{
      "", "','", "';'", "'='", "'['", "']'", "'{'", "'}'", "'('", "')'", 
      "'if'", "'else'", "'while'", "'break'", "'continue'", "'return'", 
      "'const'", "'=='", "'!='", "'<'", "'>'", "'<='", "'>='", "'!'", "'&&'", 
      "'||'", "'+'", "'-'", "'*'", "'/'", "'%'", "'int'", "'float'", "'void'"
    },
    std::vector<std::string>{
      "", "Comma", "SemiColon", "Assign", "LeftBracket", "RightBracket", 
      "LeftBrace", "RightBrace", "LeftParen", "RightParen", "If", "Else", 
      "While", "Break", "Continue", "Return", "Const", "Equal", "NonEqual", 
      "Less", "Greater", "LessEqual", "GreaterEqual", "Not", "And", "Or", 
      "Plus", "Minus", "Multiply", "Divide", "Modulo", "Int", "Float", "Void", 
      "Identifier", "IntConst", "FloatConst", "LineComment", "BlockComment", 
      "WhiteSpace"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,39,224,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,1,0,4,0,32,8,0,11,0,12,0,33,1,1,1,1,3,1,38,8,1,1,2,3,2,41,8,2,1,2,
  	1,2,1,2,1,2,5,2,47,8,2,10,2,12,2,50,9,2,1,2,1,2,1,3,1,3,1,3,1,3,1,3,5,
  	3,59,8,3,10,3,12,3,62,9,3,1,3,1,3,3,3,66,8,3,1,4,1,4,1,4,1,4,5,4,72,8,
  	4,10,4,12,4,75,9,4,3,4,77,8,4,1,4,1,4,3,4,81,8,4,1,5,1,5,1,5,1,5,1,5,
  	1,5,5,5,89,8,5,10,5,12,5,92,9,5,3,5,94,8,5,1,5,1,5,1,5,1,6,1,6,1,6,1,
  	6,1,6,1,6,1,6,1,6,5,6,107,8,6,10,6,12,6,110,9,6,3,6,112,8,6,1,7,1,7,5,
  	7,116,8,7,10,7,12,7,119,9,7,1,7,1,7,1,8,3,8,124,8,8,1,8,1,8,1,9,1,9,1,
  	9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,3,9,142,8,9,1,9,1,9,1,9,
  	1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,1,9,3,9,156,8,9,1,9,1,9,3,9,160,8,9,1,
  	10,1,10,1,10,1,10,1,10,5,10,167,8,10,10,10,12,10,170,9,10,1,11,1,11,1,
  	11,1,11,1,11,5,11,177,8,11,10,11,12,11,180,9,11,3,11,182,8,11,1,11,1,
  	11,1,12,1,12,1,12,1,12,1,13,1,13,1,13,1,13,1,13,1,13,1,13,3,13,197,8,
  	13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,13,1,
  	13,1,13,1,13,1,13,1,13,5,13,217,8,13,10,13,12,13,220,9,13,1,14,1,14,1,
  	14,0,1,26,15,0,2,4,6,8,10,12,14,16,18,20,22,24,26,28,0,8,1,0,31,32,1,
  	0,31,33,2,0,23,23,26,27,1,0,28,30,1,0,26,27,1,0,19,22,1,0,17,18,1,0,35,
  	36,246,0,31,1,0,0,0,2,37,1,0,0,0,4,40,1,0,0,0,6,53,1,0,0,0,8,80,1,0,0,
  	0,10,82,1,0,0,0,12,98,1,0,0,0,14,113,1,0,0,0,16,123,1,0,0,0,18,159,1,
  	0,0,0,20,161,1,0,0,0,22,171,1,0,0,0,24,185,1,0,0,0,26,196,1,0,0,0,28,
  	221,1,0,0,0,30,32,3,2,1,0,31,30,1,0,0,0,32,33,1,0,0,0,33,31,1,0,0,0,33,
  	34,1,0,0,0,34,1,1,0,0,0,35,38,3,4,2,0,36,38,3,10,5,0,37,35,1,0,0,0,37,
  	36,1,0,0,0,38,3,1,0,0,0,39,41,5,16,0,0,40,39,1,0,0,0,40,41,1,0,0,0,41,
  	42,1,0,0,0,42,43,7,0,0,0,43,48,3,6,3,0,44,45,5,1,0,0,45,47,3,6,3,0,46,
  	44,1,0,0,0,47,50,1,0,0,0,48,46,1,0,0,0,48,49,1,0,0,0,49,51,1,0,0,0,50,
  	48,1,0,0,0,51,52,5,2,0,0,52,5,1,0,0,0,53,60,5,34,0,0,54,55,5,4,0,0,55,
  	56,3,26,13,0,56,57,5,5,0,0,57,59,1,0,0,0,58,54,1,0,0,0,59,62,1,0,0,0,
  	60,58,1,0,0,0,60,61,1,0,0,0,61,65,1,0,0,0,62,60,1,0,0,0,63,64,5,3,0,0,
  	64,66,3,8,4,0,65,63,1,0,0,0,65,66,1,0,0,0,66,7,1,0,0,0,67,76,5,6,0,0,
  	68,73,3,8,4,0,69,70,5,1,0,0,70,72,3,8,4,0,71,69,1,0,0,0,72,75,1,0,0,0,
  	73,71,1,0,0,0,73,74,1,0,0,0,74,77,1,0,0,0,75,73,1,0,0,0,76,68,1,0,0,0,
  	76,77,1,0,0,0,77,78,1,0,0,0,78,81,5,7,0,0,79,81,3,26,13,0,80,67,1,0,0,
  	0,80,79,1,0,0,0,81,9,1,0,0,0,82,83,7,1,0,0,83,84,5,34,0,0,84,93,5,8,0,
  	0,85,90,3,12,6,0,86,87,5,1,0,0,87,89,3,12,6,0,88,86,1,0,0,0,89,92,1,0,
  	0,0,90,88,1,0,0,0,90,91,1,0,0,0,91,94,1,0,0,0,92,90,1,0,0,0,93,85,1,0,
  	0,0,93,94,1,0,0,0,94,95,1,0,0,0,95,96,5,9,0,0,96,97,3,14,7,0,97,11,1,
  	0,0,0,98,99,7,0,0,0,99,111,5,34,0,0,100,101,5,4,0,0,101,108,5,5,0,0,102,
  	103,5,4,0,0,103,104,3,26,13,0,104,105,5,5,0,0,105,107,1,0,0,0,106,102,
  	1,0,0,0,107,110,1,0,0,0,108,106,1,0,0,0,108,109,1,0,0,0,109,112,1,0,0,
  	0,110,108,1,0,0,0,111,100,1,0,0,0,111,112,1,0,0,0,112,13,1,0,0,0,113,
  	117,5,6,0,0,114,116,3,18,9,0,115,114,1,0,0,0,116,119,1,0,0,0,117,115,
  	1,0,0,0,117,118,1,0,0,0,118,120,1,0,0,0,119,117,1,0,0,0,120,121,5,7,0,
  	0,121,15,1,0,0,0,122,124,3,26,13,0,123,122,1,0,0,0,123,124,1,0,0,0,124,
  	125,1,0,0,0,125,126,5,2,0,0,126,17,1,0,0,0,127,128,3,20,10,0,128,129,
  	5,3,0,0,129,130,3,26,13,0,130,131,5,2,0,0,131,160,1,0,0,0,132,160,3,16,
  	8,0,133,160,3,14,7,0,134,135,5,10,0,0,135,136,5,8,0,0,136,137,3,26,13,
  	0,137,138,5,9,0,0,138,141,3,18,9,0,139,140,5,11,0,0,140,142,3,18,9,0,
  	141,139,1,0,0,0,141,142,1,0,0,0,142,160,1,0,0,0,143,144,5,12,0,0,144,
  	145,5,8,0,0,145,146,3,26,13,0,146,147,5,9,0,0,147,148,3,18,9,0,148,160,
  	1,0,0,0,149,150,5,13,0,0,150,160,5,2,0,0,151,152,5,14,0,0,152,160,5,2,
  	0,0,153,155,5,15,0,0,154,156,3,26,13,0,155,154,1,0,0,0,155,156,1,0,0,
  	0,156,157,1,0,0,0,157,160,5,2,0,0,158,160,3,4,2,0,159,127,1,0,0,0,159,
  	132,1,0,0,0,159,133,1,0,0,0,159,134,1,0,0,0,159,143,1,0,0,0,159,149,1,
  	0,0,0,159,151,1,0,0,0,159,153,1,0,0,0,159,158,1,0,0,0,160,19,1,0,0,0,
  	161,168,5,34,0,0,162,163,5,4,0,0,163,164,3,26,13,0,164,165,5,5,0,0,165,
  	167,1,0,0,0,166,162,1,0,0,0,167,170,1,0,0,0,168,166,1,0,0,0,168,169,1,
  	0,0,0,169,21,1,0,0,0,170,168,1,0,0,0,171,172,5,34,0,0,172,181,5,8,0,0,
  	173,178,3,26,13,0,174,175,5,1,0,0,175,177,3,26,13,0,176,174,1,0,0,0,177,
  	180,1,0,0,0,178,176,1,0,0,0,178,179,1,0,0,0,179,182,1,0,0,0,180,178,1,
  	0,0,0,181,173,1,0,0,0,181,182,1,0,0,0,182,183,1,0,0,0,183,184,5,9,0,0,
  	184,23,1,0,0,0,185,186,5,8,0,0,186,187,3,26,13,0,187,188,5,9,0,0,188,
  	25,1,0,0,0,189,190,6,13,-1,0,190,191,7,2,0,0,191,197,3,26,13,11,192,197,
  	3,24,12,0,193,197,3,28,14,0,194,197,3,20,10,0,195,197,3,22,11,0,196,189,
  	1,0,0,0,196,192,1,0,0,0,196,193,1,0,0,0,196,194,1,0,0,0,196,195,1,0,0,
  	0,197,218,1,0,0,0,198,199,10,10,0,0,199,200,7,3,0,0,200,217,3,26,13,11,
  	201,202,10,9,0,0,202,203,7,4,0,0,203,217,3,26,13,10,204,205,10,8,0,0,
  	205,206,7,5,0,0,206,217,3,26,13,9,207,208,10,7,0,0,208,209,7,6,0,0,209,
  	217,3,26,13,8,210,211,10,6,0,0,211,212,5,24,0,0,212,217,3,26,13,7,213,
  	214,10,5,0,0,214,215,5,25,0,0,215,217,3,26,13,6,216,198,1,0,0,0,216,201,
  	1,0,0,0,216,204,1,0,0,0,216,207,1,0,0,0,216,210,1,0,0,0,216,213,1,0,0,
  	0,217,220,1,0,0,0,218,216,1,0,0,0,218,219,1,0,0,0,219,27,1,0,0,0,220,
  	218,1,0,0,0,221,222,7,7,0,0,222,29,1,0,0,0,24,33,37,40,48,60,65,73,76,
  	80,90,93,108,111,117,123,141,155,159,168,178,181,196,216,218
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

sysyParser::sysyParser(TokenStream *input) : sysyParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

sysyParser::sysyParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  sysyParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *sysyParserStaticData->atn, sysyParserStaticData->decisionToDFA, sysyParserStaticData->sharedContextCache, options);
}

sysyParser::~sysyParser() {
  delete _interpreter;
}

const atn::ATN& sysyParser::getATN() const {
  return *sysyParserStaticData->atn;
}

std::string sysyParser::getGrammarFileName() const {
  return "sysy.g4";
}

const std::vector<std::string>& sysyParser::getRuleNames() const {
  return sysyParserStaticData->ruleNames;
}

const dfa::Vocabulary& sysyParser::getVocabulary() const {
  return sysyParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView sysyParser::getSerializedATN() const {
  return sysyParserStaticData->serializedATN;
}


//----------------- CompUnitContext ------------------------------------------------------------------

sysyParser::CompUnitContext::CompUnitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<sysyParser::GlobalDefContext *> sysyParser::CompUnitContext::globalDef() {
  return getRuleContexts<sysyParser::GlobalDefContext>();
}

sysyParser::GlobalDefContext* sysyParser::CompUnitContext::globalDef(size_t i) {
  return getRuleContext<sysyParser::GlobalDefContext>(i);
}


size_t sysyParser::CompUnitContext::getRuleIndex() const {
  return sysyParser::RuleCompUnit;
}


std::any sysyParser::CompUnitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitCompUnit(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::CompUnitContext* sysyParser::compUnit() {
  CompUnitContext *_localctx = _tracker.createInstance<CompUnitContext>(_ctx, getState());
  enterRule(_localctx, 0, sysyParser::RuleCompUnit);
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
    setState(31); 
    _errHandler->sync(this);
    _la = _input->LA(1);
    do {
      setState(30);
      globalDef();
      setState(33); 
      _errHandler->sync(this);
      _la = _input->LA(1);
    } while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 15032451072) != 0));
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- GlobalDefContext ------------------------------------------------------------------

sysyParser::GlobalDefContext::GlobalDefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

sysyParser::VardeclContext* sysyParser::GlobalDefContext::vardecl() {
  return getRuleContext<sysyParser::VardeclContext>(0);
}

sysyParser::FuncdefContext* sysyParser::GlobalDefContext::funcdef() {
  return getRuleContext<sysyParser::FuncdefContext>(0);
}


size_t sysyParser::GlobalDefContext::getRuleIndex() const {
  return sysyParser::RuleGlobalDef;
}


std::any sysyParser::GlobalDefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitGlobalDef(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::GlobalDefContext* sysyParser::globalDef() {
  GlobalDefContext *_localctx = _tracker.createInstance<GlobalDefContext>(_ctx, getState());
  enterRule(_localctx, 2, sysyParser::RuleGlobalDef);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(37);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 1, _ctx)) {
    case 1: {
      setState(35);
      vardecl();
      break;
    }

    case 2: {
      setState(36);
      funcdef();
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

//----------------- VardeclContext ------------------------------------------------------------------

sysyParser::VardeclContext::VardeclContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<sysyParser::VardefContext *> sysyParser::VardeclContext::vardef() {
  return getRuleContexts<sysyParser::VardefContext>();
}

sysyParser::VardefContext* sysyParser::VardeclContext::vardef(size_t i) {
  return getRuleContext<sysyParser::VardefContext>(i);
}

tree::TerminalNode* sysyParser::VardeclContext::SemiColon() {
  return getToken(sysyParser::SemiColon, 0);
}

tree::TerminalNode* sysyParser::VardeclContext::Int() {
  return getToken(sysyParser::Int, 0);
}

tree::TerminalNode* sysyParser::VardeclContext::Float() {
  return getToken(sysyParser::Float, 0);
}

tree::TerminalNode* sysyParser::VardeclContext::Const() {
  return getToken(sysyParser::Const, 0);
}

std::vector<tree::TerminalNode *> sysyParser::VardeclContext::Comma() {
  return getTokens(sysyParser::Comma);
}

tree::TerminalNode* sysyParser::VardeclContext::Comma(size_t i) {
  return getToken(sysyParser::Comma, i);
}


size_t sysyParser::VardeclContext::getRuleIndex() const {
  return sysyParser::RuleVardecl;
}


std::any sysyParser::VardeclContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitVardecl(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::VardeclContext* sysyParser::vardecl() {
  VardeclContext *_localctx = _tracker.createInstance<VardeclContext>(_ctx, getState());
  enterRule(_localctx, 4, sysyParser::RuleVardecl);
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
    setState(40);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == sysyParser::Const) {
      setState(39);
      match(sysyParser::Const);
    }
    setState(42);
    _la = _input->LA(1);
    if (!(_la == sysyParser::Int

    || _la == sysyParser::Float)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(43);
    vardef();
    setState(48);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == sysyParser::Comma) {
      setState(44);
      match(sysyParser::Comma);
      setState(45);
      vardef();
      setState(50);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(51);
    match(sysyParser::SemiColon);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VardefContext ------------------------------------------------------------------

sysyParser::VardefContext::VardefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::VardefContext::Identifier() {
  return getToken(sysyParser::Identifier, 0);
}

std::vector<tree::TerminalNode *> sysyParser::VardefContext::LeftBracket() {
  return getTokens(sysyParser::LeftBracket);
}

tree::TerminalNode* sysyParser::VardefContext::LeftBracket(size_t i) {
  return getToken(sysyParser::LeftBracket, i);
}

std::vector<sysyParser::ExpContext *> sysyParser::VardefContext::exp() {
  return getRuleContexts<sysyParser::ExpContext>();
}

sysyParser::ExpContext* sysyParser::VardefContext::exp(size_t i) {
  return getRuleContext<sysyParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> sysyParser::VardefContext::RightBracket() {
  return getTokens(sysyParser::RightBracket);
}

tree::TerminalNode* sysyParser::VardefContext::RightBracket(size_t i) {
  return getToken(sysyParser::RightBracket, i);
}

tree::TerminalNode* sysyParser::VardefContext::Assign() {
  return getToken(sysyParser::Assign, 0);
}

sysyParser::VarInitContext* sysyParser::VardefContext::varInit() {
  return getRuleContext<sysyParser::VarInitContext>(0);
}


size_t sysyParser::VardefContext::getRuleIndex() const {
  return sysyParser::RuleVardef;
}


std::any sysyParser::VardefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitVardef(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::VardefContext* sysyParser::vardef() {
  VardefContext *_localctx = _tracker.createInstance<VardefContext>(_ctx, getState());
  enterRule(_localctx, 6, sysyParser::RuleVardef);
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
    setState(53);
    match(sysyParser::Identifier);
    setState(60);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == sysyParser::LeftBracket) {
      setState(54);
      match(sysyParser::LeftBracket);
      setState(55);
      exp(0);
      setState(56);
      match(sysyParser::RightBracket);
      setState(62);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(65);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == sysyParser::Assign) {
      setState(63);
      match(sysyParser::Assign);
      setState(64);
      varInit();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VarInitContext ------------------------------------------------------------------

sysyParser::VarInitContext::VarInitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::VarInitContext::LeftBrace() {
  return getToken(sysyParser::LeftBrace, 0);
}

tree::TerminalNode* sysyParser::VarInitContext::RightBrace() {
  return getToken(sysyParser::RightBrace, 0);
}

std::vector<sysyParser::VarInitContext *> sysyParser::VarInitContext::varInit() {
  return getRuleContexts<sysyParser::VarInitContext>();
}

sysyParser::VarInitContext* sysyParser::VarInitContext::varInit(size_t i) {
  return getRuleContext<sysyParser::VarInitContext>(i);
}

std::vector<tree::TerminalNode *> sysyParser::VarInitContext::Comma() {
  return getTokens(sysyParser::Comma);
}

tree::TerminalNode* sysyParser::VarInitContext::Comma(size_t i) {
  return getToken(sysyParser::Comma, i);
}

sysyParser::ExpContext* sysyParser::VarInitContext::exp() {
  return getRuleContext<sysyParser::ExpContext>(0);
}


size_t sysyParser::VarInitContext::getRuleIndex() const {
  return sysyParser::RuleVarInit;
}


std::any sysyParser::VarInitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitVarInit(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::VarInitContext* sysyParser::varInit() {
  VarInitContext *_localctx = _tracker.createInstance<VarInitContext>(_ctx, getState());
  enterRule(_localctx, 8, sysyParser::RuleVarInit);
  size_t _la = 0;

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
      case sysyParser::LeftBrace: {
        enterOuterAlt(_localctx, 1);
        setState(67);
        match(sysyParser::LeftBrace);
        setState(76);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 120468799808) != 0)) {
          setState(68);
          varInit();
          setState(73);
          _errHandler->sync(this);
          _la = _input->LA(1);
          while (_la == sysyParser::Comma) {
            setState(69);
            match(sysyParser::Comma);
            setState(70);
            varInit();
            setState(75);
            _errHandler->sync(this);
            _la = _input->LA(1);
          }
        }
        setState(78);
        match(sysyParser::RightBrace);
        break;
      }

      case sysyParser::LeftParen:
      case sysyParser::Not:
      case sysyParser::Plus:
      case sysyParser::Minus:
      case sysyParser::Identifier:
      case sysyParser::IntConst:
      case sysyParser::FloatConst: {
        enterOuterAlt(_localctx, 2);
        setState(79);
        exp(0);
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

//----------------- FuncdefContext ------------------------------------------------------------------

sysyParser::FuncdefContext::FuncdefContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::FuncdefContext::Identifier() {
  return getToken(sysyParser::Identifier, 0);
}

tree::TerminalNode* sysyParser::FuncdefContext::LeftParen() {
  return getToken(sysyParser::LeftParen, 0);
}

tree::TerminalNode* sysyParser::FuncdefContext::RightParen() {
  return getToken(sysyParser::RightParen, 0);
}

sysyParser::BlockContext* sysyParser::FuncdefContext::block() {
  return getRuleContext<sysyParser::BlockContext>(0);
}

tree::TerminalNode* sysyParser::FuncdefContext::Void() {
  return getToken(sysyParser::Void, 0);
}

tree::TerminalNode* sysyParser::FuncdefContext::Int() {
  return getToken(sysyParser::Int, 0);
}

tree::TerminalNode* sysyParser::FuncdefContext::Float() {
  return getToken(sysyParser::Float, 0);
}

std::vector<sysyParser::FuncparamContext *> sysyParser::FuncdefContext::funcparam() {
  return getRuleContexts<sysyParser::FuncparamContext>();
}

sysyParser::FuncparamContext* sysyParser::FuncdefContext::funcparam(size_t i) {
  return getRuleContext<sysyParser::FuncparamContext>(i);
}

std::vector<tree::TerminalNode *> sysyParser::FuncdefContext::Comma() {
  return getTokens(sysyParser::Comma);
}

tree::TerminalNode* sysyParser::FuncdefContext::Comma(size_t i) {
  return getToken(sysyParser::Comma, i);
}


size_t sysyParser::FuncdefContext::getRuleIndex() const {
  return sysyParser::RuleFuncdef;
}


std::any sysyParser::FuncdefContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitFuncdef(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::FuncdefContext* sysyParser::funcdef() {
  FuncdefContext *_localctx = _tracker.createInstance<FuncdefContext>(_ctx, getState());
  enterRule(_localctx, 10, sysyParser::RuleFuncdef);
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
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 15032385536) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(83);
    match(sysyParser::Identifier);
    setState(84);
    match(sysyParser::LeftParen);
    setState(93);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == sysyParser::Int

    || _la == sysyParser::Float) {
      setState(85);
      funcparam();
      setState(90);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == sysyParser::Comma) {
        setState(86);
        match(sysyParser::Comma);
        setState(87);
        funcparam();
        setState(92);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
    }
    setState(95);
    match(sysyParser::RightParen);
    setState(96);
    block();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FuncparamContext ------------------------------------------------------------------

sysyParser::FuncparamContext::FuncparamContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::FuncparamContext::Identifier() {
  return getToken(sysyParser::Identifier, 0);
}

tree::TerminalNode* sysyParser::FuncparamContext::Int() {
  return getToken(sysyParser::Int, 0);
}

tree::TerminalNode* sysyParser::FuncparamContext::Float() {
  return getToken(sysyParser::Float, 0);
}

std::vector<tree::TerminalNode *> sysyParser::FuncparamContext::LeftBracket() {
  return getTokens(sysyParser::LeftBracket);
}

tree::TerminalNode* sysyParser::FuncparamContext::LeftBracket(size_t i) {
  return getToken(sysyParser::LeftBracket, i);
}

std::vector<tree::TerminalNode *> sysyParser::FuncparamContext::RightBracket() {
  return getTokens(sysyParser::RightBracket);
}

tree::TerminalNode* sysyParser::FuncparamContext::RightBracket(size_t i) {
  return getToken(sysyParser::RightBracket, i);
}

std::vector<sysyParser::ExpContext *> sysyParser::FuncparamContext::exp() {
  return getRuleContexts<sysyParser::ExpContext>();
}

sysyParser::ExpContext* sysyParser::FuncparamContext::exp(size_t i) {
  return getRuleContext<sysyParser::ExpContext>(i);
}


size_t sysyParser::FuncparamContext::getRuleIndex() const {
  return sysyParser::RuleFuncparam;
}


std::any sysyParser::FuncparamContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitFuncparam(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::FuncparamContext* sysyParser::funcparam() {
  FuncparamContext *_localctx = _tracker.createInstance<FuncparamContext>(_ctx, getState());
  enterRule(_localctx, 12, sysyParser::RuleFuncparam);
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
    setState(98);
    _la = _input->LA(1);
    if (!(_la == sysyParser::Int

    || _la == sysyParser::Float)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
    setState(99);
    match(sysyParser::Identifier);
    setState(111);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == sysyParser::LeftBracket) {
      setState(100);
      match(sysyParser::LeftBracket);
      setState(101);
      match(sysyParser::RightBracket);
      setState(108);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == sysyParser::LeftBracket) {
        setState(102);
        match(sysyParser::LeftBracket);
        setState(103);
        exp(0);
        setState(104);
        match(sysyParser::RightBracket);
        setState(110);
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

sysyParser::BlockContext::BlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::BlockContext::LeftBrace() {
  return getToken(sysyParser::LeftBrace, 0);
}

tree::TerminalNode* sysyParser::BlockContext::RightBrace() {
  return getToken(sysyParser::RightBrace, 0);
}

std::vector<sysyParser::StmtContext *> sysyParser::BlockContext::stmt() {
  return getRuleContexts<sysyParser::StmtContext>();
}

sysyParser::StmtContext* sysyParser::BlockContext::stmt(size_t i) {
  return getRuleContext<sysyParser::StmtContext>(i);
}


size_t sysyParser::BlockContext::getRuleIndex() const {
  return sysyParser::RuleBlock;
}


std::any sysyParser::BlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitBlock(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::BlockContext* sysyParser::block() {
  BlockContext *_localctx = _tracker.createInstance<BlockContext>(_ctx, getState());
  enterRule(_localctx, 14, sysyParser::RuleBlock);
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
    setState(113);
    match(sysyParser::LeftBrace);
    setState(117);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 126911378756) != 0)) {
      setState(114);
      stmt();
      setState(119);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(120);
    match(sysyParser::RightBrace);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpStmtContext ------------------------------------------------------------------

sysyParser::ExpStmtContext::ExpStmtContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::ExpStmtContext::SemiColon() {
  return getToken(sysyParser::SemiColon, 0);
}

sysyParser::ExpContext* sysyParser::ExpStmtContext::exp() {
  return getRuleContext<sysyParser::ExpContext>(0);
}


size_t sysyParser::ExpStmtContext::getRuleIndex() const {
  return sysyParser::RuleExpStmt;
}


std::any sysyParser::ExpStmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitExpStmt(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::ExpStmtContext* sysyParser::expStmt() {
  ExpStmtContext *_localctx = _tracker.createInstance<ExpStmtContext>(_ctx, getState());
  enterRule(_localctx, 16, sysyParser::RuleExpStmt);
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
    setState(123);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 120468799744) != 0)) {
      setState(122);
      exp(0);
    }
    setState(125);
    match(sysyParser::SemiColon);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StmtContext ------------------------------------------------------------------

sysyParser::StmtContext::StmtContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

sysyParser::LvalContext* sysyParser::StmtContext::lval() {
  return getRuleContext<sysyParser::LvalContext>(0);
}

tree::TerminalNode* sysyParser::StmtContext::Assign() {
  return getToken(sysyParser::Assign, 0);
}

sysyParser::ExpContext* sysyParser::StmtContext::exp() {
  return getRuleContext<sysyParser::ExpContext>(0);
}

tree::TerminalNode* sysyParser::StmtContext::SemiColon() {
  return getToken(sysyParser::SemiColon, 0);
}

sysyParser::ExpStmtContext* sysyParser::StmtContext::expStmt() {
  return getRuleContext<sysyParser::ExpStmtContext>(0);
}

sysyParser::BlockContext* sysyParser::StmtContext::block() {
  return getRuleContext<sysyParser::BlockContext>(0);
}

tree::TerminalNode* sysyParser::StmtContext::If() {
  return getToken(sysyParser::If, 0);
}

tree::TerminalNode* sysyParser::StmtContext::LeftParen() {
  return getToken(sysyParser::LeftParen, 0);
}

tree::TerminalNode* sysyParser::StmtContext::RightParen() {
  return getToken(sysyParser::RightParen, 0);
}

std::vector<sysyParser::StmtContext *> sysyParser::StmtContext::stmt() {
  return getRuleContexts<sysyParser::StmtContext>();
}

sysyParser::StmtContext* sysyParser::StmtContext::stmt(size_t i) {
  return getRuleContext<sysyParser::StmtContext>(i);
}

tree::TerminalNode* sysyParser::StmtContext::Else() {
  return getToken(sysyParser::Else, 0);
}

tree::TerminalNode* sysyParser::StmtContext::While() {
  return getToken(sysyParser::While, 0);
}

tree::TerminalNode* sysyParser::StmtContext::Break() {
  return getToken(sysyParser::Break, 0);
}

tree::TerminalNode* sysyParser::StmtContext::Continue() {
  return getToken(sysyParser::Continue, 0);
}

tree::TerminalNode* sysyParser::StmtContext::Return() {
  return getToken(sysyParser::Return, 0);
}

sysyParser::VardeclContext* sysyParser::StmtContext::vardecl() {
  return getRuleContext<sysyParser::VardeclContext>(0);
}


size_t sysyParser::StmtContext::getRuleIndex() const {
  return sysyParser::RuleStmt;
}


std::any sysyParser::StmtContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitStmt(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::StmtContext* sysyParser::stmt() {
  StmtContext *_localctx = _tracker.createInstance<StmtContext>(_ctx, getState());
  enterRule(_localctx, 18, sysyParser::RuleStmt);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(159);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 17, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(127);
      lval();
      setState(128);
      match(sysyParser::Assign);
      setState(129);
      exp(0);
      setState(130);
      match(sysyParser::SemiColon);
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(132);
      expStmt();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(133);
      block();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(134);
      match(sysyParser::If);
      setState(135);
      match(sysyParser::LeftParen);
      setState(136);
      exp(0);
      setState(137);
      match(sysyParser::RightParen);
      setState(138);
      stmt();
      setState(141);
      _errHandler->sync(this);

      switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 15, _ctx)) {
      case 1: {
        setState(139);
        match(sysyParser::Else);
        setState(140);
        stmt();
        break;
      }

      default:
        break;
      }
      break;
    }

    case 5: {
      enterOuterAlt(_localctx, 5);
      setState(143);
      match(sysyParser::While);
      setState(144);
      match(sysyParser::LeftParen);
      setState(145);
      exp(0);
      setState(146);
      match(sysyParser::RightParen);
      setState(147);
      stmt();
      break;
    }

    case 6: {
      enterOuterAlt(_localctx, 6);
      setState(149);
      match(sysyParser::Break);
      setState(150);
      match(sysyParser::SemiColon);
      break;
    }

    case 7: {
      enterOuterAlt(_localctx, 7);
      setState(151);
      match(sysyParser::Continue);
      setState(152);
      match(sysyParser::SemiColon);
      break;
    }

    case 8: {
      enterOuterAlt(_localctx, 8);
      setState(153);
      match(sysyParser::Return);
      setState(155);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if ((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 120468799744) != 0)) {
        setState(154);
        exp(0);
      }
      setState(157);
      match(sysyParser::SemiColon);
      break;
    }

    case 9: {
      enterOuterAlt(_localctx, 9);
      setState(158);
      vardecl();
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

//----------------- LvalContext ------------------------------------------------------------------

sysyParser::LvalContext::LvalContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::LvalContext::Identifier() {
  return getToken(sysyParser::Identifier, 0);
}

std::vector<tree::TerminalNode *> sysyParser::LvalContext::LeftBracket() {
  return getTokens(sysyParser::LeftBracket);
}

tree::TerminalNode* sysyParser::LvalContext::LeftBracket(size_t i) {
  return getToken(sysyParser::LeftBracket, i);
}

std::vector<sysyParser::ExpContext *> sysyParser::LvalContext::exp() {
  return getRuleContexts<sysyParser::ExpContext>();
}

sysyParser::ExpContext* sysyParser::LvalContext::exp(size_t i) {
  return getRuleContext<sysyParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> sysyParser::LvalContext::RightBracket() {
  return getTokens(sysyParser::RightBracket);
}

tree::TerminalNode* sysyParser::LvalContext::RightBracket(size_t i) {
  return getToken(sysyParser::RightBracket, i);
}


size_t sysyParser::LvalContext::getRuleIndex() const {
  return sysyParser::RuleLval;
}


std::any sysyParser::LvalContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitLval(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::LvalContext* sysyParser::lval() {
  LvalContext *_localctx = _tracker.createInstance<LvalContext>(_ctx, getState());
  enterRule(_localctx, 20, sysyParser::RuleLval);

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
    setState(161);
    match(sysyParser::Identifier);
    setState(168);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        setState(162);
        match(sysyParser::LeftBracket);
        setState(163);
        exp(0);
        setState(164);
        match(sysyParser::RightBracket); 
      }
      setState(170);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 18, _ctx);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FuncCallContext ------------------------------------------------------------------

sysyParser::FuncCallContext::FuncCallContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::FuncCallContext::Identifier() {
  return getToken(sysyParser::Identifier, 0);
}

tree::TerminalNode* sysyParser::FuncCallContext::LeftParen() {
  return getToken(sysyParser::LeftParen, 0);
}

tree::TerminalNode* sysyParser::FuncCallContext::RightParen() {
  return getToken(sysyParser::RightParen, 0);
}

std::vector<sysyParser::ExpContext *> sysyParser::FuncCallContext::exp() {
  return getRuleContexts<sysyParser::ExpContext>();
}

sysyParser::ExpContext* sysyParser::FuncCallContext::exp(size_t i) {
  return getRuleContext<sysyParser::ExpContext>(i);
}

std::vector<tree::TerminalNode *> sysyParser::FuncCallContext::Comma() {
  return getTokens(sysyParser::Comma);
}

tree::TerminalNode* sysyParser::FuncCallContext::Comma(size_t i) {
  return getToken(sysyParser::Comma, i);
}


size_t sysyParser::FuncCallContext::getRuleIndex() const {
  return sysyParser::RuleFuncCall;
}


std::any sysyParser::FuncCallContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitFuncCall(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::FuncCallContext* sysyParser::funcCall() {
  FuncCallContext *_localctx = _tracker.createInstance<FuncCallContext>(_ctx, getState());
  enterRule(_localctx, 22, sysyParser::RuleFuncCall);
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
    match(sysyParser::Identifier);
    setState(172);
    match(sysyParser::LeftParen);
    setState(181);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 120468799744) != 0)) {
      setState(173);
      exp(0);
      setState(178);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == sysyParser::Comma) {
        setState(174);
        match(sysyParser::Comma);
        setState(175);
        exp(0);
        setState(180);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
    }
    setState(183);
    match(sysyParser::RightParen);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParenExpContext ------------------------------------------------------------------

sysyParser::ParenExpContext::ParenExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::ParenExpContext::LeftParen() {
  return getToken(sysyParser::LeftParen, 0);
}

sysyParser::ExpContext* sysyParser::ParenExpContext::exp() {
  return getRuleContext<sysyParser::ExpContext>(0);
}

tree::TerminalNode* sysyParser::ParenExpContext::RightParen() {
  return getToken(sysyParser::RightParen, 0);
}


size_t sysyParser::ParenExpContext::getRuleIndex() const {
  return sysyParser::RuleParenExp;
}


std::any sysyParser::ParenExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitParenExp(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::ParenExpContext* sysyParser::parenExp() {
  ParenExpContext *_localctx = _tracker.createInstance<ParenExpContext>(_ctx, getState());
  enterRule(_localctx, 24, sysyParser::RuleParenExp);

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
    match(sysyParser::LeftParen);
    setState(186);
    exp(0);
    setState(187);
    match(sysyParser::RightParen);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpContext ------------------------------------------------------------------

sysyParser::ExpContext::ExpContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<sysyParser::ExpContext *> sysyParser::ExpContext::exp() {
  return getRuleContexts<sysyParser::ExpContext>();
}

sysyParser::ExpContext* sysyParser::ExpContext::exp(size_t i) {
  return getRuleContext<sysyParser::ExpContext>(i);
}

tree::TerminalNode* sysyParser::ExpContext::Plus() {
  return getToken(sysyParser::Plus, 0);
}

tree::TerminalNode* sysyParser::ExpContext::Minus() {
  return getToken(sysyParser::Minus, 0);
}

tree::TerminalNode* sysyParser::ExpContext::Not() {
  return getToken(sysyParser::Not, 0);
}

sysyParser::ParenExpContext* sysyParser::ExpContext::parenExp() {
  return getRuleContext<sysyParser::ParenExpContext>(0);
}

sysyParser::NumberContext* sysyParser::ExpContext::number() {
  return getRuleContext<sysyParser::NumberContext>(0);
}

sysyParser::LvalContext* sysyParser::ExpContext::lval() {
  return getRuleContext<sysyParser::LvalContext>(0);
}

sysyParser::FuncCallContext* sysyParser::ExpContext::funcCall() {
  return getRuleContext<sysyParser::FuncCallContext>(0);
}

tree::TerminalNode* sysyParser::ExpContext::Multiply() {
  return getToken(sysyParser::Multiply, 0);
}

tree::TerminalNode* sysyParser::ExpContext::Divide() {
  return getToken(sysyParser::Divide, 0);
}

tree::TerminalNode* sysyParser::ExpContext::Modulo() {
  return getToken(sysyParser::Modulo, 0);
}

tree::TerminalNode* sysyParser::ExpContext::Less() {
  return getToken(sysyParser::Less, 0);
}

tree::TerminalNode* sysyParser::ExpContext::Greater() {
  return getToken(sysyParser::Greater, 0);
}

tree::TerminalNode* sysyParser::ExpContext::LessEqual() {
  return getToken(sysyParser::LessEqual, 0);
}

tree::TerminalNode* sysyParser::ExpContext::GreaterEqual() {
  return getToken(sysyParser::GreaterEqual, 0);
}

tree::TerminalNode* sysyParser::ExpContext::Equal() {
  return getToken(sysyParser::Equal, 0);
}

tree::TerminalNode* sysyParser::ExpContext::NonEqual() {
  return getToken(sysyParser::NonEqual, 0);
}

tree::TerminalNode* sysyParser::ExpContext::And() {
  return getToken(sysyParser::And, 0);
}

tree::TerminalNode* sysyParser::ExpContext::Or() {
  return getToken(sysyParser::Or, 0);
}


size_t sysyParser::ExpContext::getRuleIndex() const {
  return sysyParser::RuleExp;
}


std::any sysyParser::ExpContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitExp(this);
  else
    return visitor->visitChildren(this);
}


sysyParser::ExpContext* sysyParser::exp() {
   return exp(0);
}

sysyParser::ExpContext* sysyParser::exp(int precedence) {
  ParserRuleContext *parentContext = _ctx;
  size_t parentState = getState();
  sysyParser::ExpContext *_localctx = _tracker.createInstance<ExpContext>(_ctx, parentState);
  sysyParser::ExpContext *previousContext = _localctx;
  (void)previousContext; // Silence compiler, in case the context is not used by generated code.
  size_t startState = 26;
  enterRecursionRule(_localctx, 26, sysyParser::RuleExp, precedence);

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
    setState(196);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 21, _ctx)) {
    case 1: {
      setState(190);
      _la = _input->LA(1);
      if (!((((_la & ~ 0x3fULL) == 0) &&
        ((1ULL << _la) & 209715200) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(191);
      exp(11);
      break;
    }

    case 2: {
      setState(192);
      parenExp();
      break;
    }

    case 3: {
      setState(193);
      number();
      break;
    }

    case 4: {
      setState(194);
      lval();
      break;
    }

    case 5: {
      setState(195);
      funcCall();
      break;
    }

    default:
      break;
    }
    _ctx->stop = _input->LT(-1);
    setState(218);
    _errHandler->sync(this);
    alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx);
    while (alt != 2 && alt != atn::ATN::INVALID_ALT_NUMBER) {
      if (alt == 1) {
        if (!_parseListeners.empty())
          triggerExitRuleEvent();
        previousContext = _localctx;
        setState(216);
        _errHandler->sync(this);
        switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 22, _ctx)) {
        case 1: {
          _localctx = _tracker.createInstance<ExpContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExp);
          setState(198);

          if (!(precpred(_ctx, 10))) throw FailedPredicateException(this, "precpred(_ctx, 10)");
          setState(199);
          _la = _input->LA(1);
          if (!((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & 1879048192) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(200);
          exp(11);
          break;
        }

        case 2: {
          _localctx = _tracker.createInstance<ExpContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExp);
          setState(201);

          if (!(precpred(_ctx, 9))) throw FailedPredicateException(this, "precpred(_ctx, 9)");
          setState(202);
          _la = _input->LA(1);
          if (!(_la == sysyParser::Plus

          || _la == sysyParser::Minus)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(203);
          exp(10);
          break;
        }

        case 3: {
          _localctx = _tracker.createInstance<ExpContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExp);
          setState(204);

          if (!(precpred(_ctx, 8))) throw FailedPredicateException(this, "precpred(_ctx, 8)");
          setState(205);
          _la = _input->LA(1);
          if (!((((_la & ~ 0x3fULL) == 0) &&
            ((1ULL << _la) & 7864320) != 0))) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(206);
          exp(9);
          break;
        }

        case 4: {
          _localctx = _tracker.createInstance<ExpContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExp);
          setState(207);

          if (!(precpred(_ctx, 7))) throw FailedPredicateException(this, "precpred(_ctx, 7)");
          setState(208);
          _la = _input->LA(1);
          if (!(_la == sysyParser::Equal

          || _la == sysyParser::NonEqual)) {
          _errHandler->recoverInline(this);
          }
          else {
            _errHandler->reportMatch(this);
            consume();
          }
          setState(209);
          exp(8);
          break;
        }

        case 5: {
          _localctx = _tracker.createInstance<ExpContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExp);
          setState(210);

          if (!(precpred(_ctx, 6))) throw FailedPredicateException(this, "precpred(_ctx, 6)");
          setState(211);
          match(sysyParser::And);
          setState(212);
          exp(7);
          break;
        }

        case 6: {
          _localctx = _tracker.createInstance<ExpContext>(parentContext, parentState);
          pushNewRecursionContext(_localctx, startState, RuleExp);
          setState(213);

          if (!(precpred(_ctx, 5))) throw FailedPredicateException(this, "precpred(_ctx, 5)");
          setState(214);
          match(sysyParser::Or);
          setState(215);
          exp(6);
          break;
        }

        default:
          break;
        } 
      }
      setState(220);
      _errHandler->sync(this);
      alt = getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 23, _ctx);
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

sysyParser::NumberContext::NumberContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* sysyParser::NumberContext::FloatConst() {
  return getToken(sysyParser::FloatConst, 0);
}

tree::TerminalNode* sysyParser::NumberContext::IntConst() {
  return getToken(sysyParser::IntConst, 0);
}


size_t sysyParser::NumberContext::getRuleIndex() const {
  return sysyParser::RuleNumber;
}


std::any sysyParser::NumberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<sysyVisitor*>(visitor))
    return parserVisitor->visitNumber(this);
  else
    return visitor->visitChildren(this);
}

sysyParser::NumberContext* sysyParser::number() {
  NumberContext *_localctx = _tracker.createInstance<NumberContext>(_ctx, getState());
  enterRule(_localctx, 28, sysyParser::RuleNumber);
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
    setState(221);
    _la = _input->LA(1);
    if (!(_la == sysyParser::IntConst

    || _la == sysyParser::FloatConst)) {
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

bool sysyParser::sempred(RuleContext *context, size_t ruleIndex, size_t predicateIndex) {
  switch (ruleIndex) {
    case 13: return expSempred(antlrcpp::downCast<ExpContext *>(context), predicateIndex);

  default:
    break;
  }
  return true;
}

bool sysyParser::expSempred(ExpContext *_localctx, size_t predicateIndex) {
  switch (predicateIndex) {
    case 0: return precpred(_ctx, 10);
    case 1: return precpred(_ctx, 9);
    case 2: return precpred(_ctx, 8);
    case 3: return precpred(_ctx, 7);
    case 4: return precpred(_ctx, 6);
    case 5: return precpred(_ctx, 5);

  default:
    break;
  }
  return true;
}

void sysyParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  sysyParserInitialize();
#else
  ::antlr4::internal::call_once(sysyParserOnceFlag, sysyParserInitialize);
#endif
}
