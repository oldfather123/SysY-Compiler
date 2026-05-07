# 新增功能使用说明

## 1. 自定义log名字

现在你可以使用 `--log-name` 参数来自定义log文件夹的名字，而不是使用默认的时间戳：

```bash
# 使用自定义名字
python test.py --save-log --log-name my_experiment

# 使用默认时间戳
python test.py --save-log

# 重复测试3次并保存到自定义log
python test.py --save-log --log-name baseline_3x -n 3
```

## 2. 重复测试

使用 `-n` 参数可以让每个测试用例重复运行多次，结果会取平均值：

```bash
# 每个测试用例重复5次
python test.py -n 5

# 重复3次并保存结果
python test.py -n 3 --save-log --log-name repeated_test
```

## 3. 命令记录功能

现在每次运行 `--save-log` 时，会自动记录使用的完整命令：

```bash
# 运行测试并保存命令信息
python test.py --save-log --log-name experiment -n 5 --time

# 生成的 command.log 会包含：
# Command used to run this test
# python3 test.py --save-log --log-name experiment -n 5 --time
# 
# Timestamp: 2025-08-04 01:54:44
```

## 4. 比较不同配置

修改 `debug.h` 中的优化pass设置，然后运行不同的测试：

```bash
# 第一次测试（比如开启所有优化）
python test.py --save-log --log-name all_optimizations -n 3

# 修改debug.h，关闭某些优化，然后第二次测试
python test.py --save-log --log-name some_optimizations -n 3

# 使用新的diff.py比较两次测试结果
python diff.py test/log/all_optimizations test/log/some_optimizations
```

## 5. Log文件结构

每次运行 `--save-log` 后，会在 `test/log/` 下创建文件夹，包含：
- `time.log`: 每个测试用例的编译时间（微秒）
- `passes.log`: 使用的编译器pass配置（JSON格式）
- `command.log`: 运行测试时使用的完整命令和时间戳

## 6. 增强的diff.py功能

新的diff.py提供更丰富的比较信息：

```bash
# 比较两个log目录（新功能）
python diff.py test/log/baseline test/log/experiment

# 显示内容包括：
# 1. 使用的命令信息和时间戳
# 2. Pass配置差异
# 3. 时间比较（比例>1显示绿色，<1显示红色）
# 4. 统计信息
```

输出示例：
```
Command Information:
baseline: python3 test.py --save-log --log-name baseline -n 3
    Timestamp: 2025-08-04 01:54:44
experiment: python3 test.py --save-log --log-name experiment -n 5 --time
    Timestamp: 2025-08-04 02:15:20

Pass Configuration Differences:
Pass Name                      baseline        experiment     
------------------------------------------------------------
OPEN_GVN                       1               0              
OPEN_LICM                      1               0              

Timing Comparison Results:
Test Case                           Ratio      baseline (μs)   experiment (μs)
-------------------------------------------------------------------------------------
01_mm1                              1.0231↑     112980          110425         
01_mm2                              0.9302↓     379062          407933         
...

统计信息:
总测试用例: 58
baseline比experiment慢10%以上的用例: 7 (12.1%)
baseline比experiment快10%以上的用例: 25 (43.1%)
```

## 7. 实际使用工作流

```bash
# 1. 建立基线
python test.py --save-log --log-name baseline -n 3

# 2. 修改debug.h中的某些pass设置
# 3. 运行对比测试
python test.py --save-log --log-name optimized -n 3

# 4. 比较结果
python diff.py test/log/baseline test/log/optimized

# 5. 如果结果满意，可以继续下一轮优化
python test.py --save-log --log-name further_optimized -n 3
python diff.py test/log/optimized test/log/further_optimized
```

## 8. 支持的diff.py用法

```bash
# 比较两个log目录（新功能，推荐）
python diff.py test/log/baseline test/log/experiment

# 比较两个传统log文件（向后兼容）
python diff.py 1.log 2.log
```
