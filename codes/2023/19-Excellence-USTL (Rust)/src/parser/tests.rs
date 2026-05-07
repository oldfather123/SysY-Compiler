#[test]
fn test_nested_paren() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let code = "
int name[10][10] = {};
int main(){
    {
        {
                    {
                        a14 = temp % 2;
                        if (a14 < 0) a14 = -a14;
                        temp = temp / 2;
                    };
                    {
                        a15 = temp % 2;
                        if (a15 < 0) a15 = -a15;
                        temp = temp / 2;
                    };
        }
    }
}
";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(code);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_parse() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let code = "/*/skipher/*/
//int main(){
int main(){
	////return 0;}/*
	/*}
	//}return 1;*/
	//}return 2;*//*
	return 3;
	//*/
}
//";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(code);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_while() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};

    let code = "void test2(){
    while(invoke(value[][10],table) + 100){
        break;
    }
}";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(code);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("void test2()int测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_fns() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let codes = "
void test1(){
    int a = 0;
    int b = 0;int d = 0;
    float c = 0;
}
void test2()int{
    int a = 0;
    int b = 0;int d = 0;
    float e = 0;
}";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(codes);
    match result {
        CompileResult::Ok => {
            panic!("void test2()int测试不通过");
        }
        CompileResult::AstErr => {}
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_error() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let codes = "void test1(){
    int a < = 0;
    int b = 0;int d = 0;
    float c = 0;
}";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(codes);
    match result {
        CompileResult::Ok => {
            panic!("void test2()int测试不通过");
        }
        CompileResult::AstErr => {}
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_arr() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let codes = "
   int main(){
    const int a[4][2] = {{1, 2}, {3, 4}, {}, 7};
    const int N = 3;
    int b[4][2] = {};
    int c[4][2] = {1, 2, 3, 4, 5, 6, 7, 8};
    int d[N + 1][2] = {1, 2, {3}, {5}, a[3][0], 8};
    int e[4][2][1] = {{d[2][1], {c[2][1]}}, {3, 4}, {5, 6}, {7, 8}};
    return ;
}
";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(codes);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("数组测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_aaa() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let codes = "
   //test local var define
int main(){
    int a, b0, _c;
    a = 1;
    b0 = 2;
    _c = 3;
    return ;
}
";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(codes);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("数组测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_main11() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let codes = "int main(){
    return 3;
}";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(codes);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("return测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_exprs() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let codes = "
int main(){
    float e = 0 * 1  + 10 / 20 * (100 + 300) * 50;
    e[3][4] = 0 * 1  + 10 / 20 * (100 + 300) * array[10][100 + N + invoke()];
    invoke();
    (+1+exprs(invoke(),name[10][10]));
}
";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(codes);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("表达式测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test96_matrix_add() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};

    let codes = r"int M;
int L;
int N;


int add(float a0[],float a1[], float a2[],float b0[],float b1[],float b2[],float c0[],float c1[],float c2[])
{
    int i;
    i=0;
    while(i<M)
    {
        c0[i]=a0[i]+b0[i];
        c1[i]=a1[i]+b1[i];
        c2[i]=a2[i]+b2[i];
        i=i+1;
    }

    return 10+1;

}

int main()
{
    N=3;
    M=3;
    L=3;
    float a0[3], a1[3], a2[3], b0[3], b1[3], b2[3], c0[6], c1[3], c2[3];
    int i;
    i=0;
    while(i<M)
    {
        a0[i]=i;
        a1[i]=i;
        a2[i]=i;
        b0[i]=i;
        b1[i]=i;
        b2[i]=i;
        i=i+1;
    }
    i=add( a0, a1,  a2, b0, b1, b2, c0, c1, c2);
    int x;
    while(i<N)
    {
        x = c0[i];
        putint(x);
        i=i+1;
    }
    x = 10;
    putch(x);
    i=0;
    while(i<N)
    {
        x = c1[i];
        putint(x);
        i=i+1;
    }
    x = 10;
    putch(x);
    i=0;
    while(i<N)
    {
        x = c2[i];
        putint(x);
        i=i+1;
    }
    x = 10;
    putch(x);

    return 0;
}";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(codes);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("表达式测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_hide2() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let codes = r"/*/skipher/*/
//int main(){
int main(){
	////return 0;}/*
	/*}
	//}return 1;*/
	//}return 2;*//*
	return 3;
	//*/
}
//";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(codes);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("表达式测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_hide_12() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};
    let codes = r"int quick_read(){
	int ch = getch(); int x = 0, f = 0;
	while (ch < 48 || ch > 57){
		if (ch == 45) f = 1;
		ch = getch();
	}
	while (ch >= 48 && ch <=57){
		x = x * 10 + ch - 48;
		ch = getch();
	}
	if (f) return -x;
	else return x;
}
int n, m, fa[100005];
void init(){
	int i = 1;
	while (i <= n){
		fa[i] = i;
		i = i + 1;
	}
}
int find(int x){
	if (fa[x] == x) return x;
	else{
		int pa = find(fa[x]);
		fa[x] = pa;
		return pa;
	}
}
int same(int x, int y){
	if (find(x) == find(y)) return 1;
	return 0;
}
int main(){
	n = quick_read(); m = quick_read();
	init();
	while (m){
		int ch = getch();
		while (ch != 81 && ch != 85)
			ch = getch();

		if (ch == 81){ // query
			int x = quick_read(), y = quick_read();
			putint(same(x, y));
			putch(10);
		}else{ // union
			int x = find(quick_read()), y = find(quick_read());
			fa[x] = y;
		}
		m = m - 1;
	}
	return 0;
}";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(codes);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("表达式测试不通过");
        }
        CompileResult::HirErr => {}
    }
}

#[test]
fn test_empty() {
    use crate::asm::RISCVBackend;
    use std::path::Path;

    use crate::compiler::{CompileResult, Compiler, DebugStop};

    let code = "int main() {
  int a = 10;
  ;;;;
  return a * 2 + 1;
}
";
    let mut compiler = Compiler::new("./res/temp.S", &RISCVBackend);
    compiler.set_compile_stop(DebugStop::Ast);
    let result = compiler.compile_text(code);
    match result {
        CompileResult::Ok => {}
        CompileResult::AstErr => {
            panic!("表达式测试不通过");
        }
        CompileResult::HirErr => {}
    }
}
