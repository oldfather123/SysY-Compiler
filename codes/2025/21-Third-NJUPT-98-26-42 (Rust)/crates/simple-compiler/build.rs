use cranelift_isle::codegen::CodegenOptions;
use cranelift_isle::compile;
use std::env;
use std::fs;
use std::path::Path;

fn main() {
    let out_dir = env::var("OUT_DIR").unwrap();
    let dest_path = Path::new(&out_dir).join("arithmetic_generated.rs");

    // 在构建时编译 ISLE 文件
    println!("cargo:rerun-if-changed=arithmetic.isle");

    // 使用 ISLE 编译器编译算术规则
    let isle_file = Path::new("arithmetic.isle");

    if isle_file.exists() {
        let options = CodegenOptions {
            exclude_global_allow_pragmas: false,
            ..Default::default()
        };

        match compile::from_files(std::iter::once(isle_file), &options) {
            Ok(generated_code) => {
                fs::write(&dest_path, &generated_code).unwrap();
                println!("Generated ISLE code at: {}", dest_path.display());
            }
            Err(errors) => {
                eprintln!("ISLE compilation failed:");
                for error in errors.errors {
                    eprintln!("  {:?}", error);
                }
                panic!("ISLE compilation failed");
            }
        }
    } else {
        // 如果 ISLE 文件不存在，生成占位代码
        let placeholder_code = r#"
// 占位代码 - arithmetic.isle 文件未找到

pub fn generated_arithmetic_optimizer() -> &'static str {
    "ISLE 文件未找到，使用占位代码"
}

pub fn optimize_expression(expr: &str) -> String {
    format!("未优化的表达式: {}", expr)
}
"#;
        fs::write(&dest_path, placeholder_code).unwrap();
        println!("Generated placeholder code at: {}", dest_path.display());
    }
}
