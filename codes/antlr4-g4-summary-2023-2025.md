# 2023-2025 使用 ANTLR4 的项目及 `.g4` 文件情况

本文档从 `codes/2023`、`codes/2024` 与 `codes/2025` 中筛选所有使用 ANTLR4 的编译器项目，并检查项目目录中是否保留了 ANTLR4 语法源文件 `.g4`。

## 判定口径

- **使用 ANTLR4**：项目内存在 `.g4` 文件，或生成的 Lexer/Parser/Visitor 文件头包含 `Generated from ... by ANTLR`。
- **写出 `.g4` 文件**：项目目录内能找到 `.g4` 文件。若生成文件头提到 `.g4`，但项目内未实际保留该文件，则记为“否”。
- 已排除嵌入的 ANTLR runtime 自带文件，例如 `org/antlr`、`antlr4-runtime` 下的文件。

## 2023 年

| 编译器目录 | 是否写出 `.g4` 文件 | `.g4` 文件 | ANTLR4 证据 |
| --- | --- | --- | --- |
| `2-First-BIT (Java)` | 是 | `antlr/SysY.g4` | 生成文件头标明由 `SysY.g4` 生成 |
| `3-First-SYSU (C++)` | 是 | `src/frontEnd/SysY2022.g4` | 生成文件头标明由 `src/frontEnd/SysY2022.g4` 生成 |
| `6-Second-USTC (C++)` | 否 | 未找到 | `sysyParser.cpp`、`sysyLexer.cpp` 头部标明由 `src/ast/grammer/sysy.g4` 生成 |
| `10-Third-UESTC (Rust)` | 是 | `src/frontend/antlr_dep/Lexer.g4`、`src/frontend/antlr_dep/SysY.g4` | `src/frontend/antlr_dep/*` 由 `SysY.g4` 生成 |
| `11-Third-NEU (C++)` | 是 | `antlr/C.g4`、`antlr/SysY.g4` | 生成文件头标明由 `SysY.g4` 生成 |
| `12-Third-NUDT (C++)` | 是 | `src/antlr4/SysY.g4` | 生成文件头标明由 `./src/antlr4/SysY.g4` 生成 |
| `13-Third-HFUT (C++)` | 是 | `grammar/SYSYLexer.g4`、`grammar/SYSYParser.g4` | `src/antlr/*` 由对应 `.g4` 生成 |
| `18-Excellence-HUST (C++)` | 是 | `LLVMIRParser/LLVMIR.g4`、`SYSYParser/Sysy.g4` | 生成文件头标明由 `LLVMIR.g4`、`Sysy.g4` 生成 |
| `20-Excellence-NJU (Java)` | 是 | `src/antlr/SysYLexer.g4`、`src/antlr/SysYParser.g4` | 生成文件头标明由对应 `.g4` 生成 |
| `21-Excellence-NJU (C++)` | 是 | `SysYLexer.g4`、`SysYParser.g4` | 生成文件头标明由对应 `.g4` 生成 |
| `23-Excellence-TYUT (C++)` | 否 | 未找到 | `antlr/SysyParser.cpp`、`antlr/SysyLexer.cpp` 头部标明由 `Sysy.g4` 生成 |
| `24-WildcardSecond-Beihang (Java)` | 是 | `src/frontend/sysy.g4` | 生成文件头标明由 `sysy.g4` 生成 |
| `25-WildcardExcellence-Beihang (Java)` | 是 | `SysY.g4` | 生成文件头标明由 `SysY.g4` 生成 |

## 2024 年

| 编译器目录 | 是否写出 `.g4` 文件 | `.g4` 文件 | ANTLR4 证据 |
| --- | --- | --- | --- |
| `4-Second-Tsinghua-100-57-69 (C++)` | 是 | `src/frontend/SysY.g4`、`src/frontend/tokens.g4`、`src/frontend/literals.g4` | 生成文件头标明由 `SysY.g4` 生成 |
| `7-Second-NUDT-100-46-61 (C++)` | 否 | 未找到 | `src/SysYParser.cpp`、`src/SysYLexer.cpp` 头部标明 `Generated from SysY.g4 by ANTLR 4.12.0` |
| `8-Second-NJU-100-44-59 (C++)` | 是 | `src/nnvm/nnvm/Frontend/SysYParser.g4`、`src/nnvm/nnvm/Frontend/SysYLexer.g4` | 生成文件头标明由对应 `.g4` 生成 |
| `12-Third-SYSU-100-37-55 (C++)` | 是 | `scarab/src/FrontEnd/SysY2022.g4` | 生成文件头标明由 `SysY2022.g4` 生成 |
| `15-Third-SYSU-100-35-52 (C++)` | 是 | `src/frontEnd/SysY2022.g4` | 生成文件头标明由 `SysY2022.g4` 生成 |
| `16-Third-NJU-100-33-51 (C++)` | 是 | `parser/SysYParser.g4` | 生成文件头标明由 `parser/SysYParser.g4` 生成 |
| `18-Third-NUDT-100-33-51 (C++)` | 是 | `src/frontend/SysY.g4` | 生成文件头标明由 `SysY.g4` 生成 |
| `21-Third-WHU-100-27-47 (C++)` | 是 | `src/frontend/Sysy.g4` | `src/frontend/generated/*` 由 `Sysy.g4` 生成 |
| `24-Excellence-OUC-100-23-44 (C++)` | 否 | 未找到 | `src/parser/SysYParser.cpp`、`src/parser/SysYLexer.cpp` 头部标明 `Generated from SysY.g4 by ANTLR 4.13.1` |
| `28-Excellence-NJU-100-16-39 (Java)` | 是 | `src/antlr/SysYParser.g4`、`src/antlr/SysYLexer.g4` | 生成文件头标明由对应 `.g4` 生成 |
| `29-Excellence-CJLU-99-15-38 (Rust)` | 是 | `src/antlr_parser/C.g4` | 项目保留 ANTLR 语法文件与生成的 Rust 前端代码 |
| `33-Excellence-NJU-100-12-28 (Java)` | 是 | `src/main/java/cn/edu/nju/software/frontend/parser/SysYParser.g4`、`src/main/java/cn/edu/nju/software/frontend/lexer/SysYLexer.g4` | 生成文件头标明由对应 `.g4` 生成 |

## 2025 年

| 编译器目录 | 是否写出 `.g4` 文件 | `.g4` 文件 | ANTLR4 证据 |
| --- | --- | --- | --- |
| `3-First-NJU-100-69-76 (Java)` | 否 | 未找到 | `src/org/systemf/compiler/parser/SysYParser.java`、`SysYLexer.java` 头部标明由 `SysYParser.g4`/`SysYLexer.g4` 生成 |
| `4-Second-WHU-100-66-74 (C++)` | 是 | `src/frontend/SysYParser.g4`、`src/frontend/SysYLexer.g4` | `src/frontend/generated/*` 由对应 `.g4` 生成 |
| `9-Second-WHU-100-52-63 (C++)` | 是 | `myCompiler/src/SysY.g4` | `myCompiler/src/frontend/generate/*` 由 `SysY.g4` 生成 |
| `17-Third-NUDT-99-30-45 (C++)` | 是 | `src/frontend/SysY.g4` | 生成文件头标明由 `SysY.g4` 生成 |
| `19-Third-HLJUST-99-29-44 (C++)` | 是 | `SysyLex.g4`、`Sysy22.g4` | `src/grammar/*` 由 `Sysy22.g4` 生成 |
| `22-Excellence-XJTU-100-22-40 (Java)` | 否 | 未找到 | `src/main/java/cn/edu/xjtu/sysy/parse/SysYParser.java`、`SysYLexer.java` 头部标明 `Generated from SysY.g4 by ANTLR 4.12.0` |
| `28-Excellence-DUT-84-0-19 (C++)` | 是 | `Sysy.g4`、`SysyLex.g4` | `src/antlr4/*` 由 `Sysy.g4` 生成 |

## 汇总统计

| 年份 | ANTLR4 项目数 | 保留 `.g4` | 未保留 `.g4` |
| --- | ---: | ---: | ---: |
| 2023 | 13 | 11 | 2 |
| 2024 | 12 | 10 | 2 |
| 2025 | 7 | 5 | 2 |
| 合计 | 32 | 26 | 6 |

## 未保留 `.g4` 的 ANTLR4 项目

以下项目可以从生成文件头确认使用了 ANTLR4，但在项目目录内没有找到实际 `.g4` 文件：

- `codes/2023/6-Second-USTC (C++)`
- `codes/2023/23-Excellence-TYUT (C++)`
- `codes/2024/7-Second-NUDT-100-46-61 (C++)`
- `codes/2024/24-Excellence-OUC-100-23-44 (C++)`
- `codes/2025/3-First-NJU-100-69-76 (Java)`
- `codes/2025/22-Excellence-XJTU-100-22-40 (Java)`
