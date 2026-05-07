use cranelift_isle::codegen::CodegenOptions;
use cranelift_isle::compile;
use std::fs;
use std::path::Path;

fn main() {
    env_logger::init();

    println!("ISLE 简单编译器示例");
    println!("==================");

    // ISLE 文件路径
    let isle_file = Path::new("crates/simple-compiler/arithmetic.isle");

    if !isle_file.exists() {
        eprintln!("错误: 找不到 arithmetic.isle 文件");
        eprintln!("请确保在项目目录中运行此程序");
        return;
    }

    // 创建输出目录
    let output_dir = Path::new("target/generated");
    if let Err(e) = fs::create_dir_all(output_dir) {
        eprintln!("错误: 无法创建输出目录: {}", e);
        return;
    }

    // 清理旧的生成文件
    let output_file = output_dir.join("generated_arithmetic.rs");
    if output_file.exists() {
        println!("🗑️  清理旧的生成文件: {}", output_file.display());
        if let Err(e) = fs::remove_file(&output_file) {
            eprintln!("警告: 无法删除旧文件: {}", e);
        }
    }

    // 编译选项
    let options = CodegenOptions {
        exclude_global_allow_pragmas: false,
        ..Default::default()
    };

    // 编译 ISLE 文件
    match compile::from_files(std::iter::once(isle_file), &options) {
        Ok(generated_code) => {
            println!("✓ ISLE 编译成功!");
            println!("生成的 Rust 代码长度: {} 字符", generated_code.len());

            // 写入生成的代码到 target/generated 目录
            if let Err(e) = fs::write(&output_file, &generated_code) {
                eprintln!("错误: 无法写入生成的代码文件: {}", e);
            } else {
                println!("✓ 生成的代码已保存到: {}", output_file.display());
                println!(
                    "📁 输出文件位置: {}",
                    output_file
                        .canonicalize()
                        .unwrap_or(output_file.to_path_buf())
                        .display()
                );
            }

            // 显示生成代码的开头部分
            println!("\n生成的代码预览:");
            println!("{}", "-".repeat(50));
            let preview = generated_code.chars().take(500).collect::<String>();
            println!("{}", preview);
            if generated_code.len() > 500 {
                println!("... (省略剩余内容)");
            }
        }
        Err(errors) => {
            eprintln!("✗ ISLE 编译失败:");
            for error in errors.errors {
                eprintln!("  {:?}", error);
            }
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_isle_compilation() {
        let isle_file = Path::new("arithmetic.isle");
        let options = CodegenOptions::default();

        // 如果文件存在，测试编译
        if isle_file.exists() {
            let result = compile::from_files(std::iter::once(isle_file), &options);
            assert!(result.is_ok(), "ISLE 编译应该成功");
        }
    }
}
