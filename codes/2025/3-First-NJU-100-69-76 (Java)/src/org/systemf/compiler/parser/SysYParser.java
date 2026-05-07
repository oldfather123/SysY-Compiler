// Generated from org/systemf/compiler/parser/SysYParser.g4 by ANTLR 4.13.2
package org.systemf.compiler.parser;
import org.antlr.v4.runtime.atn.*;
import org.antlr.v4.runtime.dfa.DFA;
import org.antlr.v4.runtime.*;
import org.antlr.v4.runtime.misc.*;
import org.antlr.v4.runtime.tree.*;
import java.util.List;
import java.util.Iterator;
import java.util.ArrayList;

@SuppressWarnings({"all", "warnings", "unchecked", "unused", "cast", "CheckReturnValue", "this-escape"})
public class SysYParser extends Parser {
	static { RuntimeMetaData.checkVersion("4.13.2", RuntimeMetaData.VERSION); }

	protected static final DFA[] _decisionToDFA;
	protected static final PredictionContextCache _sharedContextCache =
		new PredictionContextCache();
	public static final int
		CONST=1, INT=2, FLOAT=3, VOID=4, IF=5, ELSE=6, WHILE=7, BREAK=8, CONTINUE=9, 
		RETURN=10, PLUS=11, MINUS=12, MUL=13, DIV=14, MOD=15, ASSIGN=16, EQ=17, 
		NEQ=18, LT=19, GT=20, LE=21, GE=22, NOT=23, AND=24, OR=25, L_PAREN=26, 
		R_PAREN=27, L_BRACE=28, R_BRACE=29, L_BRACKT=30, R_BRACKT=31, COMMA=32, 
		SEMICOLON=33, IDENT=34, INTEGER_CONST=35, FLOAT_CONST=36, WS=37, LINE_COMMENT=38, 
		MULTILINE_COMMENT=39;
	public static final int
		RULE_program = 0, RULE_basicType = 1, RULE_retType = 2, RULE_constPrefix = 3, 
		RULE_incompleteArray = 4, RULE_arrayPostfixSingle = 5, RULE_arrayPostfix = 6, 
		RULE_eqInitializeVal = 7, RULE_initializer = 8, RULE_varDefEntry = 9, 
		RULE_varDef = 10, RULE_funcParam = 11, RULE_funcDef = 12, RULE_varAccess = 13, 
		RULE_funcRealParam = 14, RULE_expr = 15, RULE_stmtBlock = 16, RULE_stmt = 17;
	private static String[] makeRuleNames() {
		return new String[] {
			"program", "basicType", "retType", "constPrefix", "incompleteArray", 
			"arrayPostfixSingle", "arrayPostfix", "eqInitializeVal", "initializer", 
			"varDefEntry", "varDef", "funcParam", "funcDef", "varAccess", "funcRealParam", 
			"expr", "stmtBlock", "stmt"
		};
	}
	public static final String[] ruleNames = makeRuleNames();

	private static String[] makeLiteralNames() {
		return new String[] {
			null, "'const'", "'int'", "'float'", "'void'", "'if'", "'else'", "'while'", 
			"'break'", "'continue'", "'return'", "'+'", "'-'", "'*'", "'/'", "'%'", 
			"'='", "'=='", "'!='", "'<'", "'>'", "'<='", "'>='", "'!'", "'&&'", "'||'", 
			"'('", "')'", "'{'", "'}'", "'['", "']'", "','", "';'"
		};
	}
	private static final String[] _LITERAL_NAMES = makeLiteralNames();
	private static String[] makeSymbolicNames() {
		return new String[] {
			null, "CONST", "INT", "FLOAT", "VOID", "IF", "ELSE", "WHILE", "BREAK", 
			"CONTINUE", "RETURN", "PLUS", "MINUS", "MUL", "DIV", "MOD", "ASSIGN", 
			"EQ", "NEQ", "LT", "GT", "LE", "GE", "NOT", "AND", "OR", "L_PAREN", "R_PAREN", 
			"L_BRACE", "R_BRACE", "L_BRACKT", "R_BRACKT", "COMMA", "SEMICOLON", "IDENT", 
			"INTEGER_CONST", "FLOAT_CONST", "WS", "LINE_COMMENT", "MULTILINE_COMMENT"
		};
	}
	private static final String[] _SYMBOLIC_NAMES = makeSymbolicNames();
	public static final Vocabulary VOCABULARY = new VocabularyImpl(_LITERAL_NAMES, _SYMBOLIC_NAMES);

	/**
	 * @deprecated Use {@link #VOCABULARY} instead.
	 */
	@Deprecated
	public static final String[] tokenNames;
	static {
		tokenNames = new String[_SYMBOLIC_NAMES.length];
		for (int i = 0; i < tokenNames.length; i++) {
			tokenNames[i] = VOCABULARY.getLiteralName(i);
			if (tokenNames[i] == null) {
				tokenNames[i] = VOCABULARY.getSymbolicName(i);
			}

			if (tokenNames[i] == null) {
				tokenNames[i] = "<INVALID>";
			}
		}
	}

	@Override
	@Deprecated
	public String[] getTokenNames() {
		return tokenNames;
	}

	@Override

	public Vocabulary getVocabulary() {
		return VOCABULARY;
	}

	@Override
	public String getGrammarFileName() { return "SysYParser.g4"; }

	@Override
	public String[] getRuleNames() { return ruleNames; }

	@Override
	public String getSerializedATN() { return _serializedATN; }

	@Override
	public ATN getATN() { return _ATN; }

	public SysYParser(TokenStream input) {
		super(input);
		_interp = new ParserATNSimulator(this,_ATN,_decisionToDFA,_sharedContextCache);
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ProgramContext extends ParserRuleContext {
		public TerminalNode EOF() { return getToken(SysYParser.EOF, 0); }
		public List<VarDefContext> varDef() {
			return getRuleContexts(VarDefContext.class);
		}
		public VarDefContext varDef(int i) {
			return getRuleContext(VarDefContext.class,i);
		}
		public List<FuncDefContext> funcDef() {
			return getRuleContexts(FuncDefContext.class);
		}
		public FuncDefContext funcDef(int i) {
			return getRuleContext(FuncDefContext.class,i);
		}
		public ProgramContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_program; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterProgram(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitProgram(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitProgram(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ProgramContext program() throws RecognitionException {
		ProgramContext _localctx = new ProgramContext(_ctx, getState());
		enterRule(_localctx, 0, RULE_program);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(40);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 30L) != 0)) {
				{
				setState(38);
				_errHandler.sync(this);
				switch ( getInterpreter().adaptivePredict(_input,0,_ctx) ) {
				case 1:
					{
					setState(36);
					varDef();
					}
					break;
				case 2:
					{
					setState(37);
					funcDef();
					}
					break;
				}
				}
				setState(42);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(43);
			match(EOF);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class BasicTypeContext extends ParserRuleContext {
		public TerminalNode INT() { return getToken(SysYParser.INT, 0); }
		public TerminalNode FLOAT() { return getToken(SysYParser.FLOAT, 0); }
		public BasicTypeContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_basicType; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterBasicType(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitBasicType(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitBasicType(this);
			else return visitor.visitChildren(this);
		}
	}

	public final BasicTypeContext basicType() throws RecognitionException {
		BasicTypeContext _localctx = new BasicTypeContext(_ctx, getState());
		enterRule(_localctx, 2, RULE_basicType);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(45);
			_la = _input.LA(1);
			if ( !(_la==INT || _la==FLOAT) ) {
			_errHandler.recoverInline(this);
			}
			else {
				if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
				_errHandler.reportMatch(this);
				consume();
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class RetTypeContext extends ParserRuleContext {
		public BasicTypeContext basicType() {
			return getRuleContext(BasicTypeContext.class,0);
		}
		public TerminalNode VOID() { return getToken(SysYParser.VOID, 0); }
		public RetTypeContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_retType; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterRetType(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitRetType(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitRetType(this);
			else return visitor.visitChildren(this);
		}
	}

	public final RetTypeContext retType() throws RecognitionException {
		RetTypeContext _localctx = new RetTypeContext(_ctx, getState());
		enterRule(_localctx, 4, RULE_retType);
		try {
			setState(49);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case INT:
			case FLOAT:
				enterOuterAlt(_localctx, 1);
				{
				setState(47);
				basicType();
				}
				break;
			case VOID:
				enterOuterAlt(_localctx, 2);
				{
				setState(48);
				match(VOID);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ConstPrefixContext extends ParserRuleContext {
		public TerminalNode CONST() { return getToken(SysYParser.CONST, 0); }
		public ConstPrefixContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_constPrefix; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterConstPrefix(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitConstPrefix(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitConstPrefix(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ConstPrefixContext constPrefix() throws RecognitionException {
		ConstPrefixContext _localctx = new ConstPrefixContext(_ctx, getState());
		enterRule(_localctx, 6, RULE_constPrefix);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(52);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==CONST) {
				{
				setState(51);
				match(CONST);
				}
			}

			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class IncompleteArrayContext extends ParserRuleContext {
		public TerminalNode L_BRACKT() { return getToken(SysYParser.L_BRACKT, 0); }
		public TerminalNode R_BRACKT() { return getToken(SysYParser.R_BRACKT, 0); }
		public IncompleteArrayContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_incompleteArray; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterIncompleteArray(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitIncompleteArray(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitIncompleteArray(this);
			else return visitor.visitChildren(this);
		}
	}

	public final IncompleteArrayContext incompleteArray() throws RecognitionException {
		IncompleteArrayContext _localctx = new IncompleteArrayContext(_ctx, getState());
		enterRule(_localctx, 8, RULE_incompleteArray);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(54);
			match(L_BRACKT);
			setState(55);
			match(R_BRACKT);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ArrayPostfixSingleContext extends ParserRuleContext {
		public ExprContext length;
		public TerminalNode L_BRACKT() { return getToken(SysYParser.L_BRACKT, 0); }
		public TerminalNode R_BRACKT() { return getToken(SysYParser.R_BRACKT, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public ArrayPostfixSingleContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_arrayPostfixSingle; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterArrayPostfixSingle(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitArrayPostfixSingle(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitArrayPostfixSingle(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ArrayPostfixSingleContext arrayPostfixSingle() throws RecognitionException {
		ArrayPostfixSingleContext _localctx = new ArrayPostfixSingleContext(_ctx, getState());
		enterRule(_localctx, 10, RULE_arrayPostfixSingle);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(57);
			match(L_BRACKT);
			setState(58);
			((ArrayPostfixSingleContext)_localctx).length = expr(0);
			setState(59);
			match(R_BRACKT);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ArrayPostfixContext extends ParserRuleContext {
		public List<ArrayPostfixSingleContext> arrayPostfixSingle() {
			return getRuleContexts(ArrayPostfixSingleContext.class);
		}
		public ArrayPostfixSingleContext arrayPostfixSingle(int i) {
			return getRuleContext(ArrayPostfixSingleContext.class,i);
		}
		public ArrayPostfixContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_arrayPostfix; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterArrayPostfix(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitArrayPostfix(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitArrayPostfix(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ArrayPostfixContext arrayPostfix() throws RecognitionException {
		ArrayPostfixContext _localctx = new ArrayPostfixContext(_ctx, getState());
		enterRule(_localctx, 12, RULE_arrayPostfix);
		try {
			int _alt;
			enterOuterAlt(_localctx, 1);
			{
			setState(64);
			_errHandler.sync(this);
			_alt = getInterpreter().adaptivePredict(_input,4,_ctx);
			while ( _alt!=2 && _alt!=org.antlr.v4.runtime.atn.ATN.INVALID_ALT_NUMBER ) {
				if ( _alt==1 ) {
					{
					{
					setState(61);
					arrayPostfixSingle();
					}
					} 
				}
				setState(66);
				_errHandler.sync(this);
				_alt = getInterpreter().adaptivePredict(_input,4,_ctx);
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class EqInitializeValContext extends ParserRuleContext {
		public EqInitializeValContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_eqInitializeVal; }
	 
		public EqInitializeValContext() { }
		public void copyFrom(EqInitializeValContext ctx) {
			super.copyFrom(ctx);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class SingleContext extends EqInitializeValContext {
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public SingleContext(EqInitializeValContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterSingle(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitSingle(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitSingle(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class ArrayContext extends EqInitializeValContext {
		public TerminalNode L_BRACE() { return getToken(SysYParser.L_BRACE, 0); }
		public TerminalNode R_BRACE() { return getToken(SysYParser.R_BRACE, 0); }
		public List<EqInitializeValContext> eqInitializeVal() {
			return getRuleContexts(EqInitializeValContext.class);
		}
		public EqInitializeValContext eqInitializeVal(int i) {
			return getRuleContext(EqInitializeValContext.class,i);
		}
		public List<TerminalNode> COMMA() { return getTokens(SysYParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(SysYParser.COMMA, i);
		}
		public ArrayContext(EqInitializeValContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterArray(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitArray(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitArray(this);
			else return visitor.visitChildren(this);
		}
	}

	public final EqInitializeValContext eqInitializeVal() throws RecognitionException {
		EqInitializeValContext _localctx = new EqInitializeValContext(_ctx, getState());
		enterRule(_localctx, 14, RULE_eqInitializeVal);
		int _la;
		try {
			setState(80);
			_errHandler.sync(this);
			switch (_input.LA(1)) {
			case PLUS:
			case MINUS:
			case NOT:
			case L_PAREN:
			case IDENT:
			case INTEGER_CONST:
			case FLOAT_CONST:
				_localctx = new SingleContext(_localctx);
				enterOuterAlt(_localctx, 1);
				{
				setState(67);
				expr(0);
				}
				break;
			case L_BRACE:
				_localctx = new ArrayContext(_localctx);
				enterOuterAlt(_localctx, 2);
				{
				setState(68);
				match(L_BRACE);
				setState(77);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 120603023360L) != 0)) {
					{
					setState(69);
					eqInitializeVal();
					setState(74);
					_errHandler.sync(this);
					_la = _input.LA(1);
					while (_la==COMMA) {
						{
						{
						setState(70);
						match(COMMA);
						setState(71);
						eqInitializeVal();
						}
						}
						setState(76);
						_errHandler.sync(this);
						_la = _input.LA(1);
					}
					}
				}

				setState(79);
				match(R_BRACE);
				}
				break;
			default:
				throw new NoViableAltException(this);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class InitializerContext extends ParserRuleContext {
		public EqInitializeValContext value;
		public TerminalNode ASSIGN() { return getToken(SysYParser.ASSIGN, 0); }
		public EqInitializeValContext eqInitializeVal() {
			return getRuleContext(EqInitializeValContext.class,0);
		}
		public InitializerContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_initializer; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterInitializer(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitInitializer(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitInitializer(this);
			else return visitor.visitChildren(this);
		}
	}

	public final InitializerContext initializer() throws RecognitionException {
		InitializerContext _localctx = new InitializerContext(_ctx, getState());
		enterRule(_localctx, 16, RULE_initializer);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(82);
			match(ASSIGN);
			setState(83);
			((InitializerContext)_localctx).value = eqInitializeVal();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class VarDefEntryContext extends ParserRuleContext {
		public Token name;
		public InitializerContext init;
		public ArrayPostfixContext arrayPostfix() {
			return getRuleContext(ArrayPostfixContext.class,0);
		}
		public TerminalNode IDENT() { return getToken(SysYParser.IDENT, 0); }
		public InitializerContext initializer() {
			return getRuleContext(InitializerContext.class,0);
		}
		public VarDefEntryContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_varDefEntry; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterVarDefEntry(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitVarDefEntry(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitVarDefEntry(this);
			else return visitor.visitChildren(this);
		}
	}

	public final VarDefEntryContext varDefEntry() throws RecognitionException {
		VarDefEntryContext _localctx = new VarDefEntryContext(_ctx, getState());
		enterRule(_localctx, 18, RULE_varDefEntry);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(85);
			((VarDefEntryContext)_localctx).name = match(IDENT);
			setState(86);
			arrayPostfix();
			setState(88);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==ASSIGN) {
				{
				setState(87);
				((VarDefEntryContext)_localctx).init = initializer();
				}
			}

			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class VarDefContext extends ParserRuleContext {
		public BasicTypeContext type;
		public ConstPrefixContext constPrefix() {
			return getRuleContext(ConstPrefixContext.class,0);
		}
		public List<VarDefEntryContext> varDefEntry() {
			return getRuleContexts(VarDefEntryContext.class);
		}
		public VarDefEntryContext varDefEntry(int i) {
			return getRuleContext(VarDefEntryContext.class,i);
		}
		public TerminalNode SEMICOLON() { return getToken(SysYParser.SEMICOLON, 0); }
		public BasicTypeContext basicType() {
			return getRuleContext(BasicTypeContext.class,0);
		}
		public List<TerminalNode> COMMA() { return getTokens(SysYParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(SysYParser.COMMA, i);
		}
		public VarDefContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_varDef; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterVarDef(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitVarDef(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitVarDef(this);
			else return visitor.visitChildren(this);
		}
	}

	public final VarDefContext varDef() throws RecognitionException {
		VarDefContext _localctx = new VarDefContext(_ctx, getState());
		enterRule(_localctx, 20, RULE_varDef);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(90);
			constPrefix();
			setState(91);
			((VarDefContext)_localctx).type = basicType();
			setState(92);
			varDefEntry();
			setState(97);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while (_la==COMMA) {
				{
				{
				setState(93);
				match(COMMA);
				setState(94);
				varDefEntry();
				}
				}
				setState(99);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(100);
			match(SEMICOLON);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class FuncParamContext extends ParserRuleContext {
		public BasicTypeContext type;
		public Token name;
		public BasicTypeContext basicType() {
			return getRuleContext(BasicTypeContext.class,0);
		}
		public TerminalNode IDENT() { return getToken(SysYParser.IDENT, 0); }
		public IncompleteArrayContext incompleteArray() {
			return getRuleContext(IncompleteArrayContext.class,0);
		}
		public ArrayPostfixContext arrayPostfix() {
			return getRuleContext(ArrayPostfixContext.class,0);
		}
		public FuncParamContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_funcParam; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterFuncParam(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitFuncParam(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitFuncParam(this);
			else return visitor.visitChildren(this);
		}
	}

	public final FuncParamContext funcParam() throws RecognitionException {
		FuncParamContext _localctx = new FuncParamContext(_ctx, getState());
		enterRule(_localctx, 22, RULE_funcParam);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(102);
			((FuncParamContext)_localctx).type = basicType();
			setState(103);
			((FuncParamContext)_localctx).name = match(IDENT);
			setState(107);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==L_BRACKT) {
				{
				setState(104);
				incompleteArray();
				setState(105);
				arrayPostfix();
				}
			}

			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class FuncDefContext extends ParserRuleContext {
		public RetTypeContext r_type;
		public Token name;
		public TerminalNode L_PAREN() { return getToken(SysYParser.L_PAREN, 0); }
		public TerminalNode R_PAREN() { return getToken(SysYParser.R_PAREN, 0); }
		public StmtBlockContext stmtBlock() {
			return getRuleContext(StmtBlockContext.class,0);
		}
		public RetTypeContext retType() {
			return getRuleContext(RetTypeContext.class,0);
		}
		public TerminalNode IDENT() { return getToken(SysYParser.IDENT, 0); }
		public List<FuncParamContext> funcParam() {
			return getRuleContexts(FuncParamContext.class);
		}
		public FuncParamContext funcParam(int i) {
			return getRuleContext(FuncParamContext.class,i);
		}
		public List<TerminalNode> COMMA() { return getTokens(SysYParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(SysYParser.COMMA, i);
		}
		public FuncDefContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_funcDef; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterFuncDef(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitFuncDef(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitFuncDef(this);
			else return visitor.visitChildren(this);
		}
	}

	public final FuncDefContext funcDef() throws RecognitionException {
		FuncDefContext _localctx = new FuncDefContext(_ctx, getState());
		enterRule(_localctx, 24, RULE_funcDef);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(109);
			((FuncDefContext)_localctx).r_type = retType();
			setState(110);
			((FuncDefContext)_localctx).name = match(IDENT);
			setState(111);
			match(L_PAREN);
			setState(120);
			_errHandler.sync(this);
			_la = _input.LA(1);
			if (_la==INT || _la==FLOAT) {
				{
				setState(112);
				funcParam();
				setState(117);
				_errHandler.sync(this);
				_la = _input.LA(1);
				while (_la==COMMA) {
					{
					{
					setState(113);
					match(COMMA);
					setState(114);
					funcParam();
					}
					}
					setState(119);
					_errHandler.sync(this);
					_la = _input.LA(1);
				}
				}
			}

			setState(122);
			match(R_PAREN);
			setState(123);
			stmtBlock();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class VarAccessContext extends ParserRuleContext {
		public TerminalNode IDENT() { return getToken(SysYParser.IDENT, 0); }
		public ArrayPostfixContext arrayPostfix() {
			return getRuleContext(ArrayPostfixContext.class,0);
		}
		public VarAccessContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_varAccess; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterVarAccess(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitVarAccess(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitVarAccess(this);
			else return visitor.visitChildren(this);
		}
	}

	public final VarAccessContext varAccess() throws RecognitionException {
		VarAccessContext _localctx = new VarAccessContext(_ctx, getState());
		enterRule(_localctx, 26, RULE_varAccess);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(125);
			match(IDENT);
			setState(126);
			arrayPostfix();
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class FuncRealParamContext extends ParserRuleContext {
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public FuncRealParamContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_funcRealParam; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterFuncRealParam(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitFuncRealParam(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitFuncRealParam(this);
			else return visitor.visitChildren(this);
		}
	}

	public final FuncRealParamContext funcRealParam() throws RecognitionException {
		FuncRealParamContext _localctx = new FuncRealParamContext(_ctx, getState());
		enterRule(_localctx, 28, RULE_funcRealParam);
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(128);
			expr(0);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class ExprContext extends ParserRuleContext {
		public ExprContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_expr; }
	 
		public ExprContext() { }
		public void copyFrom(ExprContext ctx) {
			super.copyFrom(ctx);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class ParenContext extends ExprContext {
		public TerminalNode L_PAREN() { return getToken(SysYParser.L_PAREN, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public TerminalNode R_PAREN() { return getToken(SysYParser.R_PAREN, 0); }
		public ParenContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterParen(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitParen(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitParen(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class AccessContext extends ExprContext {
		public VarAccessContext varAccess() {
			return getRuleContext(VarAccessContext.class,0);
		}
		public AccessContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterAccess(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitAccess(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitAccess(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class OrContext extends ExprContext {
		public ExprContext l;
		public ExprContext r;
		public TerminalNode OR() { return getToken(SysYParser.OR, 0); }
		public List<ExprContext> expr() {
			return getRuleContexts(ExprContext.class);
		}
		public ExprContext expr(int i) {
			return getRuleContext(ExprContext.class,i);
		}
		public OrContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterOr(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitOr(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitOr(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class EqsContext extends ExprContext {
		public ExprContext l;
		public Token op;
		public ExprContext r;
		public List<ExprContext> expr() {
			return getRuleContexts(ExprContext.class);
		}
		public ExprContext expr(int i) {
			return getRuleContext(ExprContext.class,i);
		}
		public TerminalNode EQ() { return getToken(SysYParser.EQ, 0); }
		public TerminalNode NEQ() { return getToken(SysYParser.NEQ, 0); }
		public EqsContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterEqs(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitEqs(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitEqs(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class AndContext extends ExprContext {
		public ExprContext l;
		public ExprContext r;
		public TerminalNode AND() { return getToken(SysYParser.AND, 0); }
		public List<ExprContext> expr() {
			return getRuleContexts(ExprContext.class);
		}
		public ExprContext expr(int i) {
			return getRuleContext(ExprContext.class,i);
		}
		public AndContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterAnd(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitAnd(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitAnd(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class FunctionCallContext extends ExprContext {
		public Token func;
		public TerminalNode L_PAREN() { return getToken(SysYParser.L_PAREN, 0); }
		public TerminalNode R_PAREN() { return getToken(SysYParser.R_PAREN, 0); }
		public TerminalNode IDENT() { return getToken(SysYParser.IDENT, 0); }
		public List<FuncRealParamContext> funcRealParam() {
			return getRuleContexts(FuncRealParamContext.class);
		}
		public FuncRealParamContext funcRealParam(int i) {
			return getRuleContext(FuncRealParamContext.class,i);
		}
		public List<TerminalNode> COMMA() { return getTokens(SysYParser.COMMA); }
		public TerminalNode COMMA(int i) {
			return getToken(SysYParser.COMMA, i);
		}
		public FunctionCallContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterFunctionCall(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitFunctionCall(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitFunctionCall(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class ConstFloatContext extends ExprContext {
		public Token value;
		public TerminalNode FLOAT_CONST() { return getToken(SysYParser.FLOAT_CONST, 0); }
		public ConstFloatContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterConstFloat(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitConstFloat(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitConstFloat(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class ConstIntContext extends ExprContext {
		public Token value;
		public TerminalNode INTEGER_CONST() { return getToken(SysYParser.INTEGER_CONST, 0); }
		public ConstIntContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterConstInt(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitConstInt(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitConstInt(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class UnaryContext extends ExprContext {
		public Token op;
		public ExprContext x;
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public TerminalNode PLUS() { return getToken(SysYParser.PLUS, 0); }
		public TerminalNode MINUS() { return getToken(SysYParser.MINUS, 0); }
		public TerminalNode NOT() { return getToken(SysYParser.NOT, 0); }
		public UnaryContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterUnary(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitUnary(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitUnary(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class MulsContext extends ExprContext {
		public ExprContext l;
		public Token op;
		public ExprContext r;
		public List<ExprContext> expr() {
			return getRuleContexts(ExprContext.class);
		}
		public ExprContext expr(int i) {
			return getRuleContext(ExprContext.class,i);
		}
		public TerminalNode MUL() { return getToken(SysYParser.MUL, 0); }
		public TerminalNode DIV() { return getToken(SysYParser.DIV, 0); }
		public TerminalNode MOD() { return getToken(SysYParser.MOD, 0); }
		public MulsContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterMuls(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitMuls(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitMuls(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class AddsContext extends ExprContext {
		public ExprContext l;
		public Token op;
		public ExprContext r;
		public List<ExprContext> expr() {
			return getRuleContexts(ExprContext.class);
		}
		public ExprContext expr(int i) {
			return getRuleContext(ExprContext.class,i);
		}
		public TerminalNode PLUS() { return getToken(SysYParser.PLUS, 0); }
		public TerminalNode MINUS() { return getToken(SysYParser.MINUS, 0); }
		public AddsContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterAdds(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitAdds(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitAdds(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class RelsContext extends ExprContext {
		public ExprContext l;
		public Token op;
		public ExprContext r;
		public List<ExprContext> expr() {
			return getRuleContexts(ExprContext.class);
		}
		public ExprContext expr(int i) {
			return getRuleContext(ExprContext.class,i);
		}
		public TerminalNode LT() { return getToken(SysYParser.LT, 0); }
		public TerminalNode GT() { return getToken(SysYParser.GT, 0); }
		public TerminalNode LE() { return getToken(SysYParser.LE, 0); }
		public TerminalNode GE() { return getToken(SysYParser.GE, 0); }
		public RelsContext(ExprContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterRels(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitRels(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitRels(this);
			else return visitor.visitChildren(this);
		}
	}

	public final ExprContext expr() throws RecognitionException {
		return expr(0);
	}

	private ExprContext expr(int _p) throws RecognitionException {
		ParserRuleContext _parentctx = _ctx;
		int _parentState = getState();
		ExprContext _localctx = new ExprContext(_ctx, _parentState);
		ExprContext _prevctx = _localctx;
		int _startState = 30;
		enterRecursionRule(_localctx, 30, RULE_expr, _p);
		int _la;
		try {
			int _alt;
			enterOuterAlt(_localctx, 1);
			{
			setState(153);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,15,_ctx) ) {
			case 1:
				{
				_localctx = new ConstIntContext(_localctx);
				_ctx = _localctx;
				_prevctx = _localctx;

				setState(131);
				((ConstIntContext)_localctx).value = match(INTEGER_CONST);
				}
				break;
			case 2:
				{
				_localctx = new ConstFloatContext(_localctx);
				_ctx = _localctx;
				_prevctx = _localctx;
				setState(132);
				((ConstFloatContext)_localctx).value = match(FLOAT_CONST);
				}
				break;
			case 3:
				{
				_localctx = new FunctionCallContext(_localctx);
				_ctx = _localctx;
				_prevctx = _localctx;
				setState(133);
				((FunctionCallContext)_localctx).func = match(IDENT);
				setState(134);
				match(L_PAREN);
				setState(143);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 120334587904L) != 0)) {
					{
					setState(135);
					funcRealParam();
					setState(140);
					_errHandler.sync(this);
					_la = _input.LA(1);
					while (_la==COMMA) {
						{
						{
						setState(136);
						match(COMMA);
						setState(137);
						funcRealParam();
						}
						}
						setState(142);
						_errHandler.sync(this);
						_la = _input.LA(1);
					}
					}
				}

				setState(145);
				match(R_PAREN);
				}
				break;
			case 4:
				{
				_localctx = new AccessContext(_localctx);
				_ctx = _localctx;
				_prevctx = _localctx;
				setState(146);
				varAccess();
				}
				break;
			case 5:
				{
				_localctx = new ParenContext(_localctx);
				_ctx = _localctx;
				_prevctx = _localctx;
				setState(147);
				match(L_PAREN);
				setState(148);
				expr(0);
				setState(149);
				match(R_PAREN);
				}
				break;
			case 6:
				{
				_localctx = new UnaryContext(_localctx);
				_ctx = _localctx;
				_prevctx = _localctx;
				setState(151);
				((UnaryContext)_localctx).op = _input.LT(1);
				_la = _input.LA(1);
				if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 8394752L) != 0)) ) {
					((UnaryContext)_localctx).op = (Token)_errHandler.recoverInline(this);
				}
				else {
					if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
					_errHandler.reportMatch(this);
					consume();
				}
				setState(152);
				((UnaryContext)_localctx).x = expr(7);
				}
				break;
			}
			_ctx.stop = _input.LT(-1);
			setState(175);
			_errHandler.sync(this);
			_alt = getInterpreter().adaptivePredict(_input,17,_ctx);
			while ( _alt!=2 && _alt!=org.antlr.v4.runtime.atn.ATN.INVALID_ALT_NUMBER ) {
				if ( _alt==1 ) {
					if ( _parseListeners!=null ) triggerExitRuleEvent();
					_prevctx = _localctx;
					{
					setState(173);
					_errHandler.sync(this);
					switch ( getInterpreter().adaptivePredict(_input,16,_ctx) ) {
					case 1:
						{
						_localctx = new MulsContext(new ExprContext(_parentctx, _parentState));
						((MulsContext)_localctx).l = _prevctx;
						pushNewRecursionContext(_localctx, _startState, RULE_expr);
						setState(155);
						if (!(precpred(_ctx, 6))) throw new FailedPredicateException(this, "precpred(_ctx, 6)");
						setState(156);
						((MulsContext)_localctx).op = _input.LT(1);
						_la = _input.LA(1);
						if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 57344L) != 0)) ) {
							((MulsContext)_localctx).op = (Token)_errHandler.recoverInline(this);
						}
						else {
							if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
							_errHandler.reportMatch(this);
							consume();
						}
						setState(157);
						((MulsContext)_localctx).r = expr(7);
						}
						break;
					case 2:
						{
						_localctx = new AddsContext(new ExprContext(_parentctx, _parentState));
						((AddsContext)_localctx).l = _prevctx;
						pushNewRecursionContext(_localctx, _startState, RULE_expr);
						setState(158);
						if (!(precpred(_ctx, 5))) throw new FailedPredicateException(this, "precpred(_ctx, 5)");
						setState(159);
						((AddsContext)_localctx).op = _input.LT(1);
						_la = _input.LA(1);
						if ( !(_la==PLUS || _la==MINUS) ) {
							((AddsContext)_localctx).op = (Token)_errHandler.recoverInline(this);
						}
						else {
							if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
							_errHandler.reportMatch(this);
							consume();
						}
						setState(160);
						((AddsContext)_localctx).r = expr(6);
						}
						break;
					case 3:
						{
						_localctx = new RelsContext(new ExprContext(_parentctx, _parentState));
						((RelsContext)_localctx).l = _prevctx;
						pushNewRecursionContext(_localctx, _startState, RULE_expr);
						setState(161);
						if (!(precpred(_ctx, 4))) throw new FailedPredicateException(this, "precpred(_ctx, 4)");
						setState(162);
						((RelsContext)_localctx).op = _input.LT(1);
						_la = _input.LA(1);
						if ( !((((_la) & ~0x3f) == 0 && ((1L << _la) & 7864320L) != 0)) ) {
							((RelsContext)_localctx).op = (Token)_errHandler.recoverInline(this);
						}
						else {
							if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
							_errHandler.reportMatch(this);
							consume();
						}
						setState(163);
						((RelsContext)_localctx).r = expr(5);
						}
						break;
					case 4:
						{
						_localctx = new EqsContext(new ExprContext(_parentctx, _parentState));
						((EqsContext)_localctx).l = _prevctx;
						pushNewRecursionContext(_localctx, _startState, RULE_expr);
						setState(164);
						if (!(precpred(_ctx, 3))) throw new FailedPredicateException(this, "precpred(_ctx, 3)");
						setState(165);
						((EqsContext)_localctx).op = _input.LT(1);
						_la = _input.LA(1);
						if ( !(_la==EQ || _la==NEQ) ) {
							((EqsContext)_localctx).op = (Token)_errHandler.recoverInline(this);
						}
						else {
							if ( _input.LA(1)==Token.EOF ) matchedEOF = true;
							_errHandler.reportMatch(this);
							consume();
						}
						setState(166);
						((EqsContext)_localctx).r = expr(4);
						}
						break;
					case 5:
						{
						_localctx = new AndContext(new ExprContext(_parentctx, _parentState));
						((AndContext)_localctx).l = _prevctx;
						pushNewRecursionContext(_localctx, _startState, RULE_expr);
						setState(167);
						if (!(precpred(_ctx, 2))) throw new FailedPredicateException(this, "precpred(_ctx, 2)");
						setState(168);
						match(AND);
						setState(169);
						((AndContext)_localctx).r = expr(3);
						}
						break;
					case 6:
						{
						_localctx = new OrContext(new ExprContext(_parentctx, _parentState));
						((OrContext)_localctx).l = _prevctx;
						pushNewRecursionContext(_localctx, _startState, RULE_expr);
						setState(170);
						if (!(precpred(_ctx, 1))) throw new FailedPredicateException(this, "precpred(_ctx, 1)");
						setState(171);
						match(OR);
						setState(172);
						((OrContext)_localctx).r = expr(2);
						}
						break;
					}
					} 
				}
				setState(177);
				_errHandler.sync(this);
				_alt = getInterpreter().adaptivePredict(_input,17,_ctx);
			}
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			unrollRecursionContexts(_parentctx);
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class StmtBlockContext extends ParserRuleContext {
		public TerminalNode L_BRACE() { return getToken(SysYParser.L_BRACE, 0); }
		public TerminalNode R_BRACE() { return getToken(SysYParser.R_BRACE, 0); }
		public List<VarDefContext> varDef() {
			return getRuleContexts(VarDefContext.class);
		}
		public VarDefContext varDef(int i) {
			return getRuleContext(VarDefContext.class,i);
		}
		public List<StmtContext> stmt() {
			return getRuleContexts(StmtContext.class);
		}
		public StmtContext stmt(int i) {
			return getRuleContext(StmtContext.class,i);
		}
		public StmtBlockContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_stmtBlock; }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterStmtBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitStmtBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitStmtBlock(this);
			else return visitor.visitChildren(this);
		}
	}

	public final StmtBlockContext stmtBlock() throws RecognitionException {
		StmtBlockContext _localctx = new StmtBlockContext(_ctx, getState());
		enterRule(_localctx, 32, RULE_stmtBlock);
		int _la;
		try {
			enterOuterAlt(_localctx, 1);
			{
			setState(178);
			match(L_BRACE);
			setState(183);
			_errHandler.sync(this);
			_la = _input.LA(1);
			while ((((_la) & ~0x3f) == 0 && ((1L << _la) & 129192959918L) != 0)) {
				{
				setState(181);
				_errHandler.sync(this);
				switch (_input.LA(1)) {
				case CONST:
				case INT:
				case FLOAT:
					{
					setState(179);
					varDef();
					}
					break;
				case IF:
				case WHILE:
				case BREAK:
				case CONTINUE:
				case RETURN:
				case PLUS:
				case MINUS:
				case NOT:
				case L_PAREN:
				case L_BRACE:
				case SEMICOLON:
				case IDENT:
				case INTEGER_CONST:
				case FLOAT_CONST:
					{
					setState(180);
					stmt();
					}
					break;
				default:
					throw new NoViableAltException(this);
				}
				}
				setState(185);
				_errHandler.sync(this);
				_la = _input.LA(1);
			}
			setState(186);
			match(R_BRACE);
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	@SuppressWarnings("CheckReturnValue")
	public static class StmtContext extends ParserRuleContext {
		public StmtContext(ParserRuleContext parent, int invokingState) {
			super(parent, invokingState);
		}
		@Override public int getRuleIndex() { return RULE_stmt; }
	 
		public StmtContext() { }
		public void copyFrom(StmtContext ctx) {
			super.copyFrom(ctx);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class ExpressionContext extends StmtContext {
		public TerminalNode SEMICOLON() { return getToken(SysYParser.SEMICOLON, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public ExpressionContext(StmtContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterExpression(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitExpression(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitExpression(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class BreakContext extends StmtContext {
		public TerminalNode BREAK() { return getToken(SysYParser.BREAK, 0); }
		public TerminalNode SEMICOLON() { return getToken(SysYParser.SEMICOLON, 0); }
		public BreakContext(StmtContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterBreak(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitBreak(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitBreak(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class AssignmentContext extends StmtContext {
		public VarAccessContext lvalue;
		public ExprContext value;
		public TerminalNode ASSIGN() { return getToken(SysYParser.ASSIGN, 0); }
		public TerminalNode SEMICOLON() { return getToken(SysYParser.SEMICOLON, 0); }
		public VarAccessContext varAccess() {
			return getRuleContext(VarAccessContext.class,0);
		}
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public AssignmentContext(StmtContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterAssignment(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitAssignment(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitAssignment(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class ContinueContext extends StmtContext {
		public TerminalNode CONTINUE() { return getToken(SysYParser.CONTINUE, 0); }
		public TerminalNode SEMICOLON() { return getToken(SysYParser.SEMICOLON, 0); }
		public ContinueContext(StmtContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterContinue(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitContinue(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitContinue(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class BlockContext extends StmtContext {
		public StmtBlockContext stmtBlock() {
			return getRuleContext(StmtBlockContext.class,0);
		}
		public BlockContext(StmtContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterBlock(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitBlock(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitBlock(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class WhileContext extends StmtContext {
		public ExprContext cond;
		public StmtContext stmtTrue;
		public TerminalNode WHILE() { return getToken(SysYParser.WHILE, 0); }
		public TerminalNode L_PAREN() { return getToken(SysYParser.L_PAREN, 0); }
		public TerminalNode R_PAREN() { return getToken(SysYParser.R_PAREN, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public StmtContext stmt() {
			return getRuleContext(StmtContext.class,0);
		}
		public WhileContext(StmtContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterWhile(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitWhile(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitWhile(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class IfContext extends StmtContext {
		public ExprContext cond;
		public StmtContext stmtTrue;
		public StmtContext stmtFalse;
		public TerminalNode IF() { return getToken(SysYParser.IF, 0); }
		public TerminalNode L_PAREN() { return getToken(SysYParser.L_PAREN, 0); }
		public TerminalNode R_PAREN() { return getToken(SysYParser.R_PAREN, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public List<StmtContext> stmt() {
			return getRuleContexts(StmtContext.class);
		}
		public StmtContext stmt(int i) {
			return getRuleContext(StmtContext.class,i);
		}
		public TerminalNode ELSE() { return getToken(SysYParser.ELSE, 0); }
		public IfContext(StmtContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterIf(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitIf(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitIf(this);
			else return visitor.visitChildren(this);
		}
	}
	@SuppressWarnings("CheckReturnValue")
	public static class ReturnContext extends StmtContext {
		public ExprContext ret;
		public TerminalNode RETURN() { return getToken(SysYParser.RETURN, 0); }
		public TerminalNode SEMICOLON() { return getToken(SysYParser.SEMICOLON, 0); }
		public ExprContext expr() {
			return getRuleContext(ExprContext.class,0);
		}
		public ReturnContext(StmtContext ctx) { copyFrom(ctx); }
		@Override
		public void enterRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).enterReturn(this);
		}
		@Override
		public void exitRule(ParseTreeListener listener) {
			if ( listener instanceof SysYParserListener ) ((SysYParserListener)listener).exitReturn(this);
		}
		@Override
		public <T> T accept(ParseTreeVisitor<? extends T> visitor) {
			if ( visitor instanceof SysYParserVisitor ) return ((SysYParserVisitor<? extends T>)visitor).visitReturn(this);
			else return visitor.visitChildren(this);
		}
	}

	public final StmtContext stmt() throws RecognitionException {
		StmtContext _localctx = new StmtContext(_ctx, getState());
		enterRule(_localctx, 34, RULE_stmt);
		int _la;
		try {
			setState(222);
			_errHandler.sync(this);
			switch ( getInterpreter().adaptivePredict(_input,23,_ctx) ) {
			case 1:
				_localctx = new ExpressionContext(_localctx);
				enterOuterAlt(_localctx, 1);
				{
				setState(189);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 120334587904L) != 0)) {
					{
					setState(188);
					expr(0);
					}
				}

				setState(191);
				match(SEMICOLON);
				}
				break;
			case 2:
				_localctx = new AssignmentContext(_localctx);
				enterOuterAlt(_localctx, 2);
				{
				setState(192);
				((AssignmentContext)_localctx).lvalue = varAccess();
				setState(193);
				match(ASSIGN);
				setState(194);
				((AssignmentContext)_localctx).value = expr(0);
				setState(195);
				match(SEMICOLON);
				}
				break;
			case 3:
				_localctx = new BlockContext(_localctx);
				enterOuterAlt(_localctx, 3);
				{
				setState(197);
				stmtBlock();
				}
				break;
			case 4:
				_localctx = new IfContext(_localctx);
				enterOuterAlt(_localctx, 4);
				{
				setState(198);
				match(IF);
				setState(199);
				match(L_PAREN);
				setState(200);
				((IfContext)_localctx).cond = expr(0);
				setState(201);
				match(R_PAREN);
				setState(202);
				((IfContext)_localctx).stmtTrue = stmt();
				setState(205);
				_errHandler.sync(this);
				switch ( getInterpreter().adaptivePredict(_input,21,_ctx) ) {
				case 1:
					{
					setState(203);
					match(ELSE);
					setState(204);
					((IfContext)_localctx).stmtFalse = stmt();
					}
					break;
				}
				}
				break;
			case 5:
				_localctx = new WhileContext(_localctx);
				enterOuterAlt(_localctx, 5);
				{
				setState(207);
				match(WHILE);
				setState(208);
				match(L_PAREN);
				setState(209);
				((WhileContext)_localctx).cond = expr(0);
				setState(210);
				match(R_PAREN);
				setState(211);
				((WhileContext)_localctx).stmtTrue = stmt();
				}
				break;
			case 6:
				_localctx = new BreakContext(_localctx);
				enterOuterAlt(_localctx, 6);
				{
				setState(213);
				match(BREAK);
				setState(214);
				match(SEMICOLON);
				}
				break;
			case 7:
				_localctx = new ContinueContext(_localctx);
				enterOuterAlt(_localctx, 7);
				{
				setState(215);
				match(CONTINUE);
				setState(216);
				match(SEMICOLON);
				}
				break;
			case 8:
				_localctx = new ReturnContext(_localctx);
				enterOuterAlt(_localctx, 8);
				{
				setState(217);
				match(RETURN);
				setState(219);
				_errHandler.sync(this);
				_la = _input.LA(1);
				if ((((_la) & ~0x3f) == 0 && ((1L << _la) & 120334587904L) != 0)) {
					{
					setState(218);
					((ReturnContext)_localctx).ret = expr(0);
					}
				}

				setState(221);
				match(SEMICOLON);
				}
				break;
			}
		}
		catch (RecognitionException re) {
			_localctx.exception = re;
			_errHandler.reportError(this, re);
			_errHandler.recover(this, re);
		}
		finally {
			exitRule();
		}
		return _localctx;
	}

	public boolean sempred(RuleContext _localctx, int ruleIndex, int predIndex) {
		switch (ruleIndex) {
		case 15:
			return expr_sempred((ExprContext)_localctx, predIndex);
		}
		return true;
	}
	private boolean expr_sempred(ExprContext _localctx, int predIndex) {
		switch (predIndex) {
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
		}
		return true;
	}

	public static final String _serializedATN =
		"\u0004\u0001\'\u00e1\u0002\u0000\u0007\u0000\u0002\u0001\u0007\u0001\u0002"+
		"\u0002\u0007\u0002\u0002\u0003\u0007\u0003\u0002\u0004\u0007\u0004\u0002"+
		"\u0005\u0007\u0005\u0002\u0006\u0007\u0006\u0002\u0007\u0007\u0007\u0002"+
		"\b\u0007\b\u0002\t\u0007\t\u0002\n\u0007\n\u0002\u000b\u0007\u000b\u0002"+
		"\f\u0007\f\u0002\r\u0007\r\u0002\u000e\u0007\u000e\u0002\u000f\u0007\u000f"+
		"\u0002\u0010\u0007\u0010\u0002\u0011\u0007\u0011\u0001\u0000\u0001\u0000"+
		"\u0005\u0000\'\b\u0000\n\u0000\f\u0000*\t\u0000\u0001\u0000\u0001\u0000"+
		"\u0001\u0001\u0001\u0001\u0001\u0002\u0001\u0002\u0003\u00022\b\u0002"+
		"\u0001\u0003\u0003\u00035\b\u0003\u0001\u0004\u0001\u0004\u0001\u0004"+
		"\u0001\u0005\u0001\u0005\u0001\u0005\u0001\u0005\u0001\u0006\u0005\u0006"+
		"?\b\u0006\n\u0006\f\u0006B\t\u0006\u0001\u0007\u0001\u0007\u0001\u0007"+
		"\u0001\u0007\u0001\u0007\u0005\u0007I\b\u0007\n\u0007\f\u0007L\t\u0007"+
		"\u0003\u0007N\b\u0007\u0001\u0007\u0003\u0007Q\b\u0007\u0001\b\u0001\b"+
		"\u0001\b\u0001\t\u0001\t\u0001\t\u0003\tY\b\t\u0001\n\u0001\n\u0001\n"+
		"\u0001\n\u0001\n\u0005\n`\b\n\n\n\f\nc\t\n\u0001\n\u0001\n\u0001\u000b"+
		"\u0001\u000b\u0001\u000b\u0001\u000b\u0001\u000b\u0003\u000bl\b\u000b"+
		"\u0001\f\u0001\f\u0001\f\u0001\f\u0001\f\u0001\f\u0005\ft\b\f\n\f\f\f"+
		"w\t\f\u0003\fy\b\f\u0001\f\u0001\f\u0001\f\u0001\r\u0001\r\u0001\r\u0001"+
		"\u000e\u0001\u000e\u0001\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001"+
		"\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0005\u000f\u008b\b\u000f\n"+
		"\u000f\f\u000f\u008e\t\u000f\u0003\u000f\u0090\b\u000f\u0001\u000f\u0001"+
		"\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001"+
		"\u000f\u0003\u000f\u009a\b\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001"+
		"\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001"+
		"\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001\u000f\u0001"+
		"\u000f\u0001\u000f\u0001\u000f\u0005\u000f\u00ae\b\u000f\n\u000f\f\u000f"+
		"\u00b1\t\u000f\u0001\u0010\u0001\u0010\u0001\u0010\u0005\u0010\u00b6\b"+
		"\u0010\n\u0010\f\u0010\u00b9\t\u0010\u0001\u0010\u0001\u0010\u0001\u0011"+
		"\u0003\u0011\u00be\b\u0011\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011"+
		"\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011"+
		"\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011\u0003\u0011\u00ce\b\u0011"+
		"\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011"+
		"\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011\u0001\u0011"+
		"\u0003\u0011\u00dc\b\u0011\u0001\u0011\u0003\u0011\u00df\b\u0011\u0001"+
		"\u0011\u0000\u0001\u001e\u0012\u0000\u0002\u0004\u0006\b\n\f\u000e\u0010"+
		"\u0012\u0014\u0016\u0018\u001a\u001c\u001e \"\u0000\u0006\u0001\u0000"+
		"\u0002\u0003\u0002\u0000\u000b\f\u0017\u0017\u0001\u0000\r\u000f\u0001"+
		"\u0000\u000b\f\u0001\u0000\u0013\u0016\u0001\u0000\u0011\u0012\u00f4\u0000"+
		"(\u0001\u0000\u0000\u0000\u0002-\u0001\u0000\u0000\u0000\u00041\u0001"+
		"\u0000\u0000\u0000\u00064\u0001\u0000\u0000\u0000\b6\u0001\u0000\u0000"+
		"\u0000\n9\u0001\u0000\u0000\u0000\f@\u0001\u0000\u0000\u0000\u000eP\u0001"+
		"\u0000\u0000\u0000\u0010R\u0001\u0000\u0000\u0000\u0012U\u0001\u0000\u0000"+
		"\u0000\u0014Z\u0001\u0000\u0000\u0000\u0016f\u0001\u0000\u0000\u0000\u0018"+
		"m\u0001\u0000\u0000\u0000\u001a}\u0001\u0000\u0000\u0000\u001c\u0080\u0001"+
		"\u0000\u0000\u0000\u001e\u0099\u0001\u0000\u0000\u0000 \u00b2\u0001\u0000"+
		"\u0000\u0000\"\u00de\u0001\u0000\u0000\u0000$\'\u0003\u0014\n\u0000%\'"+
		"\u0003\u0018\f\u0000&$\u0001\u0000\u0000\u0000&%\u0001\u0000\u0000\u0000"+
		"\'*\u0001\u0000\u0000\u0000(&\u0001\u0000\u0000\u0000()\u0001\u0000\u0000"+
		"\u0000)+\u0001\u0000\u0000\u0000*(\u0001\u0000\u0000\u0000+,\u0005\u0000"+
		"\u0000\u0001,\u0001\u0001\u0000\u0000\u0000-.\u0007\u0000\u0000\u0000"+
		".\u0003\u0001\u0000\u0000\u0000/2\u0003\u0002\u0001\u000002\u0005\u0004"+
		"\u0000\u00001/\u0001\u0000\u0000\u000010\u0001\u0000\u0000\u00002\u0005"+
		"\u0001\u0000\u0000\u000035\u0005\u0001\u0000\u000043\u0001\u0000\u0000"+
		"\u000045\u0001\u0000\u0000\u00005\u0007\u0001\u0000\u0000\u000067\u0005"+
		"\u001e\u0000\u000078\u0005\u001f\u0000\u00008\t\u0001\u0000\u0000\u0000"+
		"9:\u0005\u001e\u0000\u0000:;\u0003\u001e\u000f\u0000;<\u0005\u001f\u0000"+
		"\u0000<\u000b\u0001\u0000\u0000\u0000=?\u0003\n\u0005\u0000>=\u0001\u0000"+
		"\u0000\u0000?B\u0001\u0000\u0000\u0000@>\u0001\u0000\u0000\u0000@A\u0001"+
		"\u0000\u0000\u0000A\r\u0001\u0000\u0000\u0000B@\u0001\u0000\u0000\u0000"+
		"CQ\u0003\u001e\u000f\u0000DM\u0005\u001c\u0000\u0000EJ\u0003\u000e\u0007"+
		"\u0000FG\u0005 \u0000\u0000GI\u0003\u000e\u0007\u0000HF\u0001\u0000\u0000"+
		"\u0000IL\u0001\u0000\u0000\u0000JH\u0001\u0000\u0000\u0000JK\u0001\u0000"+
		"\u0000\u0000KN\u0001\u0000\u0000\u0000LJ\u0001\u0000\u0000\u0000ME\u0001"+
		"\u0000\u0000\u0000MN\u0001\u0000\u0000\u0000NO\u0001\u0000\u0000\u0000"+
		"OQ\u0005\u001d\u0000\u0000PC\u0001\u0000\u0000\u0000PD\u0001\u0000\u0000"+
		"\u0000Q\u000f\u0001\u0000\u0000\u0000RS\u0005\u0010\u0000\u0000ST\u0003"+
		"\u000e\u0007\u0000T\u0011\u0001\u0000\u0000\u0000UV\u0005\"\u0000\u0000"+
		"VX\u0003\f\u0006\u0000WY\u0003\u0010\b\u0000XW\u0001\u0000\u0000\u0000"+
		"XY\u0001\u0000\u0000\u0000Y\u0013\u0001\u0000\u0000\u0000Z[\u0003\u0006"+
		"\u0003\u0000[\\\u0003\u0002\u0001\u0000\\a\u0003\u0012\t\u0000]^\u0005"+
		" \u0000\u0000^`\u0003\u0012\t\u0000_]\u0001\u0000\u0000\u0000`c\u0001"+
		"\u0000\u0000\u0000a_\u0001\u0000\u0000\u0000ab\u0001\u0000\u0000\u0000"+
		"bd\u0001\u0000\u0000\u0000ca\u0001\u0000\u0000\u0000de\u0005!\u0000\u0000"+
		"e\u0015\u0001\u0000\u0000\u0000fg\u0003\u0002\u0001\u0000gk\u0005\"\u0000"+
		"\u0000hi\u0003\b\u0004\u0000ij\u0003\f\u0006\u0000jl\u0001\u0000\u0000"+
		"\u0000kh\u0001\u0000\u0000\u0000kl\u0001\u0000\u0000\u0000l\u0017\u0001"+
		"\u0000\u0000\u0000mn\u0003\u0004\u0002\u0000no\u0005\"\u0000\u0000ox\u0005"+
		"\u001a\u0000\u0000pu\u0003\u0016\u000b\u0000qr\u0005 \u0000\u0000rt\u0003"+
		"\u0016\u000b\u0000sq\u0001\u0000\u0000\u0000tw\u0001\u0000\u0000\u0000"+
		"us\u0001\u0000\u0000\u0000uv\u0001\u0000\u0000\u0000vy\u0001\u0000\u0000"+
		"\u0000wu\u0001\u0000\u0000\u0000xp\u0001\u0000\u0000\u0000xy\u0001\u0000"+
		"\u0000\u0000yz\u0001\u0000\u0000\u0000z{\u0005\u001b\u0000\u0000{|\u0003"+
		" \u0010\u0000|\u0019\u0001\u0000\u0000\u0000}~\u0005\"\u0000\u0000~\u007f"+
		"\u0003\f\u0006\u0000\u007f\u001b\u0001\u0000\u0000\u0000\u0080\u0081\u0003"+
		"\u001e\u000f\u0000\u0081\u001d\u0001\u0000\u0000\u0000\u0082\u0083\u0006"+
		"\u000f\uffff\uffff\u0000\u0083\u009a\u0005#\u0000\u0000\u0084\u009a\u0005"+
		"$\u0000\u0000\u0085\u0086\u0005\"\u0000\u0000\u0086\u008f\u0005\u001a"+
		"\u0000\u0000\u0087\u008c\u0003\u001c\u000e\u0000\u0088\u0089\u0005 \u0000"+
		"\u0000\u0089\u008b\u0003\u001c\u000e\u0000\u008a\u0088\u0001\u0000\u0000"+
		"\u0000\u008b\u008e\u0001\u0000\u0000\u0000\u008c\u008a\u0001\u0000\u0000"+
		"\u0000\u008c\u008d\u0001\u0000\u0000\u0000\u008d\u0090\u0001\u0000\u0000"+
		"\u0000\u008e\u008c\u0001\u0000\u0000\u0000\u008f\u0087\u0001\u0000\u0000"+
		"\u0000\u008f\u0090\u0001\u0000\u0000\u0000\u0090\u0091\u0001\u0000\u0000"+
		"\u0000\u0091\u009a\u0005\u001b\u0000\u0000\u0092\u009a\u0003\u001a\r\u0000"+
		"\u0093\u0094\u0005\u001a\u0000\u0000\u0094\u0095\u0003\u001e\u000f\u0000"+
		"\u0095\u0096\u0005\u001b\u0000\u0000\u0096\u009a\u0001\u0000\u0000\u0000"+
		"\u0097\u0098\u0007\u0001\u0000\u0000\u0098\u009a\u0003\u001e\u000f\u0007"+
		"\u0099\u0082\u0001\u0000\u0000\u0000\u0099\u0084\u0001\u0000\u0000\u0000"+
		"\u0099\u0085\u0001\u0000\u0000\u0000\u0099\u0092\u0001\u0000\u0000\u0000"+
		"\u0099\u0093\u0001\u0000\u0000\u0000\u0099\u0097\u0001\u0000\u0000\u0000"+
		"\u009a\u00af\u0001\u0000\u0000\u0000\u009b\u009c\n\u0006\u0000\u0000\u009c"+
		"\u009d\u0007\u0002\u0000\u0000\u009d\u00ae\u0003\u001e\u000f\u0007\u009e"+
		"\u009f\n\u0005\u0000\u0000\u009f\u00a0\u0007\u0003\u0000\u0000\u00a0\u00ae"+
		"\u0003\u001e\u000f\u0006\u00a1\u00a2\n\u0004\u0000\u0000\u00a2\u00a3\u0007"+
		"\u0004\u0000\u0000\u00a3\u00ae\u0003\u001e\u000f\u0005\u00a4\u00a5\n\u0003"+
		"\u0000\u0000\u00a5\u00a6\u0007\u0005\u0000\u0000\u00a6\u00ae\u0003\u001e"+
		"\u000f\u0004\u00a7\u00a8\n\u0002\u0000\u0000\u00a8\u00a9\u0005\u0018\u0000"+
		"\u0000\u00a9\u00ae\u0003\u001e\u000f\u0003\u00aa\u00ab\n\u0001\u0000\u0000"+
		"\u00ab\u00ac\u0005\u0019\u0000\u0000\u00ac\u00ae\u0003\u001e\u000f\u0002"+
		"\u00ad\u009b\u0001\u0000\u0000\u0000\u00ad\u009e\u0001\u0000\u0000\u0000"+
		"\u00ad\u00a1\u0001\u0000\u0000\u0000\u00ad\u00a4\u0001\u0000\u0000\u0000"+
		"\u00ad\u00a7\u0001\u0000\u0000\u0000\u00ad\u00aa\u0001\u0000\u0000\u0000"+
		"\u00ae\u00b1\u0001\u0000\u0000\u0000\u00af\u00ad\u0001\u0000\u0000\u0000"+
		"\u00af\u00b0\u0001\u0000\u0000\u0000\u00b0\u001f\u0001\u0000\u0000\u0000"+
		"\u00b1\u00af\u0001\u0000\u0000\u0000\u00b2\u00b7\u0005\u001c\u0000\u0000"+
		"\u00b3\u00b6\u0003\u0014\n\u0000\u00b4\u00b6\u0003\"\u0011\u0000\u00b5"+
		"\u00b3\u0001\u0000\u0000\u0000\u00b5\u00b4\u0001\u0000\u0000\u0000\u00b6"+
		"\u00b9\u0001\u0000\u0000\u0000\u00b7\u00b5\u0001\u0000\u0000\u0000\u00b7"+
		"\u00b8\u0001\u0000\u0000\u0000\u00b8\u00ba\u0001\u0000\u0000\u0000\u00b9"+
		"\u00b7\u0001\u0000\u0000\u0000\u00ba\u00bb\u0005\u001d\u0000\u0000\u00bb"+
		"!\u0001\u0000\u0000\u0000\u00bc\u00be\u0003\u001e\u000f\u0000\u00bd\u00bc"+
		"\u0001\u0000\u0000\u0000\u00bd\u00be\u0001\u0000\u0000\u0000\u00be\u00bf"+
		"\u0001\u0000\u0000\u0000\u00bf\u00df\u0005!\u0000\u0000\u00c0\u00c1\u0003"+
		"\u001a\r\u0000\u00c1\u00c2\u0005\u0010\u0000\u0000\u00c2\u00c3\u0003\u001e"+
		"\u000f\u0000\u00c3\u00c4\u0005!\u0000\u0000\u00c4\u00df\u0001\u0000\u0000"+
		"\u0000\u00c5\u00df\u0003 \u0010\u0000\u00c6\u00c7\u0005\u0005\u0000\u0000"+
		"\u00c7\u00c8\u0005\u001a\u0000\u0000\u00c8\u00c9\u0003\u001e\u000f\u0000"+
		"\u00c9\u00ca\u0005\u001b\u0000\u0000\u00ca\u00cd\u0003\"\u0011\u0000\u00cb"+
		"\u00cc\u0005\u0006\u0000\u0000\u00cc\u00ce\u0003\"\u0011\u0000\u00cd\u00cb"+
		"\u0001\u0000\u0000\u0000\u00cd\u00ce\u0001\u0000\u0000\u0000\u00ce\u00df"+
		"\u0001\u0000\u0000\u0000\u00cf\u00d0\u0005\u0007\u0000\u0000\u00d0\u00d1"+
		"\u0005\u001a\u0000\u0000\u00d1\u00d2\u0003\u001e\u000f\u0000\u00d2\u00d3"+
		"\u0005\u001b\u0000\u0000\u00d3\u00d4\u0003\"\u0011\u0000\u00d4\u00df\u0001"+
		"\u0000\u0000\u0000\u00d5\u00d6\u0005\b\u0000\u0000\u00d6\u00df\u0005!"+
		"\u0000\u0000\u00d7\u00d8\u0005\t\u0000\u0000\u00d8\u00df\u0005!\u0000"+
		"\u0000\u00d9\u00db\u0005\n\u0000\u0000\u00da\u00dc\u0003\u001e\u000f\u0000"+
		"\u00db\u00da\u0001\u0000\u0000\u0000\u00db\u00dc\u0001\u0000\u0000\u0000"+
		"\u00dc\u00dd\u0001\u0000\u0000\u0000\u00dd\u00df\u0005!\u0000\u0000\u00de"+
		"\u00bd\u0001\u0000\u0000\u0000\u00de\u00c0\u0001\u0000\u0000\u0000\u00de"+
		"\u00c5\u0001\u0000\u0000\u0000\u00de\u00c6\u0001\u0000\u0000\u0000\u00de"+
		"\u00cf\u0001\u0000\u0000\u0000\u00de\u00d5\u0001\u0000\u0000\u0000\u00de"+
		"\u00d7\u0001\u0000\u0000\u0000\u00de\u00d9\u0001\u0000\u0000\u0000\u00df"+
		"#\u0001\u0000\u0000\u0000\u0018&(14@JMPXakux\u008c\u008f\u0099\u00ad\u00af"+
		"\u00b5\u00b7\u00bd\u00cd\u00db\u00de";
	public static final ATN _ATN =
		new ATNDeserializer().deserialize(_serializedATN.toCharArray());
	static {
		_decisionToDFA = new DFA[_ATN.getNumberOfDecisions()];
		for (int i = 0; i < _ATN.getNumberOfDecisions(); i++) {
			_decisionToDFA[i] = new DFA(_ATN.getDecisionState(i), i);
		}
	}
}