import argparse
import os
import time
import subprocess


# Usage examples:
# python3 grade.py 3 1 last (测试lab3进阶要求，使用functional_test目录)
# python3 grade.py 5 0 curr (测试lab5基本要求，使用functional_test_curr目录)
# python3 grade.py 6 0 extr (测试lab6基本要求，使用functional_test_extr目录)

# 记录开始时间
start_time = time.time()

parser = argparse.ArgumentParser()
parser.add_argument('lab', type=int, default=1)
parser.add_argument('is_advance', type=int, default=1)
parser.add_argument('test_version', choices=['last', 'curr','extr'], default='last', 
                   help="'last' for functional_test, 'curr' for functional_test_curr,'extr' for functional_test_extr")
args = parser.parse_args()

# Determine test directory based on test_version argument
test_dir_dict = {"last":"functional_test",
                 "curr":"functional_test_curr",
                 "extr":"functional_test_extr"}
test_dir = test_dir_dict[args.test_version]

# 记录编译开始时间
compile_start_time = time.time()

# 编译阶段
if(args.lab == 1):
    print("lab1和lab2不支持自动评测, 请自行检查输入和输出")
elif(args.lab == 2):
    os.system("rm -rf test_output/functional_testIR/*.ll")
    if(args.is_advance==False):
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Basic test_output/functional_testIR 0 semant")
    else:
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Advanced test_output/functional_testIR 0 semant")
elif(args.lab == 3):
    os.system("rm -rf test_output/functional_testIR/*.ll")
    if(args.is_advance==False):
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Basic test_output/functional_testIR 0 llvm")
    else:
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Advanced test_output/functional_testIR 1 llvm")
elif(args.lab == 4):
    os.system("rm -rf test_output/functional_testIR/*.ll")
    if(args.is_advance==False):
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Basic test_output/functional_testIR 0 llvm")
    else:
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Advanced test_output/functional_testIR 1 llvm")
elif(args.lab == 5):
    os.system("rm -rf test_output/functional_testAsm/*.s")
    if(args.is_advance==False):
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Basic test_output/functional_testSelect 0 select")
    else:
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Advanced test_output/functional_testSelect 1 select")
elif(args.lab == 6):
    os.system("rm -rf test_output/functional_testAsm/*.s")
    if(args.is_advance==False):
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Basic test_output/functional_testAsm 0 target")
    else:
        os.system(f"python3 test_with_timing.py testcase/{test_dir}/Advanced test_output/functional_testAsm 1 target")
else:
    print("未知lab, 请检查输入")

# 记录编译结束时间
compile_end_time = time.time()
compile_time = compile_end_time - compile_start_time

# 计算并输出时间统计
end_time = time.time()
total_time = end_time - start_time