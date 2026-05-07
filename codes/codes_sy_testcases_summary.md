# codes 目录下 SY 测试用例汇总

搜索根目录：`/mnt/d/Code/CSC-Compiler/codes`

## 总体情况

- `.sy` 文件总数：812
- 不同的 `.sy` 文件名数量：696
- 在不同目录中重复使用或重名的文件名数量：116

## 按年份统计

| 年份 | 数量 |
|---|---:|
| 2025 | 348 |
| 2024 | 439 |
| 2023 | 25 |

## 按项目统计

| 项目 | 数量 |
|---|---:|
| `2024/2-First-NKU-100-77-83 (C++)` | 392 |
| `2025/8-Second-NKU-100-54-64 (C++)` | 186 |
| `2025/19-Third-HLJUST-99-29-44 (C++)` | 96 |
| `2025/21-Third-NJUPT-98-26-42 (Rust)` | 49 |
| `2024/21-Third-WHU-100-27-47 (C++)` | 45 |
| `2023/10-Third-UESTC (Rust)` | 24 |
| `2025/1-Grand-Cambridge-100-78-83 (C++)` | 17 |
| `2024/9-Second-Beihang-100-41-58 (Java)` | 1 |
| `2024/6-Third-Beihang-100-46-61 (Java)` | 1 |
| `2023/15-Third-WHU (C++)` | 1 |

## 主要测试用例目录

| 目录 | 数量 |
|---|---:|
| `/mnt/d/Code/CSC-Compiler/codes/2024/2-First-NKU-100-77-83 (C++)/testcase/functional_test` | 290 |
| `/mnt/d/Code/CSC-Compiler/codes/2024/2-First-NKU-100-77-83 (C++)/testcase/performance` | 102 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/functional_test/Advanced` | 100 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/19-Third-HLJUST-99-29-44 (C++)/tests/test` | 86 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/functional_test/Basic` | 50 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/21-Third-NJUPT-98-26-42 (Rust)/tests_self` | 49 |
| `/mnt/d/Code/CSC-Compiler/codes/2024/21-Third-WHU-100-27-47 (C++)/test_cases/custom` | 27 |
| `/mnt/d/Code/CSC-Compiler/codes/2023/10-Third-UESTC (Rust)/test/homemade` | 24 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/semant` | 18 |
| `/mnt/d/Code/CSC-Compiler/codes/2024/21-Third-WHU-100-27-47 (C++)/test_cases` | 18 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/1-Grand-Cambridge-100-78-83 (C++)/test/custom` | 17 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/19-Third-HLJUST-99-29-44 (C++)/tests` | 10 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/_readonly_test` | 6 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/optimize_test/sccp` | 2 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/optimize_test/eliUnreachablebb` | 2 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/optimize_test/basic_mem2reg` | 2 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/optimize_test/adce` | 2 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/optimize_test/scalar_licm` | 1 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/optimize_test/scalar_cse` | 1 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)/testcase/optimize_test/mem2reg` | 1 |
| `/mnt/d/Code/CSC-Compiler/codes/2025/8-Second-NKU-100-54-64 (C++)` | 1 |
| `/mnt/d/Code/CSC-Compiler/codes/2024/9-Second-Beihang-100-41-58 (Java)` | 1 |
| `/mnt/d/Code/CSC-Compiler/codes/2024/6-Third-Beihang-100-46-61 (Java)` | 1 |
| `/mnt/d/Code/CSC-Compiler/codes/2023/15-Third-WHU (C++)/compiler/Frontend` | 1 |

## 常用命令

列出所有 `.sy` 文件：

```sh
rg --files -g '*.sy' /mnt/d/Code/CSC-Compiler/codes
```

按所属目录统计：

```sh
find /mnt/d/Code/CSC-Compiler/codes -type f -name '*.sy' -printf '%h\n' | sort | uniq -c | sort -nr
```

按项目统计：

```sh
find /mnt/d/Code/CSC-Compiler/codes -type f -name '*.sy' -printf '%p\n' | awk -F/ '{print $7 "/" $8}' | sort | uniq -c | sort -nr
```
