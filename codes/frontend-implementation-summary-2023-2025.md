# 2023-2025 编译器前端实现方法汇总

本文档基于 `codes/2023`、`codes/2024` 与 `codes/2025` 下的本地文件进行静态归纳，重点查看语法文件、生成代码头、Lexer/Parser 源码、AST/Visitor/语义分析相关文件。未实际编译或运行各项目。

## 判定口径

- **ANTLR4**：存在 `.g4` 语法文件，或生成的 `SysYParser`/`SysYLexer` 文件头标明由 ANTLR 生成。
- **Flex/Bison 或 Lex/Yacc**：存在 `.l`/`.lex` 与 `.y` 文件，或只保留了 `lex.yy.*`、`y.tab.*`、`*.tab.*` 等生成产物。
- **手写前端**：未见生成器语法文件，前端由项目内 Lexer/Scanner、Parser/Syntax、AST、语义分析代码直接实现。
- **Rust 解析库**：Rust 项目中若显式使用 `lalrpop`、`pest`、`winnow` 等，单独标出。
- **无代码**：目录标注为“无代码”或仅见文档，无法从源码判断具体实现。

## 2023 年

| 编译器目录 | 前端实现方法 | 依据 |
| --- | --- | --- |
| `1-Grand-SUSTech (无代码)` | 无源码可判定 | 目录标注“无代码”，仅见 README |
| `2-First-BIT (Java)` | ANTLR4 | `antlr/SysY.g4`、`src/main/java/.../frontend/antlr/SysYLexer.java`、`SysYParser.java` |
| `3-First-SYSU (C++)` | ANTLR4 | `src/frontEnd/SysY2022.g4`、生成的 `SysY2022Lexer/Parser` |
| `4-Second-Beihang (Java)` | 手写 Java 前端 | `src/Frontend/Lexer.java`、`src/Frontend/Parser.java`、`src/Frontend/AST.java` |
| `5-Second-NKU (C++)` | Flex/Bison | `src/frontend/lexer.l`、`src/frontend/parser.y` |
| `6-Second-USTC (C++)` | ANTLR4 生成前端 | `sysyParser.cpp` 头部标明 `Generated from .../sysy.g4 by ANTLR 4.13.0` |
| `7-Second-USTC (C++)` | Flex/Bison 生成前端 | `src/frontend/sysY_scanner.cc` 使用 `FlexLexer`，`src/frontend/sysY_parser.cc` 来源行指向 `sysY_parser.yy` |
| `8-Third-BIT (无代码)` | 无源码可判定 | 目录标注“无代码”，仅见 README |
| `9-Third-BUPT (Rust)` | Rust nom 解析组合子 | `Cargo.toml` 依赖 `nom = "7"`，`src/lex.rs` 与 `src/ast/parse.rs` 使用 `nom` |
| `10-Third-UESTC (Rust)` | ANTLR4 Rust 生成前端 | `src/frontend/antlr_dep/SysY.g4`、生成的 `sysylexer.rs`、`sysyparser.rs` |
| `11-Third-NEU (C++)` | ANTLR4 | `antlr/SysY.g4`、生成的 `SysYLexer/SysYParser` |
| `12-Third-NUDT (C++)` | ANTLR4 | `src/antlr4/SysY.g4`、生成的 `SysYLexer/SysYParser` |
| `13-Third-HFUT (C++)` | ANTLR4 | `grammar/SYSYLexer.g4`、`grammar/SYSYParser.g4`、`src/antlr/*` |
| `14-Third-HUST (Java)` | 手写 Java 前端 | `src/compile/lexical/LexicalParser.java`、`src/compile/syntax/SyntaxParser.java`、`syntax/ast/*` |
| `15-Third-WHU (C++)` | 手写 C++ 前端 | `compiler/Frontend/Parser.cpp`、`AST.cpp`、`SemanticAnalyzer.cpp` |
| `16-Excellence-HITSZ (Rust)` | Rust LALRPOP | `src/SysYRust.lalrpop` |
| `17-Excellence-SCUT (C++)` | Flex/Bison | `sysy.l`、`sysy.y`、`lex.yy.c` |
| `18-Excellence-HUST (C++)` | ANTLR4 | `SYSYParser/Sysy.g4`、生成的 `SysyLexer/SysyParser` |
| `19-Excellence-USTL (Rust)` | 手写 Rust 前端 | `src/lexer/*`、`src/parser/parse_*.rs`，`Cargo.toml` 未见生成器依赖 |
| `20-Excellence-NJU (Java)` | ANTLR4 | `src/antlr/SysYLexer.g4`、`src/antlr/SysYParser.g4`、生成的 Java Lexer/Parser |
| `21-Excellence-NJU (C++)` | ANTLR4 | `SysYLexer.g4`、`SysYParser.g4`、`SysYLexer/*`、`SysYParser/*` |
| `22-Excellence-SCUJJ (C)` | Lex + 手写 C 解析器 | `src/lexer/lex.l`、`src/lexer/lex.yy.c`、`src/parser/parser.c` |
| `23-Excellence-TYUT (C++)` | ANTLR4 生成前端 | `antlr/SysyParser.cpp` 头部标明 `Generated from Sysy.g4 by ANTLR 4.12.0` |
| `24-WildcardSecond-Beihang (Java)` | ANTLR4 | `src/frontend/sysy.g4`、生成的 `sysyLexer.java`、`sysyParser.java` |
| `25-WildcardExcellence-Beihang (Java)` | ANTLR4 | `SysY.g4`、生成的 `src/frontend/SysYLexer.java`、`SysYParser.java` |
| `26-WildcardExcellence-HUST (C++)` | Flex/Bison | `src/parser/tokens.l`、`src/parser/parser.y`、生成的 `parser.cpp` |

## 2024 年

| 编译器目录 | 前端实现方法 | 依据 |
| --- | --- | --- |
| `1-First-Beihang-100-79-84 (Java)` | 手写 Java 前端 | `src/frontend/lexer/Lexer.java`、`src/frontend/syntaxChecker/Parser.java` |
| `2-First-NKU-100-77-83 (C++)` | Flex/Bison | `lexer/SysY_lexer.l`、`parser/SysY_parser.y` |
| `3-First-NKU-100-62-72 (Rust)` | Rust LALRPOP | `src/frontend/sysy/parser.lalrpop` |
| `4-Second-Tsinghua-100-57-69 (C++)` | ANTLR4 | `src/frontend/SysY.g4`、`tokens.g4`、生成的 `SysYLexer/SysYParser` |
| `5-Second-UESTC-100-49-64 (C++)` | Flex/Bison | `yacc/lex.l`、`yacc/parser.y` |
| `6-Third-Beihang-100-46-61 (Java)` | 手写 Java 前端 | `src/Frontend/Lexer.java`、`src/Frontend/Parser.java` |
| `7-Second-NUDT-100-46-61 (C++)` | ANTLR4 生成前端 | `src/SysYParser.cpp` 头部标明 `Generated from SysY.g4 by ANTLR 4.12.0` |
| `8-Second-NJU-100-44-59 (C++)` | ANTLR4 | `src/nnvm/nnvm/Frontend/SysYLexer.g4`、`SysYParser.g4` |
| `9-Second-Beihang-100-41-58 (Java)` | 手写 Java 前端 | `src/frontend/Lexer.java`、`src/frontend/Parser.java`、`src/frontend/AST/*` |
| `10-Second-UESTC-100-41-57 (C++)` | Flex/Bison | `frontend/lexer.l`、`frontend/parser.y` |
| `11-Second-Tsinghua-100-39-57 (Rust)` | Rust pest + PrattParser | `frontend/parser/src/parser.rs` 使用 `pest_derive::Parser` 与 `sysy2022.pest` |
| `12-Third-SYSU-100-37-55 (C++)` | ANTLR4 | `scarab/src/FrontEnd/SysY2022.g4` |
| `13-Third-Beihang-100-36-54 (C++)` | 手写 C++ 前端 | `src/frontend/lexer.cpp`、`src/frontend/parser.cpp` |
| `14-Third-USTC-100-36-53 (C++)` | Flex/Bison | `src/parser/lexical_analyzer.l`、`src/parser/syntax_analyzer.y` |
| `15-Third-SYSU-100-35-52 (C++)` | ANTLR4 | `src/frontEnd/SysY2022.g4` |
| `16-Third-NJU-100-33-51 (C++)` | ANTLR4 | `parser/SysYParser.g4`、生成的 Parser/Lexer |
| `17-Third-Beihang-100-33-51 (C++)` | 手写 C++ 前端 | `include/parser.h`、`src/parser.cpp` |
| `18-Third-NUDT-100-33-51 (C++)` | ANTLR4 | `src/frontend/SysY.g4` |
| `19-Third-HFUT-XC-98-32-51 (C++)` | 手写 C++ 前端 | `include/frontend/parser.hpp`、`src/frontend/parser.cpp` |
| `20-Third-Beihang-100-31-49 (Java)` | 手写 Java 前端 | `src/front/lexer/Lexer.java`、`src/front/parser/Parser.java` |
| `21-Third-WHU-100-27-47 (C++)` | ANTLR4 | `src/frontend/Sysy.g4`、`src/frontend/generated/*` |
| `22-Third-Beihang-98-28-47 (Java)` | 手写 Java 前端 | `src/frontend/lexer/Lexer.java`、`src/frontend/syntax/Parser.java` |
| `23-Third-HITSZ-100-27-47 (Rust)` | Rust winnow 解析组合子 | `Cargo.toml` 依赖 `winnow`，`src/frontend/parser.rs` 直接组合解析函数 |
| `24-Excellence-OUC-100-23-44 (C++)` | ANTLR4 生成前端 | `src/parser/SysYParser.cpp` 头部标明 `Generated from SysY.g4 by ANTLR 4.13.1` |
| `25-Excellence-BNU-99-21-43 (C++)` | 手写 C++ 前端 | `workspace/source/frontend/lexer.cpp`、`abstract_syntax_tree.cpp` |
| `26-Excellence-Beihang-99-20-41 (Java)` | 手写 Java 前端 | `src/frontend/lexer/Lexer.java`、`src/frontend/parser/Parser.java`、`src/frontend/ast/*` |
| `27-Excellence-HDU-100-29-41 (无代码)` | 无源码可判定 | 目录标注“无代码” |
| `28-Excellence-NJU-100-16-39 (Java)` | ANTLR4 | `src/antlr/SysYLexer.g4`、`SysYParser.g4` |
| `29-Excellence-CJLU-99-15-38 (Rust)` | ANTLR4 | `src/antlr_parser/C.g4`、生成的 Rust Lexer/Parser |
| `30-Excellence-Beihang-98-13-36 (Java)` | 手写 Java 前端 | `src/handler/SyntaxHandler.java` 等手写语法处理代码 |
| `31-Excellence-HFUT-100-12-36 (C++)` | Flex/Bison | `src/parser/lexer.l`、`src/parser/parser.y` |
| `32-Excellence-USTC-100-10-35 (C++)` | Flex/Bison | `src/parser/lexical_analyzer.l`、`src/parser/syntax_analyzer.y` |
| `33-Excellence-NJU-100-12-28 (Java)` | ANTLR4 | `src/main/java/.../frontend/lexer/SysYLexer.g4`、`frontend/parser/SysYParser.g4` |
| `34-Excellence-HUST-100-11-27 (无代码)` | 无源码可判定 | 目录标注“无代码” |
| `35-Excellence-NPU-95-0-26 (C++)` | Lex/Yacc 生成前端 | `src/resource/frontend/sysy.lex.cpp`、`sysy.tab.cpp`、`sysy.tab.hpp` |
| `36-Excellence-SZU-91-0-25 (C++)` | 手写 C++ 前端 | `src/frontend/Scanner.cpp`、`Parser.cpp`、`TypeChecker.cpp` |

## 2025 年

| 编译器目录 | 前端实现方法 | 依据 |
| --- | --- | --- |
| `1-Grand-Cambridge-100-78-83 (C++)` | 手写 C++ 前端 | `src/parse/Lexer.cpp`、`src/parse/Parser.cpp` |
| `2-First-Beihang-100-72-78 (C++)` | 手写 C++ 前端 | `src/frontend/lexer.cpp`、`src/frontend/parser.cpp` |
| `3-First-NJU-100-69-76 (Java)` | ANTLR4 生成前端 | `src/org/systemf/compiler/parser/SysYParser.java` 头部标明由 ANTLR 4.13.2 生成 |
| `4-Second-WHU-100-66-74 (C++)` | ANTLR4 | `src/frontend/SysYLexer.g4`、`SysYParser.g4` |
| `5-Second-Beihang-100-65-73 (Java)` | 手写 Java 前端 | `src/frontend/lexer/Lexer.java`、`src/frontend/syntax/SyntaxParser.java` |
| `6-Second-UESTC-100-56-66 (C++)` | Flex/Bison | `lib/lexer/lexer.l`、`lib/parser/parser.y` |
| `7-Second-BUPT-100-56-65 (C++)` | Lex/Yacc 生成前端 | `src/sy_parser/lex.yy.c`、`src/sy_parser/y.tab.c`、`include/sy_parser/y.tab.h` |
| `8-Second-NKU-100-54-64 (C++)` | Flex/Bison | `parser/lexer.l`、`parser/yacc.y` |
| `9-Second-WHU-100-52-63 (C++)` | ANTLR4 | `myCompiler/src/SysY.g4`、`frontend/generate/*` |
| `10-Third-HUST-100-44-57 (无代码)` | 无源码可判定 | 目录标注“无代码” |
| `11-Third-OUC-100-43-55 (C++)` | Flex/Bison | `src/sysy/lexer.l`、`src/sysy/parser.y` |
| `12-Third-CQU-100-42-55 (C++)` | 手写 C++ 前端 | `src/front/lexical.cpp`、`syntax.cpp`、`semantic.cpp` |
| `13-Third-NKU-100-41-54 (C++)` | Flex/Bison | `front_end/sysy_lexer.l`、`front_end/sysy_parser.y` |
| `14-Third-Beihang-100-41-54 (Java)` | 手写 Java 前端 | `src/frontend/lexer/handler/LexerAnalyzer.java`、`src/frontend/syntactic/handler/SyntacticParser.java` |
| `15-Third-USTC-100-39-52 (C++)` | Flex/Bison | `src/parser/lexical_analyzer.l`、`src/parser/syntax_analyzer.y` |
| `16-Third-ECNU-100-32-47 (C++)` | Lex/Yacc/Bison 风格前端 | `src/frontend/lexer/aaac.lex`、`src/frontend/parser/aaac.y`、生成的 `lex.cc`/`parse.cc` |
| `17-Third-NUDT-99-30-45 (C++)` | ANTLR4 | `src/frontend/SysY.g4`、生成的 `SysYLexer/SysYParser` |
| `18-Third-NJU-99-29-45 (无代码)` | 无源码可判定 | 目录标注“无代码” |
| `19-Third-HLJUST-99-29-44 (C++)` | ANTLR4 | `SysyLex.g4`、`Sysy22.g4`、`src/grammar/*` |
| `20-Third-BNU-100-27-43 (C++)` | Lex/Yacc 生成前端 | `src/lex.yy.cpp`、`src/sysy.tab.cpp`、`src/sysy.tab.h` |
| `21-Third-NJUPT-98-26-42 (Rust)` | 手写 Rust 前端 | `compiler/src/lexer/*`、`compiler/src/parser/parse.rs`、README 支持 lexer/parser stage |
| `22-Excellence-XJTU-100-22-40 (Java)` | ANTLR4 生成前端 | `src/main/java/cn/edu/xjtu/sysy/parse/SysYParser.java` 头部标明由 ANTLR 4.12.0 生成 |
| `23-Excellence-HUST-100-22-39 (C++)` | Flex/Bison | `src/frontend/lexer/lexer.l`、`src/frontend/parser/parser.y` |
| `24-Excellence-USTC-100-19-37 (C++)` | Flex/Bison | `src/parser/lexical_analyzer.l`、`src/parser/syntax_analyzer.y` |
| `25-Excellence-MJU-99-19-37 (C++)` | Flex/Bison | `src/parser/lexical_analyzer.l`、`src/parser/parser.y` |
| `26-Excellence-Beihang-99-18-36 (C++)` | 手写 C++ 前端 | `include/Frontend/Lexer.h`、`Parser.h`、`src/Frontend/Lexer.cpp`、`Parser.cpp` |
| `27-Excellence-UESTC-92-0-20 (C++)` | Flex/Bison | `yacc/lex.l`、`yacc/parser.y` |
| `28-Excellence-DUT-84-0-19 (C++)` | ANTLR4 | `SysyLex.g4`、`Sysy.g4`、`src/antlr4/*` |
| `29-Excellence-RUC-71-0-16 (C++)` | Flex/Bison | `lexer/lex.l`、`parser/parser.y` |

## 粗略统计

| 年份 | ANTLR4 | Flex/Bison | 手写前端 | Rust 专用解析库 | 无代码 |
| --- | ---: | ---: | ---: | ---: | ---: |
| 2023 | 13 | 5 | 4 | 2 | 2 |
| 2024 | 12 | 7 | 12 | 3 | 2 |
| 2025 | 7 | 13 | 7 | 0 | 2 |

> 注：统计按“主要前端入口”归类。ANTLR 生成文件但缺少原始 `.g4` 的项目仍计入 ANTLR4；只保留 `lex.yy`/`y.tab` 生成产物的项目计入 Lex/Yacc 系列。
