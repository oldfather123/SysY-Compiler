// Generated from src/frontend/antlr_dep/SysY.g4 by ANTLR 4.8
#![allow(dead_code)]
#![allow(non_snake_case)]
#![allow(non_upper_case_globals)]
#![allow(nonstandard_style)]
#![allow(unused_imports)]
#![allow(unused_mut)]
#![allow(unused_braces)]
use super::sysylistener::*;
use super::sysyvisitor::*;
use antlr_rust::atn::{ATN, INVALID_ALT};
use antlr_rust::atn_deserializer::ATNDeserializer;
use antlr_rust::dfa::DFA;
use antlr_rust::error_strategy::{DefaultErrorStrategy, ErrorStrategy};
use antlr_rust::errors::*;
use antlr_rust::int_stream::EOF;
use antlr_rust::parser::{BaseParser, Parser, ParserNodeType, ParserRecog};
use antlr_rust::parser_atn_simulator::ParserATNSimulator;
use antlr_rust::parser_rule_context::{cast, cast_mut, BaseParserRuleContext, ParserRuleContext};
use antlr_rust::recognizer::{Actions, Recognizer};
use antlr_rust::rule_context::{BaseRuleContext, CustomRuleContext, RuleContext};
use antlr_rust::token::{OwningToken, Token, TOKEN_EOF};
use antlr_rust::token_factory::{CommonTokenFactory, TokenAware, TokenFactory};
use antlr_rust::token_stream::TokenStream;
use antlr_rust::tree::*;
use antlr_rust::vocabulary::{Vocabulary, VocabularyImpl};
use antlr_rust::PredictionContextCache;
use antlr_rust::TokenSource;

use antlr_rust::lazy_static;
use antlr_rust::{TidAble, TidExt};

use std::any::{Any, TypeId};
use std::borrow::{Borrow, BorrowMut};
use std::cell::RefCell;
use std::convert::TryFrom;
use std::marker::PhantomData;
use std::ops::{Deref, DerefMut};
use std::rc::Rc;
use std::sync::Arc;

pub const T__0: isize = 1;
pub const Int: isize = 2;
pub const Float: isize = 3;
pub const Void: isize = 4;
pub const Const: isize = 5;
pub const Return: isize = 6;
pub const If: isize = 7;
pub const Else: isize = 8;
pub const For: isize = 9;
pub const While: isize = 10;
pub const Do: isize = 11;
pub const Break: isize = 12;
pub const Continue: isize = 13;
pub const Lparen: isize = 14;
pub const Rparen: isize = 15;
pub const Lbrkt: isize = 16;
pub const Rbrkt: isize = 17;
pub const Lbrace: isize = 18;
pub const Rbrace: isize = 19;
pub const Comma: isize = 20;
pub const Semicolon: isize = 21;
pub const Question: isize = 22;
pub const Colon: isize = 23;
pub const Minus: isize = 24;
pub const Exclamation: isize = 25;
pub const Tilde: isize = 26;
pub const Addition: isize = 27;
pub const Multiplication: isize = 28;
pub const Division: isize = 29;
pub const Modulo: isize = 30;
pub const LAND: isize = 31;
pub const LOR: isize = 32;
pub const EQ: isize = 33;
pub const NEQ: isize = 34;
pub const LT: isize = 35;
pub const LE: isize = 36;
pub const GT: isize = 37;
pub const GE: isize = 38;
pub const IntLiteral: isize = 39;
pub const FloatLiteral: isize = 40;
pub const Identifier: isize = 41;
pub const WS: isize = 42;
pub const LINE_COMMENT: isize = 43;
pub const COMMENT: isize = 44;
pub const DigitSequence: isize = 45;
pub const RULE_compUnit: usize = 0;
pub const RULE_decl: usize = 1;
pub const RULE_constDecl: usize = 2;
pub const RULE_bType: usize = 3;
pub const RULE_constDef: usize = 4;
pub const RULE_constInitVal: usize = 5;
pub const RULE_varDecl: usize = 6;
pub const RULE_varDef: usize = 7;
pub const RULE_initVal: usize = 8;
pub const RULE_funcDef: usize = 9;
pub const RULE_funcType: usize = 10;
pub const RULE_funcFParams: usize = 11;
pub const RULE_funcFParam: usize = 12;
pub const RULE_block: usize = 13;
pub const RULE_blockItem: usize = 14;
pub const RULE_stmt: usize = 15;
pub const RULE_exp: usize = 16;
pub const RULE_cond: usize = 17;
pub const RULE_lVal: usize = 18;
pub const RULE_primaryExp: usize = 19;
pub const RULE_number: usize = 20;
pub const RULE_unaryExp: usize = 21;
pub const RULE_unaryOp: usize = 22;
pub const RULE_funcRParams: usize = 23;
pub const RULE_funcRParam: usize = 24;
pub const RULE_mulExp: usize = 25;
pub const RULE_addExp: usize = 26;
pub const RULE_relExp: usize = 27;
pub const RULE_eqExp: usize = 28;
pub const RULE_lAndExp: usize = 29;
pub const RULE_lOrExp: usize = 30;
pub const RULE_constExp: usize = 31;
pub const ruleNames: [&'static str; 32] = [
    "compUnit",
    "decl",
    "constDecl",
    "bType",
    "constDef",
    "constInitVal",
    "varDecl",
    "varDef",
    "initVal",
    "funcDef",
    "funcType",
    "funcFParams",
    "funcFParam",
    "block",
    "blockItem",
    "stmt",
    "exp",
    "cond",
    "lVal",
    "primaryExp",
    "number",
    "unaryExp",
    "unaryOp",
    "funcRParams",
    "funcRParam",
    "mulExp",
    "addExp",
    "relExp",
    "eqExp",
    "lAndExp",
    "lOrExp",
    "constExp",
];

pub const _LITERAL_NAMES: [Option<&'static str>; 39] = [
    None,
    Some("'='"),
    Some("'int'"),
    Some("'float'"),
    Some("'void'"),
    Some("'const'"),
    Some("'return'"),
    Some("'if'"),
    Some("'else'"),
    Some("'for'"),
    Some("'while'"),
    Some("'do'"),
    Some("'break'"),
    Some("'continue'"),
    Some("'('"),
    Some("')'"),
    Some("'['"),
    Some("']'"),
    Some("'{'"),
    Some("'}'"),
    Some("','"),
    Some("';'"),
    Some("'?'"),
    Some("':'"),
    Some("'-'"),
    Some("'!'"),
    Some("'~'"),
    Some("'+'"),
    Some("'*'"),
    Some("'/'"),
    Some("'%'"),
    Some("'&&'"),
    Some("'||'"),
    Some("'=='"),
    Some("'!='"),
    Some("'<'"),
    Some("'<='"),
    Some("'>'"),
    Some("'>='"),
];
pub const _SYMBOLIC_NAMES: [Option<&'static str>; 46] = [
    None,
    None,
    Some("Int"),
    Some("Float"),
    Some("Void"),
    Some("Const"),
    Some("Return"),
    Some("If"),
    Some("Else"),
    Some("For"),
    Some("While"),
    Some("Do"),
    Some("Break"),
    Some("Continue"),
    Some("Lparen"),
    Some("Rparen"),
    Some("Lbrkt"),
    Some("Rbrkt"),
    Some("Lbrace"),
    Some("Rbrace"),
    Some("Comma"),
    Some("Semicolon"),
    Some("Question"),
    Some("Colon"),
    Some("Minus"),
    Some("Exclamation"),
    Some("Tilde"),
    Some("Addition"),
    Some("Multiplication"),
    Some("Division"),
    Some("Modulo"),
    Some("LAND"),
    Some("LOR"),
    Some("EQ"),
    Some("NEQ"),
    Some("LT"),
    Some("LE"),
    Some("GT"),
    Some("GE"),
    Some("IntLiteral"),
    Some("FloatLiteral"),
    Some("Identifier"),
    Some("WS"),
    Some("LINE_COMMENT"),
    Some("COMMENT"),
    Some("DigitSequence"),
];
lazy_static! {
    static ref _shared_context_cache: Arc<PredictionContextCache> =
        Arc::new(PredictionContextCache::new());
    static ref VOCABULARY: Box<dyn Vocabulary> = Box::new(VocabularyImpl::new(
        _LITERAL_NAMES.iter(),
        _SYMBOLIC_NAMES.iter(),
        None
    ));
}

type BaseParserType<'input, I> = BaseParser<
    'input,
    SysYParserExt<'input>,
    I,
    SysYParserContextType,
    dyn SysYListener<'input> + 'input,
>;

type TokenType<'input> = <LocalTokenFactory<'input> as TokenFactory<'input>>::Tok;
pub type LocalTokenFactory<'input> = CommonTokenFactory;

pub type SysYTreeWalker<'input, 'a> =
    ParseTreeWalker<'input, 'a, SysYParserContextType, dyn SysYListener<'input> + 'a>;

/// Parser for SysY grammar
pub struct SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    base: BaseParserType<'input, I>,
    interpreter: Arc<ParserATNSimulator>,
    _shared_context_cache: Box<PredictionContextCache>,
    pub err_handler: H,
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn get_serialized_atn() -> &'static str {
        _serializedATN
    }

    pub fn set_error_strategy(&mut self, strategy: H) {
        self.err_handler = strategy
    }

    pub fn with_strategy(input: I, strategy: H) -> Self {
        antlr_rust::recognizer::check_version("0", "3");
        let interpreter = Arc::new(ParserATNSimulator::new(
            _ATN.clone(),
            _decision_to_DFA.clone(),
            _shared_context_cache.clone(),
        ));
        Self {
            base: BaseParser::new_base_parser(
                input,
                Arc::clone(&interpreter),
                SysYParserExt {
                    _pd: Default::default(),
                },
            ),
            interpreter,
            _shared_context_cache: Box::new(PredictionContextCache::new()),
            err_handler: strategy,
        }
    }
}

type DynStrategy<'input, I> = Box<dyn ErrorStrategy<'input, BaseParserType<'input, I>> + 'input>;

impl<'input, I> SysYParser<'input, I, DynStrategy<'input, I>>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
{
    pub fn with_dyn_strategy(input: I) -> Self {
        Self::with_strategy(input, Box::new(DefaultErrorStrategy::new()))
    }
}

impl<'input, I> SysYParser<'input, I, DefaultErrorStrategy<'input, SysYParserContextType>>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
{
    pub fn new(input: I) -> Self {
        Self::with_strategy(input, DefaultErrorStrategy::new())
    }
}

/// Trait for monomorphized trait object that corresponds to the nodes of parse tree generated for SysYParser
pub trait SysYParserContext<'input>:
    for<'x> Listenable<dyn SysYListener<'input> + 'x>
    + for<'x> Visitable<dyn SysYVisitor<'input> + 'x>
    + ParserRuleContext<'input, TF = LocalTokenFactory<'input>, Ctx = SysYParserContextType>
{
}

antlr_rust::coerce_from! { 'input : SysYParserContext<'input> }

impl<'input, 'x, T> VisitableDyn<T> for dyn SysYParserContext<'input> + 'input
where
    T: SysYVisitor<'input> + 'x,
{
    fn accept_dyn(&self, visitor: &mut T) {
        self.accept(visitor as &mut (dyn SysYVisitor<'input> + 'x))
    }
}

impl<'input> SysYParserContext<'input> for TerminalNode<'input, SysYParserContextType> {}
impl<'input> SysYParserContext<'input> for ErrorNode<'input, SysYParserContextType> {}

antlr_rust::tid! { impl<'input> TidAble<'input> for dyn SysYParserContext<'input> + 'input }

antlr_rust::tid! { impl<'input> TidAble<'input> for dyn SysYListener<'input> + 'input }

pub struct SysYParserContextType;
antlr_rust::tid! {SysYParserContextType}

impl<'input> ParserNodeType<'input> for SysYParserContextType {
    type TF = LocalTokenFactory<'input>;
    type Type = dyn SysYParserContext<'input> + 'input;
}

impl<'input, I, H> Deref for SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    type Target = BaseParserType<'input, I>;

    fn deref(&self) -> &Self::Target {
        &self.base
    }
}

impl<'input, I, H> DerefMut for SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    fn deref_mut(&mut self) -> &mut Self::Target {
        &mut self.base
    }
}

pub struct SysYParserExt<'input> {
    _pd: PhantomData<&'input str>,
}

impl<'input> SysYParserExt<'input> {}
antlr_rust::tid! { SysYParserExt<'a> }

impl<'input> TokenAware<'input> for SysYParserExt<'input> {
    type TF = LocalTokenFactory<'input>;
}

impl<'input, I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>>
    ParserRecog<'input, BaseParserType<'input, I>> for SysYParserExt<'input>
{
}

impl<'input, I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>>
    Actions<'input, BaseParserType<'input, I>> for SysYParserExt<'input>
{
    fn get_grammar_file_name(&self) -> &str {
        "SysY.g4"
    }

    fn get_rule_names(&self) -> &[&str] {
        &ruleNames
    }

    fn get_vocabulary(&self) -> &dyn Vocabulary {
        &**VOCABULARY
    }
    fn sempred(
        _localctx: Option<&(dyn SysYParserContext<'input> + 'input)>,
        rule_index: isize,
        pred_index: isize,
        recog: &mut BaseParserType<'input, I>,
    ) -> bool {
        match rule_index {
            25 => SysYParser::<'input, I, _>::mulExp_sempred(
                _localctx.and_then(|x| x.downcast_ref()),
                pred_index,
                recog,
            ),
            26 => SysYParser::<'input, I, _>::addExp_sempred(
                _localctx.and_then(|x| x.downcast_ref()),
                pred_index,
                recog,
            ),
            27 => SysYParser::<'input, I, _>::relExp_sempred(
                _localctx.and_then(|x| x.downcast_ref()),
                pred_index,
                recog,
            ),
            28 => SysYParser::<'input, I, _>::eqExp_sempred(
                _localctx.and_then(|x| x.downcast_ref()),
                pred_index,
                recog,
            ),
            29 => SysYParser::<'input, I, _>::lAndExp_sempred(
                _localctx.and_then(|x| x.downcast_ref()),
                pred_index,
                recog,
            ),
            30 => SysYParser::<'input, I, _>::lOrExp_sempred(
                _localctx.and_then(|x| x.downcast_ref()),
                pred_index,
                recog,
            ),
            _ => true,
        }
    }
}

impl<'input, I> SysYParser<'input, I, DefaultErrorStrategy<'input, SysYParserContextType>>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
{
    fn mulExp_sempred(
        _localctx: Option<&MulExpContext<'input>>,
        pred_index: isize,
        recog: &mut <Self as Deref>::Target,
    ) -> bool {
        match pred_index {
            0 => recog.precpred(None, 1),
            _ => true,
        }
    }
    fn addExp_sempred(
        _localctx: Option<&AddExpContext<'input>>,
        pred_index: isize,
        recog: &mut <Self as Deref>::Target,
    ) -> bool {
        match pred_index {
            1 => recog.precpred(None, 1),
            _ => true,
        }
    }
    fn relExp_sempred(
        _localctx: Option<&RelExpContext<'input>>,
        pred_index: isize,
        recog: &mut <Self as Deref>::Target,
    ) -> bool {
        match pred_index {
            2 => recog.precpred(None, 1),
            _ => true,
        }
    }
    fn eqExp_sempred(
        _localctx: Option<&EqExpContext<'input>>,
        pred_index: isize,
        recog: &mut <Self as Deref>::Target,
    ) -> bool {
        match pred_index {
            3 => recog.precpred(None, 1),
            _ => true,
        }
    }
    fn lAndExp_sempred(
        _localctx: Option<&LAndExpContext<'input>>,
        pred_index: isize,
        recog: &mut <Self as Deref>::Target,
    ) -> bool {
        match pred_index {
            4 => recog.precpred(None, 1),
            _ => true,
        }
    }
    fn lOrExp_sempred(
        _localctx: Option<&LOrExpContext<'input>>,
        pred_index: isize,
        recog: &mut <Self as Deref>::Target,
    ) -> bool {
        match pred_index {
            5 => recog.precpred(None, 1),
            _ => true,
        }
    }
}
//------------------- compUnit ----------------
pub type CompUnitContextAll<'input> = CompUnitContext<'input>;

pub type CompUnitContext<'input> = BaseParserRuleContext<'input, CompUnitContextExt<'input>>;

#[derive(Clone)]
pub struct CompUnitContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for CompUnitContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for CompUnitContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_compUnit(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_compUnit(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for CompUnitContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_compUnit(self);
    }
}

impl<'input> CustomRuleContext<'input> for CompUnitContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_compUnit
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_compUnit }
}
antlr_rust::tid! {CompUnitContextExt<'a>}

impl<'input> CompUnitContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<CompUnitContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            CompUnitContextExt { ph: PhantomData },
        ))
    }
}

pub trait CompUnitContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<CompUnitContextExt<'input>>
{
    /// Retrieves first TerminalNode corresponding to token EOF
    /// Returns `None` if there is no child corresponding to token EOF
    fn EOF(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(EOF, 0)
    }
    fn decl_all(&self) -> Vec<Rc<DeclContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn decl(&self, i: usize) -> Option<Rc<DeclContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    fn funcDef_all(&self) -> Vec<Rc<FuncDefContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn funcDef(&self, i: usize) -> Option<Rc<FuncDefContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
}

impl<'input> CompUnitContextAttrs<'input> for CompUnitContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn compUnit(&mut self) -> Result<Rc<CompUnitContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = CompUnitContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 0, RULE_compUnit);
        let mut _localctx: Rc<CompUnitContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                recog.base.set_state(68);
                recog.err_handler.sync(&mut recog.base)?;
                _la = recog.base.input.la(1);
                while ((_la) & !0x3f) == 0
                    && ((1usize << _la)
                        & ((1usize << Int)
                            | (1usize << Float)
                            | (1usize << Void)
                            | (1usize << Const)))
                        != 0
                {
                    {
                        recog.base.set_state(66);
                        recog.err_handler.sync(&mut recog.base)?;
                        match recog.interpreter.adaptive_predict(0, &mut recog.base)? {
                            1 => {
                                {
                                    /*InvokeRule decl*/
                                    recog.base.set_state(64);
                                    recog.decl()?;
                                }
                            }
                            2 => {
                                {
                                    /*InvokeRule funcDef*/
                                    recog.base.set_state(65);
                                    recog.funcDef()?;
                                }
                            }

                            _ => {}
                        }
                    }
                    recog.base.set_state(70);
                    recog.err_handler.sync(&mut recog.base)?;
                    _la = recog.base.input.la(1);
                }
                recog.base.set_state(71);
                recog.base.match_token(EOF, &mut recog.err_handler)?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- decl ----------------
pub type DeclContextAll<'input> = DeclContext<'input>;

pub type DeclContext<'input> = BaseParserRuleContext<'input, DeclContextExt<'input>>;

#[derive(Clone)]
pub struct DeclContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for DeclContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for DeclContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_decl(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_decl(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for DeclContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_decl(self);
    }
}

impl<'input> CustomRuleContext<'input> for DeclContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_decl
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_decl }
}
antlr_rust::tid! {DeclContextExt<'a>}

impl<'input> DeclContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<DeclContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            DeclContextExt { ph: PhantomData },
        ))
    }
}

pub trait DeclContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<DeclContextExt<'input>>
{
    fn constDecl(&self) -> Option<Rc<ConstDeclContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn varDecl(&self) -> Option<Rc<VarDeclContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> DeclContextAttrs<'input> for DeclContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn decl(&mut self) -> Result<Rc<DeclContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = DeclContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 2, RULE_decl);
        let mut _localctx: Rc<DeclContextAll> = _localctx;
        let result: Result<(), ANTLRError> = (|| {
            recog.base.set_state(75);
            recog.err_handler.sync(&mut recog.base)?;
            match recog.base.input.la(1) {
                Const => {
                    //recog.base.enter_outer_alt(_localctx.clone(), 1);
                    recog.base.enter_outer_alt(None, 1);
                    {
                        /*InvokeRule constDecl*/
                        recog.base.set_state(73);
                        recog.constDecl()?;
                    }
                }

                Int | Float => {
                    //recog.base.enter_outer_alt(_localctx.clone(), 2);
                    recog.base.enter_outer_alt(None, 2);
                    {
                        /*InvokeRule varDecl*/
                        recog.base.set_state(74);
                        recog.varDecl()?;
                    }
                }

                _ => Err(ANTLRError::NoAltError(NoViableAltError::new(
                    &mut recog.base,
                )))?,
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- constDecl ----------------
pub type ConstDeclContextAll<'input> = ConstDeclContext<'input>;

pub type ConstDeclContext<'input> = BaseParserRuleContext<'input, ConstDeclContextExt<'input>>;

#[derive(Clone)]
pub struct ConstDeclContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for ConstDeclContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ConstDeclContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_constDecl(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_constDecl(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ConstDeclContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_constDecl(self);
    }
}

impl<'input> CustomRuleContext<'input> for ConstDeclContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_constDecl
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_constDecl }
}
antlr_rust::tid! {ConstDeclContextExt<'a>}

impl<'input> ConstDeclContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<ConstDeclContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            ConstDeclContextExt { ph: PhantomData },
        ))
    }
}

pub trait ConstDeclContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<ConstDeclContextExt<'input>>
{
    /// Retrieves first TerminalNode corresponding to token Const
    /// Returns `None` if there is no child corresponding to token Const
    fn Const(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Const, 0)
    }
    fn bType(&self) -> Option<Rc<BTypeContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn constDef_all(&self) -> Vec<Rc<ConstDefContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn constDef(&self, i: usize) -> Option<Rc<ConstDefContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves first TerminalNode corresponding to token Semicolon
    /// Returns `None` if there is no child corresponding to token Semicolon
    fn Semicolon(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Semicolon, 0)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Comma in current rule
    fn Comma_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Comma, starting from 0.
    /// Returns `None` if number of children corresponding to token Comma is less or equal than `i`.
    fn Comma(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Comma, i)
    }
}

impl<'input> ConstDeclContextAttrs<'input> for ConstDeclContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn constDecl(&mut self) -> Result<Rc<ConstDeclContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = ConstDeclContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 4, RULE_constDecl);
        let mut _localctx: Rc<ConstDeclContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                recog.base.set_state(77);
                recog.base.match_token(Const, &mut recog.err_handler)?;

                /*InvokeRule bType*/
                recog.base.set_state(78);
                recog.bType()?;

                /*InvokeRule constDef*/
                recog.base.set_state(79);
                recog.constDef()?;

                recog.base.set_state(84);
                recog.err_handler.sync(&mut recog.base)?;
                _la = recog.base.input.la(1);
                while _la == Comma {
                    {
                        {
                            recog.base.set_state(80);
                            recog.base.match_token(Comma, &mut recog.err_handler)?;

                            /*InvokeRule constDef*/
                            recog.base.set_state(81);
                            recog.constDef()?;
                        }
                    }
                    recog.base.set_state(86);
                    recog.err_handler.sync(&mut recog.base)?;
                    _la = recog.base.input.la(1);
                }
                recog.base.set_state(87);
                recog.base.match_token(Semicolon, &mut recog.err_handler)?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- bType ----------------
pub type BTypeContextAll<'input> = BTypeContext<'input>;

pub type BTypeContext<'input> = BaseParserRuleContext<'input, BTypeContextExt<'input>>;

#[derive(Clone)]
pub struct BTypeContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for BTypeContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for BTypeContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_bType(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_bType(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for BTypeContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_bType(self);
    }
}

impl<'input> CustomRuleContext<'input> for BTypeContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_bType
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_bType }
}
antlr_rust::tid! {BTypeContextExt<'a>}

impl<'input> BTypeContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<BTypeContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            BTypeContextExt { ph: PhantomData },
        ))
    }
}

pub trait BTypeContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<BTypeContextExt<'input>>
{
    /// Retrieves first TerminalNode corresponding to token Int
    /// Returns `None` if there is no child corresponding to token Int
    fn Int(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Int, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Float
    /// Returns `None` if there is no child corresponding to token Float
    fn Float(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Float, 0)
    }
}

impl<'input> BTypeContextAttrs<'input> for BTypeContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn bType(&mut self) -> Result<Rc<BTypeContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = BTypeContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 6, RULE_bType);
        let mut _localctx: Rc<BTypeContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                recog.base.set_state(89);
                _la = recog.base.input.la(1);
                if { !(_la == Int || _la == Float) } {
                    recog.err_handler.recover_inline(&mut recog.base)?;
                } else {
                    if recog.base.input.la(1) == TOKEN_EOF {
                        recog.base.matched_eof = true
                    };
                    recog.err_handler.report_match(&mut recog.base);
                    recog.base.consume(&mut recog.err_handler);
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- constDef ----------------
pub type ConstDefContextAll<'input> = ConstDefContext<'input>;

pub type ConstDefContext<'input> = BaseParserRuleContext<'input, ConstDefContextExt<'input>>;

#[derive(Clone)]
pub struct ConstDefContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for ConstDefContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ConstDefContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_constDef(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_constDef(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ConstDefContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_constDef(self);
    }
}

impl<'input> CustomRuleContext<'input> for ConstDefContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_constDef
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_constDef }
}
antlr_rust::tid! {ConstDefContextExt<'a>}

impl<'input> ConstDefContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<ConstDefContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            ConstDefContextExt { ph: PhantomData },
        ))
    }
}

pub trait ConstDefContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<ConstDefContextExt<'input>>
{
    /// Retrieves first TerminalNode corresponding to token Identifier
    /// Returns `None` if there is no child corresponding to token Identifier
    fn Identifier(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Identifier, 0)
    }
    fn constInitVal(&self) -> Option<Rc<ConstInitValContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Lbrkt in current rule
    fn Lbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Lbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Lbrkt is less or equal than `i`.
    fn Lbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lbrkt, i)
    }
    fn constExp_all(&self) -> Vec<Rc<ConstExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn constExp(&self, i: usize) -> Option<Rc<ConstExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Rbrkt in current rule
    fn Rbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Rbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Rbrkt is less or equal than `i`.
    fn Rbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rbrkt, i)
    }
}

impl<'input> ConstDefContextAttrs<'input> for ConstDefContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn constDef(&mut self) -> Result<Rc<ConstDefContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = ConstDefContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 8, RULE_constDef);
        let mut _localctx: Rc<ConstDefContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                recog.base.set_state(91);
                recog.base.match_token(Identifier, &mut recog.err_handler)?;

                recog.base.set_state(98);
                recog.err_handler.sync(&mut recog.base)?;
                _la = recog.base.input.la(1);
                while _la == Lbrkt {
                    {
                        {
                            recog.base.set_state(92);
                            recog.base.match_token(Lbrkt, &mut recog.err_handler)?;

                            /*InvokeRule constExp*/
                            recog.base.set_state(93);
                            recog.constExp()?;

                            recog.base.set_state(94);
                            recog.base.match_token(Rbrkt, &mut recog.err_handler)?;
                        }
                    }
                    recog.base.set_state(100);
                    recog.err_handler.sync(&mut recog.base)?;
                    _la = recog.base.input.la(1);
                }
                recog.base.set_state(101);
                recog.base.match_token(T__0, &mut recog.err_handler)?;

                /*InvokeRule constInitVal*/
                recog.base.set_state(102);
                recog.constInitVal()?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- constInitVal ----------------
#[derive(Debug)]
pub enum ConstInitValContextAll<'input> {
    ListConstInitValContext(ListConstInitValContext<'input>),
    ScalarConstInitValContext(ScalarConstInitValContext<'input>),
    Error(ConstInitValContext<'input>),
}
antlr_rust::tid! {ConstInitValContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for ConstInitValContextAll<'input> {}

impl<'input> SysYParserContext<'input> for ConstInitValContextAll<'input> {}

impl<'input> Deref for ConstInitValContextAll<'input> {
    type Target = dyn ConstInitValContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use ConstInitValContextAll::*;
        match self {
            ListConstInitValContext(inner) => inner,
            ScalarConstInitValContext(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ConstInitValContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ConstInitValContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type ConstInitValContext<'input> =
    BaseParserRuleContext<'input, ConstInitValContextExt<'input>>;

#[derive(Clone)]
pub struct ConstInitValContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for ConstInitValContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ConstInitValContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ConstInitValContext<'input> {}

impl<'input> CustomRuleContext<'input> for ConstInitValContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_constInitVal
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_constInitVal }
}
antlr_rust::tid! {ConstInitValContextExt<'a>}

impl<'input> ConstInitValContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<ConstInitValContextAll<'input>> {
        Rc::new(ConstInitValContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                ConstInitValContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait ConstInitValContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<ConstInitValContextExt<'input>>
{
}

impl<'input> ConstInitValContextAttrs<'input> for ConstInitValContext<'input> {}

pub type ListConstInitValContext<'input> =
    BaseParserRuleContext<'input, ListConstInitValContextExt<'input>>;

pub trait ListConstInitValContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Lbrace
    /// Returns `None` if there is no child corresponding to token Lbrace
    fn Lbrace(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lbrace, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Rbrace
    /// Returns `None` if there is no child corresponding to token Rbrace
    fn Rbrace(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rbrace, 0)
    }
    fn constInitVal_all(&self) -> Vec<Rc<ConstInitValContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn constInitVal(&self, i: usize) -> Option<Rc<ConstInitValContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Comma in current rule
    fn Comma_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Comma, starting from 0.
    /// Returns `None` if number of children corresponding to token Comma is less or equal than `i`.
    fn Comma(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Comma, i)
    }
}

impl<'input> ListConstInitValContextAttrs<'input> for ListConstInitValContext<'input> {}

pub struct ListConstInitValContextExt<'input> {
    base: ConstInitValContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {ListConstInitValContextExt<'a>}

impl<'input> SysYParserContext<'input> for ListConstInitValContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ListConstInitValContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_listConstInitVal(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_listConstInitVal(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ListConstInitValContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_listConstInitVal(self);
    }
}

impl<'input> CustomRuleContext<'input> for ListConstInitValContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_constInitVal
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_constInitVal }
}

impl<'input> Borrow<ConstInitValContextExt<'input>> for ListConstInitValContext<'input> {
    fn borrow(&self) -> &ConstInitValContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<ConstInitValContextExt<'input>> for ListConstInitValContext<'input> {
    fn borrow_mut(&mut self) -> &mut ConstInitValContextExt<'input> {
        &mut self.base
    }
}

impl<'input> ConstInitValContextAttrs<'input> for ListConstInitValContext<'input> {}

impl<'input> ListConstInitValContextExt<'input> {
    fn new(ctx: &dyn ConstInitValContextAttrs<'input>) -> Rc<ConstInitValContextAll<'input>> {
        Rc::new(ConstInitValContextAll::ListConstInitValContext(
            BaseParserRuleContext::copy_from(
                ctx,
                ListConstInitValContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type ScalarConstInitValContext<'input> =
    BaseParserRuleContext<'input, ScalarConstInitValContextExt<'input>>;

pub trait ScalarConstInitValContextAttrs<'input>: SysYParserContext<'input> {
    fn constExp(&self) -> Option<Rc<ConstExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> ScalarConstInitValContextAttrs<'input> for ScalarConstInitValContext<'input> {}

pub struct ScalarConstInitValContextExt<'input> {
    base: ConstInitValContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {ScalarConstInitValContextExt<'a>}

impl<'input> SysYParserContext<'input> for ScalarConstInitValContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ScalarConstInitValContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_scalarConstInitVal(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_scalarConstInitVal(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ScalarConstInitValContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_scalarConstInitVal(self);
    }
}

impl<'input> CustomRuleContext<'input> for ScalarConstInitValContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_constInitVal
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_constInitVal }
}

impl<'input> Borrow<ConstInitValContextExt<'input>> for ScalarConstInitValContext<'input> {
    fn borrow(&self) -> &ConstInitValContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<ConstInitValContextExt<'input>> for ScalarConstInitValContext<'input> {
    fn borrow_mut(&mut self) -> &mut ConstInitValContextExt<'input> {
        &mut self.base
    }
}

impl<'input> ConstInitValContextAttrs<'input> for ScalarConstInitValContext<'input> {}

impl<'input> ScalarConstInitValContextExt<'input> {
    fn new(ctx: &dyn ConstInitValContextAttrs<'input>) -> Rc<ConstInitValContextAll<'input>> {
        Rc::new(ConstInitValContextAll::ScalarConstInitValContext(
            BaseParserRuleContext::copy_from(
                ctx,
                ScalarConstInitValContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn constInitVal(&mut self) -> Result<Rc<ConstInitValContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = ConstInitValContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_rule(_localctx.clone(), 10, RULE_constInitVal);
        let mut _localctx: Rc<ConstInitValContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            recog.base.set_state(117);
            recog.err_handler.sync(&mut recog.base)?;
            match recog.base.input.la(1) {
                Lparen | Minus | Exclamation | Addition | IntLiteral | FloatLiteral
                | Identifier => {
                    let tmp = ScalarConstInitValContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 1);
                    _localctx = tmp;
                    {
                        /*InvokeRule constExp*/
                        recog.base.set_state(104);
                        recog.constExp()?;
                    }
                }

                Lbrace => {
                    let tmp = ListConstInitValContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 2);
                    _localctx = tmp;
                    {
                        recog.base.set_state(105);
                        recog.base.match_token(Lbrace, &mut recog.err_handler)?;

                        recog.base.set_state(114);
                        recog.err_handler.sync(&mut recog.base)?;
                        _la = recog.base.input.la(1);
                        if ((_la - 14) & !0x3f) == 0
                            && ((1usize << (_la - 14))
                                & ((1usize << (Lparen - 14))
                                    | (1usize << (Lbrace - 14))
                                    | (1usize << (Minus - 14))
                                    | (1usize << (Exclamation - 14))
                                    | (1usize << (Addition - 14))
                                    | (1usize << (IntLiteral - 14))
                                    | (1usize << (FloatLiteral - 14))
                                    | (1usize << (Identifier - 14))))
                                != 0
                        {
                            {
                                /*InvokeRule constInitVal*/
                                recog.base.set_state(106);
                                recog.constInitVal()?;

                                recog.base.set_state(111);
                                recog.err_handler.sync(&mut recog.base)?;
                                _la = recog.base.input.la(1);
                                while _la == Comma {
                                    {
                                        {
                                            recog.base.set_state(107);
                                            recog
                                                .base
                                                .match_token(Comma, &mut recog.err_handler)?;

                                            /*InvokeRule constInitVal*/
                                            recog.base.set_state(108);
                                            recog.constInitVal()?;
                                        }
                                    }
                                    recog.base.set_state(113);
                                    recog.err_handler.sync(&mut recog.base)?;
                                    _la = recog.base.input.la(1);
                                }
                            }
                        }

                        recog.base.set_state(116);
                        recog.base.match_token(Rbrace, &mut recog.err_handler)?;
                    }
                }

                _ => Err(ANTLRError::NoAltError(NoViableAltError::new(
                    &mut recog.base,
                )))?,
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- varDecl ----------------
pub type VarDeclContextAll<'input> = VarDeclContext<'input>;

pub type VarDeclContext<'input> = BaseParserRuleContext<'input, VarDeclContextExt<'input>>;

#[derive(Clone)]
pub struct VarDeclContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for VarDeclContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for VarDeclContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_varDecl(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_varDecl(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for VarDeclContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_varDecl(self);
    }
}

impl<'input> CustomRuleContext<'input> for VarDeclContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_varDecl
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_varDecl }
}
antlr_rust::tid! {VarDeclContextExt<'a>}

impl<'input> VarDeclContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<VarDeclContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            VarDeclContextExt { ph: PhantomData },
        ))
    }
}

pub trait VarDeclContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<VarDeclContextExt<'input>>
{
    fn bType(&self) -> Option<Rc<BTypeContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn varDef_all(&self) -> Vec<Rc<VarDefContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn varDef(&self, i: usize) -> Option<Rc<VarDefContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves first TerminalNode corresponding to token Semicolon
    /// Returns `None` if there is no child corresponding to token Semicolon
    fn Semicolon(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Semicolon, 0)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Comma in current rule
    fn Comma_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Comma, starting from 0.
    /// Returns `None` if number of children corresponding to token Comma is less or equal than `i`.
    fn Comma(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Comma, i)
    }
}

impl<'input> VarDeclContextAttrs<'input> for VarDeclContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn varDecl(&mut self) -> Result<Rc<VarDeclContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = VarDeclContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 12, RULE_varDecl);
        let mut _localctx: Rc<VarDeclContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                /*InvokeRule bType*/
                recog.base.set_state(119);
                recog.bType()?;

                /*InvokeRule varDef*/
                recog.base.set_state(120);
                recog.varDef()?;

                recog.base.set_state(125);
                recog.err_handler.sync(&mut recog.base)?;
                _la = recog.base.input.la(1);
                while _la == Comma {
                    {
                        {
                            recog.base.set_state(121);
                            recog.base.match_token(Comma, &mut recog.err_handler)?;

                            /*InvokeRule varDef*/
                            recog.base.set_state(122);
                            recog.varDef()?;
                        }
                    }
                    recog.base.set_state(127);
                    recog.err_handler.sync(&mut recog.base)?;
                    _la = recog.base.input.la(1);
                }
                recog.base.set_state(128);
                recog.base.match_token(Semicolon, &mut recog.err_handler)?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- varDef ----------------
#[derive(Debug)]
pub enum VarDefContextAll<'input> {
    UninitVarDefContext(UninitVarDefContext<'input>),
    InitVarDefContext(InitVarDefContext<'input>),
    Error(VarDefContext<'input>),
}
antlr_rust::tid! {VarDefContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for VarDefContextAll<'input> {}

impl<'input> SysYParserContext<'input> for VarDefContextAll<'input> {}

impl<'input> Deref for VarDefContextAll<'input> {
    type Target = dyn VarDefContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use VarDefContextAll::*;
        match self {
            UninitVarDefContext(inner) => inner,
            InitVarDefContext(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for VarDefContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for VarDefContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type VarDefContext<'input> = BaseParserRuleContext<'input, VarDefContextExt<'input>>;

#[derive(Clone)]
pub struct VarDefContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for VarDefContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for VarDefContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for VarDefContext<'input> {}

impl<'input> CustomRuleContext<'input> for VarDefContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_varDef
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_varDef }
}
antlr_rust::tid! {VarDefContextExt<'a>}

impl<'input> VarDefContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<VarDefContextAll<'input>> {
        Rc::new(VarDefContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                VarDefContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait VarDefContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<VarDefContextExt<'input>>
{
}

impl<'input> VarDefContextAttrs<'input> for VarDefContext<'input> {}

pub type UninitVarDefContext<'input> =
    BaseParserRuleContext<'input, UninitVarDefContextExt<'input>>;

pub trait UninitVarDefContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Identifier
    /// Returns `None` if there is no child corresponding to token Identifier
    fn Identifier(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Identifier, 0)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Lbrkt in current rule
    fn Lbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Lbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Lbrkt is less or equal than `i`.
    fn Lbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lbrkt, i)
    }
    fn constExp_all(&self) -> Vec<Rc<ConstExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn constExp(&self, i: usize) -> Option<Rc<ConstExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Rbrkt in current rule
    fn Rbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Rbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Rbrkt is less or equal than `i`.
    fn Rbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rbrkt, i)
    }
}

impl<'input> UninitVarDefContextAttrs<'input> for UninitVarDefContext<'input> {}

pub struct UninitVarDefContextExt<'input> {
    base: VarDefContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {UninitVarDefContextExt<'a>}

impl<'input> SysYParserContext<'input> for UninitVarDefContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for UninitVarDefContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_uninitVarDef(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_uninitVarDef(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for UninitVarDefContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_uninitVarDef(self);
    }
}

impl<'input> CustomRuleContext<'input> for UninitVarDefContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_varDef
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_varDef }
}

impl<'input> Borrow<VarDefContextExt<'input>> for UninitVarDefContext<'input> {
    fn borrow(&self) -> &VarDefContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<VarDefContextExt<'input>> for UninitVarDefContext<'input> {
    fn borrow_mut(&mut self) -> &mut VarDefContextExt<'input> {
        &mut self.base
    }
}

impl<'input> VarDefContextAttrs<'input> for UninitVarDefContext<'input> {}

impl<'input> UninitVarDefContextExt<'input> {
    fn new(ctx: &dyn VarDefContextAttrs<'input>) -> Rc<VarDefContextAll<'input>> {
        Rc::new(VarDefContextAll::UninitVarDefContext(
            BaseParserRuleContext::copy_from(
                ctx,
                UninitVarDefContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type InitVarDefContext<'input> = BaseParserRuleContext<'input, InitVarDefContextExt<'input>>;

pub trait InitVarDefContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Identifier
    /// Returns `None` if there is no child corresponding to token Identifier
    fn Identifier(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Identifier, 0)
    }
    fn initVal(&self) -> Option<Rc<InitValContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Lbrkt in current rule
    fn Lbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Lbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Lbrkt is less or equal than `i`.
    fn Lbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lbrkt, i)
    }
    fn constExp_all(&self) -> Vec<Rc<ConstExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn constExp(&self, i: usize) -> Option<Rc<ConstExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Rbrkt in current rule
    fn Rbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Rbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Rbrkt is less or equal than `i`.
    fn Rbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rbrkt, i)
    }
}

impl<'input> InitVarDefContextAttrs<'input> for InitVarDefContext<'input> {}

pub struct InitVarDefContextExt<'input> {
    base: VarDefContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {InitVarDefContextExt<'a>}

impl<'input> SysYParserContext<'input> for InitVarDefContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for InitVarDefContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_initVarDef(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_initVarDef(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for InitVarDefContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_initVarDef(self);
    }
}

impl<'input> CustomRuleContext<'input> for InitVarDefContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_varDef
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_varDef }
}

impl<'input> Borrow<VarDefContextExt<'input>> for InitVarDefContext<'input> {
    fn borrow(&self) -> &VarDefContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<VarDefContextExt<'input>> for InitVarDefContext<'input> {
    fn borrow_mut(&mut self) -> &mut VarDefContextExt<'input> {
        &mut self.base
    }
}

impl<'input> VarDefContextAttrs<'input> for InitVarDefContext<'input> {}

impl<'input> InitVarDefContextExt<'input> {
    fn new(ctx: &dyn VarDefContextAttrs<'input>) -> Rc<VarDefContextAll<'input>> {
        Rc::new(VarDefContextAll::InitVarDefContext(
            BaseParserRuleContext::copy_from(
                ctx,
                InitVarDefContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn varDef(&mut self) -> Result<Rc<VarDefContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = VarDefContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 14, RULE_varDef);
        let mut _localctx: Rc<VarDefContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            recog.base.set_state(152);
            recog.err_handler.sync(&mut recog.base)?;
            match recog.interpreter.adaptive_predict(11, &mut recog.base)? {
                1 => {
                    let tmp = UninitVarDefContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 1);
                    _localctx = tmp;
                    {
                        recog.base.set_state(130);
                        recog.base.match_token(Identifier, &mut recog.err_handler)?;

                        recog.base.set_state(137);
                        recog.err_handler.sync(&mut recog.base)?;
                        _la = recog.base.input.la(1);
                        while _la == Lbrkt {
                            {
                                {
                                    recog.base.set_state(131);
                                    recog.base.match_token(Lbrkt, &mut recog.err_handler)?;

                                    /*InvokeRule constExp*/
                                    recog.base.set_state(132);
                                    recog.constExp()?;

                                    recog.base.set_state(133);
                                    recog.base.match_token(Rbrkt, &mut recog.err_handler)?;
                                }
                            }
                            recog.base.set_state(139);
                            recog.err_handler.sync(&mut recog.base)?;
                            _la = recog.base.input.la(1);
                        }
                    }
                }
                2 => {
                    let tmp = InitVarDefContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 2);
                    _localctx = tmp;
                    {
                        recog.base.set_state(140);
                        recog.base.match_token(Identifier, &mut recog.err_handler)?;

                        recog.base.set_state(147);
                        recog.err_handler.sync(&mut recog.base)?;
                        _la = recog.base.input.la(1);
                        while _la == Lbrkt {
                            {
                                {
                                    recog.base.set_state(141);
                                    recog.base.match_token(Lbrkt, &mut recog.err_handler)?;

                                    /*InvokeRule constExp*/
                                    recog.base.set_state(142);
                                    recog.constExp()?;

                                    recog.base.set_state(143);
                                    recog.base.match_token(Rbrkt, &mut recog.err_handler)?;
                                }
                            }
                            recog.base.set_state(149);
                            recog.err_handler.sync(&mut recog.base)?;
                            _la = recog.base.input.la(1);
                        }
                        recog.base.set_state(150);
                        recog.base.match_token(T__0, &mut recog.err_handler)?;

                        /*InvokeRule initVal*/
                        recog.base.set_state(151);
                        recog.initVal()?;
                    }
                }

                _ => {}
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- initVal ----------------
#[derive(Debug)]
pub enum InitValContextAll<'input> {
    ScalarInitValContext(ScalarInitValContext<'input>),
    ListInitvalContext(ListInitvalContext<'input>),
    Error(InitValContext<'input>),
}
antlr_rust::tid! {InitValContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for InitValContextAll<'input> {}

impl<'input> SysYParserContext<'input> for InitValContextAll<'input> {}

impl<'input> Deref for InitValContextAll<'input> {
    type Target = dyn InitValContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use InitValContextAll::*;
        match self {
            ScalarInitValContext(inner) => inner,
            ListInitvalContext(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for InitValContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for InitValContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type InitValContext<'input> = BaseParserRuleContext<'input, InitValContextExt<'input>>;

#[derive(Clone)]
pub struct InitValContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for InitValContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for InitValContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for InitValContext<'input> {}

impl<'input> CustomRuleContext<'input> for InitValContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_initVal
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_initVal }
}
antlr_rust::tid! {InitValContextExt<'a>}

impl<'input> InitValContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<InitValContextAll<'input>> {
        Rc::new(InitValContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                InitValContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait InitValContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<InitValContextExt<'input>>
{
}

impl<'input> InitValContextAttrs<'input> for InitValContext<'input> {}

pub type ScalarInitValContext<'input> =
    BaseParserRuleContext<'input, ScalarInitValContextExt<'input>>;

pub trait ScalarInitValContextAttrs<'input>: SysYParserContext<'input> {
    fn exp(&self) -> Option<Rc<ExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> ScalarInitValContextAttrs<'input> for ScalarInitValContext<'input> {}

pub struct ScalarInitValContextExt<'input> {
    base: InitValContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {ScalarInitValContextExt<'a>}

impl<'input> SysYParserContext<'input> for ScalarInitValContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ScalarInitValContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_scalarInitVal(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_scalarInitVal(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ScalarInitValContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_scalarInitVal(self);
    }
}

impl<'input> CustomRuleContext<'input> for ScalarInitValContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_initVal
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_initVal }
}

impl<'input> Borrow<InitValContextExt<'input>> for ScalarInitValContext<'input> {
    fn borrow(&self) -> &InitValContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<InitValContextExt<'input>> for ScalarInitValContext<'input> {
    fn borrow_mut(&mut self) -> &mut InitValContextExt<'input> {
        &mut self.base
    }
}

impl<'input> InitValContextAttrs<'input> for ScalarInitValContext<'input> {}

impl<'input> ScalarInitValContextExt<'input> {
    fn new(ctx: &dyn InitValContextAttrs<'input>) -> Rc<InitValContextAll<'input>> {
        Rc::new(InitValContextAll::ScalarInitValContext(
            BaseParserRuleContext::copy_from(
                ctx,
                ScalarInitValContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type ListInitvalContext<'input> = BaseParserRuleContext<'input, ListInitvalContextExt<'input>>;

pub trait ListInitvalContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Lbrace
    /// Returns `None` if there is no child corresponding to token Lbrace
    fn Lbrace(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lbrace, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Rbrace
    /// Returns `None` if there is no child corresponding to token Rbrace
    fn Rbrace(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rbrace, 0)
    }
    fn initVal_all(&self) -> Vec<Rc<InitValContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn initVal(&self, i: usize) -> Option<Rc<InitValContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Comma in current rule
    fn Comma_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Comma, starting from 0.
    /// Returns `None` if number of children corresponding to token Comma is less or equal than `i`.
    fn Comma(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Comma, i)
    }
}

impl<'input> ListInitvalContextAttrs<'input> for ListInitvalContext<'input> {}

pub struct ListInitvalContextExt<'input> {
    base: InitValContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {ListInitvalContextExt<'a>}

impl<'input> SysYParserContext<'input> for ListInitvalContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ListInitvalContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_listInitval(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_listInitval(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ListInitvalContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_listInitval(self);
    }
}

impl<'input> CustomRuleContext<'input> for ListInitvalContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_initVal
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_initVal }
}

impl<'input> Borrow<InitValContextExt<'input>> for ListInitvalContext<'input> {
    fn borrow(&self) -> &InitValContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<InitValContextExt<'input>> for ListInitvalContext<'input> {
    fn borrow_mut(&mut self) -> &mut InitValContextExt<'input> {
        &mut self.base
    }
}

impl<'input> InitValContextAttrs<'input> for ListInitvalContext<'input> {}

impl<'input> ListInitvalContextExt<'input> {
    fn new(ctx: &dyn InitValContextAttrs<'input>) -> Rc<InitValContextAll<'input>> {
        Rc::new(InitValContextAll::ListInitvalContext(
            BaseParserRuleContext::copy_from(
                ctx,
                ListInitvalContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn initVal(&mut self) -> Result<Rc<InitValContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = InitValContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 16, RULE_initVal);
        let mut _localctx: Rc<InitValContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            recog.base.set_state(167);
            recog.err_handler.sync(&mut recog.base)?;
            match recog.base.input.la(1) {
                Lparen | Minus | Exclamation | Addition | IntLiteral | FloatLiteral
                | Identifier => {
                    let tmp = ScalarInitValContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 1);
                    _localctx = tmp;
                    {
                        /*InvokeRule exp*/
                        recog.base.set_state(154);
                        recog.exp()?;
                    }
                }

                Lbrace => {
                    let tmp = ListInitvalContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 2);
                    _localctx = tmp;
                    {
                        recog.base.set_state(155);
                        recog.base.match_token(Lbrace, &mut recog.err_handler)?;

                        recog.base.set_state(164);
                        recog.err_handler.sync(&mut recog.base)?;
                        _la = recog.base.input.la(1);
                        if ((_la - 14) & !0x3f) == 0
                            && ((1usize << (_la - 14))
                                & ((1usize << (Lparen - 14))
                                    | (1usize << (Lbrace - 14))
                                    | (1usize << (Minus - 14))
                                    | (1usize << (Exclamation - 14))
                                    | (1usize << (Addition - 14))
                                    | (1usize << (IntLiteral - 14))
                                    | (1usize << (FloatLiteral - 14))
                                    | (1usize << (Identifier - 14))))
                                != 0
                        {
                            {
                                /*InvokeRule initVal*/
                                recog.base.set_state(156);
                                recog.initVal()?;

                                recog.base.set_state(161);
                                recog.err_handler.sync(&mut recog.base)?;
                                _la = recog.base.input.la(1);
                                while _la == Comma {
                                    {
                                        {
                                            recog.base.set_state(157);
                                            recog
                                                .base
                                                .match_token(Comma, &mut recog.err_handler)?;

                                            /*InvokeRule initVal*/
                                            recog.base.set_state(158);
                                            recog.initVal()?;
                                        }
                                    }
                                    recog.base.set_state(163);
                                    recog.err_handler.sync(&mut recog.base)?;
                                    _la = recog.base.input.la(1);
                                }
                            }
                        }

                        recog.base.set_state(166);
                        recog.base.match_token(Rbrace, &mut recog.err_handler)?;
                    }
                }

                _ => Err(ANTLRError::NoAltError(NoViableAltError::new(
                    &mut recog.base,
                )))?,
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- funcDef ----------------
pub type FuncDefContextAll<'input> = FuncDefContext<'input>;

pub type FuncDefContext<'input> = BaseParserRuleContext<'input, FuncDefContextExt<'input>>;

#[derive(Clone)]
pub struct FuncDefContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for FuncDefContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for FuncDefContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_funcDef(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_funcDef(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for FuncDefContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_funcDef(self);
    }
}

impl<'input> CustomRuleContext<'input> for FuncDefContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_funcDef
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_funcDef }
}
antlr_rust::tid! {FuncDefContextExt<'a>}

impl<'input> FuncDefContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<FuncDefContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            FuncDefContextExt { ph: PhantomData },
        ))
    }
}

pub trait FuncDefContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<FuncDefContextExt<'input>>
{
    fn funcType(&self) -> Option<Rc<FuncTypeContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token Identifier
    /// Returns `None` if there is no child corresponding to token Identifier
    fn Identifier(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Identifier, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Lparen
    /// Returns `None` if there is no child corresponding to token Lparen
    fn Lparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lparen, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Rparen
    /// Returns `None` if there is no child corresponding to token Rparen
    fn Rparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rparen, 0)
    }
    fn block(&self) -> Option<Rc<BlockContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn funcFParams(&self) -> Option<Rc<FuncFParamsContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> FuncDefContextAttrs<'input> for FuncDefContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn funcDef(&mut self) -> Result<Rc<FuncDefContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = FuncDefContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 18, RULE_funcDef);
        let mut _localctx: Rc<FuncDefContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                /*InvokeRule funcType*/
                recog.base.set_state(169);
                recog.funcType()?;

                recog.base.set_state(170);
                recog.base.match_token(Identifier, &mut recog.err_handler)?;

                recog.base.set_state(171);
                recog.base.match_token(Lparen, &mut recog.err_handler)?;

                recog.base.set_state(173);
                recog.err_handler.sync(&mut recog.base)?;
                _la = recog.base.input.la(1);
                if _la == Int || _la == Float {
                    {
                        /*InvokeRule funcFParams*/
                        recog.base.set_state(172);
                        recog.funcFParams()?;
                    }
                }

                recog.base.set_state(175);
                recog.base.match_token(Rparen, &mut recog.err_handler)?;

                /*InvokeRule block*/
                recog.base.set_state(176);
                recog.block()?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- funcType ----------------
pub type FuncTypeContextAll<'input> = FuncTypeContext<'input>;

pub type FuncTypeContext<'input> = BaseParserRuleContext<'input, FuncTypeContextExt<'input>>;

#[derive(Clone)]
pub struct FuncTypeContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for FuncTypeContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for FuncTypeContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_funcType(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_funcType(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for FuncTypeContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_funcType(self);
    }
}

impl<'input> CustomRuleContext<'input> for FuncTypeContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_funcType
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_funcType }
}
antlr_rust::tid! {FuncTypeContextExt<'a>}

impl<'input> FuncTypeContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<FuncTypeContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            FuncTypeContextExt { ph: PhantomData },
        ))
    }
}

pub trait FuncTypeContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<FuncTypeContextExt<'input>>
{
    /// Retrieves first TerminalNode corresponding to token Void
    /// Returns `None` if there is no child corresponding to token Void
    fn Void(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Void, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Int
    /// Returns `None` if there is no child corresponding to token Int
    fn Int(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Int, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Float
    /// Returns `None` if there is no child corresponding to token Float
    fn Float(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Float, 0)
    }
}

impl<'input> FuncTypeContextAttrs<'input> for FuncTypeContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn funcType(&mut self) -> Result<Rc<FuncTypeContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = FuncTypeContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 20, RULE_funcType);
        let mut _localctx: Rc<FuncTypeContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                recog.base.set_state(178);
                _la = recog.base.input.la(1);
                if {
                    !(((_la) & !0x3f) == 0
                        && ((1usize << _la)
                            & ((1usize << Int) | (1usize << Float) | (1usize << Void)))
                            != 0)
                } {
                    recog.err_handler.recover_inline(&mut recog.base)?;
                } else {
                    if recog.base.input.la(1) == TOKEN_EOF {
                        recog.base.matched_eof = true
                    };
                    recog.err_handler.report_match(&mut recog.base);
                    recog.base.consume(&mut recog.err_handler);
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- funcFParams ----------------
pub type FuncFParamsContextAll<'input> = FuncFParamsContext<'input>;

pub type FuncFParamsContext<'input> = BaseParserRuleContext<'input, FuncFParamsContextExt<'input>>;

#[derive(Clone)]
pub struct FuncFParamsContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for FuncFParamsContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for FuncFParamsContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_funcFParams(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_funcFParams(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for FuncFParamsContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_funcFParams(self);
    }
}

impl<'input> CustomRuleContext<'input> for FuncFParamsContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_funcFParams
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_funcFParams }
}
antlr_rust::tid! {FuncFParamsContextExt<'a>}

impl<'input> FuncFParamsContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<FuncFParamsContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            FuncFParamsContextExt { ph: PhantomData },
        ))
    }
}

pub trait FuncFParamsContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<FuncFParamsContextExt<'input>>
{
    fn funcFParam_all(&self) -> Vec<Rc<FuncFParamContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn funcFParam(&self, i: usize) -> Option<Rc<FuncFParamContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Comma in current rule
    fn Comma_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Comma, starting from 0.
    /// Returns `None` if number of children corresponding to token Comma is less or equal than `i`.
    fn Comma(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Comma, i)
    }
}

impl<'input> FuncFParamsContextAttrs<'input> for FuncFParamsContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn funcFParams(&mut self) -> Result<Rc<FuncFParamsContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = FuncFParamsContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_rule(_localctx.clone(), 22, RULE_funcFParams);
        let mut _localctx: Rc<FuncFParamsContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                /*InvokeRule funcFParam*/
                recog.base.set_state(180);
                recog.funcFParam()?;

                recog.base.set_state(185);
                recog.err_handler.sync(&mut recog.base)?;
                _la = recog.base.input.la(1);
                while _la == Comma {
                    {
                        {
                            recog.base.set_state(181);
                            recog.base.match_token(Comma, &mut recog.err_handler)?;

                            /*InvokeRule funcFParam*/
                            recog.base.set_state(182);
                            recog.funcFParam()?;
                        }
                    }
                    recog.base.set_state(187);
                    recog.err_handler.sync(&mut recog.base)?;
                    _la = recog.base.input.la(1);
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- funcFParam ----------------
pub type FuncFParamContextAll<'input> = FuncFParamContext<'input>;

pub type FuncFParamContext<'input> = BaseParserRuleContext<'input, FuncFParamContextExt<'input>>;

#[derive(Clone)]
pub struct FuncFParamContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for FuncFParamContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for FuncFParamContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_funcFParam(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_funcFParam(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for FuncFParamContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_funcFParam(self);
    }
}

impl<'input> CustomRuleContext<'input> for FuncFParamContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_funcFParam
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_funcFParam }
}
antlr_rust::tid! {FuncFParamContextExt<'a>}

impl<'input> FuncFParamContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<FuncFParamContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            FuncFParamContextExt { ph: PhantomData },
        ))
    }
}

pub trait FuncFParamContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<FuncFParamContextExt<'input>>
{
    fn bType(&self) -> Option<Rc<BTypeContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token Identifier
    /// Returns `None` if there is no child corresponding to token Identifier
    fn Identifier(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Identifier, 0)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Lbrkt in current rule
    fn Lbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        let terminalvec: Vec<Rc<TerminalNode<'input, SysYParserContextType>>> =
            self.children_of_type();
        let ret = terminalvec
            .into_iter()
            .filter(|n| n.symbol.token_type == Lbrkt)
            .collect();
        ret
    }
    /// Retrieves 'i's TerminalNode corresponding to token Lbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Lbrkt is less or equal than `i`.
    fn Lbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lbrkt, i)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Rbrkt in current rule
    fn Rbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Rbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Rbrkt is less or equal than `i`.
    fn Rbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rbrkt, i)
    }
    fn constExp_all(&self) -> Vec<Rc<ConstExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn constExp(&self, i: usize) -> Option<Rc<ConstExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
}

impl<'input> FuncFParamContextAttrs<'input> for FuncFParamContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn funcFParam(&mut self) -> Result<Rc<FuncFParamContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = FuncFParamContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_rule(_localctx.clone(), 24, RULE_funcFParam);
        let mut _localctx: Rc<FuncFParamContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                /*InvokeRule bType*/
                recog.base.set_state(188);
                recog.bType()?;

                recog.base.set_state(189);
                recog.base.match_token(Identifier, &mut recog.err_handler)?;

                recog.base.set_state(201);
                recog.err_handler.sync(&mut recog.base)?;
                _la = recog.base.input.la(1);
                if _la == Lbrkt {
                    {
                        recog.base.set_state(190);
                        recog.base.match_token(Lbrkt, &mut recog.err_handler)?;

                        recog.base.set_state(191);
                        recog.base.match_token(Rbrkt, &mut recog.err_handler)?;

                        recog.base.set_state(198);
                        recog.err_handler.sync(&mut recog.base)?;
                        _la = recog.base.input.la(1);
                        while _la == Lbrkt {
                            {
                                {
                                    recog.base.set_state(192);
                                    recog.base.match_token(Lbrkt, &mut recog.err_handler)?;

                                    /*InvokeRule constExp*/
                                    recog.base.set_state(193);
                                    recog.constExp()?;

                                    recog.base.set_state(194);
                                    recog.base.match_token(Rbrkt, &mut recog.err_handler)?;
                                }
                            }
                            recog.base.set_state(200);
                            recog.err_handler.sync(&mut recog.base)?;
                            _la = recog.base.input.la(1);
                        }
                    }
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- block ----------------
pub type BlockContextAll<'input> = BlockContext<'input>;

pub type BlockContext<'input> = BaseParserRuleContext<'input, BlockContextExt<'input>>;

#[derive(Clone)]
pub struct BlockContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for BlockContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for BlockContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_block(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_block(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for BlockContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_block(self);
    }
}

impl<'input> CustomRuleContext<'input> for BlockContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_block
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_block }
}
antlr_rust::tid! {BlockContextExt<'a>}

impl<'input> BlockContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<BlockContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            BlockContextExt { ph: PhantomData },
        ))
    }
}

pub trait BlockContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<BlockContextExt<'input>>
{
    /// Retrieves first TerminalNode corresponding to token Lbrace
    /// Returns `None` if there is no child corresponding to token Lbrace
    fn Lbrace(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lbrace, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Rbrace
    /// Returns `None` if there is no child corresponding to token Rbrace
    fn Rbrace(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rbrace, 0)
    }
    fn blockItem_all(&self) -> Vec<Rc<BlockItemContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn blockItem(&self, i: usize) -> Option<Rc<BlockItemContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
}

impl<'input> BlockContextAttrs<'input> for BlockContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn block(&mut self) -> Result<Rc<BlockContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = BlockContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 26, RULE_block);
        let mut _localctx: Rc<BlockContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                recog.base.set_state(203);
                recog.base.match_token(Lbrace, &mut recog.err_handler)?;

                recog.base.set_state(207);
                recog.err_handler.sync(&mut recog.base)?;
                _la = recog.base.input.la(1);
                while (((_la) & !0x3f) == 0
                    && ((1usize << _la)
                        & ((1usize << Int)
                            | (1usize << Float)
                            | (1usize << Const)
                            | (1usize << Return)
                            | (1usize << If)
                            | (1usize << While)
                            | (1usize << Break)
                            | (1usize << Continue)
                            | (1usize << Lparen)
                            | (1usize << Lbrace)
                            | (1usize << Semicolon)
                            | (1usize << Minus)
                            | (1usize << Exclamation)
                            | (1usize << Addition)))
                        != 0)
                    || (((_la - 39) & !0x3f) == 0
                        && ((1usize << (_la - 39))
                            & ((1usize << (IntLiteral - 39))
                                | (1usize << (FloatLiteral - 39))
                                | (1usize << (Identifier - 39))))
                            != 0)
                {
                    {
                        {
                            /*InvokeRule blockItem*/
                            recog.base.set_state(204);
                            recog.blockItem()?;
                        }
                    }
                    recog.base.set_state(209);
                    recog.err_handler.sync(&mut recog.base)?;
                    _la = recog.base.input.la(1);
                }
                recog.base.set_state(210);
                recog.base.match_token(Rbrace, &mut recog.err_handler)?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- blockItem ----------------
pub type BlockItemContextAll<'input> = BlockItemContext<'input>;

pub type BlockItemContext<'input> = BaseParserRuleContext<'input, BlockItemContextExt<'input>>;

#[derive(Clone)]
pub struct BlockItemContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for BlockItemContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for BlockItemContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_blockItem(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_blockItem(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for BlockItemContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_blockItem(self);
    }
}

impl<'input> CustomRuleContext<'input> for BlockItemContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_blockItem
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_blockItem }
}
antlr_rust::tid! {BlockItemContextExt<'a>}

impl<'input> BlockItemContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<BlockItemContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            BlockItemContextExt { ph: PhantomData },
        ))
    }
}

pub trait BlockItemContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<BlockItemContextExt<'input>>
{
    fn decl(&self) -> Option<Rc<DeclContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn stmt(&self) -> Option<Rc<StmtContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> BlockItemContextAttrs<'input> for BlockItemContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn blockItem(&mut self) -> Result<Rc<BlockItemContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = BlockItemContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 28, RULE_blockItem);
        let mut _localctx: Rc<BlockItemContextAll> = _localctx;
        let result: Result<(), ANTLRError> = (|| {
            recog.base.set_state(214);
            recog.err_handler.sync(&mut recog.base)?;
            match recog.base.input.la(1) {
                Int | Float | Const => {
                    //recog.base.enter_outer_alt(_localctx.clone(), 1);
                    recog.base.enter_outer_alt(None, 1);
                    {
                        /*InvokeRule decl*/
                        recog.base.set_state(212);
                        recog.decl()?;
                    }
                }

                Return | If | While | Break | Continue | Lparen | Lbrace | Semicolon | Minus
                | Exclamation | Addition | IntLiteral | FloatLiteral | Identifier => {
                    //recog.base.enter_outer_alt(_localctx.clone(), 2);
                    recog.base.enter_outer_alt(None, 2);
                    {
                        /*InvokeRule stmt*/
                        recog.base.set_state(213);
                        recog.stmt()?;
                    }
                }

                _ => Err(ANTLRError::NoAltError(NoViableAltError::new(
                    &mut recog.base,
                )))?,
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- stmt ----------------
#[derive(Debug)]
pub enum StmtContextAll<'input> {
    WhileStmtContext(WhileStmtContext<'input>),
    BlockStmtContext(BlockStmtContext<'input>),
    AssignmentContext(AssignmentContext<'input>),
    IfStmt1Context(IfStmt1Context<'input>),
    BreakStmtContext(BreakStmtContext<'input>),
    ExpStmtContext(ExpStmtContext<'input>),
    IfStmt2Context(IfStmt2Context<'input>),
    ReturnStmtContext(ReturnStmtContext<'input>),
    ContinueStmtContext(ContinueStmtContext<'input>),
    Error(StmtContext<'input>),
}
antlr_rust::tid! {StmtContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for StmtContextAll<'input> {}

impl<'input> SysYParserContext<'input> for StmtContextAll<'input> {}

impl<'input> Deref for StmtContextAll<'input> {
    type Target = dyn StmtContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use StmtContextAll::*;
        match self {
            WhileStmtContext(inner) => inner,
            BlockStmtContext(inner) => inner,
            AssignmentContext(inner) => inner,
            IfStmt1Context(inner) => inner,
            BreakStmtContext(inner) => inner,
            ExpStmtContext(inner) => inner,
            IfStmt2Context(inner) => inner,
            ReturnStmtContext(inner) => inner,
            ContinueStmtContext(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for StmtContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for StmtContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type StmtContext<'input> = BaseParserRuleContext<'input, StmtContextExt<'input>>;

#[derive(Clone)]
pub struct StmtContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for StmtContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for StmtContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for StmtContext<'input> {}

impl<'input> CustomRuleContext<'input> for StmtContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}
antlr_rust::tid! {StmtContextExt<'a>}

impl<'input> StmtContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                StmtContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait StmtContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<StmtContextExt<'input>>
{
}

impl<'input> StmtContextAttrs<'input> for StmtContext<'input> {}

pub type WhileStmtContext<'input> = BaseParserRuleContext<'input, WhileStmtContextExt<'input>>;

pub trait WhileStmtContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token While
    /// Returns `None` if there is no child corresponding to token While
    fn While(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(While, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Lparen
    /// Returns `None` if there is no child corresponding to token Lparen
    fn Lparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lparen, 0)
    }
    fn cond(&self) -> Option<Rc<CondContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token Rparen
    /// Returns `None` if there is no child corresponding to token Rparen
    fn Rparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rparen, 0)
    }
    fn stmt(&self) -> Option<Rc<StmtContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> WhileStmtContextAttrs<'input> for WhileStmtContext<'input> {}

pub struct WhileStmtContextExt<'input> {
    base: StmtContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {WhileStmtContextExt<'a>}

impl<'input> SysYParserContext<'input> for WhileStmtContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for WhileStmtContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_whileStmt(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_whileStmt(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for WhileStmtContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_whileStmt(self);
    }
}

impl<'input> CustomRuleContext<'input> for WhileStmtContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}

impl<'input> Borrow<StmtContextExt<'input>> for WhileStmtContext<'input> {
    fn borrow(&self) -> &StmtContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<StmtContextExt<'input>> for WhileStmtContext<'input> {
    fn borrow_mut(&mut self) -> &mut StmtContextExt<'input> {
        &mut self.base
    }
}

impl<'input> StmtContextAttrs<'input> for WhileStmtContext<'input> {}

impl<'input> WhileStmtContextExt<'input> {
    fn new(ctx: &dyn StmtContextAttrs<'input>) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::WhileStmtContext(
            BaseParserRuleContext::copy_from(
                ctx,
                WhileStmtContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type BlockStmtContext<'input> = BaseParserRuleContext<'input, BlockStmtContextExt<'input>>;

pub trait BlockStmtContextAttrs<'input>: SysYParserContext<'input> {
    fn block(&self) -> Option<Rc<BlockContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> BlockStmtContextAttrs<'input> for BlockStmtContext<'input> {}

pub struct BlockStmtContextExt<'input> {
    base: StmtContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {BlockStmtContextExt<'a>}

impl<'input> SysYParserContext<'input> for BlockStmtContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for BlockStmtContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_blockStmt(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_blockStmt(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for BlockStmtContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_blockStmt(self);
    }
}

impl<'input> CustomRuleContext<'input> for BlockStmtContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}

impl<'input> Borrow<StmtContextExt<'input>> for BlockStmtContext<'input> {
    fn borrow(&self) -> &StmtContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<StmtContextExt<'input>> for BlockStmtContext<'input> {
    fn borrow_mut(&mut self) -> &mut StmtContextExt<'input> {
        &mut self.base
    }
}

impl<'input> StmtContextAttrs<'input> for BlockStmtContext<'input> {}

impl<'input> BlockStmtContextExt<'input> {
    fn new(ctx: &dyn StmtContextAttrs<'input>) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::BlockStmtContext(
            BaseParserRuleContext::copy_from(
                ctx,
                BlockStmtContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type AssignmentContext<'input> = BaseParserRuleContext<'input, AssignmentContextExt<'input>>;

pub trait AssignmentContextAttrs<'input>: SysYParserContext<'input> {
    fn lVal(&self) -> Option<Rc<LValContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn exp(&self) -> Option<Rc<ExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token Semicolon
    /// Returns `None` if there is no child corresponding to token Semicolon
    fn Semicolon(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Semicolon, 0)
    }
}

impl<'input> AssignmentContextAttrs<'input> for AssignmentContext<'input> {}

pub struct AssignmentContextExt<'input> {
    base: StmtContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {AssignmentContextExt<'a>}

impl<'input> SysYParserContext<'input> for AssignmentContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for AssignmentContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_assignment(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_assignment(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for AssignmentContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_assignment(self);
    }
}

impl<'input> CustomRuleContext<'input> for AssignmentContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}

impl<'input> Borrow<StmtContextExt<'input>> for AssignmentContext<'input> {
    fn borrow(&self) -> &StmtContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<StmtContextExt<'input>> for AssignmentContext<'input> {
    fn borrow_mut(&mut self) -> &mut StmtContextExt<'input> {
        &mut self.base
    }
}

impl<'input> StmtContextAttrs<'input> for AssignmentContext<'input> {}

impl<'input> AssignmentContextExt<'input> {
    fn new(ctx: &dyn StmtContextAttrs<'input>) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::AssignmentContext(
            BaseParserRuleContext::copy_from(
                ctx,
                AssignmentContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type IfStmt1Context<'input> = BaseParserRuleContext<'input, IfStmt1ContextExt<'input>>;

pub trait IfStmt1ContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token If
    /// Returns `None` if there is no child corresponding to token If
    fn If(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(If, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Lparen
    /// Returns `None` if there is no child corresponding to token Lparen
    fn Lparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lparen, 0)
    }
    fn cond(&self) -> Option<Rc<CondContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token Rparen
    /// Returns `None` if there is no child corresponding to token Rparen
    fn Rparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rparen, 0)
    }
    fn stmt(&self) -> Option<Rc<StmtContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> IfStmt1ContextAttrs<'input> for IfStmt1Context<'input> {}

pub struct IfStmt1ContextExt<'input> {
    base: StmtContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {IfStmt1ContextExt<'a>}

impl<'input> SysYParserContext<'input> for IfStmt1Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for IfStmt1Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_ifStmt1(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_ifStmt1(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for IfStmt1Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_ifStmt1(self);
    }
}

impl<'input> CustomRuleContext<'input> for IfStmt1ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}

impl<'input> Borrow<StmtContextExt<'input>> for IfStmt1Context<'input> {
    fn borrow(&self) -> &StmtContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<StmtContextExt<'input>> for IfStmt1Context<'input> {
    fn borrow_mut(&mut self) -> &mut StmtContextExt<'input> {
        &mut self.base
    }
}

impl<'input> StmtContextAttrs<'input> for IfStmt1Context<'input> {}

impl<'input> IfStmt1ContextExt<'input> {
    fn new(ctx: &dyn StmtContextAttrs<'input>) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::IfStmt1Context(
            BaseParserRuleContext::copy_from(
                ctx,
                IfStmt1ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type BreakStmtContext<'input> = BaseParserRuleContext<'input, BreakStmtContextExt<'input>>;

pub trait BreakStmtContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Break
    /// Returns `None` if there is no child corresponding to token Break
    fn Break(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Break, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Semicolon
    /// Returns `None` if there is no child corresponding to token Semicolon
    fn Semicolon(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Semicolon, 0)
    }
}

impl<'input> BreakStmtContextAttrs<'input> for BreakStmtContext<'input> {}

pub struct BreakStmtContextExt<'input> {
    base: StmtContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {BreakStmtContextExt<'a>}

impl<'input> SysYParserContext<'input> for BreakStmtContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for BreakStmtContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_breakStmt(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_breakStmt(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for BreakStmtContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_breakStmt(self);
    }
}

impl<'input> CustomRuleContext<'input> for BreakStmtContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}

impl<'input> Borrow<StmtContextExt<'input>> for BreakStmtContext<'input> {
    fn borrow(&self) -> &StmtContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<StmtContextExt<'input>> for BreakStmtContext<'input> {
    fn borrow_mut(&mut self) -> &mut StmtContextExt<'input> {
        &mut self.base
    }
}

impl<'input> StmtContextAttrs<'input> for BreakStmtContext<'input> {}

impl<'input> BreakStmtContextExt<'input> {
    fn new(ctx: &dyn StmtContextAttrs<'input>) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::BreakStmtContext(
            BaseParserRuleContext::copy_from(
                ctx,
                BreakStmtContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type ExpStmtContext<'input> = BaseParserRuleContext<'input, ExpStmtContextExt<'input>>;

pub trait ExpStmtContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Semicolon
    /// Returns `None` if there is no child corresponding to token Semicolon
    fn Semicolon(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Semicolon, 0)
    }
    fn exp(&self) -> Option<Rc<ExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> ExpStmtContextAttrs<'input> for ExpStmtContext<'input> {}

pub struct ExpStmtContextExt<'input> {
    base: StmtContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {ExpStmtContextExt<'a>}

impl<'input> SysYParserContext<'input> for ExpStmtContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ExpStmtContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_expStmt(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_expStmt(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ExpStmtContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_expStmt(self);
    }
}

impl<'input> CustomRuleContext<'input> for ExpStmtContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}

impl<'input> Borrow<StmtContextExt<'input>> for ExpStmtContext<'input> {
    fn borrow(&self) -> &StmtContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<StmtContextExt<'input>> for ExpStmtContext<'input> {
    fn borrow_mut(&mut self) -> &mut StmtContextExt<'input> {
        &mut self.base
    }
}

impl<'input> StmtContextAttrs<'input> for ExpStmtContext<'input> {}

impl<'input> ExpStmtContextExt<'input> {
    fn new(ctx: &dyn StmtContextAttrs<'input>) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::ExpStmtContext(
            BaseParserRuleContext::copy_from(
                ctx,
                ExpStmtContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type IfStmt2Context<'input> = BaseParserRuleContext<'input, IfStmt2ContextExt<'input>>;

pub trait IfStmt2ContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token If
    /// Returns `None` if there is no child corresponding to token If
    fn If(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(If, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Lparen
    /// Returns `None` if there is no child corresponding to token Lparen
    fn Lparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lparen, 0)
    }
    fn cond(&self) -> Option<Rc<CondContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token Rparen
    /// Returns `None` if there is no child corresponding to token Rparen
    fn Rparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rparen, 0)
    }
    fn stmt_all(&self) -> Vec<Rc<StmtContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn stmt(&self, i: usize) -> Option<Rc<StmtContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves first TerminalNode corresponding to token Else
    /// Returns `None` if there is no child corresponding to token Else
    fn Else(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Else, 0)
    }
}

impl<'input> IfStmt2ContextAttrs<'input> for IfStmt2Context<'input> {}

pub struct IfStmt2ContextExt<'input> {
    base: StmtContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {IfStmt2ContextExt<'a>}

impl<'input> SysYParserContext<'input> for IfStmt2Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for IfStmt2Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_ifStmt2(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_ifStmt2(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for IfStmt2Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_ifStmt2(self);
    }
}

impl<'input> CustomRuleContext<'input> for IfStmt2ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}

impl<'input> Borrow<StmtContextExt<'input>> for IfStmt2Context<'input> {
    fn borrow(&self) -> &StmtContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<StmtContextExt<'input>> for IfStmt2Context<'input> {
    fn borrow_mut(&mut self) -> &mut StmtContextExt<'input> {
        &mut self.base
    }
}

impl<'input> StmtContextAttrs<'input> for IfStmt2Context<'input> {}

impl<'input> IfStmt2ContextExt<'input> {
    fn new(ctx: &dyn StmtContextAttrs<'input>) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::IfStmt2Context(
            BaseParserRuleContext::copy_from(
                ctx,
                IfStmt2ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type ReturnStmtContext<'input> = BaseParserRuleContext<'input, ReturnStmtContextExt<'input>>;

pub trait ReturnStmtContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Return
    /// Returns `None` if there is no child corresponding to token Return
    fn Return(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Return, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Semicolon
    /// Returns `None` if there is no child corresponding to token Semicolon
    fn Semicolon(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Semicolon, 0)
    }
    fn exp(&self) -> Option<Rc<ExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> ReturnStmtContextAttrs<'input> for ReturnStmtContext<'input> {}

pub struct ReturnStmtContextExt<'input> {
    base: StmtContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {ReturnStmtContextExt<'a>}

impl<'input> SysYParserContext<'input> for ReturnStmtContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ReturnStmtContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_returnStmt(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_returnStmt(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ReturnStmtContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_returnStmt(self);
    }
}

impl<'input> CustomRuleContext<'input> for ReturnStmtContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}

impl<'input> Borrow<StmtContextExt<'input>> for ReturnStmtContext<'input> {
    fn borrow(&self) -> &StmtContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<StmtContextExt<'input>> for ReturnStmtContext<'input> {
    fn borrow_mut(&mut self) -> &mut StmtContextExt<'input> {
        &mut self.base
    }
}

impl<'input> StmtContextAttrs<'input> for ReturnStmtContext<'input> {}

impl<'input> ReturnStmtContextExt<'input> {
    fn new(ctx: &dyn StmtContextAttrs<'input>) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::ReturnStmtContext(
            BaseParserRuleContext::copy_from(
                ctx,
                ReturnStmtContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type ContinueStmtContext<'input> =
    BaseParserRuleContext<'input, ContinueStmtContextExt<'input>>;

pub trait ContinueStmtContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Continue
    /// Returns `None` if there is no child corresponding to token Continue
    fn Continue(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Continue, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Semicolon
    /// Returns `None` if there is no child corresponding to token Semicolon
    fn Semicolon(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Semicolon, 0)
    }
}

impl<'input> ContinueStmtContextAttrs<'input> for ContinueStmtContext<'input> {}

pub struct ContinueStmtContextExt<'input> {
    base: StmtContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {ContinueStmtContextExt<'a>}

impl<'input> SysYParserContext<'input> for ContinueStmtContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ContinueStmtContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_continueStmt(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_continueStmt(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ContinueStmtContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_continueStmt(self);
    }
}

impl<'input> CustomRuleContext<'input> for ContinueStmtContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_stmt
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_stmt }
}

impl<'input> Borrow<StmtContextExt<'input>> for ContinueStmtContext<'input> {
    fn borrow(&self) -> &StmtContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<StmtContextExt<'input>> for ContinueStmtContext<'input> {
    fn borrow_mut(&mut self) -> &mut StmtContextExt<'input> {
        &mut self.base
    }
}

impl<'input> StmtContextAttrs<'input> for ContinueStmtContext<'input> {}

impl<'input> ContinueStmtContextExt<'input> {
    fn new(ctx: &dyn StmtContextAttrs<'input>) -> Rc<StmtContextAll<'input>> {
        Rc::new(StmtContextAll::ContinueStmtContext(
            BaseParserRuleContext::copy_from(
                ctx,
                ContinueStmtContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn stmt(&mut self) -> Result<Rc<StmtContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = StmtContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 30, RULE_stmt);
        let mut _localctx: Rc<StmtContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            recog.base.set_state(255);
            recog.err_handler.sync(&mut recog.base)?;
            match recog.interpreter.adaptive_predict(23, &mut recog.base)? {
                1 => {
                    let tmp = AssignmentContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 1);
                    _localctx = tmp;
                    {
                        /*InvokeRule lVal*/
                        recog.base.set_state(216);
                        recog.lVal()?;

                        recog.base.set_state(217);
                        recog.base.match_token(T__0, &mut recog.err_handler)?;

                        /*InvokeRule exp*/
                        recog.base.set_state(218);
                        recog.exp()?;

                        recog.base.set_state(219);
                        recog.base.match_token(Semicolon, &mut recog.err_handler)?;
                    }
                }
                2 => {
                    let tmp = ExpStmtContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 2);
                    _localctx = tmp;
                    {
                        recog.base.set_state(222);
                        recog.err_handler.sync(&mut recog.base)?;
                        _la = recog.base.input.la(1);
                        if ((_la - 14) & !0x3f) == 0
                            && ((1usize << (_la - 14))
                                & ((1usize << (Lparen - 14))
                                    | (1usize << (Minus - 14))
                                    | (1usize << (Exclamation - 14))
                                    | (1usize << (Addition - 14))
                                    | (1usize << (IntLiteral - 14))
                                    | (1usize << (FloatLiteral - 14))
                                    | (1usize << (Identifier - 14))))
                                != 0
                        {
                            {
                                /*InvokeRule exp*/
                                recog.base.set_state(221);
                                recog.exp()?;
                            }
                        }

                        recog.base.set_state(224);
                        recog.base.match_token(Semicolon, &mut recog.err_handler)?;
                    }
                }
                3 => {
                    let tmp = BlockStmtContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 3);
                    _localctx = tmp;
                    {
                        /*InvokeRule block*/
                        recog.base.set_state(225);
                        recog.block()?;
                    }
                }
                4 => {
                    let tmp = IfStmt1ContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 4);
                    _localctx = tmp;
                    {
                        recog.base.set_state(226);
                        recog.base.match_token(If, &mut recog.err_handler)?;

                        recog.base.set_state(227);
                        recog.base.match_token(Lparen, &mut recog.err_handler)?;

                        /*InvokeRule cond*/
                        recog.base.set_state(228);
                        recog.cond()?;

                        recog.base.set_state(229);
                        recog.base.match_token(Rparen, &mut recog.err_handler)?;

                        /*InvokeRule stmt*/
                        recog.base.set_state(230);
                        recog.stmt()?;
                    }
                }
                5 => {
                    let tmp = IfStmt2ContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 5);
                    _localctx = tmp;
                    {
                        recog.base.set_state(232);
                        recog.base.match_token(If, &mut recog.err_handler)?;

                        recog.base.set_state(233);
                        recog.base.match_token(Lparen, &mut recog.err_handler)?;

                        /*InvokeRule cond*/
                        recog.base.set_state(234);
                        recog.cond()?;

                        recog.base.set_state(235);
                        recog.base.match_token(Rparen, &mut recog.err_handler)?;

                        /*InvokeRule stmt*/
                        recog.base.set_state(236);
                        recog.stmt()?;

                        recog.base.set_state(237);
                        recog.base.match_token(Else, &mut recog.err_handler)?;

                        /*InvokeRule stmt*/
                        recog.base.set_state(238);
                        recog.stmt()?;
                    }
                }
                6 => {
                    let tmp = WhileStmtContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 6);
                    _localctx = tmp;
                    {
                        recog.base.set_state(240);
                        recog.base.match_token(While, &mut recog.err_handler)?;

                        recog.base.set_state(241);
                        recog.base.match_token(Lparen, &mut recog.err_handler)?;

                        /*InvokeRule cond*/
                        recog.base.set_state(242);
                        recog.cond()?;

                        recog.base.set_state(243);
                        recog.base.match_token(Rparen, &mut recog.err_handler)?;

                        /*InvokeRule stmt*/
                        recog.base.set_state(244);
                        recog.stmt()?;
                    }
                }
                7 => {
                    let tmp = BreakStmtContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 7);
                    _localctx = tmp;
                    {
                        recog.base.set_state(246);
                        recog.base.match_token(Break, &mut recog.err_handler)?;

                        recog.base.set_state(247);
                        recog.base.match_token(Semicolon, &mut recog.err_handler)?;
                    }
                }
                8 => {
                    let tmp = ContinueStmtContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 8);
                    _localctx = tmp;
                    {
                        recog.base.set_state(248);
                        recog.base.match_token(Continue, &mut recog.err_handler)?;

                        recog.base.set_state(249);
                        recog.base.match_token(Semicolon, &mut recog.err_handler)?;
                    }
                }
                9 => {
                    let tmp = ReturnStmtContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 9);
                    _localctx = tmp;
                    {
                        recog.base.set_state(250);
                        recog.base.match_token(Return, &mut recog.err_handler)?;

                        recog.base.set_state(252);
                        recog.err_handler.sync(&mut recog.base)?;
                        _la = recog.base.input.la(1);
                        if ((_la - 14) & !0x3f) == 0
                            && ((1usize << (_la - 14))
                                & ((1usize << (Lparen - 14))
                                    | (1usize << (Minus - 14))
                                    | (1usize << (Exclamation - 14))
                                    | (1usize << (Addition - 14))
                                    | (1usize << (IntLiteral - 14))
                                    | (1usize << (FloatLiteral - 14))
                                    | (1usize << (Identifier - 14))))
                                != 0
                        {
                            {
                                /*InvokeRule exp*/
                                recog.base.set_state(251);
                                recog.exp()?;
                            }
                        }

                        recog.base.set_state(254);
                        recog.base.match_token(Semicolon, &mut recog.err_handler)?;
                    }
                }

                _ => {}
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- exp ----------------
pub type ExpContextAll<'input> = ExpContext<'input>;

pub type ExpContext<'input> = BaseParserRuleContext<'input, ExpContextExt<'input>>;

#[derive(Clone)]
pub struct ExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for ExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ExpContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_exp(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_exp(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ExpContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_exp(self);
    }
}

impl<'input> CustomRuleContext<'input> for ExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_exp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_exp }
}
antlr_rust::tid! {ExpContextExt<'a>}

impl<'input> ExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<ExpContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            ExpContextExt { ph: PhantomData },
        ))
    }
}

pub trait ExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<ExpContextExt<'input>>
{
    fn addExp(&self) -> Option<Rc<AddExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> ExpContextAttrs<'input> for ExpContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn exp(&mut self) -> Result<Rc<ExpContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = ExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 32, RULE_exp);
        let mut _localctx: Rc<ExpContextAll> = _localctx;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                /*InvokeRule addExp*/
                recog.base.set_state(257);
                recog.addExp_rec(0)?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- cond ----------------
pub type CondContextAll<'input> = CondContext<'input>;

pub type CondContext<'input> = BaseParserRuleContext<'input, CondContextExt<'input>>;

#[derive(Clone)]
pub struct CondContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for CondContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for CondContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_cond(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_cond(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for CondContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_cond(self);
    }
}

impl<'input> CustomRuleContext<'input> for CondContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_cond
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_cond }
}
antlr_rust::tid! {CondContextExt<'a>}

impl<'input> CondContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<CondContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            CondContextExt { ph: PhantomData },
        ))
    }
}

pub trait CondContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<CondContextExt<'input>>
{
    fn lOrExp(&self) -> Option<Rc<LOrExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> CondContextAttrs<'input> for CondContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn cond(&mut self) -> Result<Rc<CondContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = CondContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 34, RULE_cond);
        let mut _localctx: Rc<CondContextAll> = _localctx;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                /*InvokeRule lOrExp*/
                recog.base.set_state(259);
                recog.lOrExp_rec(0)?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- lVal ----------------
pub type LValContextAll<'input> = LValContext<'input>;

pub type LValContext<'input> = BaseParserRuleContext<'input, LValContextExt<'input>>;

#[derive(Clone)]
pub struct LValContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for LValContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for LValContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_lVal(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_lVal(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for LValContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_lVal(self);
    }
}

impl<'input> CustomRuleContext<'input> for LValContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_lVal
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_lVal }
}
antlr_rust::tid! {LValContextExt<'a>}

impl<'input> LValContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<LValContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            LValContextExt { ph: PhantomData },
        ))
    }
}

pub trait LValContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<LValContextExt<'input>>
{
    /// Retrieves first TerminalNode corresponding to token Identifier
    /// Returns `None` if there is no child corresponding to token Identifier
    fn Identifier(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Identifier, 0)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Lbrkt in current rule
    fn Lbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Lbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Lbrkt is less or equal than `i`.
    fn Lbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lbrkt, i)
    }
    fn exp_all(&self) -> Vec<Rc<ExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn exp(&self, i: usize) -> Option<Rc<ExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Rbrkt in current rule
    fn Rbrkt_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Rbrkt, starting from 0.
    /// Returns `None` if number of children corresponding to token Rbrkt is less or equal than `i`.
    fn Rbrkt(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rbrkt, i)
    }
}

impl<'input> LValContextAttrs<'input> for LValContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn lVal(&mut self) -> Result<Rc<LValContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = LValContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 36, RULE_lVal);
        let mut _localctx: Rc<LValContextAll> = _localctx;
        let result: Result<(), ANTLRError> = (|| {
            let mut _alt: isize;
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                recog.base.set_state(261);
                recog.base.match_token(Identifier, &mut recog.err_handler)?;

                recog.base.set_state(268);
                recog.err_handler.sync(&mut recog.base)?;
                _alt = recog.interpreter.adaptive_predict(24, &mut recog.base)?;
                while { _alt != 2 && _alt != INVALID_ALT } {
                    if _alt == 1 {
                        {
                            {
                                recog.base.set_state(262);
                                recog.base.match_token(Lbrkt, &mut recog.err_handler)?;

                                /*InvokeRule exp*/
                                recog.base.set_state(263);
                                recog.exp()?;

                                recog.base.set_state(264);
                                recog.base.match_token(Rbrkt, &mut recog.err_handler)?;
                            }
                        }
                    }
                    recog.base.set_state(270);
                    recog.err_handler.sync(&mut recog.base)?;
                    _alt = recog.interpreter.adaptive_predict(24, &mut recog.base)?;
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- primaryExp ----------------
#[derive(Debug)]
pub enum PrimaryExpContextAll<'input> {
    PrimaryExp2Context(PrimaryExp2Context<'input>),
    PrimaryExp1Context(PrimaryExp1Context<'input>),
    PrimaryExp3Context(PrimaryExp3Context<'input>),
    Error(PrimaryExpContext<'input>),
}
antlr_rust::tid! {PrimaryExpContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for PrimaryExpContextAll<'input> {}

impl<'input> SysYParserContext<'input> for PrimaryExpContextAll<'input> {}

impl<'input> Deref for PrimaryExpContextAll<'input> {
    type Target = dyn PrimaryExpContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use PrimaryExpContextAll::*;
        match self {
            PrimaryExp2Context(inner) => inner,
            PrimaryExp1Context(inner) => inner,
            PrimaryExp3Context(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for PrimaryExpContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for PrimaryExpContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type PrimaryExpContext<'input> = BaseParserRuleContext<'input, PrimaryExpContextExt<'input>>;

#[derive(Clone)]
pub struct PrimaryExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for PrimaryExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for PrimaryExpContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for PrimaryExpContext<'input> {}

impl<'input> CustomRuleContext<'input> for PrimaryExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_primaryExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_primaryExp }
}
antlr_rust::tid! {PrimaryExpContextExt<'a>}

impl<'input> PrimaryExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<PrimaryExpContextAll<'input>> {
        Rc::new(PrimaryExpContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                PrimaryExpContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait PrimaryExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<PrimaryExpContextExt<'input>>
{
}

impl<'input> PrimaryExpContextAttrs<'input> for PrimaryExpContext<'input> {}

pub type PrimaryExp2Context<'input> = BaseParserRuleContext<'input, PrimaryExp2ContextExt<'input>>;

pub trait PrimaryExp2ContextAttrs<'input>: SysYParserContext<'input> {
    fn lVal(&self) -> Option<Rc<LValContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> PrimaryExp2ContextAttrs<'input> for PrimaryExp2Context<'input> {}

pub struct PrimaryExp2ContextExt<'input> {
    base: PrimaryExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {PrimaryExp2ContextExt<'a>}

impl<'input> SysYParserContext<'input> for PrimaryExp2Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for PrimaryExp2Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_primaryExp2(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_primaryExp2(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for PrimaryExp2Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_primaryExp2(self);
    }
}

impl<'input> CustomRuleContext<'input> for PrimaryExp2ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_primaryExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_primaryExp }
}

impl<'input> Borrow<PrimaryExpContextExt<'input>> for PrimaryExp2Context<'input> {
    fn borrow(&self) -> &PrimaryExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<PrimaryExpContextExt<'input>> for PrimaryExp2Context<'input> {
    fn borrow_mut(&mut self) -> &mut PrimaryExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> PrimaryExpContextAttrs<'input> for PrimaryExp2Context<'input> {}

impl<'input> PrimaryExp2ContextExt<'input> {
    fn new(ctx: &dyn PrimaryExpContextAttrs<'input>) -> Rc<PrimaryExpContextAll<'input>> {
        Rc::new(PrimaryExpContextAll::PrimaryExp2Context(
            BaseParserRuleContext::copy_from(
                ctx,
                PrimaryExp2ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type PrimaryExp1Context<'input> = BaseParserRuleContext<'input, PrimaryExp1ContextExt<'input>>;

pub trait PrimaryExp1ContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Lparen
    /// Returns `None` if there is no child corresponding to token Lparen
    fn Lparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lparen, 0)
    }
    fn exp(&self) -> Option<Rc<ExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token Rparen
    /// Returns `None` if there is no child corresponding to token Rparen
    fn Rparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rparen, 0)
    }
}

impl<'input> PrimaryExp1ContextAttrs<'input> for PrimaryExp1Context<'input> {}

pub struct PrimaryExp1ContextExt<'input> {
    base: PrimaryExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {PrimaryExp1ContextExt<'a>}

impl<'input> SysYParserContext<'input> for PrimaryExp1Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for PrimaryExp1Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_primaryExp1(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_primaryExp1(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for PrimaryExp1Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_primaryExp1(self);
    }
}

impl<'input> CustomRuleContext<'input> for PrimaryExp1ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_primaryExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_primaryExp }
}

impl<'input> Borrow<PrimaryExpContextExt<'input>> for PrimaryExp1Context<'input> {
    fn borrow(&self) -> &PrimaryExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<PrimaryExpContextExt<'input>> for PrimaryExp1Context<'input> {
    fn borrow_mut(&mut self) -> &mut PrimaryExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> PrimaryExpContextAttrs<'input> for PrimaryExp1Context<'input> {}

impl<'input> PrimaryExp1ContextExt<'input> {
    fn new(ctx: &dyn PrimaryExpContextAttrs<'input>) -> Rc<PrimaryExpContextAll<'input>> {
        Rc::new(PrimaryExpContextAll::PrimaryExp1Context(
            BaseParserRuleContext::copy_from(
                ctx,
                PrimaryExp1ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type PrimaryExp3Context<'input> = BaseParserRuleContext<'input, PrimaryExp3ContextExt<'input>>;

pub trait PrimaryExp3ContextAttrs<'input>: SysYParserContext<'input> {
    fn number(&self) -> Option<Rc<NumberContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> PrimaryExp3ContextAttrs<'input> for PrimaryExp3Context<'input> {}

pub struct PrimaryExp3ContextExt<'input> {
    base: PrimaryExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {PrimaryExp3ContextExt<'a>}

impl<'input> SysYParserContext<'input> for PrimaryExp3Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for PrimaryExp3Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_primaryExp3(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_primaryExp3(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for PrimaryExp3Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_primaryExp3(self);
    }
}

impl<'input> CustomRuleContext<'input> for PrimaryExp3ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_primaryExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_primaryExp }
}

impl<'input> Borrow<PrimaryExpContextExt<'input>> for PrimaryExp3Context<'input> {
    fn borrow(&self) -> &PrimaryExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<PrimaryExpContextExt<'input>> for PrimaryExp3Context<'input> {
    fn borrow_mut(&mut self) -> &mut PrimaryExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> PrimaryExpContextAttrs<'input> for PrimaryExp3Context<'input> {}

impl<'input> PrimaryExp3ContextExt<'input> {
    fn new(ctx: &dyn PrimaryExpContextAttrs<'input>) -> Rc<PrimaryExpContextAll<'input>> {
        Rc::new(PrimaryExpContextAll::PrimaryExp3Context(
            BaseParserRuleContext::copy_from(
                ctx,
                PrimaryExp3ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn primaryExp(&mut self) -> Result<Rc<PrimaryExpContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = PrimaryExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_rule(_localctx.clone(), 38, RULE_primaryExp);
        let mut _localctx: Rc<PrimaryExpContextAll> = _localctx;
        let result: Result<(), ANTLRError> = (|| {
            recog.base.set_state(277);
            recog.err_handler.sync(&mut recog.base)?;
            match recog.base.input.la(1) {
                Lparen => {
                    let tmp = PrimaryExp1ContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 1);
                    _localctx = tmp;
                    {
                        recog.base.set_state(271);
                        recog.base.match_token(Lparen, &mut recog.err_handler)?;

                        /*InvokeRule exp*/
                        recog.base.set_state(272);
                        recog.exp()?;

                        recog.base.set_state(273);
                        recog.base.match_token(Rparen, &mut recog.err_handler)?;
                    }
                }

                Identifier => {
                    let tmp = PrimaryExp2ContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 2);
                    _localctx = tmp;
                    {
                        /*InvokeRule lVal*/
                        recog.base.set_state(275);
                        recog.lVal()?;
                    }
                }

                IntLiteral | FloatLiteral => {
                    let tmp = PrimaryExp3ContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 3);
                    _localctx = tmp;
                    {
                        /*InvokeRule number*/
                        recog.base.set_state(276);
                        recog.number()?;
                    }
                }

                _ => Err(ANTLRError::NoAltError(NoViableAltError::new(
                    &mut recog.base,
                )))?,
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- number ----------------
#[derive(Debug)]
pub enum NumberContextAll<'input> {
    IntnumContext(IntnumContext<'input>),
    FloatnumContext(FloatnumContext<'input>),
    Error(NumberContext<'input>),
}
antlr_rust::tid! {NumberContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for NumberContextAll<'input> {}

impl<'input> SysYParserContext<'input> for NumberContextAll<'input> {}

impl<'input> Deref for NumberContextAll<'input> {
    type Target = dyn NumberContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use NumberContextAll::*;
        match self {
            IntnumContext(inner) => inner,
            FloatnumContext(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for NumberContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for NumberContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type NumberContext<'input> = BaseParserRuleContext<'input, NumberContextExt<'input>>;

#[derive(Clone)]
pub struct NumberContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for NumberContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for NumberContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for NumberContext<'input> {}

impl<'input> CustomRuleContext<'input> for NumberContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_number
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_number }
}
antlr_rust::tid! {NumberContextExt<'a>}

impl<'input> NumberContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<NumberContextAll<'input>> {
        Rc::new(NumberContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                NumberContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait NumberContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<NumberContextExt<'input>>
{
}

impl<'input> NumberContextAttrs<'input> for NumberContext<'input> {}

pub type IntnumContext<'input> = BaseParserRuleContext<'input, IntnumContextExt<'input>>;

pub trait IntnumContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token IntLiteral
    /// Returns `None` if there is no child corresponding to token IntLiteral
    fn IntLiteral(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(IntLiteral, 0)
    }
}

impl<'input> IntnumContextAttrs<'input> for IntnumContext<'input> {}

pub struct IntnumContextExt<'input> {
    base: NumberContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {IntnumContextExt<'a>}

impl<'input> SysYParserContext<'input> for IntnumContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for IntnumContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_intnum(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_intnum(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for IntnumContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_intnum(self);
    }
}

impl<'input> CustomRuleContext<'input> for IntnumContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_number
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_number }
}

impl<'input> Borrow<NumberContextExt<'input>> for IntnumContext<'input> {
    fn borrow(&self) -> &NumberContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<NumberContextExt<'input>> for IntnumContext<'input> {
    fn borrow_mut(&mut self) -> &mut NumberContextExt<'input> {
        &mut self.base
    }
}

impl<'input> NumberContextAttrs<'input> for IntnumContext<'input> {}

impl<'input> IntnumContextExt<'input> {
    fn new(ctx: &dyn NumberContextAttrs<'input>) -> Rc<NumberContextAll<'input>> {
        Rc::new(NumberContextAll::IntnumContext(
            BaseParserRuleContext::copy_from(
                ctx,
                IntnumContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type FloatnumContext<'input> = BaseParserRuleContext<'input, FloatnumContextExt<'input>>;

pub trait FloatnumContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token FloatLiteral
    /// Returns `None` if there is no child corresponding to token FloatLiteral
    fn FloatLiteral(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(FloatLiteral, 0)
    }
}

impl<'input> FloatnumContextAttrs<'input> for FloatnumContext<'input> {}

pub struct FloatnumContextExt<'input> {
    base: NumberContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {FloatnumContextExt<'a>}

impl<'input> SysYParserContext<'input> for FloatnumContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for FloatnumContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_floatnum(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_floatnum(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for FloatnumContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_floatnum(self);
    }
}

impl<'input> CustomRuleContext<'input> for FloatnumContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_number
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_number }
}

impl<'input> Borrow<NumberContextExt<'input>> for FloatnumContext<'input> {
    fn borrow(&self) -> &NumberContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<NumberContextExt<'input>> for FloatnumContext<'input> {
    fn borrow_mut(&mut self) -> &mut NumberContextExt<'input> {
        &mut self.base
    }
}

impl<'input> NumberContextAttrs<'input> for FloatnumContext<'input> {}

impl<'input> FloatnumContextExt<'input> {
    fn new(ctx: &dyn NumberContextAttrs<'input>) -> Rc<NumberContextAll<'input>> {
        Rc::new(NumberContextAll::FloatnumContext(
            BaseParserRuleContext::copy_from(
                ctx,
                FloatnumContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn number(&mut self) -> Result<Rc<NumberContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = NumberContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 40, RULE_number);
        let mut _localctx: Rc<NumberContextAll> = _localctx;
        let result: Result<(), ANTLRError> = (|| {
            recog.base.set_state(281);
            recog.err_handler.sync(&mut recog.base)?;
            match recog.base.input.la(1) {
                IntLiteral => {
                    let tmp = IntnumContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 1);
                    _localctx = tmp;
                    {
                        recog.base.set_state(279);
                        recog.base.match_token(IntLiteral, &mut recog.err_handler)?;
                    }
                }

                FloatLiteral => {
                    let tmp = FloatnumContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 2);
                    _localctx = tmp;
                    {
                        recog.base.set_state(280);
                        recog
                            .base
                            .match_token(FloatLiteral, &mut recog.err_handler)?;
                    }
                }

                _ => Err(ANTLRError::NoAltError(NoViableAltError::new(
                    &mut recog.base,
                )))?,
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- unaryExp ----------------
#[derive(Debug)]
pub enum UnaryExpContextAll<'input> {
    Unary1Context(Unary1Context<'input>),
    Unary2Context(Unary2Context<'input>),
    Unary3Context(Unary3Context<'input>),
    Error(UnaryExpContext<'input>),
}
antlr_rust::tid! {UnaryExpContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for UnaryExpContextAll<'input> {}

impl<'input> SysYParserContext<'input> for UnaryExpContextAll<'input> {}

impl<'input> Deref for UnaryExpContextAll<'input> {
    type Target = dyn UnaryExpContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use UnaryExpContextAll::*;
        match self {
            Unary1Context(inner) => inner,
            Unary2Context(inner) => inner,
            Unary3Context(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for UnaryExpContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for UnaryExpContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type UnaryExpContext<'input> = BaseParserRuleContext<'input, UnaryExpContextExt<'input>>;

#[derive(Clone)]
pub struct UnaryExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for UnaryExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for UnaryExpContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for UnaryExpContext<'input> {}

impl<'input> CustomRuleContext<'input> for UnaryExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_unaryExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_unaryExp }
}
antlr_rust::tid! {UnaryExpContextExt<'a>}

impl<'input> UnaryExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<UnaryExpContextAll<'input>> {
        Rc::new(UnaryExpContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                UnaryExpContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait UnaryExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<UnaryExpContextExt<'input>>
{
}

impl<'input> UnaryExpContextAttrs<'input> for UnaryExpContext<'input> {}

pub type Unary1Context<'input> = BaseParserRuleContext<'input, Unary1ContextExt<'input>>;

pub trait Unary1ContextAttrs<'input>: SysYParserContext<'input> {
    fn primaryExp(&self) -> Option<Rc<PrimaryExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> Unary1ContextAttrs<'input> for Unary1Context<'input> {}

pub struct Unary1ContextExt<'input> {
    base: UnaryExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Unary1ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Unary1Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Unary1Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_unary1(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_unary1(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Unary1Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_unary1(self);
    }
}

impl<'input> CustomRuleContext<'input> for Unary1ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_unaryExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_unaryExp }
}

impl<'input> Borrow<UnaryExpContextExt<'input>> for Unary1Context<'input> {
    fn borrow(&self) -> &UnaryExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<UnaryExpContextExt<'input>> for Unary1Context<'input> {
    fn borrow_mut(&mut self) -> &mut UnaryExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> UnaryExpContextAttrs<'input> for Unary1Context<'input> {}

impl<'input> Unary1ContextExt<'input> {
    fn new(ctx: &dyn UnaryExpContextAttrs<'input>) -> Rc<UnaryExpContextAll<'input>> {
        Rc::new(UnaryExpContextAll::Unary1Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Unary1ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type Unary2Context<'input> = BaseParserRuleContext<'input, Unary2ContextExt<'input>>;

pub trait Unary2ContextAttrs<'input>: SysYParserContext<'input> {
    /// Retrieves first TerminalNode corresponding to token Identifier
    /// Returns `None` if there is no child corresponding to token Identifier
    fn Identifier(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Identifier, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Lparen
    /// Returns `None` if there is no child corresponding to token Lparen
    fn Lparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Lparen, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Rparen
    /// Returns `None` if there is no child corresponding to token Rparen
    fn Rparen(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Rparen, 0)
    }
    fn funcRParams(&self) -> Option<Rc<FuncRParamsContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> Unary2ContextAttrs<'input> for Unary2Context<'input> {}

pub struct Unary2ContextExt<'input> {
    base: UnaryExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Unary2ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Unary2Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Unary2Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_unary2(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_unary2(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Unary2Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_unary2(self);
    }
}

impl<'input> CustomRuleContext<'input> for Unary2ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_unaryExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_unaryExp }
}

impl<'input> Borrow<UnaryExpContextExt<'input>> for Unary2Context<'input> {
    fn borrow(&self) -> &UnaryExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<UnaryExpContextExt<'input>> for Unary2Context<'input> {
    fn borrow_mut(&mut self) -> &mut UnaryExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> UnaryExpContextAttrs<'input> for Unary2Context<'input> {}

impl<'input> Unary2ContextExt<'input> {
    fn new(ctx: &dyn UnaryExpContextAttrs<'input>) -> Rc<UnaryExpContextAll<'input>> {
        Rc::new(UnaryExpContextAll::Unary2Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Unary2ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type Unary3Context<'input> = BaseParserRuleContext<'input, Unary3ContextExt<'input>>;

pub trait Unary3ContextAttrs<'input>: SysYParserContext<'input> {
    fn unaryOp(&self) -> Option<Rc<UnaryOpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn unaryExp(&self) -> Option<Rc<UnaryExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> Unary3ContextAttrs<'input> for Unary3Context<'input> {}

pub struct Unary3ContextExt<'input> {
    base: UnaryExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Unary3ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Unary3Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Unary3Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_unary3(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_unary3(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Unary3Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_unary3(self);
    }
}

impl<'input> CustomRuleContext<'input> for Unary3ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_unaryExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_unaryExp }
}

impl<'input> Borrow<UnaryExpContextExt<'input>> for Unary3Context<'input> {
    fn borrow(&self) -> &UnaryExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<UnaryExpContextExt<'input>> for Unary3Context<'input> {
    fn borrow_mut(&mut self) -> &mut UnaryExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> UnaryExpContextAttrs<'input> for Unary3Context<'input> {}

impl<'input> Unary3ContextExt<'input> {
    fn new(ctx: &dyn UnaryExpContextAttrs<'input>) -> Rc<UnaryExpContextAll<'input>> {
        Rc::new(UnaryExpContextAll::Unary3Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Unary3ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn unaryExp(&mut self) -> Result<Rc<UnaryExpContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = UnaryExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 42, RULE_unaryExp);
        let mut _localctx: Rc<UnaryExpContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            recog.base.set_state(293);
            recog.err_handler.sync(&mut recog.base)?;
            match recog.interpreter.adaptive_predict(28, &mut recog.base)? {
                1 => {
                    let tmp = Unary1ContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 1);
                    _localctx = tmp;
                    {
                        /*InvokeRule primaryExp*/
                        recog.base.set_state(283);
                        recog.primaryExp()?;
                    }
                }
                2 => {
                    let tmp = Unary2ContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 2);
                    _localctx = tmp;
                    {
                        recog.base.set_state(284);
                        recog.base.match_token(Identifier, &mut recog.err_handler)?;

                        recog.base.set_state(285);
                        recog.base.match_token(Lparen, &mut recog.err_handler)?;

                        recog.base.set_state(287);
                        recog.err_handler.sync(&mut recog.base)?;
                        _la = recog.base.input.la(1);
                        if ((_la - 14) & !0x3f) == 0
                            && ((1usize << (_la - 14))
                                & ((1usize << (Lparen - 14))
                                    | (1usize << (Minus - 14))
                                    | (1usize << (Exclamation - 14))
                                    | (1usize << (Addition - 14))
                                    | (1usize << (IntLiteral - 14))
                                    | (1usize << (FloatLiteral - 14))
                                    | (1usize << (Identifier - 14))))
                                != 0
                        {
                            {
                                /*InvokeRule funcRParams*/
                                recog.base.set_state(286);
                                recog.funcRParams()?;
                            }
                        }

                        recog.base.set_state(289);
                        recog.base.match_token(Rparen, &mut recog.err_handler)?;
                    }
                }
                3 => {
                    let tmp = Unary3ContextExt::new(&**_localctx);
                    recog.base.enter_outer_alt(Some(tmp.clone()), 3);
                    _localctx = tmp;
                    {
                        /*InvokeRule unaryOp*/
                        recog.base.set_state(290);
                        recog.unaryOp()?;

                        /*InvokeRule unaryExp*/
                        recog.base.set_state(291);
                        recog.unaryExp()?;
                    }
                }

                _ => {}
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- unaryOp ----------------
pub type UnaryOpContextAll<'input> = UnaryOpContext<'input>;

pub type UnaryOpContext<'input> = BaseParserRuleContext<'input, UnaryOpContextExt<'input>>;

#[derive(Clone)]
pub struct UnaryOpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for UnaryOpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for UnaryOpContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_unaryOp(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_unaryOp(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for UnaryOpContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_unaryOp(self);
    }
}

impl<'input> CustomRuleContext<'input> for UnaryOpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_unaryOp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_unaryOp }
}
antlr_rust::tid! {UnaryOpContextExt<'a>}

impl<'input> UnaryOpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<UnaryOpContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            UnaryOpContextExt { ph: PhantomData },
        ))
    }
}

pub trait UnaryOpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<UnaryOpContextExt<'input>>
{
    /// Retrieves first TerminalNode corresponding to token Addition
    /// Returns `None` if there is no child corresponding to token Addition
    fn Addition(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Addition, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Minus
    /// Returns `None` if there is no child corresponding to token Minus
    fn Minus(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Minus, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Exclamation
    /// Returns `None` if there is no child corresponding to token Exclamation
    fn Exclamation(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Exclamation, 0)
    }
}

impl<'input> UnaryOpContextAttrs<'input> for UnaryOpContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn unaryOp(&mut self) -> Result<Rc<UnaryOpContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = UnaryOpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 44, RULE_unaryOp);
        let mut _localctx: Rc<UnaryOpContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                recog.base.set_state(295);
                _la = recog.base.input.la(1);
                if {
                    !(((_la) & !0x3f) == 0
                        && ((1usize << _la)
                            & ((1usize << Minus) | (1usize << Exclamation) | (1usize << Addition)))
                            != 0)
                } {
                    recog.err_handler.recover_inline(&mut recog.base)?;
                } else {
                    if recog.base.input.la(1) == TOKEN_EOF {
                        recog.base.matched_eof = true
                    };
                    recog.err_handler.report_match(&mut recog.base);
                    recog.base.consume(&mut recog.err_handler);
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- funcRParams ----------------
pub type FuncRParamsContextAll<'input> = FuncRParamsContext<'input>;

pub type FuncRParamsContext<'input> = BaseParserRuleContext<'input, FuncRParamsContextExt<'input>>;

#[derive(Clone)]
pub struct FuncRParamsContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for FuncRParamsContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for FuncRParamsContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_funcRParams(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_funcRParams(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for FuncRParamsContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_funcRParams(self);
    }
}

impl<'input> CustomRuleContext<'input> for FuncRParamsContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_funcRParams
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_funcRParams }
}
antlr_rust::tid! {FuncRParamsContextExt<'a>}

impl<'input> FuncRParamsContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<FuncRParamsContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            FuncRParamsContextExt { ph: PhantomData },
        ))
    }
}

pub trait FuncRParamsContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<FuncRParamsContextExt<'input>>
{
    fn funcRParam_all(&self) -> Vec<Rc<FuncRParamContextAll<'input>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    fn funcRParam(&self, i: usize) -> Option<Rc<FuncRParamContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(i)
    }
    /// Retrieves all `TerminalNode`s corresponding to token Comma in current rule
    fn Comma_all(&self) -> Vec<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.children_of_type()
    }
    /// Retrieves 'i's TerminalNode corresponding to token Comma, starting from 0.
    /// Returns `None` if number of children corresponding to token Comma is less or equal than `i`.
    fn Comma(&self, i: usize) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Comma, i)
    }
}

impl<'input> FuncRParamsContextAttrs<'input> for FuncRParamsContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn funcRParams(&mut self) -> Result<Rc<FuncRParamsContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = FuncRParamsContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_rule(_localctx.clone(), 46, RULE_funcRParams);
        let mut _localctx: Rc<FuncRParamsContextAll> = _localctx;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                /*InvokeRule funcRParam*/
                recog.base.set_state(297);
                recog.funcRParam()?;

                recog.base.set_state(302);
                recog.err_handler.sync(&mut recog.base)?;
                _la = recog.base.input.la(1);
                while _la == Comma {
                    {
                        {
                            recog.base.set_state(298);
                            recog.base.match_token(Comma, &mut recog.err_handler)?;

                            /*InvokeRule funcRParam*/
                            recog.base.set_state(299);
                            recog.funcRParam()?;
                        }
                    }
                    recog.base.set_state(304);
                    recog.err_handler.sync(&mut recog.base)?;
                    _la = recog.base.input.la(1);
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- funcRParam ----------------
pub type FuncRParamContextAll<'input> = FuncRParamContext<'input>;

pub type FuncRParamContext<'input> = BaseParserRuleContext<'input, FuncRParamContextExt<'input>>;

#[derive(Clone)]
pub struct FuncRParamContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for FuncRParamContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for FuncRParamContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_funcRParam(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_funcRParam(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for FuncRParamContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_funcRParam(self);
    }
}

impl<'input> CustomRuleContext<'input> for FuncRParamContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_funcRParam
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_funcRParam }
}
antlr_rust::tid! {FuncRParamContextExt<'a>}

impl<'input> FuncRParamContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<FuncRParamContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            FuncRParamContextExt { ph: PhantomData },
        ))
    }
}

pub trait FuncRParamContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<FuncRParamContextExt<'input>>
{
    fn exp(&self) -> Option<Rc<ExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> FuncRParamContextAttrs<'input> for FuncRParamContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn funcRParam(&mut self) -> Result<Rc<FuncRParamContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = FuncRParamContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_rule(_localctx.clone(), 48, RULE_funcRParam);
        let mut _localctx: Rc<FuncRParamContextAll> = _localctx;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                /*InvokeRule exp*/
                recog.base.set_state(305);
                recog.exp()?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}
//------------------- mulExp ----------------
#[derive(Debug)]
pub enum MulExpContextAll<'input> {
    Mul2Context(Mul2Context<'input>),
    Mul1Context(Mul1Context<'input>),
    Error(MulExpContext<'input>),
}
antlr_rust::tid! {MulExpContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for MulExpContextAll<'input> {}

impl<'input> SysYParserContext<'input> for MulExpContextAll<'input> {}

impl<'input> Deref for MulExpContextAll<'input> {
    type Target = dyn MulExpContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use MulExpContextAll::*;
        match self {
            Mul2Context(inner) => inner,
            Mul1Context(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for MulExpContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for MulExpContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type MulExpContext<'input> = BaseParserRuleContext<'input, MulExpContextExt<'input>>;

#[derive(Clone)]
pub struct MulExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for MulExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for MulExpContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for MulExpContext<'input> {}

impl<'input> CustomRuleContext<'input> for MulExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_mulExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_mulExp }
}
antlr_rust::tid! {MulExpContextExt<'a>}

impl<'input> MulExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<MulExpContextAll<'input>> {
        Rc::new(MulExpContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                MulExpContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait MulExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<MulExpContextExt<'input>>
{
}

impl<'input> MulExpContextAttrs<'input> for MulExpContext<'input> {}

pub type Mul2Context<'input> = BaseParserRuleContext<'input, Mul2ContextExt<'input>>;

pub trait Mul2ContextAttrs<'input>: SysYParserContext<'input> {
    fn mulExp(&self) -> Option<Rc<MulExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn unaryExp(&self) -> Option<Rc<UnaryExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token Multiplication
    /// Returns `None` if there is no child corresponding to token Multiplication
    fn Multiplication(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Multiplication, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Division
    /// Returns `None` if there is no child corresponding to token Division
    fn Division(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Division, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Modulo
    /// Returns `None` if there is no child corresponding to token Modulo
    fn Modulo(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Modulo, 0)
    }
}

impl<'input> Mul2ContextAttrs<'input> for Mul2Context<'input> {}

pub struct Mul2ContextExt<'input> {
    base: MulExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Mul2ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Mul2Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Mul2Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_mul2(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_mul2(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Mul2Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_mul2(self);
    }
}

impl<'input> CustomRuleContext<'input> for Mul2ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_mulExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_mulExp }
}

impl<'input> Borrow<MulExpContextExt<'input>> for Mul2Context<'input> {
    fn borrow(&self) -> &MulExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<MulExpContextExt<'input>> for Mul2Context<'input> {
    fn borrow_mut(&mut self) -> &mut MulExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> MulExpContextAttrs<'input> for Mul2Context<'input> {}

impl<'input> Mul2ContextExt<'input> {
    fn new(ctx: &dyn MulExpContextAttrs<'input>) -> Rc<MulExpContextAll<'input>> {
        Rc::new(MulExpContextAll::Mul2Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Mul2ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type Mul1Context<'input> = BaseParserRuleContext<'input, Mul1ContextExt<'input>>;

pub trait Mul1ContextAttrs<'input>: SysYParserContext<'input> {
    fn unaryExp(&self) -> Option<Rc<UnaryExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> Mul1ContextAttrs<'input> for Mul1Context<'input> {}

pub struct Mul1ContextExt<'input> {
    base: MulExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Mul1ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Mul1Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Mul1Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_mul1(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_mul1(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Mul1Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_mul1(self);
    }
}

impl<'input> CustomRuleContext<'input> for Mul1ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_mulExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_mulExp }
}

impl<'input> Borrow<MulExpContextExt<'input>> for Mul1Context<'input> {
    fn borrow(&self) -> &MulExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<MulExpContextExt<'input>> for Mul1Context<'input> {
    fn borrow_mut(&mut self) -> &mut MulExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> MulExpContextAttrs<'input> for Mul1Context<'input> {}

impl<'input> Mul1ContextExt<'input> {
    fn new(ctx: &dyn MulExpContextAttrs<'input>) -> Rc<MulExpContextAll<'input>> {
        Rc::new(MulExpContextAll::Mul1Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Mul1ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn mulExp(&mut self) -> Result<Rc<MulExpContextAll<'input>>, ANTLRError> {
        self.mulExp_rec(0)
    }

    fn mulExp_rec(&mut self, _p: isize) -> Result<Rc<MulExpContextAll<'input>>, ANTLRError> {
        let recog = self;
        let _parentctx = recog.ctx.take();
        let _parentState = recog.base.get_state();
        let mut _localctx = MulExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_recursion_rule(_localctx.clone(), 50, RULE_mulExp, _p);
        let mut _localctx: Rc<MulExpContextAll> = _localctx;
        let mut _prevctx = _localctx.clone();
        let _startState = 50;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            let mut _alt: isize;
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                {
                    let mut tmp = Mul1ContextExt::new(&**_localctx);
                    recog.ctx = Some(tmp.clone());
                    _localctx = tmp;
                    _prevctx = _localctx.clone();

                    /*InvokeRule unaryExp*/
                    recog.base.set_state(308);
                    recog.unaryExp()?;
                }

                let tmp = recog.input.lt(-1).cloned();
                recog.ctx.as_ref().unwrap().set_stop(tmp);
                recog.base.set_state(315);
                recog.err_handler.sync(&mut recog.base)?;
                _alt = recog.interpreter.adaptive_predict(30, &mut recog.base)?;
                while { _alt != 2 && _alt != INVALID_ALT } {
                    if _alt == 1 {
                        recog.trigger_exit_rule_event();
                        _prevctx = _localctx.clone();
                        {
                            {
                                /*recRuleLabeledAltStartAction*/
                                let mut tmp = Mul2ContextExt::new(&**MulExpContextExt::new(
                                    _parentctx.clone(),
                                    _parentState,
                                ));
                                recog.push_new_recursion_context(
                                    tmp.clone(),
                                    _startState,
                                    RULE_mulExp,
                                );
                                _localctx = tmp;
                                recog.base.set_state(310);
                                if !({ recog.precpred(None, 1) }) {
                                    Err(FailedPredicateError::new(
                                        &mut recog.base,
                                        Some("recog.precpred(None, 1)".to_owned()),
                                        None,
                                    ))?;
                                }
                                recog.base.set_state(311);
                                _la = recog.base.input.la(1);
                                if {
                                    !(((_la) & !0x3f) == 0
                                        && ((1usize << _la)
                                            & ((1usize << Multiplication)
                                                | (1usize << Division)
                                                | (1usize << Modulo)))
                                            != 0)
                                } {
                                    recog.err_handler.recover_inline(&mut recog.base)?;
                                } else {
                                    if recog.base.input.la(1) == TOKEN_EOF {
                                        recog.base.matched_eof = true
                                    };
                                    recog.err_handler.report_match(&mut recog.base);
                                    recog.base.consume(&mut recog.err_handler);
                                }
                                /*InvokeRule unaryExp*/
                                recog.base.set_state(312);
                                recog.unaryExp()?;
                            }
                        }
                    }
                    recog.base.set_state(317);
                    recog.err_handler.sync(&mut recog.base)?;
                    _alt = recog.interpreter.adaptive_predict(30, &mut recog.base)?;
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.unroll_recursion_context(_parentctx);

        Ok(_localctx)
    }
}
//------------------- addExp ----------------
#[derive(Debug)]
pub enum AddExpContextAll<'input> {
    Add2Context(Add2Context<'input>),
    Add1Context(Add1Context<'input>),
    Error(AddExpContext<'input>),
}
antlr_rust::tid! {AddExpContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for AddExpContextAll<'input> {}

impl<'input> SysYParserContext<'input> for AddExpContextAll<'input> {}

impl<'input> Deref for AddExpContextAll<'input> {
    type Target = dyn AddExpContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use AddExpContextAll::*;
        match self {
            Add2Context(inner) => inner,
            Add1Context(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for AddExpContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for AddExpContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type AddExpContext<'input> = BaseParserRuleContext<'input, AddExpContextExt<'input>>;

#[derive(Clone)]
pub struct AddExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for AddExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for AddExpContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for AddExpContext<'input> {}

impl<'input> CustomRuleContext<'input> for AddExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_addExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_addExp }
}
antlr_rust::tid! {AddExpContextExt<'a>}

impl<'input> AddExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<AddExpContextAll<'input>> {
        Rc::new(AddExpContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                AddExpContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait AddExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<AddExpContextExt<'input>>
{
}

impl<'input> AddExpContextAttrs<'input> for AddExpContext<'input> {}

pub type Add2Context<'input> = BaseParserRuleContext<'input, Add2ContextExt<'input>>;

pub trait Add2ContextAttrs<'input>: SysYParserContext<'input> {
    fn addExp(&self) -> Option<Rc<AddExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn mulExp(&self) -> Option<Rc<MulExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token Addition
    /// Returns `None` if there is no child corresponding to token Addition
    fn Addition(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Addition, 0)
    }
    /// Retrieves first TerminalNode corresponding to token Minus
    /// Returns `None` if there is no child corresponding to token Minus
    fn Minus(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(Minus, 0)
    }
}

impl<'input> Add2ContextAttrs<'input> for Add2Context<'input> {}

pub struct Add2ContextExt<'input> {
    base: AddExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Add2ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Add2Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Add2Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_add2(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_add2(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Add2Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_add2(self);
    }
}

impl<'input> CustomRuleContext<'input> for Add2ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_addExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_addExp }
}

impl<'input> Borrow<AddExpContextExt<'input>> for Add2Context<'input> {
    fn borrow(&self) -> &AddExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<AddExpContextExt<'input>> for Add2Context<'input> {
    fn borrow_mut(&mut self) -> &mut AddExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> AddExpContextAttrs<'input> for Add2Context<'input> {}

impl<'input> Add2ContextExt<'input> {
    fn new(ctx: &dyn AddExpContextAttrs<'input>) -> Rc<AddExpContextAll<'input>> {
        Rc::new(AddExpContextAll::Add2Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Add2ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type Add1Context<'input> = BaseParserRuleContext<'input, Add1ContextExt<'input>>;

pub trait Add1ContextAttrs<'input>: SysYParserContext<'input> {
    fn mulExp(&self) -> Option<Rc<MulExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> Add1ContextAttrs<'input> for Add1Context<'input> {}

pub struct Add1ContextExt<'input> {
    base: AddExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Add1ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Add1Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Add1Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_add1(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_add1(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Add1Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_add1(self);
    }
}

impl<'input> CustomRuleContext<'input> for Add1ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_addExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_addExp }
}

impl<'input> Borrow<AddExpContextExt<'input>> for Add1Context<'input> {
    fn borrow(&self) -> &AddExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<AddExpContextExt<'input>> for Add1Context<'input> {
    fn borrow_mut(&mut self) -> &mut AddExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> AddExpContextAttrs<'input> for Add1Context<'input> {}

impl<'input> Add1ContextExt<'input> {
    fn new(ctx: &dyn AddExpContextAttrs<'input>) -> Rc<AddExpContextAll<'input>> {
        Rc::new(AddExpContextAll::Add1Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Add1ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn addExp(&mut self) -> Result<Rc<AddExpContextAll<'input>>, ANTLRError> {
        self.addExp_rec(0)
    }

    fn addExp_rec(&mut self, _p: isize) -> Result<Rc<AddExpContextAll<'input>>, ANTLRError> {
        let recog = self;
        let _parentctx = recog.ctx.take();
        let _parentState = recog.base.get_state();
        let mut _localctx = AddExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_recursion_rule(_localctx.clone(), 52, RULE_addExp, _p);
        let mut _localctx: Rc<AddExpContextAll> = _localctx;
        let mut _prevctx = _localctx.clone();
        let _startState = 52;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            let mut _alt: isize;
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                {
                    let mut tmp = Add1ContextExt::new(&**_localctx);
                    recog.ctx = Some(tmp.clone());
                    _localctx = tmp;
                    _prevctx = _localctx.clone();

                    /*InvokeRule mulExp*/
                    recog.base.set_state(319);
                    recog.mulExp_rec(0)?;
                }

                let tmp = recog.input.lt(-1).cloned();
                recog.ctx.as_ref().unwrap().set_stop(tmp);
                recog.base.set_state(326);
                recog.err_handler.sync(&mut recog.base)?;
                _alt = recog.interpreter.adaptive_predict(31, &mut recog.base)?;
                while { _alt != 2 && _alt != INVALID_ALT } {
                    if _alt == 1 {
                        recog.trigger_exit_rule_event();
                        _prevctx = _localctx.clone();
                        {
                            {
                                /*recRuleLabeledAltStartAction*/
                                let mut tmp = Add2ContextExt::new(&**AddExpContextExt::new(
                                    _parentctx.clone(),
                                    _parentState,
                                ));
                                recog.push_new_recursion_context(
                                    tmp.clone(),
                                    _startState,
                                    RULE_addExp,
                                );
                                _localctx = tmp;
                                recog.base.set_state(321);
                                if !({ recog.precpred(None, 1) }) {
                                    Err(FailedPredicateError::new(
                                        &mut recog.base,
                                        Some("recog.precpred(None, 1)".to_owned()),
                                        None,
                                    ))?;
                                }
                                recog.base.set_state(322);
                                _la = recog.base.input.la(1);
                                if { !(_la == Minus || _la == Addition) } {
                                    recog.err_handler.recover_inline(&mut recog.base)?;
                                } else {
                                    if recog.base.input.la(1) == TOKEN_EOF {
                                        recog.base.matched_eof = true
                                    };
                                    recog.err_handler.report_match(&mut recog.base);
                                    recog.base.consume(&mut recog.err_handler);
                                }
                                /*InvokeRule mulExp*/
                                recog.base.set_state(323);
                                recog.mulExp_rec(0)?;
                            }
                        }
                    }
                    recog.base.set_state(328);
                    recog.err_handler.sync(&mut recog.base)?;
                    _alt = recog.interpreter.adaptive_predict(31, &mut recog.base)?;
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.unroll_recursion_context(_parentctx);

        Ok(_localctx)
    }
}
//------------------- relExp ----------------
#[derive(Debug)]
pub enum RelExpContextAll<'input> {
    Rel2Context(Rel2Context<'input>),
    Rel1Context(Rel1Context<'input>),
    Error(RelExpContext<'input>),
}
antlr_rust::tid! {RelExpContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for RelExpContextAll<'input> {}

impl<'input> SysYParserContext<'input> for RelExpContextAll<'input> {}

impl<'input> Deref for RelExpContextAll<'input> {
    type Target = dyn RelExpContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use RelExpContextAll::*;
        match self {
            Rel2Context(inner) => inner,
            Rel1Context(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for RelExpContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for RelExpContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type RelExpContext<'input> = BaseParserRuleContext<'input, RelExpContextExt<'input>>;

#[derive(Clone)]
pub struct RelExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for RelExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for RelExpContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for RelExpContext<'input> {}

impl<'input> CustomRuleContext<'input> for RelExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_relExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_relExp }
}
antlr_rust::tid! {RelExpContextExt<'a>}

impl<'input> RelExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<RelExpContextAll<'input>> {
        Rc::new(RelExpContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                RelExpContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait RelExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<RelExpContextExt<'input>>
{
}

impl<'input> RelExpContextAttrs<'input> for RelExpContext<'input> {}

pub type Rel2Context<'input> = BaseParserRuleContext<'input, Rel2ContextExt<'input>>;

pub trait Rel2ContextAttrs<'input>: SysYParserContext<'input> {
    fn relExp(&self) -> Option<Rc<RelExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn addExp(&self) -> Option<Rc<AddExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token LT
    /// Returns `None` if there is no child corresponding to token LT
    fn LT(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(LT, 0)
    }
    /// Retrieves first TerminalNode corresponding to token GT
    /// Returns `None` if there is no child corresponding to token GT
    fn GT(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(GT, 0)
    }
    /// Retrieves first TerminalNode corresponding to token LE
    /// Returns `None` if there is no child corresponding to token LE
    fn LE(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(LE, 0)
    }
    /// Retrieves first TerminalNode corresponding to token GE
    /// Returns `None` if there is no child corresponding to token GE
    fn GE(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(GE, 0)
    }
}

impl<'input> Rel2ContextAttrs<'input> for Rel2Context<'input> {}

pub struct Rel2ContextExt<'input> {
    base: RelExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Rel2ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Rel2Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Rel2Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_rel2(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_rel2(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Rel2Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_rel2(self);
    }
}

impl<'input> CustomRuleContext<'input> for Rel2ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_relExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_relExp }
}

impl<'input> Borrow<RelExpContextExt<'input>> for Rel2Context<'input> {
    fn borrow(&self) -> &RelExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<RelExpContextExt<'input>> for Rel2Context<'input> {
    fn borrow_mut(&mut self) -> &mut RelExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> RelExpContextAttrs<'input> for Rel2Context<'input> {}

impl<'input> Rel2ContextExt<'input> {
    fn new(ctx: &dyn RelExpContextAttrs<'input>) -> Rc<RelExpContextAll<'input>> {
        Rc::new(RelExpContextAll::Rel2Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Rel2ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type Rel1Context<'input> = BaseParserRuleContext<'input, Rel1ContextExt<'input>>;

pub trait Rel1ContextAttrs<'input>: SysYParserContext<'input> {
    fn addExp(&self) -> Option<Rc<AddExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> Rel1ContextAttrs<'input> for Rel1Context<'input> {}

pub struct Rel1ContextExt<'input> {
    base: RelExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Rel1ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Rel1Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Rel1Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_rel1(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_rel1(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Rel1Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_rel1(self);
    }
}

impl<'input> CustomRuleContext<'input> for Rel1ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_relExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_relExp }
}

impl<'input> Borrow<RelExpContextExt<'input>> for Rel1Context<'input> {
    fn borrow(&self) -> &RelExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<RelExpContextExt<'input>> for Rel1Context<'input> {
    fn borrow_mut(&mut self) -> &mut RelExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> RelExpContextAttrs<'input> for Rel1Context<'input> {}

impl<'input> Rel1ContextExt<'input> {
    fn new(ctx: &dyn RelExpContextAttrs<'input>) -> Rc<RelExpContextAll<'input>> {
        Rc::new(RelExpContextAll::Rel1Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Rel1ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn relExp(&mut self) -> Result<Rc<RelExpContextAll<'input>>, ANTLRError> {
        self.relExp_rec(0)
    }

    fn relExp_rec(&mut self, _p: isize) -> Result<Rc<RelExpContextAll<'input>>, ANTLRError> {
        let recog = self;
        let _parentctx = recog.ctx.take();
        let _parentState = recog.base.get_state();
        let mut _localctx = RelExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_recursion_rule(_localctx.clone(), 54, RULE_relExp, _p);
        let mut _localctx: Rc<RelExpContextAll> = _localctx;
        let mut _prevctx = _localctx.clone();
        let _startState = 54;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            let mut _alt: isize;
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                {
                    let mut tmp = Rel1ContextExt::new(&**_localctx);
                    recog.ctx = Some(tmp.clone());
                    _localctx = tmp;
                    _prevctx = _localctx.clone();

                    /*InvokeRule addExp*/
                    recog.base.set_state(330);
                    recog.addExp_rec(0)?;
                }

                let tmp = recog.input.lt(-1).cloned();
                recog.ctx.as_ref().unwrap().set_stop(tmp);
                recog.base.set_state(337);
                recog.err_handler.sync(&mut recog.base)?;
                _alt = recog.interpreter.adaptive_predict(32, &mut recog.base)?;
                while { _alt != 2 && _alt != INVALID_ALT } {
                    if _alt == 1 {
                        recog.trigger_exit_rule_event();
                        _prevctx = _localctx.clone();
                        {
                            {
                                /*recRuleLabeledAltStartAction*/
                                let mut tmp = Rel2ContextExt::new(&**RelExpContextExt::new(
                                    _parentctx.clone(),
                                    _parentState,
                                ));
                                recog.push_new_recursion_context(
                                    tmp.clone(),
                                    _startState,
                                    RULE_relExp,
                                );
                                _localctx = tmp;
                                recog.base.set_state(332);
                                if !({ recog.precpred(None, 1) }) {
                                    Err(FailedPredicateError::new(
                                        &mut recog.base,
                                        Some("recog.precpred(None, 1)".to_owned()),
                                        None,
                                    ))?;
                                }
                                recog.base.set_state(333);
                                _la = recog.base.input.la(1);
                                if {
                                    !(((_la - 35) & !0x3f) == 0
                                        && ((1usize << (_la - 35))
                                            & ((1usize << (LT - 35))
                                                | (1usize << (LE - 35))
                                                | (1usize << (GT - 35))
                                                | (1usize << (GE - 35))))
                                            != 0)
                                } {
                                    recog.err_handler.recover_inline(&mut recog.base)?;
                                } else {
                                    if recog.base.input.la(1) == TOKEN_EOF {
                                        recog.base.matched_eof = true
                                    };
                                    recog.err_handler.report_match(&mut recog.base);
                                    recog.base.consume(&mut recog.err_handler);
                                }
                                /*InvokeRule addExp*/
                                recog.base.set_state(334);
                                recog.addExp_rec(0)?;
                            }
                        }
                    }
                    recog.base.set_state(339);
                    recog.err_handler.sync(&mut recog.base)?;
                    _alt = recog.interpreter.adaptive_predict(32, &mut recog.base)?;
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.unroll_recursion_context(_parentctx);

        Ok(_localctx)
    }
}
//------------------- eqExp ----------------
#[derive(Debug)]
pub enum EqExpContextAll<'input> {
    Eq1Context(Eq1Context<'input>),
    Eq2Context(Eq2Context<'input>),
    Error(EqExpContext<'input>),
}
antlr_rust::tid! {EqExpContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for EqExpContextAll<'input> {}

impl<'input> SysYParserContext<'input> for EqExpContextAll<'input> {}

impl<'input> Deref for EqExpContextAll<'input> {
    type Target = dyn EqExpContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use EqExpContextAll::*;
        match self {
            Eq1Context(inner) => inner,
            Eq2Context(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for EqExpContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for EqExpContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type EqExpContext<'input> = BaseParserRuleContext<'input, EqExpContextExt<'input>>;

#[derive(Clone)]
pub struct EqExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for EqExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for EqExpContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for EqExpContext<'input> {}

impl<'input> CustomRuleContext<'input> for EqExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_eqExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_eqExp }
}
antlr_rust::tid! {EqExpContextExt<'a>}

impl<'input> EqExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<EqExpContextAll<'input>> {
        Rc::new(EqExpContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                EqExpContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait EqExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<EqExpContextExt<'input>>
{
}

impl<'input> EqExpContextAttrs<'input> for EqExpContext<'input> {}

pub type Eq1Context<'input> = BaseParserRuleContext<'input, Eq1ContextExt<'input>>;

pub trait Eq1ContextAttrs<'input>: SysYParserContext<'input> {
    fn relExp(&self) -> Option<Rc<RelExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> Eq1ContextAttrs<'input> for Eq1Context<'input> {}

pub struct Eq1ContextExt<'input> {
    base: EqExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Eq1ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Eq1Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Eq1Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_eq1(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_eq1(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Eq1Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_eq1(self);
    }
}

impl<'input> CustomRuleContext<'input> for Eq1ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_eqExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_eqExp }
}

impl<'input> Borrow<EqExpContextExt<'input>> for Eq1Context<'input> {
    fn borrow(&self) -> &EqExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<EqExpContextExt<'input>> for Eq1Context<'input> {
    fn borrow_mut(&mut self) -> &mut EqExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> EqExpContextAttrs<'input> for Eq1Context<'input> {}

impl<'input> Eq1ContextExt<'input> {
    fn new(ctx: &dyn EqExpContextAttrs<'input>) -> Rc<EqExpContextAll<'input>> {
        Rc::new(EqExpContextAll::Eq1Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Eq1ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type Eq2Context<'input> = BaseParserRuleContext<'input, Eq2ContextExt<'input>>;

pub trait Eq2ContextAttrs<'input>: SysYParserContext<'input> {
    fn eqExp(&self) -> Option<Rc<EqExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    fn relExp(&self) -> Option<Rc<RelExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token EQ
    /// Returns `None` if there is no child corresponding to token EQ
    fn EQ(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(EQ, 0)
    }
    /// Retrieves first TerminalNode corresponding to token NEQ
    /// Returns `None` if there is no child corresponding to token NEQ
    fn NEQ(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(NEQ, 0)
    }
}

impl<'input> Eq2ContextAttrs<'input> for Eq2Context<'input> {}

pub struct Eq2ContextExt<'input> {
    base: EqExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {Eq2ContextExt<'a>}

impl<'input> SysYParserContext<'input> for Eq2Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for Eq2Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_eq2(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_eq2(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for Eq2Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_eq2(self);
    }
}

impl<'input> CustomRuleContext<'input> for Eq2ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_eqExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_eqExp }
}

impl<'input> Borrow<EqExpContextExt<'input>> for Eq2Context<'input> {
    fn borrow(&self) -> &EqExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<EqExpContextExt<'input>> for Eq2Context<'input> {
    fn borrow_mut(&mut self) -> &mut EqExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> EqExpContextAttrs<'input> for Eq2Context<'input> {}

impl<'input> Eq2ContextExt<'input> {
    fn new(ctx: &dyn EqExpContextAttrs<'input>) -> Rc<EqExpContextAll<'input>> {
        Rc::new(EqExpContextAll::Eq2Context(
            BaseParserRuleContext::copy_from(
                ctx,
                Eq2ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn eqExp(&mut self) -> Result<Rc<EqExpContextAll<'input>>, ANTLRError> {
        self.eqExp_rec(0)
    }

    fn eqExp_rec(&mut self, _p: isize) -> Result<Rc<EqExpContextAll<'input>>, ANTLRError> {
        let recog = self;
        let _parentctx = recog.ctx.take();
        let _parentState = recog.base.get_state();
        let mut _localctx = EqExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_recursion_rule(_localctx.clone(), 56, RULE_eqExp, _p);
        let mut _localctx: Rc<EqExpContextAll> = _localctx;
        let mut _prevctx = _localctx.clone();
        let _startState = 56;
        let mut _la: isize = -1;
        let result: Result<(), ANTLRError> = (|| {
            let mut _alt: isize;
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                {
                    let mut tmp = Eq1ContextExt::new(&**_localctx);
                    recog.ctx = Some(tmp.clone());
                    _localctx = tmp;
                    _prevctx = _localctx.clone();

                    /*InvokeRule relExp*/
                    recog.base.set_state(341);
                    recog.relExp_rec(0)?;
                }

                let tmp = recog.input.lt(-1).cloned();
                recog.ctx.as_ref().unwrap().set_stop(tmp);
                recog.base.set_state(348);
                recog.err_handler.sync(&mut recog.base)?;
                _alt = recog.interpreter.adaptive_predict(33, &mut recog.base)?;
                while { _alt != 2 && _alt != INVALID_ALT } {
                    if _alt == 1 {
                        recog.trigger_exit_rule_event();
                        _prevctx = _localctx.clone();
                        {
                            {
                                /*recRuleLabeledAltStartAction*/
                                let mut tmp = Eq2ContextExt::new(&**EqExpContextExt::new(
                                    _parentctx.clone(),
                                    _parentState,
                                ));
                                recog.push_new_recursion_context(
                                    tmp.clone(),
                                    _startState,
                                    RULE_eqExp,
                                );
                                _localctx = tmp;
                                recog.base.set_state(343);
                                if !({ recog.precpred(None, 1) }) {
                                    Err(FailedPredicateError::new(
                                        &mut recog.base,
                                        Some("recog.precpred(None, 1)".to_owned()),
                                        None,
                                    ))?;
                                }
                                recog.base.set_state(344);
                                _la = recog.base.input.la(1);
                                if { !(_la == EQ || _la == NEQ) } {
                                    recog.err_handler.recover_inline(&mut recog.base)?;
                                } else {
                                    if recog.base.input.la(1) == TOKEN_EOF {
                                        recog.base.matched_eof = true
                                    };
                                    recog.err_handler.report_match(&mut recog.base);
                                    recog.base.consume(&mut recog.err_handler);
                                }
                                /*InvokeRule relExp*/
                                recog.base.set_state(345);
                                recog.relExp_rec(0)?;
                            }
                        }
                    }
                    recog.base.set_state(350);
                    recog.err_handler.sync(&mut recog.base)?;
                    _alt = recog.interpreter.adaptive_predict(33, &mut recog.base)?;
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.unroll_recursion_context(_parentctx);

        Ok(_localctx)
    }
}
//------------------- lAndExp ----------------
#[derive(Debug)]
pub enum LAndExpContextAll<'input> {
    LAnd2Context(LAnd2Context<'input>),
    LAnd1Context(LAnd1Context<'input>),
    Error(LAndExpContext<'input>),
}
antlr_rust::tid! {LAndExpContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for LAndExpContextAll<'input> {}

impl<'input> SysYParserContext<'input> for LAndExpContextAll<'input> {}

impl<'input> Deref for LAndExpContextAll<'input> {
    type Target = dyn LAndExpContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use LAndExpContextAll::*;
        match self {
            LAnd2Context(inner) => inner,
            LAnd1Context(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for LAndExpContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for LAndExpContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type LAndExpContext<'input> = BaseParserRuleContext<'input, LAndExpContextExt<'input>>;

#[derive(Clone)]
pub struct LAndExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for LAndExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for LAndExpContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for LAndExpContext<'input> {}

impl<'input> CustomRuleContext<'input> for LAndExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_lAndExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_lAndExp }
}
antlr_rust::tid! {LAndExpContextExt<'a>}

impl<'input> LAndExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<LAndExpContextAll<'input>> {
        Rc::new(LAndExpContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                LAndExpContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait LAndExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<LAndExpContextExt<'input>>
{
}

impl<'input> LAndExpContextAttrs<'input> for LAndExpContext<'input> {}

pub type LAnd2Context<'input> = BaseParserRuleContext<'input, LAnd2ContextExt<'input>>;

pub trait LAnd2ContextAttrs<'input>: SysYParserContext<'input> {
    fn lAndExp(&self) -> Option<Rc<LAndExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token LAND
    /// Returns `None` if there is no child corresponding to token LAND
    fn LAND(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(LAND, 0)
    }
    fn eqExp(&self) -> Option<Rc<EqExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> LAnd2ContextAttrs<'input> for LAnd2Context<'input> {}

pub struct LAnd2ContextExt<'input> {
    base: LAndExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {LAnd2ContextExt<'a>}

impl<'input> SysYParserContext<'input> for LAnd2Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for LAnd2Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_lAnd2(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_lAnd2(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for LAnd2Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_lAnd2(self);
    }
}

impl<'input> CustomRuleContext<'input> for LAnd2ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_lAndExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_lAndExp }
}

impl<'input> Borrow<LAndExpContextExt<'input>> for LAnd2Context<'input> {
    fn borrow(&self) -> &LAndExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<LAndExpContextExt<'input>> for LAnd2Context<'input> {
    fn borrow_mut(&mut self) -> &mut LAndExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> LAndExpContextAttrs<'input> for LAnd2Context<'input> {}

impl<'input> LAnd2ContextExt<'input> {
    fn new(ctx: &dyn LAndExpContextAttrs<'input>) -> Rc<LAndExpContextAll<'input>> {
        Rc::new(LAndExpContextAll::LAnd2Context(
            BaseParserRuleContext::copy_from(
                ctx,
                LAnd2ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type LAnd1Context<'input> = BaseParserRuleContext<'input, LAnd1ContextExt<'input>>;

pub trait LAnd1ContextAttrs<'input>: SysYParserContext<'input> {
    fn eqExp(&self) -> Option<Rc<EqExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> LAnd1ContextAttrs<'input> for LAnd1Context<'input> {}

pub struct LAnd1ContextExt<'input> {
    base: LAndExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {LAnd1ContextExt<'a>}

impl<'input> SysYParserContext<'input> for LAnd1Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for LAnd1Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_lAnd1(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_lAnd1(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for LAnd1Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_lAnd1(self);
    }
}

impl<'input> CustomRuleContext<'input> for LAnd1ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_lAndExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_lAndExp }
}

impl<'input> Borrow<LAndExpContextExt<'input>> for LAnd1Context<'input> {
    fn borrow(&self) -> &LAndExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<LAndExpContextExt<'input>> for LAnd1Context<'input> {
    fn borrow_mut(&mut self) -> &mut LAndExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> LAndExpContextAttrs<'input> for LAnd1Context<'input> {}

impl<'input> LAnd1ContextExt<'input> {
    fn new(ctx: &dyn LAndExpContextAttrs<'input>) -> Rc<LAndExpContextAll<'input>> {
        Rc::new(LAndExpContextAll::LAnd1Context(
            BaseParserRuleContext::copy_from(
                ctx,
                LAnd1ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn lAndExp(&mut self) -> Result<Rc<LAndExpContextAll<'input>>, ANTLRError> {
        self.lAndExp_rec(0)
    }

    fn lAndExp_rec(&mut self, _p: isize) -> Result<Rc<LAndExpContextAll<'input>>, ANTLRError> {
        let recog = self;
        let _parentctx = recog.ctx.take();
        let _parentState = recog.base.get_state();
        let mut _localctx = LAndExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_recursion_rule(_localctx.clone(), 58, RULE_lAndExp, _p);
        let mut _localctx: Rc<LAndExpContextAll> = _localctx;
        let mut _prevctx = _localctx.clone();
        let _startState = 58;
        let result: Result<(), ANTLRError> = (|| {
            let mut _alt: isize;
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                {
                    let mut tmp = LAnd1ContextExt::new(&**_localctx);
                    recog.ctx = Some(tmp.clone());
                    _localctx = tmp;
                    _prevctx = _localctx.clone();

                    /*InvokeRule eqExp*/
                    recog.base.set_state(352);
                    recog.eqExp_rec(0)?;
                }

                let tmp = recog.input.lt(-1).cloned();
                recog.ctx.as_ref().unwrap().set_stop(tmp);
                recog.base.set_state(359);
                recog.err_handler.sync(&mut recog.base)?;
                _alt = recog.interpreter.adaptive_predict(34, &mut recog.base)?;
                while { _alt != 2 && _alt != INVALID_ALT } {
                    if _alt == 1 {
                        recog.trigger_exit_rule_event();
                        _prevctx = _localctx.clone();
                        {
                            {
                                /*recRuleLabeledAltStartAction*/
                                let mut tmp = LAnd2ContextExt::new(&**LAndExpContextExt::new(
                                    _parentctx.clone(),
                                    _parentState,
                                ));
                                recog.push_new_recursion_context(
                                    tmp.clone(),
                                    _startState,
                                    RULE_lAndExp,
                                );
                                _localctx = tmp;
                                recog.base.set_state(354);
                                if !({ recog.precpred(None, 1) }) {
                                    Err(FailedPredicateError::new(
                                        &mut recog.base,
                                        Some("recog.precpred(None, 1)".to_owned()),
                                        None,
                                    ))?;
                                }
                                recog.base.set_state(355);
                                recog.base.match_token(LAND, &mut recog.err_handler)?;

                                /*InvokeRule eqExp*/
                                recog.base.set_state(356);
                                recog.eqExp_rec(0)?;
                            }
                        }
                    }
                    recog.base.set_state(361);
                    recog.err_handler.sync(&mut recog.base)?;
                    _alt = recog.interpreter.adaptive_predict(34, &mut recog.base)?;
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.unroll_recursion_context(_parentctx);

        Ok(_localctx)
    }
}
//------------------- lOrExp ----------------
#[derive(Debug)]
pub enum LOrExpContextAll<'input> {
    LOr1Context(LOr1Context<'input>),
    LOr2Context(LOr2Context<'input>),
    Error(LOrExpContext<'input>),
}
antlr_rust::tid! {LOrExpContextAll<'a>}

impl<'input> antlr_rust::parser_rule_context::DerefSeal for LOrExpContextAll<'input> {}

impl<'input> SysYParserContext<'input> for LOrExpContextAll<'input> {}

impl<'input> Deref for LOrExpContextAll<'input> {
    type Target = dyn LOrExpContextAttrs<'input> + 'input;
    fn deref(&self) -> &Self::Target {
        use LOrExpContextAll::*;
        match self {
            LOr1Context(inner) => inner,
            LOr2Context(inner) => inner,
            Error(inner) => inner,
        }
    }
}
impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for LOrExpContextAll<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        self.deref().accept(visitor)
    }
}
impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for LOrExpContextAll<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().enter(listener)
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        self.deref().exit(listener)
    }
}

pub type LOrExpContext<'input> = BaseParserRuleContext<'input, LOrExpContextExt<'input>>;

#[derive(Clone)]
pub struct LOrExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for LOrExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for LOrExpContext<'input> {}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for LOrExpContext<'input> {}

impl<'input> CustomRuleContext<'input> for LOrExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_lOrExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_lOrExp }
}
antlr_rust::tid! {LOrExpContextExt<'a>}

impl<'input> LOrExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<LOrExpContextAll<'input>> {
        Rc::new(LOrExpContextAll::Error(
            BaseParserRuleContext::new_parser_ctx(
                parent,
                invoking_state,
                LOrExpContextExt { ph: PhantomData },
            ),
        ))
    }
}

pub trait LOrExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<LOrExpContextExt<'input>>
{
}

impl<'input> LOrExpContextAttrs<'input> for LOrExpContext<'input> {}

pub type LOr1Context<'input> = BaseParserRuleContext<'input, LOr1ContextExt<'input>>;

pub trait LOr1ContextAttrs<'input>: SysYParserContext<'input> {
    fn lAndExp(&self) -> Option<Rc<LAndExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> LOr1ContextAttrs<'input> for LOr1Context<'input> {}

pub struct LOr1ContextExt<'input> {
    base: LOrExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {LOr1ContextExt<'a>}

impl<'input> SysYParserContext<'input> for LOr1Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for LOr1Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_lOr1(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_lOr1(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for LOr1Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_lOr1(self);
    }
}

impl<'input> CustomRuleContext<'input> for LOr1ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_lOrExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_lOrExp }
}

impl<'input> Borrow<LOrExpContextExt<'input>> for LOr1Context<'input> {
    fn borrow(&self) -> &LOrExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<LOrExpContextExt<'input>> for LOr1Context<'input> {
    fn borrow_mut(&mut self) -> &mut LOrExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> LOrExpContextAttrs<'input> for LOr1Context<'input> {}

impl<'input> LOr1ContextExt<'input> {
    fn new(ctx: &dyn LOrExpContextAttrs<'input>) -> Rc<LOrExpContextAll<'input>> {
        Rc::new(LOrExpContextAll::LOr1Context(
            BaseParserRuleContext::copy_from(
                ctx,
                LOr1ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

pub type LOr2Context<'input> = BaseParserRuleContext<'input, LOr2ContextExt<'input>>;

pub trait LOr2ContextAttrs<'input>: SysYParserContext<'input> {
    fn lOrExp(&self) -> Option<Rc<LOrExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
    /// Retrieves first TerminalNode corresponding to token LOR
    /// Returns `None` if there is no child corresponding to token LOR
    fn LOR(&self) -> Option<Rc<TerminalNode<'input, SysYParserContextType>>>
    where
        Self: Sized,
    {
        self.get_token(LOR, 0)
    }
    fn lAndExp(&self) -> Option<Rc<LAndExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> LOr2ContextAttrs<'input> for LOr2Context<'input> {}

pub struct LOr2ContextExt<'input> {
    base: LOrExpContextExt<'input>,
    ph: PhantomData<&'input str>,
}

antlr_rust::tid! {LOr2ContextExt<'a>}

impl<'input> SysYParserContext<'input> for LOr2Context<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for LOr2Context<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_lOr2(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_lOr2(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for LOr2Context<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_lOr2(self);
    }
}

impl<'input> CustomRuleContext<'input> for LOr2ContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_lOrExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_lOrExp }
}

impl<'input> Borrow<LOrExpContextExt<'input>> for LOr2Context<'input> {
    fn borrow(&self) -> &LOrExpContextExt<'input> {
        &self.base
    }
}
impl<'input> BorrowMut<LOrExpContextExt<'input>> for LOr2Context<'input> {
    fn borrow_mut(&mut self) -> &mut LOrExpContextExt<'input> {
        &mut self.base
    }
}

impl<'input> LOrExpContextAttrs<'input> for LOr2Context<'input> {}

impl<'input> LOr2ContextExt<'input> {
    fn new(ctx: &dyn LOrExpContextAttrs<'input>) -> Rc<LOrExpContextAll<'input>> {
        Rc::new(LOrExpContextAll::LOr2Context(
            BaseParserRuleContext::copy_from(
                ctx,
                LOr2ContextExt {
                    base: ctx.borrow().clone(),
                    ph: PhantomData,
                },
            ),
        ))
    }
}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn lOrExp(&mut self) -> Result<Rc<LOrExpContextAll<'input>>, ANTLRError> {
        self.lOrExp_rec(0)
    }

    fn lOrExp_rec(&mut self, _p: isize) -> Result<Rc<LOrExpContextAll<'input>>, ANTLRError> {
        let recog = self;
        let _parentctx = recog.ctx.take();
        let _parentState = recog.base.get_state();
        let mut _localctx = LOrExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog
            .base
            .enter_recursion_rule(_localctx.clone(), 60, RULE_lOrExp, _p);
        let mut _localctx: Rc<LOrExpContextAll> = _localctx;
        let mut _prevctx = _localctx.clone();
        let _startState = 60;
        let result: Result<(), ANTLRError> = (|| {
            let mut _alt: isize;
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                {
                    let mut tmp = LOr1ContextExt::new(&**_localctx);
                    recog.ctx = Some(tmp.clone());
                    _localctx = tmp;
                    _prevctx = _localctx.clone();

                    /*InvokeRule lAndExp*/
                    recog.base.set_state(363);
                    recog.lAndExp_rec(0)?;
                }

                let tmp = recog.input.lt(-1).cloned();
                recog.ctx.as_ref().unwrap().set_stop(tmp);
                recog.base.set_state(370);
                recog.err_handler.sync(&mut recog.base)?;
                _alt = recog.interpreter.adaptive_predict(35, &mut recog.base)?;
                while { _alt != 2 && _alt != INVALID_ALT } {
                    if _alt == 1 {
                        recog.trigger_exit_rule_event();
                        _prevctx = _localctx.clone();
                        {
                            {
                                /*recRuleLabeledAltStartAction*/
                                let mut tmp = LOr2ContextExt::new(&**LOrExpContextExt::new(
                                    _parentctx.clone(),
                                    _parentState,
                                ));
                                recog.push_new_recursion_context(
                                    tmp.clone(),
                                    _startState,
                                    RULE_lOrExp,
                                );
                                _localctx = tmp;
                                recog.base.set_state(365);
                                if !({ recog.precpred(None, 1) }) {
                                    Err(FailedPredicateError::new(
                                        &mut recog.base,
                                        Some("recog.precpred(None, 1)".to_owned()),
                                        None,
                                    ))?;
                                }
                                recog.base.set_state(366);
                                recog.base.match_token(LOR, &mut recog.err_handler)?;

                                /*InvokeRule lAndExp*/
                                recog.base.set_state(367);
                                recog.lAndExp_rec(0)?;
                            }
                        }
                    }
                    recog.base.set_state(372);
                    recog.err_handler.sync(&mut recog.base)?;
                    _alt = recog.interpreter.adaptive_predict(35, &mut recog.base)?;
                }
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.unroll_recursion_context(_parentctx);

        Ok(_localctx)
    }
}
//------------------- constExp ----------------
pub type ConstExpContextAll<'input> = ConstExpContext<'input>;

pub type ConstExpContext<'input> = BaseParserRuleContext<'input, ConstExpContextExt<'input>>;

#[derive(Clone)]
pub struct ConstExpContextExt<'input> {
    ph: PhantomData<&'input str>,
}

impl<'input> SysYParserContext<'input> for ConstExpContext<'input> {}

impl<'input, 'a> Listenable<dyn SysYListener<'input> + 'a> for ConstExpContext<'input> {
    fn enter(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.enter_every_rule(self);
        listener.enter_constExp(self);
    }
    fn exit(&self, listener: &mut (dyn SysYListener<'input> + 'a)) {
        listener.exit_constExp(self);
        listener.exit_every_rule(self);
    }
}

impl<'input, 'a> Visitable<dyn SysYVisitor<'input> + 'a> for ConstExpContext<'input> {
    fn accept(&self, visitor: &mut (dyn SysYVisitor<'input> + 'a)) {
        visitor.visit_constExp(self);
    }
}

impl<'input> CustomRuleContext<'input> for ConstExpContextExt<'input> {
    type TF = LocalTokenFactory<'input>;
    type Ctx = SysYParserContextType;
    fn get_rule_index(&self) -> usize {
        RULE_constExp
    }
    //fn type_rule_index() -> usize where Self: Sized { RULE_constExp }
}
antlr_rust::tid! {ConstExpContextExt<'a>}

impl<'input> ConstExpContextExt<'input> {
    fn new(
        parent: Option<Rc<dyn SysYParserContext<'input> + 'input>>,
        invoking_state: isize,
    ) -> Rc<ConstExpContextAll<'input>> {
        Rc::new(BaseParserRuleContext::new_parser_ctx(
            parent,
            invoking_state,
            ConstExpContextExt { ph: PhantomData },
        ))
    }
}

pub trait ConstExpContextAttrs<'input>:
    SysYParserContext<'input> + BorrowMut<ConstExpContextExt<'input>>
{
    fn addExp(&self) -> Option<Rc<AddExpContextAll<'input>>>
    where
        Self: Sized,
    {
        self.child_of_type(0)
    }
}

impl<'input> ConstExpContextAttrs<'input> for ConstExpContext<'input> {}

impl<'input, I, H> SysYParser<'input, I, H>
where
    I: TokenStream<'input, TF = LocalTokenFactory<'input>> + TidAble<'input>,
    H: ErrorStrategy<'input, BaseParserType<'input, I>>,
{
    pub fn constExp(&mut self) -> Result<Rc<ConstExpContextAll<'input>>, ANTLRError> {
        let mut recog = self;
        let _parentctx = recog.ctx.take();
        let mut _localctx = ConstExpContextExt::new(_parentctx.clone(), recog.base.get_state());
        recog.base.enter_rule(_localctx.clone(), 62, RULE_constExp);
        let mut _localctx: Rc<ConstExpContextAll> = _localctx;
        let result: Result<(), ANTLRError> = (|| {
            //recog.base.enter_outer_alt(_localctx.clone(), 1);
            recog.base.enter_outer_alt(None, 1);
            {
                /*InvokeRule addExp*/
                recog.base.set_state(373);
                recog.addExp_rec(0)?;
            }
            Ok(())
        })();
        match result {
            Ok(_) => {}
            Err(e @ ANTLRError::FallThrough(_)) => return Err(e),
            Err(ref re) => {
                //_localctx.exception = re;
                recog.err_handler.report_error(&mut recog.base, re);
                recog.err_handler.recover(&mut recog.base, re)?;
            }
        }
        recog.base.exit_rule();

        Ok(_localctx)
    }
}

lazy_static! {
    static ref _ATN: Arc<ATN> =
        Arc::new(ATNDeserializer::new(None).deserialize(_serializedATN.chars()));
    static ref _decision_to_DFA: Arc<Vec<antlr_rust::RwLock<DFA>>> = {
        let mut dfa = Vec::new();
        let size = _ATN.decision_to_state.len();
        for i in 0..size {
            dfa.push(DFA::new(_ATN.clone(), _ATN.get_decision_state(i), i as isize).into())
        }
        Arc::new(dfa)
    };
}

const _serializedATN: &'static str =
    "\x03\u{608b}\u{a72a}\u{8133}\u{b9ed}\u{417c}\u{3be7}\u{7786}\u{5964}\x03\
	\x2f\u{17a}\x04\x02\x09\x02\x04\x03\x09\x03\x04\x04\x09\x04\x04\x05\x09\
	\x05\x04\x06\x09\x06\x04\x07\x09\x07\x04\x08\x09\x08\x04\x09\x09\x09\x04\
	\x0a\x09\x0a\x04\x0b\x09\x0b\x04\x0c\x09\x0c\x04\x0d\x09\x0d\x04\x0e\x09\
	\x0e\x04\x0f\x09\x0f\x04\x10\x09\x10\x04\x11\x09\x11\x04\x12\x09\x12\x04\
	\x13\x09\x13\x04\x14\x09\x14\x04\x15\x09\x15\x04\x16\x09\x16\x04\x17\x09\
	\x17\x04\x18\x09\x18\x04\x19\x09\x19\x04\x1a\x09\x1a\x04\x1b\x09\x1b\x04\
	\x1c\x09\x1c\x04\x1d\x09\x1d\x04\x1e\x09\x1e\x04\x1f\x09\x1f\x04\x20\x09\
	\x20\x04\x21\x09\x21\x03\x02\x03\x02\x07\x02\x45\x0a\x02\x0c\x02\x0e\x02\
	\x48\x0b\x02\x03\x02\x03\x02\x03\x03\x03\x03\x05\x03\x4e\x0a\x03\x03\x04\
	\x03\x04\x03\x04\x03\x04\x03\x04\x07\x04\x55\x0a\x04\x0c\x04\x0e\x04\x58\
	\x0b\x04\x03\x04\x03\x04\x03\x05\x03\x05\x03\x06\x03\x06\x03\x06\x03\x06\
	\x03\x06\x07\x06\x63\x0a\x06\x0c\x06\x0e\x06\x66\x0b\x06\x03\x06\x03\x06\
	\x03\x06\x03\x07\x03\x07\x03\x07\x03\x07\x03\x07\x07\x07\x70\x0a\x07\x0c\
	\x07\x0e\x07\x73\x0b\x07\x05\x07\x75\x0a\x07\x03\x07\x05\x07\x78\x0a\x07\
	\x03\x08\x03\x08\x03\x08\x03\x08\x07\x08\x7e\x0a\x08\x0c\x08\x0e\x08\u{81}\
	\x0b\x08\x03\x08\x03\x08\x03\x09\x03\x09\x03\x09\x03\x09\x03\x09\x07\x09\
	\u{8a}\x0a\x09\x0c\x09\x0e\x09\u{8d}\x0b\x09\x03\x09\x03\x09\x03\x09\x03\
	\x09\x03\x09\x07\x09\u{94}\x0a\x09\x0c\x09\x0e\x09\u{97}\x0b\x09\x03\x09\
	\x03\x09\x05\x09\u{9b}\x0a\x09\x03\x0a\x03\x0a\x03\x0a\x03\x0a\x03\x0a\x07\
	\x0a\u{a2}\x0a\x0a\x0c\x0a\x0e\x0a\u{a5}\x0b\x0a\x05\x0a\u{a7}\x0a\x0a\x03\
	\x0a\x05\x0a\u{aa}\x0a\x0a\x03\x0b\x03\x0b\x03\x0b\x03\x0b\x05\x0b\u{b0}\
	\x0a\x0b\x03\x0b\x03\x0b\x03\x0b\x03\x0c\x03\x0c\x03\x0d\x03\x0d\x03\x0d\
	\x07\x0d\u{ba}\x0a\x0d\x0c\x0d\x0e\x0d\u{bd}\x0b\x0d\x03\x0e\x03\x0e\x03\
	\x0e\x03\x0e\x03\x0e\x03\x0e\x03\x0e\x03\x0e\x07\x0e\u{c7}\x0a\x0e\x0c\x0e\
	\x0e\x0e\u{ca}\x0b\x0e\x05\x0e\u{cc}\x0a\x0e\x03\x0f\x03\x0f\x07\x0f\u{d0}\
	\x0a\x0f\x0c\x0f\x0e\x0f\u{d3}\x0b\x0f\x03\x0f\x03\x0f\x03\x10\x03\x10\x05\
	\x10\u{d9}\x0a\x10\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x05\x11\
	\u{e1}\x0a\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\
	\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\
	\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\x11\x03\
	\x11\x03\x11\x03\x11\x05\x11\u{ff}\x0a\x11\x03\x11\x05\x11\u{102}\x0a\x11\
	\x03\x12\x03\x12\x03\x13\x03\x13\x03\x14\x03\x14\x03\x14\x03\x14\x03\x14\
	\x07\x14\u{10d}\x0a\x14\x0c\x14\x0e\x14\u{110}\x0b\x14\x03\x15\x03\x15\x03\
	\x15\x03\x15\x03\x15\x03\x15\x05\x15\u{118}\x0a\x15\x03\x16\x03\x16\x05\
	\x16\u{11c}\x0a\x16\x03\x17\x03\x17\x03\x17\x03\x17\x05\x17\u{122}\x0a\x17\
	\x03\x17\x03\x17\x03\x17\x03\x17\x05\x17\u{128}\x0a\x17\x03\x18\x03\x18\
	\x03\x19\x03\x19\x03\x19\x07\x19\u{12f}\x0a\x19\x0c\x19\x0e\x19\u{132}\x0b\
	\x19\x03\x1a\x03\x1a\x03\x1b\x03\x1b\x03\x1b\x03\x1b\x03\x1b\x03\x1b\x07\
	\x1b\u{13c}\x0a\x1b\x0c\x1b\x0e\x1b\u{13f}\x0b\x1b\x03\x1c\x03\x1c\x03\x1c\
	\x03\x1c\x03\x1c\x03\x1c\x07\x1c\u{147}\x0a\x1c\x0c\x1c\x0e\x1c\u{14a}\x0b\
	\x1c\x03\x1d\x03\x1d\x03\x1d\x03\x1d\x03\x1d\x03\x1d\x07\x1d\u{152}\x0a\
	\x1d\x0c\x1d\x0e\x1d\u{155}\x0b\x1d\x03\x1e\x03\x1e\x03\x1e\x03\x1e\x03\
	\x1e\x03\x1e\x07\x1e\u{15d}\x0a\x1e\x0c\x1e\x0e\x1e\u{160}\x0b\x1e\x03\x1f\
	\x03\x1f\x03\x1f\x03\x1f\x03\x1f\x03\x1f\x07\x1f\u{168}\x0a\x1f\x0c\x1f\
	\x0e\x1f\u{16b}\x0b\x1f\x03\x20\x03\x20\x03\x20\x03\x20\x03\x20\x03\x20\
	\x07\x20\u{173}\x0a\x20\x0c\x20\x0e\x20\u{176}\x0b\x20\x03\x21\x03\x21\x03\
	\x21\x02\x08\x34\x36\x38\x3a\x3c\x3e\x22\x02\x04\x06\x08\x0a\x0c\x0e\x10\
	\x12\x14\x16\x18\x1a\x1c\x1e\x20\x22\x24\x26\x28\x2a\x2c\x2e\x30\x32\x34\
	\x36\x38\x3a\x3c\x3e\x40\x02\x09\x03\x02\x04\x05\x03\x02\x04\x06\x04\x02\
	\x1a\x1b\x1d\x1d\x03\x02\x1e\x20\x04\x02\x1a\x1a\x1d\x1d\x03\x02\x25\x28\
	\x03\x02\x23\x24\x02\u{186}\x02\x46\x03\x02\x02\x02\x04\x4d\x03\x02\x02\
	\x02\x06\x4f\x03\x02\x02\x02\x08\x5b\x03\x02\x02\x02\x0a\x5d\x03\x02\x02\
	\x02\x0c\x77\x03\x02\x02\x02\x0e\x79\x03\x02\x02\x02\x10\u{9a}\x03\x02\x02\
	\x02\x12\u{a9}\x03\x02\x02\x02\x14\u{ab}\x03\x02\x02\x02\x16\u{b4}\x03\x02\
	\x02\x02\x18\u{b6}\x03\x02\x02\x02\x1a\u{be}\x03\x02\x02\x02\x1c\u{cd}\x03\
	\x02\x02\x02\x1e\u{d8}\x03\x02\x02\x02\x20\u{101}\x03\x02\x02\x02\x22\u{103}\
	\x03\x02\x02\x02\x24\u{105}\x03\x02\x02\x02\x26\u{107}\x03\x02\x02\x02\x28\
	\u{117}\x03\x02\x02\x02\x2a\u{11b}\x03\x02\x02\x02\x2c\u{127}\x03\x02\x02\
	\x02\x2e\u{129}\x03\x02\x02\x02\x30\u{12b}\x03\x02\x02\x02\x32\u{133}\x03\
	\x02\x02\x02\x34\u{135}\x03\x02\x02\x02\x36\u{140}\x03\x02\x02\x02\x38\u{14b}\
	\x03\x02\x02\x02\x3a\u{156}\x03\x02\x02\x02\x3c\u{161}\x03\x02\x02\x02\x3e\
	\u{16c}\x03\x02\x02\x02\x40\u{177}\x03\x02\x02\x02\x42\x45\x05\x04\x03\x02\
	\x43\x45\x05\x14\x0b\x02\x44\x42\x03\x02\x02\x02\x44\x43\x03\x02\x02\x02\
	\x45\x48\x03\x02\x02\x02\x46\x44\x03\x02\x02\x02\x46\x47\x03\x02\x02\x02\
	\x47\x49\x03\x02\x02\x02\x48\x46\x03\x02\x02\x02\x49\x4a\x07\x02\x02\x03\
	\x4a\x03\x03\x02\x02\x02\x4b\x4e\x05\x06\x04\x02\x4c\x4e\x05\x0e\x08\x02\
	\x4d\x4b\x03\x02\x02\x02\x4d\x4c\x03\x02\x02\x02\x4e\x05\x03\x02\x02\x02\
	\x4f\x50\x07\x07\x02\x02\x50\x51\x05\x08\x05\x02\x51\x56\x05\x0a\x06\x02\
	\x52\x53\x07\x16\x02\x02\x53\x55\x05\x0a\x06\x02\x54\x52\x03\x02\x02\x02\
	\x55\x58\x03\x02\x02\x02\x56\x54\x03\x02\x02\x02\x56\x57\x03\x02\x02\x02\
	\x57\x59\x03\x02\x02\x02\x58\x56\x03\x02\x02\x02\x59\x5a\x07\x17\x02\x02\
	\x5a\x07\x03\x02\x02\x02\x5b\x5c\x09\x02\x02\x02\x5c\x09\x03\x02\x02\x02\
	\x5d\x64\x07\x2b\x02\x02\x5e\x5f\x07\x12\x02\x02\x5f\x60\x05\x40\x21\x02\
	\x60\x61\x07\x13\x02\x02\x61\x63\x03\x02\x02\x02\x62\x5e\x03\x02\x02\x02\
	\x63\x66\x03\x02\x02\x02\x64\x62\x03\x02\x02\x02\x64\x65\x03\x02\x02\x02\
	\x65\x67\x03\x02\x02\x02\x66\x64\x03\x02\x02\x02\x67\x68\x07\x03\x02\x02\
	\x68\x69\x05\x0c\x07\x02\x69\x0b\x03\x02\x02\x02\x6a\x78\x05\x40\x21\x02\
	\x6b\x74\x07\x14\x02\x02\x6c\x71\x05\x0c\x07\x02\x6d\x6e\x07\x16\x02\x02\
	\x6e\x70\x05\x0c\x07\x02\x6f\x6d\x03\x02\x02\x02\x70\x73\x03\x02\x02\x02\
	\x71\x6f\x03\x02\x02\x02\x71\x72\x03\x02\x02\x02\x72\x75\x03\x02\x02\x02\
	\x73\x71\x03\x02\x02\x02\x74\x6c\x03\x02\x02\x02\x74\x75\x03\x02\x02\x02\
	\x75\x76\x03\x02\x02\x02\x76\x78\x07\x15\x02\x02\x77\x6a\x03\x02\x02\x02\
	\x77\x6b\x03\x02\x02\x02\x78\x0d\x03\x02\x02\x02\x79\x7a\x05\x08\x05\x02\
	\x7a\x7f\x05\x10\x09\x02\x7b\x7c\x07\x16\x02\x02\x7c\x7e\x05\x10\x09\x02\
	\x7d\x7b\x03\x02\x02\x02\x7e\u{81}\x03\x02\x02\x02\x7f\x7d\x03\x02\x02\x02\
	\x7f\u{80}\x03\x02\x02\x02\u{80}\u{82}\x03\x02\x02\x02\u{81}\x7f\x03\x02\
	\x02\x02\u{82}\u{83}\x07\x17\x02\x02\u{83}\x0f\x03\x02\x02\x02\u{84}\u{8b}\
	\x07\x2b\x02\x02\u{85}\u{86}\x07\x12\x02\x02\u{86}\u{87}\x05\x40\x21\x02\
	\u{87}\u{88}\x07\x13\x02\x02\u{88}\u{8a}\x03\x02\x02\x02\u{89}\u{85}\x03\
	\x02\x02\x02\u{8a}\u{8d}\x03\x02\x02\x02\u{8b}\u{89}\x03\x02\x02\x02\u{8b}\
	\u{8c}\x03\x02\x02\x02\u{8c}\u{9b}\x03\x02\x02\x02\u{8d}\u{8b}\x03\x02\x02\
	\x02\u{8e}\u{95}\x07\x2b\x02\x02\u{8f}\u{90}\x07\x12\x02\x02\u{90}\u{91}\
	\x05\x40\x21\x02\u{91}\u{92}\x07\x13\x02\x02\u{92}\u{94}\x03\x02\x02\x02\
	\u{93}\u{8f}\x03\x02\x02\x02\u{94}\u{97}\x03\x02\x02\x02\u{95}\u{93}\x03\
	\x02\x02\x02\u{95}\u{96}\x03\x02\x02\x02\u{96}\u{98}\x03\x02\x02\x02\u{97}\
	\u{95}\x03\x02\x02\x02\u{98}\u{99}\x07\x03\x02\x02\u{99}\u{9b}\x05\x12\x0a\
	\x02\u{9a}\u{84}\x03\x02\x02\x02\u{9a}\u{8e}\x03\x02\x02\x02\u{9b}\x11\x03\
	\x02\x02\x02\u{9c}\u{aa}\x05\x22\x12\x02\u{9d}\u{a6}\x07\x14\x02\x02\u{9e}\
	\u{a3}\x05\x12\x0a\x02\u{9f}\u{a0}\x07\x16\x02\x02\u{a0}\u{a2}\x05\x12\x0a\
	\x02\u{a1}\u{9f}\x03\x02\x02\x02\u{a2}\u{a5}\x03\x02\x02\x02\u{a3}\u{a1}\
	\x03\x02\x02\x02\u{a3}\u{a4}\x03\x02\x02\x02\u{a4}\u{a7}\x03\x02\x02\x02\
	\u{a5}\u{a3}\x03\x02\x02\x02\u{a6}\u{9e}\x03\x02\x02\x02\u{a6}\u{a7}\x03\
	\x02\x02\x02\u{a7}\u{a8}\x03\x02\x02\x02\u{a8}\u{aa}\x07\x15\x02\x02\u{a9}\
	\u{9c}\x03\x02\x02\x02\u{a9}\u{9d}\x03\x02\x02\x02\u{aa}\x13\x03\x02\x02\
	\x02\u{ab}\u{ac}\x05\x16\x0c\x02\u{ac}\u{ad}\x07\x2b\x02\x02\u{ad}\u{af}\
	\x07\x10\x02\x02\u{ae}\u{b0}\x05\x18\x0d\x02\u{af}\u{ae}\x03\x02\x02\x02\
	\u{af}\u{b0}\x03\x02\x02\x02\u{b0}\u{b1}\x03\x02\x02\x02\u{b1}\u{b2}\x07\
	\x11\x02\x02\u{b2}\u{b3}\x05\x1c\x0f\x02\u{b3}\x15\x03\x02\x02\x02\u{b4}\
	\u{b5}\x09\x03\x02\x02\u{b5}\x17\x03\x02\x02\x02\u{b6}\u{bb}\x05\x1a\x0e\
	\x02\u{b7}\u{b8}\x07\x16\x02\x02\u{b8}\u{ba}\x05\x1a\x0e\x02\u{b9}\u{b7}\
	\x03\x02\x02\x02\u{ba}\u{bd}\x03\x02\x02\x02\u{bb}\u{b9}\x03\x02\x02\x02\
	\u{bb}\u{bc}\x03\x02\x02\x02\u{bc}\x19\x03\x02\x02\x02\u{bd}\u{bb}\x03\x02\
	\x02\x02\u{be}\u{bf}\x05\x08\x05\x02\u{bf}\u{cb}\x07\x2b\x02\x02\u{c0}\u{c1}\
	\x07\x12\x02\x02\u{c1}\u{c8}\x07\x13\x02\x02\u{c2}\u{c3}\x07\x12\x02\x02\
	\u{c3}\u{c4}\x05\x40\x21\x02\u{c4}\u{c5}\x07\x13\x02\x02\u{c5}\u{c7}\x03\
	\x02\x02\x02\u{c6}\u{c2}\x03\x02\x02\x02\u{c7}\u{ca}\x03\x02\x02\x02\u{c8}\
	\u{c6}\x03\x02\x02\x02\u{c8}\u{c9}\x03\x02\x02\x02\u{c9}\u{cc}\x03\x02\x02\
	\x02\u{ca}\u{c8}\x03\x02\x02\x02\u{cb}\u{c0}\x03\x02\x02\x02\u{cb}\u{cc}\
	\x03\x02\x02\x02\u{cc}\x1b\x03\x02\x02\x02\u{cd}\u{d1}\x07\x14\x02\x02\u{ce}\
	\u{d0}\x05\x1e\x10\x02\u{cf}\u{ce}\x03\x02\x02\x02\u{d0}\u{d3}\x03\x02\x02\
	\x02\u{d1}\u{cf}\x03\x02\x02\x02\u{d1}\u{d2}\x03\x02\x02\x02\u{d2}\u{d4}\
	\x03\x02\x02\x02\u{d3}\u{d1}\x03\x02\x02\x02\u{d4}\u{d5}\x07\x15\x02\x02\
	\u{d5}\x1d\x03\x02\x02\x02\u{d6}\u{d9}\x05\x04\x03\x02\u{d7}\u{d9}\x05\x20\
	\x11\x02\u{d8}\u{d6}\x03\x02\x02\x02\u{d8}\u{d7}\x03\x02\x02\x02\u{d9}\x1f\
	\x03\x02\x02\x02\u{da}\u{db}\x05\x26\x14\x02\u{db}\u{dc}\x07\x03\x02\x02\
	\u{dc}\u{dd}\x05\x22\x12\x02\u{dd}\u{de}\x07\x17\x02\x02\u{de}\u{102}\x03\
	\x02\x02\x02\u{df}\u{e1}\x05\x22\x12\x02\u{e0}\u{df}\x03\x02\x02\x02\u{e0}\
	\u{e1}\x03\x02\x02\x02\u{e1}\u{e2}\x03\x02\x02\x02\u{e2}\u{102}\x07\x17\
	\x02\x02\u{e3}\u{102}\x05\x1c\x0f\x02\u{e4}\u{e5}\x07\x09\x02\x02\u{e5}\
	\u{e6}\x07\x10\x02\x02\u{e6}\u{e7}\x05\x24\x13\x02\u{e7}\u{e8}\x07\x11\x02\
	\x02\u{e8}\u{e9}\x05\x20\x11\x02\u{e9}\u{102}\x03\x02\x02\x02\u{ea}\u{eb}\
	\x07\x09\x02\x02\u{eb}\u{ec}\x07\x10\x02\x02\u{ec}\u{ed}\x05\x24\x13\x02\
	\u{ed}\u{ee}\x07\x11\x02\x02\u{ee}\u{ef}\x05\x20\x11\x02\u{ef}\u{f0}\x07\
	\x0a\x02\x02\u{f0}\u{f1}\x05\x20\x11\x02\u{f1}\u{102}\x03\x02\x02\x02\u{f2}\
	\u{f3}\x07\x0c\x02\x02\u{f3}\u{f4}\x07\x10\x02\x02\u{f4}\u{f5}\x05\x24\x13\
	\x02\u{f5}\u{f6}\x07\x11\x02\x02\u{f6}\u{f7}\x05\x20\x11\x02\u{f7}\u{102}\
	\x03\x02\x02\x02\u{f8}\u{f9}\x07\x0e\x02\x02\u{f9}\u{102}\x07\x17\x02\x02\
	\u{fa}\u{fb}\x07\x0f\x02\x02\u{fb}\u{102}\x07\x17\x02\x02\u{fc}\u{fe}\x07\
	\x08\x02\x02\u{fd}\u{ff}\x05\x22\x12\x02\u{fe}\u{fd}\x03\x02\x02\x02\u{fe}\
	\u{ff}\x03\x02\x02\x02\u{ff}\u{100}\x03\x02\x02\x02\u{100}\u{102}\x07\x17\
	\x02\x02\u{101}\u{da}\x03\x02\x02\x02\u{101}\u{e0}\x03\x02\x02\x02\u{101}\
	\u{e3}\x03\x02\x02\x02\u{101}\u{e4}\x03\x02\x02\x02\u{101}\u{ea}\x03\x02\
	\x02\x02\u{101}\u{f2}\x03\x02\x02\x02\u{101}\u{f8}\x03\x02\x02\x02\u{101}\
	\u{fa}\x03\x02\x02\x02\u{101}\u{fc}\x03\x02\x02\x02\u{102}\x21\x03\x02\x02\
	\x02\u{103}\u{104}\x05\x36\x1c\x02\u{104}\x23\x03\x02\x02\x02\u{105}\u{106}\
	\x05\x3e\x20\x02\u{106}\x25\x03\x02\x02\x02\u{107}\u{10e}\x07\x2b\x02\x02\
	\u{108}\u{109}\x07\x12\x02\x02\u{109}\u{10a}\x05\x22\x12\x02\u{10a}\u{10b}\
	\x07\x13\x02\x02\u{10b}\u{10d}\x03\x02\x02\x02\u{10c}\u{108}\x03\x02\x02\
	\x02\u{10d}\u{110}\x03\x02\x02\x02\u{10e}\u{10c}\x03\x02\x02\x02\u{10e}\
	\u{10f}\x03\x02\x02\x02\u{10f}\x27\x03\x02\x02\x02\u{110}\u{10e}\x03\x02\
	\x02\x02\u{111}\u{112}\x07\x10\x02\x02\u{112}\u{113}\x05\x22\x12\x02\u{113}\
	\u{114}\x07\x11\x02\x02\u{114}\u{118}\x03\x02\x02\x02\u{115}\u{118}\x05\
	\x26\x14\x02\u{116}\u{118}\x05\x2a\x16\x02\u{117}\u{111}\x03\x02\x02\x02\
	\u{117}\u{115}\x03\x02\x02\x02\u{117}\u{116}\x03\x02\x02\x02\u{118}\x29\
	\x03\x02\x02\x02\u{119}\u{11c}\x07\x29\x02\x02\u{11a}\u{11c}\x07\x2a\x02\
	\x02\u{11b}\u{119}\x03\x02\x02\x02\u{11b}\u{11a}\x03\x02\x02\x02\u{11c}\
	\x2b\x03\x02\x02\x02\u{11d}\u{128}\x05\x28\x15\x02\u{11e}\u{11f}\x07\x2b\
	\x02\x02\u{11f}\u{121}\x07\x10\x02\x02\u{120}\u{122}\x05\x30\x19\x02\u{121}\
	\u{120}\x03\x02\x02\x02\u{121}\u{122}\x03\x02\x02\x02\u{122}\u{123}\x03\
	\x02\x02\x02\u{123}\u{128}\x07\x11\x02\x02\u{124}\u{125}\x05\x2e\x18\x02\
	\u{125}\u{126}\x05\x2c\x17\x02\u{126}\u{128}\x03\x02\x02\x02\u{127}\u{11d}\
	\x03\x02\x02\x02\u{127}\u{11e}\x03\x02\x02\x02\u{127}\u{124}\x03\x02\x02\
	\x02\u{128}\x2d\x03\x02\x02\x02\u{129}\u{12a}\x09\x04\x02\x02\u{12a}\x2f\
	\x03\x02\x02\x02\u{12b}\u{130}\x05\x32\x1a\x02\u{12c}\u{12d}\x07\x16\x02\
	\x02\u{12d}\u{12f}\x05\x32\x1a\x02\u{12e}\u{12c}\x03\x02\x02\x02\u{12f}\
	\u{132}\x03\x02\x02\x02\u{130}\u{12e}\x03\x02\x02\x02\u{130}\u{131}\x03\
	\x02\x02\x02\u{131}\x31\x03\x02\x02\x02\u{132}\u{130}\x03\x02\x02\x02\u{133}\
	\u{134}\x05\x22\x12\x02\u{134}\x33\x03\x02\x02\x02\u{135}\u{136}\x08\x1b\
	\x01\x02\u{136}\u{137}\x05\x2c\x17\x02\u{137}\u{13d}\x03\x02\x02\x02\u{138}\
	\u{139}\x0c\x03\x02\x02\u{139}\u{13a}\x09\x05\x02\x02\u{13a}\u{13c}\x05\
	\x2c\x17\x02\u{13b}\u{138}\x03\x02\x02\x02\u{13c}\u{13f}\x03\x02\x02\x02\
	\u{13d}\u{13b}\x03\x02\x02\x02\u{13d}\u{13e}\x03\x02\x02\x02\u{13e}\x35\
	\x03\x02\x02\x02\u{13f}\u{13d}\x03\x02\x02\x02\u{140}\u{141}\x08\x1c\x01\
	\x02\u{141}\u{142}\x05\x34\x1b\x02\u{142}\u{148}\x03\x02\x02\x02\u{143}\
	\u{144}\x0c\x03\x02\x02\u{144}\u{145}\x09\x06\x02\x02\u{145}\u{147}\x05\
	\x34\x1b\x02\u{146}\u{143}\x03\x02\x02\x02\u{147}\u{14a}\x03\x02\x02\x02\
	\u{148}\u{146}\x03\x02\x02\x02\u{148}\u{149}\x03\x02\x02\x02\u{149}\x37\
	\x03\x02\x02\x02\u{14a}\u{148}\x03\x02\x02\x02\u{14b}\u{14c}\x08\x1d\x01\
	\x02\u{14c}\u{14d}\x05\x36\x1c\x02\u{14d}\u{153}\x03\x02\x02\x02\u{14e}\
	\u{14f}\x0c\x03\x02\x02\u{14f}\u{150}\x09\x07\x02\x02\u{150}\u{152}\x05\
	\x36\x1c\x02\u{151}\u{14e}\x03\x02\x02\x02\u{152}\u{155}\x03\x02\x02\x02\
	\u{153}\u{151}\x03\x02\x02\x02\u{153}\u{154}\x03\x02\x02\x02\u{154}\x39\
	\x03\x02\x02\x02\u{155}\u{153}\x03\x02\x02\x02\u{156}\u{157}\x08\x1e\x01\
	\x02\u{157}\u{158}\x05\x38\x1d\x02\u{158}\u{15e}\x03\x02\x02\x02\u{159}\
	\u{15a}\x0c\x03\x02\x02\u{15a}\u{15b}\x09\x08\x02\x02\u{15b}\u{15d}\x05\
	\x38\x1d\x02\u{15c}\u{159}\x03\x02\x02\x02\u{15d}\u{160}\x03\x02\x02\x02\
	\u{15e}\u{15c}\x03\x02\x02\x02\u{15e}\u{15f}\x03\x02\x02\x02\u{15f}\x3b\
	\x03\x02\x02\x02\u{160}\u{15e}\x03\x02\x02\x02\u{161}\u{162}\x08\x1f\x01\
	\x02\u{162}\u{163}\x05\x3a\x1e\x02\u{163}\u{169}\x03\x02\x02\x02\u{164}\
	\u{165}\x0c\x03\x02\x02\u{165}\u{166}\x07\x21\x02\x02\u{166}\u{168}\x05\
	\x3a\x1e\x02\u{167}\u{164}\x03\x02\x02\x02\u{168}\u{16b}\x03\x02\x02\x02\
	\u{169}\u{167}\x03\x02\x02\x02\u{169}\u{16a}\x03\x02\x02\x02\u{16a}\x3d\
	\x03\x02\x02\x02\u{16b}\u{169}\x03\x02\x02\x02\u{16c}\u{16d}\x08\x20\x01\
	\x02\u{16d}\u{16e}\x05\x3c\x1f\x02\u{16e}\u{174}\x03\x02\x02\x02\u{16f}\
	\u{170}\x0c\x03\x02\x02\u{170}\u{171}\x07\x22\x02\x02\u{171}\u{173}\x05\
	\x3c\x1f\x02\u{172}\u{16f}\x03\x02\x02\x02\u{173}\u{176}\x03\x02\x02\x02\
	\u{174}\u{172}\x03\x02\x02\x02\u{174}\u{175}\x03\x02\x02\x02\u{175}\x3f\
	\x03\x02\x02\x02\u{176}\u{174}\x03\x02\x02\x02\u{177}\u{178}\x05\x36\x1c\
	\x02\u{178}\x41\x03\x02\x02\x02\x26\x44\x46\x4d\x56\x64\x71\x74\x77\x7f\
	\u{8b}\u{95}\u{9a}\u{a3}\u{a6}\u{a9}\u{af}\u{bb}\u{c8}\u{cb}\u{d1}\u{d8}\
	\u{e0}\u{fe}\u{101}\u{10e}\u{117}\u{11b}\u{121}\u{127}\u{130}\u{13d}\u{148}\
	\u{153}\u{15e}\u{169}\u{174}";
