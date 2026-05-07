import argparse
import subprocess
import os
import time
import csv

def execute(command):
    return subprocess.run(command, capture_output=True, text=True)

def execute_with_stdin_out(command):
    return os.system(command) % 255

def check_file(file1, file2):
    try:
        result = execute(["diff", "--strip-trailing-cr", file1, file2, "-b"])
    except Exception as e:
        print(f"\033[91mUnknown Error on \033[0m{file1}, \033[91mPlease check your output file\033[0m")
        return 1
    else:
        return result.returncode

def add_returncode(file, ret):
    need_newline = False
    with open(file, "r") as f:
        try:
            content = f.read()
        except Exception as e:
            print(f"\033[91mUnknown Error on \033[0m{file}, \033[91mPlease check your output file\033[0m")
            return False
        else:
            if len(content) > 0:
                if not content.endswith("\n"):
                    need_newline = True

    with open(file, "a+") as f:
        if need_newline:
            f.write("\n")
        f.write(str(ret))
        f.write("\n")
    return False

def execute_compilation(input_file, output_file, compile_option, opt_level):
    compile_start = time.time()
    cmd = ["timeout", "180", "./compiler", compile_option, "-o", output_file, input_file]
    if opt_level:  # 只有当opt_level不为空时才添加
        cmd.append(opt_level)
    result = execute(cmd)
    compile_end = time.time()
    compile_time = compile_end - compile_start
    return result.returncode == 0, compile_time

def execute_llvm(input_file, output_file, stdin, stdout, testout, opt_level, time_stats):
    # 编译阶段
    compile_start = time.time()
    if not execute_compilation(input_file, output_file, "-llvm", opt_level)[0]:
        print(f"\033[93mCompile Error on \033[0m{input_file}")
        return 0, 0, 0

    if execute(["clang", output_file, "-c", "-o", "tmp.o", "-w"]).returncode != 0:
        print(f"\033[93mOutput Error on \033[0m{input_file}")
        return 0, 0, 0

    # if execute(["clang", "-static", "tmp.o", "lib/libsysy_x86.a", "lib/libloop_parallel_x86.a"]).returncode != 0:
    if execute(["clang", "-static", "tmp.o", "lib/libsysy_x86.a"]).returncode != 0:
        execute(["rm", "-rf", "tmp.o"])
        print(f"\033[93mLink Error on \033[0m{input_file}")
        return 0, 0, 0

    execute(["rm", "-rf", "tmp.o"])
    compile_end = time.time()
    compile_time = compile_end - compile_start

    # 运行阶段
    run_start = time.time()
    cmd = f"timeout 180 ./a.out {'< '+stdin if stdin != 'none' else ''} > {testout} 2>/dev/null"
    result = execute_with_stdin_out(cmd)
    run_end = time.time()
    run_time = run_end - run_start

    if result == 124:
        print(f"\033[93mTime Limit Exceed on \033[0m{input_file}")
        return 0, compile_time, run_time
    elif result == 139:
        print(f"\033[95mRunTime Error on \033[0m{input_file}")
        return 0, compile_time, run_time

    add_returncode(testout, result)

    if check_file(testout, stdout) == 0:
        print(f"\033[92mAccept \033[0m{input_file}")
        time_stats.append({
            'test_case': os.path.basename(input_file),
            'compile_time': f"{compile_time:.6f}",
            'run_time': f"{run_time:.6f}"
        })
        return 1, compile_time, run_time
    else:
        print(f"\033[91mWrong Answer on \033[0m{input_file}")
        return 0, compile_time, run_time

def execute_asm(input_file, output_file, stdin, stdout, testout, opt_level, time_stats):
    # 编译阶段
    compile_start = time.time()
    if not execute_compilation(input_file, output_file, "-S", opt_level)[0]:
        print(f"\033[93mCompile Error on \033[0m{input_file}")
        return 0, 0, 0

    if execute(["riscv64-unknown-linux-gnu-gcc", output_file, "-c", "-o", "tmp.o", "-w"]).returncode != 0:
        print(f"\033[93mOutput Error on \033[0m{input_file}")
        return 0, 0, 0

    # if execute(["riscv64-unknown-linux-gnu-gcc", "-static", "tmp.o", "lib/libsysy_riscv.a", "lib/libloop_parallel_riscv.a"]).returncode != 0:
    if execute(["riscv64-unknown-linux-gnu-gcc", "-static", "tmp.o", "lib/libsysy_riscv.a"]).returncode != 0:
        execute(["rm", "-rf", "tmp.o"])
        print(f"\033[93mLink Error on \033[0m{input_file}")
        return 0, 0, 0

    execute(["rm", "-rf", "tmp.o"])
    compile_end = time.time()
    compile_time = compile_end - compile_start

    # 运行阶段
    run_start = time.time()
    cmd = f"timeout 180 qemu-riscv64 ./a.out {'< '+stdin if stdin != 'none' else ''} > {testout} 2>/dev/null"
    result = execute_with_stdin_out(cmd)
    run_end = time.time()
    run_time = run_end - run_start

    if result == 124:
        print(f"\033[93mTime Limit Exceed on \033[0m{input_file}")
        return 0, compile_time, run_time
    elif result == 139:
        print(f"\033[95mRunTime Error on \033[0m{input_file}")
        return 0, compile_time, run_time

    add_returncode(testout, result)

    if check_file(testout, stdout) == 0:
        print(f"\033[92mAccept \033[0m{input_file}")
        time_stats.append({
            'test_case': os.path.basename(input_file),
            'compile_time': f"{compile_time:.6f}",
            'run_time': f"{run_time:.6f}"
        })
        return 1, compile_time, run_time
    else:
        print(f"\033[91mWrong Answer on \033[0m{input_file}")
        return 0, compile_time, run_time

def write_time_stats_to_csv(time_stats, step, opt_level):
    if not time_stats:
        return
        
    csv_file = f"{step}_time_stats.csv"
    with open(csv_file, 'w', newline='') as csvfile:
        fieldnames = ['test_case', 'compile_time', 'run_time']
        writer = csv.DictWriter(csvfile, fieldnames=fieldnames)
        
        writer.writeheader()
        for stat in time_stats:
            writer.writerow(stat)
            
    print(f"\nTime statistics written to {csv_file}")

parser = argparse.ArgumentParser()
parser.add_argument('input_folder')
parser.add_argument('output_folder')
parser.add_argument('opt', choices=['0', '1'], default='0', help="optimization level")
parser.add_argument('step', choices=["lexer", "parser", "semant", "llvm", "select", "target"])
args = parser.parse_args()

input_folder = args.input_folder
output_folder = args.output_folder
step = args.step
# 修改opt_level处理：opt=0时不传递参数，opt=1时传递-O1
opt_level = "-O1" if args.opt == '1' else ""

step_to_option = {
    "lexer": "-lexer",
    "parser": "-parser",
    "semant": "-semant",
    "llvm": "-llvm",
    "select": "-select",
    "target": "-S"  
}

ac = 0
total = 0
total_compile_time = 0
total_run_time = 0
time_stats = []

for file in os.listdir(input_folder):
    if file.endswith(".sy"):
        total += 1
        name = file[:-3]
        input_path = f"{input_folder}/{file}"
        output_path = f"{output_folder}/{name}.out"

        stdin_file = f"{input_folder}/{name}.in"
        stdout_file = f"{input_folder}/{name}.out"

        stdin = stdin_file if os.path.exists(stdin_file) else "none"
        testout = "tmp.out"

        compile_option = step_to_option[step]
        
        if step == "llvm":
            output_file = f"{output_folder}/{name}.ll"
            result, compile_time, run_time = execute_llvm(input_path, output_file, stdin, stdout_file, testout, opt_level, time_stats)
            ac += result
            total_compile_time += compile_time
            total_run_time += run_time
        elif step == "target":
            output_file = f"{output_folder}/{name}.s"
            result, compile_time, run_time = execute_asm(input_path, output_file, stdin, stdout_file, testout, opt_level, time_stats)
            ac += result
            total_compile_time += compile_time
            total_run_time += run_time
        else:
            result, compile_time = execute_compilation(input_path, output_path, compile_option, opt_level)
            if result:
                print(f"\033[92mAccept (Compilation) \033[0m{input_path}")
                ac += 1
            else:
                print(f"\033[93mCompile Error on \033[0m{input_path}")
            total_compile_time += compile_time

print(f"{step.upper()}Test-Grade:{ac}/{total}")

# 输出时间统计
print(f"总编译时间: {total_compile_time:.2f} 秒 ({total_compile_time/60:.2f} 分钟)")
print(f"总运行时间: {total_run_time:.2f} 秒 ({total_run_time/60:.2f} 分钟)")
print(f"总时间: {total_compile_time + total_run_time:.2f} 秒 ({(total_compile_time + total_run_time)/60:.2f} 分钟)")
if total_compile_time + total_run_time > 0:
    print(f"编译占比: {(total_compile_time/(total_compile_time + total_run_time))*100:.1f}%")
    print(f"运行占比: {(total_run_time/(total_compile_time + total_run_time))*100:.1f}%")

# Write time statistics to CSV if we have any
if step in ["llvm", "target"]:
    write_time_stats_to_csv(time_stats, step, opt_level)

os.system("rm -f tmp.out a.out")