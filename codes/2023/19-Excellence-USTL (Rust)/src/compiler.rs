use crate::asm::{ASMOrganizer, ASMTranslate, ISABackend};
use crate::ast::check_translate::AstChecker;
use crate::ast::table::SymbolTable;
use crate::ast::AST;
use crate::echo::{print_echo, Error};
use crate::lexer::TokenReader;
use crate::lir::LIRTranslate;
use crate::mir::MIRRefPool;
use crate::optimizer;
use crate::optimizer::{HirOpt, MirOpt, OptLevel, PassInstance};
use crate::parser::TokenParser;
use crate::session::CompileSession;
use crate::sysy_std::get_sysy_std;
use std::fs::File;
use std::io::Read;
use std::path::Path;

#[cfg(debug_assertions)]
use std::io::Write;
#[cfg(debug_assertions)]
use crate::ast::filter::{ConstArrayDetect, FeatureDetector, Filter};
use crate::hir::to_mir2::MIRTranSlate2;
#[cfg(debug_assertions)]
use crate::hir::to_string::ToCodeString;
#[cfg(debug_assertions)]
use crate::mir::to_string::ToMIRString;


#[derive(Debug)]
pub enum CompileResult {
    Ok,
    AstErr,
    HirErr,
}

#[cfg(debug_assertions)]
#[derive(PartialOrd, PartialEq, Clone, Copy)]
pub enum DebugStop {
    /// 只分析AST
    Ast = 1,
    /// AST 分析结束后
    Hir = 2,
    /// HIR优化结束后，MIR优化前
    Mir = 3,
    /// MIR优化结束后，LIR生成前
    Lir = 4,
    /// LIR转译结束后，生成汇编代码前
    Asm = 5,
    /// 完全编译
    None = 10,
}

#[cfg(debug_assertions)]
#[derive(PartialOrd, PartialEq, Clone)]
pub enum DebugAction {
    /// 无模式，依赖于DebugStop
    None,
    /// AST 检测
    ASTCheck,
    /// HIR 输出检查
    HirEmit(String),
    /// HIR 分析检查
    HirAnalyse,
    /// MIR 输出检查
    MIREmit(String),
    /// MIR Opt 输出检查
    MIROptEmit(String),
    /// AST 特性检查
    AstFeatureDetect,
    /// 局部自定义
    Custom(String),
}

#[derive(Copy, Clone, PartialOrd, Eq, PartialEq,Hash)]
pub enum CompileOptions{
    /// HIR中立即数预计算, 实际由to_mir来完成
    HirLitExprPreComp,
    /// HIR中数组元素位置预计算, 实际由to_mir来完成
    HirConstArrDimPreComp,
}

#[derive(Copy, Clone, Ord, PartialOrd, Eq, PartialEq)]
pub enum RegBackend {
    /// 消极的分配方案
    T2 = 1,
    /// Translate3方案
    T3 = 2,
}

pub struct Compiler {
    #[cfg(debug_assertions)]
    compile_stop: DebugStop,
    #[cfg(debug_assertions)]
    debug_actions: Vec<DebugAction>,
    session: CompileSession,
    input_path: Option<String>,
    out_path: String,
    warning_status: bool,
    isa_backend: &'static dyn ISABackend,
    opt_level: OptLevel,
    reg_backend: RegBackend,
}

impl Compiler {
    pub fn new(out_put: &str, backend: &'static dyn ISABackend) -> Compiler {
        Compiler {
            #[cfg(debug_assertions)]
            compile_stop: DebugStop::Asm,
            #[cfg(debug_assertions)]
            debug_actions: vec![DebugAction::None],
            session: CompileSession::new(),
            input_path: None,
            out_path: out_put.to_string(),
            warning_status: false,
            isa_backend: backend,
            opt_level: OptLevel::O0,
            reg_backend: RegBackend::T2,
        }
    }

    fn parse_module_as_ast(&mut self, text: &str) -> Result<AST, ()> {
        let lexer = TokenReader::new(&text);
        let mut parser = TokenParser::new(lexer, &mut self.session);
        let ast = parser.parse_ast();
        return ast;
    }

    pub fn compile_text(&mut self, text: &str) -> CompileResult {
        let ast = self.parse_module_as_ast(text);
        if ast.is_err() || self.session.is_error() {
            if self.warning_status {
                self.session.print_warning(text);
            }
            self.session.print_error(text);
            return CompileResult::AstErr;
        }

        let ast = ast.unwrap();
        #[cfg(debug_assertions)]
        if self.debug_actions.contains(&DebugAction::AstFeatureDetect) {
            let mut filter = Filter::new();
            filter.register_filter(FeatureDetector::Array(&ConstArrayDetect));
            filter.filter(&ast);
        }

        #[cfg(debug_assertions)]
        if self.compile_stop == DebugStop::Ast {
            return CompileResult::Ok;
        }

        let mut ast_with_std = get_sysy_std();
        ast_with_std.combine(ast); // 合并标准库

        let mut symbol_table = SymbolTable::new();
        let mut ast_checker = AstChecker::new();

        #[cfg(debug_assertions)]
        if self.compile_stop < DebugStop::Hir {
            return CompileResult::Ok;
        }

        let hir = ast_checker.analyse(ast_with_std, &mut self.session, &mut symbol_table);

        if hir.is_err() {
            self.session.print_warning(text);
            self.session.print_error(text);
            return CompileResult::HirErr;
        }

        if self.warning_status {
            // 对于没有产生调用的空Bool表达式，我们可以汇报warning
            self.session.print_warning(text);
        }

        let hir = hir.unwrap();
        #[cfg(debug_assertions)]
        for action in &self.debug_actions {
            if let DebugAction::HirEmit(path) = action {
                let code = hir.to_code(&mut symbol_table);
                let file = File::create(path);
                let file = if file.is_err() { panic!("FILE NOT FOUND") } else { file.unwrap() };
                let mut writer = std::io::BufWriter::new(file);
                writer.write_all(code.as_bytes()).unwrap();
                break;
            }
        }

        let hir_optimizer = HirOpt::input(hir, self.opt_level);
        let hir = hir_optimizer.output();

        #[cfg(debug_assertions)]
        if self.compile_stop < DebugStop::Mir {
            return CompileResult::Ok;
        }

        let mut mir_ref_pool = MIRRefPool::new();

        let cfg = MIRTranSlate2::parse_hir(hir, &mut mir_ref_pool, &mut symbol_table, &mut self.session);

        #[cfg(debug_assertions)]
        for action in &self.debug_actions {
            if let DebugAction::MIREmit(path) = action {
                let code = cfg.to_code_string(&mut mir_ref_pool, &mut symbol_table);
                let file = File::create(path);
                let file = if file.is_err() { panic!("FILE NOT FOUND") } else { file.unwrap() };
                let mut writer = std::io::BufWriter::new(file);
                writer.write_all(code.as_bytes()).unwrap();
            }
        }

        let mut linear_mir = cfg.to_liner();

        let mut mir_optimizer = MirOpt::input(linear_mir);
        if self.opt_level >= OptLevel::O1 {
            // 只有在01及以上才做的优化
            mir_optimizer.pass_block(optimizer::MIRRemoveLazyRight::instance());
        }
        if self.opt_level >= OptLevel::O2 {
            mir_optimizer.pass_fun(optimizer::MIRCommRightRemove::instance());
        }

        linear_mir = mir_optimizer.output();

        #[cfg(debug_assertions)]
        for action in &self.debug_actions {
            if let DebugAction::MIROptEmit(path) = action {
                let code = linear_mir.to_code_string(&mut mir_ref_pool, &mut symbol_table);
                let file = File::create(path);
                let file = if file.is_err() { panic!("FILE NOT FOUND") } else { file.unwrap() };
                let mut writer = std::io::BufWriter::new(file);
                writer.write_all(code.as_bytes()).unwrap();

                break;
            }
        }

        #[cfg(debug_assertions)]
        if self.compile_stop < DebugStop::Lir {
            return CompileResult::Ok;
        }

        let mut translate = LIRTranslate::new(&mut symbol_table, self.isa_backend);
        let lir = match self.reg_backend {
            RegBackend::T2 => translate.translate2(linear_mir, mir_ref_pool),
            RegBackend::T3 => translate.translate3(linear_mir, mir_ref_pool),
        };

        #[cfg(debug_assertions)]
        if self.compile_stop < DebugStop::Asm {
            return CompileResult::Ok;
        }
        let asm = ASMTranslate::new(self.isa_backend, &symbol_table);
        let mut organizer = ASMOrganizer::new(asm);
        organizer.organ(lir);
        organizer.write_to(Path::new(&self.out_path));
        CompileResult::Ok
    }

    // we can't directly use let mut compiler = Compiler::new(); to debug
    // for this is unit test,not compiler test
    // so , we need compiler to support debug ,
    #[cfg(debug_assertions)]
    pub fn set_compile_stop(&mut self, stop: DebugStop) {
        self.compile_stop = stop;
    }

    #[cfg(debug_assertions)]
    pub fn add_debug_mode(&mut self, mode: DebugAction) {
        self.session.add_debug_mode(mode.clone());
        self.debug_actions.push(mode);
    }

    pub fn set_warning_mode(&mut self, status: bool) {
        self.warning_status = status;
    }

    pub fn set_opt(&mut self, opt_level: OptLevel) {
        self.opt_level = opt_level;
        self.session.set_opt(opt_level)
    }

    pub fn set_reg_backend(&mut self, backend: RegBackend) {
        self.reg_backend = backend;
    }

    pub fn set_single_input(&mut self, input: String) {
        self.input_path = Some(input);
    }

    pub fn compile(&mut self) -> CompileResult {
        match &self.input_path {
            None => {
                self.session.report_error(Error::new("missing input file", None));
                self.session.print_error(" /*nothing*/");
                return CompileResult::AstErr;
            }
            Some(input) => self.compile_single_file(&input.clone()),
        }
    }

    pub fn compile_single_file(&mut self, path: &str) -> CompileResult {
        let file_opt = File::open(path);
        let mut file;
        match file_opt {
            Ok(ok) => {
                file = ok;
            }
            Err(e) => {
                let info = format!("open file error , {}", e.to_string());
                let error = Error::new(&info, None);
                print_echo(&error);
                return CompileResult::AstErr;
            }
        }
        let mut codes = String::new();
        loop {
            let size = file.read_to_string(&mut codes);
            match size {
                Ok(ok) => {
                    if ok == 0 {
                        break;
                    }
                }
                Err(err) => {
                    let error = Error::new(&format!("read file error , {}", err.to_string()), None);
                    print_echo(&error);
                    return CompileResult::AstErr;
                }
            }
        }
        self.compile_text(&mut codes)
    }

    pub fn open_option(&mut self,option : CompileOptions){
        self.session.open_option(option);
    }
}
