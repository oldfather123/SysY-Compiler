#!/usr/bin/env python3
"""
Compiler Test Runner
Usage:
  runner.py [-f TOML_FILE] [-c TEST_CASE] [--generate-config] [--verbose] [--compiler COMPILER_PATH] [--emulator EMULATOR_PATH] [--clean] [--ir] [--ir_exec] [--token] [--ast] [--ir_diff]
"""

import os
import sys
import toml
import glob
import time
import argparse
import subprocess
import json
import re
from datetime import datetime
# import shutil
import difflib
from shutil import get_terminal_size
from colorama import Fore, Style, init

USE_REAL_ENV = False  # if you are on riscv64 machine, set this to True
ROOT_DIR = os.path.dirname(os.path.abspath(__file__))  # root dir of project
DEFAULT_COMPILER = os.path.join(ROOT_DIR, "compiler")  # your compiler
DEFAULT_EMULATOR = None if USE_REAL_ENV else "qemu-riscv64"
CXX = "gcc" if USE_REAL_ENV else "riscv64-linux-gnu-gcc"
SYLIB = os.path.join(ROOT_DIR, "test", "lib", "libsy_riscv.a")
SYLIB_X86 = os.path.join(ROOT_DIR, "test", "lib", "sylib_x86.c")
DEFAULT_TOML = "test/config.toml"  # default config TOML
TIME_OUT = 1000  # second


def parse_debug_passes(debug_h_path):
    """
    Parse debug.h file to extract enabled passes.

    Args:
        debug_h_path (str): path to debug.h file

    Returns:
        dict: dictionary of pass names and their values
    """
    passes = {}

    if not os.path.exists(debug_h_path):
        return passes

    with open(debug_h_path, 'r') as f:
        content = f.read()

    # Find all #define statements for passes
    define_pattern = r'#define\s+(OPEN_\w+|BK_PASS_\w+)\s+(\d+)'
    matches = re.findall(define_pattern, content)

    for match in matches:
        pass_name, value = match
        passes[pass_name] = int(value)

    return passes


def save_timing_log(test_results, passes_info, log_dir, log_name, command_args=None):
    """
    Save timing and pass information to log directory.

    Args:
        test_results (list): list of test results with timing
        passes_info (dict): dictionary of enabled passes
        log_dir (str): base log directory
        log_name (str): name for the log folder (timestamp or custom name)
        command_args (list): command line arguments used to run the test
    """
    # Create log directory
    log_subdir = os.path.join(log_dir, log_name)
    os.makedirs(log_subdir, exist_ok=True)

    # Save timing data
    time_log_path = os.path.join(log_subdir, "time.log")
    with open(time_log_path, 'w') as f:
        for result in test_results:
            if 'cc_time_us' in result:
                f.write(f"{result['name']}: {result['cc_time_us']}\n")

    # Save pass information
    pass_log_path = os.path.join(log_subdir, "passes.log")
    with open(pass_log_path, 'w') as f:
        json.dump(passes_info, f, indent=2)

    # Save command line arguments
    if command_args:
        command_log_path = os.path.join(log_subdir, "command.log")
        with open(command_log_path, 'w') as f:
            f.write("# Command used to run this test\n")
            f.write("python3 test.py " + " ".join(command_args) + "\n")
            f.write(f"\n# Timestamp: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")

    print(f"Saved timing and pass info to: {log_subdir}")


def parse_time_to_microseconds(time_str):
    """
    Parse time string in format "0H-0M-0S-93047us" to microseconds.

    Args:
        time_str (str): time string

    Returns:
        int: time in microseconds
    """
    pattern = r'(\d+)H-(\d+)M-(\d+)S-(\d+)us'
    match = re.match(pattern, time_str)

    if not match:
        return 0

    h, m, s, us = map(int, match.groups())
    total_us = ((h * 3600 + m * 60 + s) * 1000000) + us
    return total_us


def run_compiler_test(test_dir, compiler, emulator, verbose=False, ir=False, ir_exec=False, token=False, ast=False, ir_diff=False, llvm_ir=False, llvm_test=False, llc_riscv=False, execonly=False):
    """
    Single Folder Test.

    Args:
        test_dir (str): test case directory
        compiler (str): your compiler path
        emulator (str): qemu path
        verbose (bool, optional): verbose output, if open, will print debug info
        ir (bool, optional): show ir output to .ir
        ir_exec (bool, optional): execute ir output to .ir_res
        token (bool, optional): show token output to .tk
        ast (bool, optional): show ast output to .json
        ir_diff (bool, optional): diff ir output file with stdout, else diff asm output
        llvm_ir (bool, optional): generate LLVM IR output to .ll
        llvm_test (bool, optional): compile and test LLVM IR using LLVM toolchain
        llc_riscv (bool, optional): use llc to generate riscv64 .s from LLVM IR and run

    Returns:
        tuple: (bool, str), which is (success, message)
    """

    test_name = os.path.basename(test_dir)
    if verbose:
        print(f"[\033[34mDEBG\033[0m] Running test: {test_name}")

    # 1. find source file
    # source_files = glob.glob(os.path.join(test_dir, "*.sy")) + glob.glob(os.path.join(test_dir, "*.c"))
    source_files = glob.glob(os.path.join(test_dir, "*.sy"))
    if not source_files:
        # return False, "No source file (*.sy or *.c)"
        return False, "No source file (*.sy)"

    source_file = source_files[0]
    base_name = os.path.splitext(os.path.basename(source_file))[0]

    # 2. get input & ref
    input_file = os.path.join(test_dir, f"{base_name}.in")
    expected_file = os.path.join(test_dir, f"{base_name}.out")
    asm_file = os.path.join(test_dir, f"{base_name}.s")
    ir_res_file = os.path.join(test_dir, f"{base_name}.ir_res")
    llvm_ir_file = os.path.join(test_dir, f"{base_name}.ll")
    llvm_exe_file = os.path.join(test_dir, f"{base_name}_llvm.exe")
    llvm_res_file = os.path.join(test_dir, f"{base_name}_llvm.res")
    res_file = os.path.join(test_dir, f"{base_name}.res")
    res_time_file = os.path.join(test_dir, f"{base_name}.time")
    llc_riscv_s_file = os.path.join(test_dir, f"{base_name}_llc_riscv.s")

    # 3. compile by your own comiler
    if not execonly:
        compile_cmd = [compiler, "-S", "-o", asm_file, source_file]
        if ir:
            compile_cmd.append("--ir")
        if ir_exec:
            compile_cmd.append("--execute_ir")
        if token:
            compile_cmd.append("--token")
        if ast:
            compile_cmd.append("--ast")
        if llvm_ir or llc_riscv:
            compile_cmd.append("--llvm-ir")

        try:
            result = subprocess.run(
                compile_cmd,
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                cwd=test_dir,
                timeout=TIME_OUT
            )

            stdout = result.stdout.decode().strip()
            stderr = result.stderr.decode().strip()

            def print_verbose_lines(output, color_code="34"):
                if output:
                    for line in output.splitlines():
                        print(f"[\033[{color_code}mDEBG\033[0m] {line}")

            if verbose:
                if stdout:
                    print_verbose_lines(stdout, "34")
                if stderr:
                    print_verbose_lines(stderr, "31")

            if result.returncode != 0:
                return False, f"Compile failed with return code {result.returncode}"

        except subprocess.TimeoutExpired:
            if verbose:
                print("[\033[31mDEBG\033[0m] Compilation timed out")
            return False, "Compilation timed out"

        except Exception as e:
            if verbose:
                print(f"[\033[31mDEBG\033[0m] Compile error: {str(e)}")
            return False, f"Compile error: {str(e)}"

    # 3.5. LLVM IR test (if enabled)
    if llvm_test and llvm_ir and os.path.exists(llvm_ir_file):
        try:
            # Compile LLVM IR to executable using LLVM toolchain
            # First, compile LLVM IR to object file
            llc_cmd = ["llc", "-march=x86-64", "-filetype=obj", llvm_ir_file, "-o", llvm_ir_file.replace(".ll", ".o")]
            result = subprocess.run(llc_cmd, cwd=test_dir, capture_output=True, text=True, timeout=TIME_OUT)

            if result.returncode != 0:
                if verbose:
                    print(f"[\033[31mDEBG\033[0m] LLVM IR compilation failed: {result.stderr}")
                return False, f"LLVM IR compilation failed: {result.stderr}"

            # Link with C runtime and x86 SyLib to create executable
            if os.path.exists(SYLIB_X86):
                clang_cmd = ["clang", llvm_ir_file.replace(".ll", ".o"), SYLIB_X86, "-o", llvm_exe_file, "-lm"]
            else:
                clang_cmd = ["clang", llvm_ir_file.replace(".ll", ".o"), "-o", llvm_exe_file, "-lm"]
            result = subprocess.run(clang_cmd, cwd=test_dir, capture_output=True, text=True, timeout=TIME_OUT)

            if result.returncode != 0:
                if verbose:
                    print(f"[\033[31mDEBG\033[0m] LLVM IR linking failed: {result.stderr}")
                return False, f"LLVM IR linking failed: {result.stderr}"

            # Execute LLVM IR compiled program
            try:
                with open(llvm_res_file, 'w') as fout:
                    if os.path.exists(input_file):
                        fin = open(input_file, 'r')

                    result = subprocess.run(
                        [llvm_exe_file],
                        stdin=fin if os.path.exists(input_file) else None,
                        stdout=fout,
                        stderr=subprocess.PIPE,
                        cwd=test_dir,
                        timeout=TIME_OUT,
                    )

                # Write return code to LLVM result file
                with open(llvm_res_file, 'a') as f:
                    f.write(f"\n{result.returncode}\n")

                if verbose:
                    print(f"[\033[32mDEBG\033[0m] LLVM IR execution completed with return code: {result.returncode}")

            except subprocess.TimeoutExpired:
                if verbose:
                    print("[\033[31mDEBG\033[0m] LLVM IR execution timed out")
                return False, "LLVM IR execution timed out"
            except Exception as e:
                if verbose:
                    print(f"[\033[31mDEBG\033[0m] LLVM IR execution error: {str(e)}")
                return False, f"LLVM IR execution error: {str(e)}"

        except subprocess.TimeoutExpired:
            if verbose:
                print("[\033[31mDEBG\033[0m] LLVM IR compilation timed out")
            return False, "LLVM IR compilation timed out"
        except Exception as e:
            if verbose:
                print(f"[\033[31mDEBG\033[0m] LLVM IR test error: {str(e)}")
            return False, f"LLVM IR test error: {str(e)}"

    # 3.6. llc riscv64 test (if enabled)
    if llc_riscv and os.path.exists(llvm_ir_file):
        try:
            llc_cmd = [
                "llc", "-march=riscv64", "-mtriple=riscv64-unknown-linux-gnu",
                "-mattr=+m,+a,+f,+d",
                # "-regalloc=fast",
                # "-O0",
                llvm_ir_file, "-o", llc_riscv_s_file
            ]
            result = subprocess.run(llc_cmd, cwd=test_dir, capture_output=True, text=True, timeout=TIME_OUT)
            if result.returncode != 0:
                if verbose:
                    print(f"[\033[31mDEBG\033[0m] llc riscv64 failed: {result.stderr}")
                return False, f"llc riscv64 failed: {result.stderr}"
            # 用 llc 生成的 .s 文件替换 asm_file
            asm_file = llc_riscv_s_file
        except subprocess.TimeoutExpired:
            if verbose:
                print("[\033[31mDEBG\033[0m] llc riscv64 timed out")
            return False, "llc riscv64 timed out"
        except Exception as e:
            if verbose:
                print(f"[\033[31mDEBG\033[0m] llc riscv64 error: {str(e)}")
            return False, f"llc riscv64 error: {str(e)}"

    # 4. execute
    # as & ld by riscv64-unknown-elf-gcc
    if not execonly:
        compile_cmd = [CXX, "-o", os.path.join(test_dir, f"{base_name}.exe"), asm_file, SYLIB, "--static", "-fPIC"]
        subprocess.run(compile_cmd, cwd=test_dir)

    # run by qemu, -L /usr/riscv64-linux-gnu
    if emulator is not None:
        qemu_run_cmd = [emulator, "-L", "/usr/riscv64-linux-gnu", os.path.join(test_dir, f"{base_name}.exe")]

        try:
            with open(res_file, 'w') as fout, open(res_time_file, 'w') as ferr:
                if os.path.exists(input_file):
                    fin = open(input_file, 'r')

                result = subprocess.run(
                    qemu_run_cmd,
                    stdin=fin if os.path.exists(input_file) else None,
                    stdout=fout,
                    stderr=ferr,
                    cwd=test_dir,
                    timeout=TIME_OUT,
                )

            # write return code to res file
            with open(res_file, 'a') as f:
                f.write(f"\n{result.returncode}\n")

        except Exception as e:
            return False, f"Run error: {str(e)}"
    else:
        # use real env
        qemu_run_cmd = [os.path.join(test_dir, f"{base_name}.exe")]

        try:
            with open(res_file, 'w') as fout, open(res_time_file, 'w') as ferr:
                if os.path.exists(input_file):
                    fin = open(input_file, 'r')

                result = subprocess.run(
                    qemu_run_cmd,
                    stdin=fin if os.path.exists(input_file) else None,
                    stdout=fout,
                    stderr=ferr,
                    cwd=test_dir,
                    timeout=TIME_OUT,
                )

            # write return code to res file
            with open(res_file, 'a') as f:
                f.write(f"\n{result.returncode}\n")

        except Exception as e:
            return False, f"Run error: {str(e)}"

    # 5. score
    if not os.path.exists(expected_file):
        return False, "Missing expected output file"

    with open(expected_file, "r") as f1:
        expected = [line for line in f1.read().splitlines() if line.strip()]
        if ir_diff:
            with open(ir_res_file, "r") as f2:  # if ir_level, use ir_res
                actual = [line for line in f2.read().splitlines() if line.strip()]
        elif llvm_test and os.path.exists(llvm_res_file):
            with open(llvm_res_file, "r") as f2:  # if llvm_test, use llvm_res
                actual = [line for line in f2.read().splitlines() if line.strip()]
        else:
            with open(res_file, "r") as f2:
                actual = [line for line in f2.read().splitlines() if line.strip()]

    if expected == actual:
        return True, ""

    # Optional: print difference
    diff = list(difflib.unified_diff(
        expected, actual,
        fromfile="expected",
        tofile="actual",
        lineterm=""
    ))

    if ir_diff:
        diff_file = os.path.join(test_dir, "ir_diff.log")
    elif llvm_test:
        diff_file = os.path.join(test_dir, "llvm_diff.log")
    else:
        diff_file = os.path.join(test_dir, "backend_diff.log")

    with open(diff_file, "w") as f:
        f.write("\n".join(diff))

    return False, "different with expected output."


def load_test_cases_from_toml(toml_file):
    """
    Get Test Cases From TOML Config.

    Args:
        toml_file (str): path to toml config file

    Returns:
        tuple: (list of test cases, dict of case to group mapping)
    """

    if toml is None:
        print("Error: TOML module not installed. Install with: pip install toml")
        sys.exit(1)

    with open(toml_file, "r") as f:
        config = toml.load(f)

    test_cases = []
    case_groups = {}

    # get test cases in test/custom
    for case in config.get("custom", []):
        test_path = os.path.join("test", "custom", case)
        test_cases.append(test_path)
        case_groups[case] = "custom"

    # get test cases in test/formal
    formal = config.get("formal", {})
    for case in formal.get("performance", []):
        test_path = os.path.join("test", "formal", "performance", case)
        test_cases.append(test_path)
        case_groups[case] = "performance"

    basic = formal.get("basic", {})
    for case in basic.get("functional", []):
        test_path = os.path.join("test", "formal", "basic", "functional", case)
        test_cases.append(test_path)
        case_groups[case] = "functional"

    for case in basic.get("h_functional", []):
        test_path = os.path.join("test", "formal", "basic", "h_functional", case)
        test_cases.append(test_path)
        case_groups[case] = "h_functional"

    # get test cases in test/formal/extra
    for case in formal.get("extra", []):
        test_path = os.path.join("test", "formal", "extra", case)
        test_cases.append(test_path)
        case_groups[case] = "extra"

    return test_cases, case_groups


def generate_full_config(output_file):
    """
    Generate Config TOML.

    Args:
        output_file (str): path to output file
    """

    if toml is None:
        print("Error: TOML module not installed. Install with: pip install toml")
        sys.exit(1)

    config = {"custom": []}
    formal_config = {"performance": []}
    basic_config = {"functional": [], "h_functional": []}

    # scan test/custom
    custom_dir = os.path.join("test", "custom")
    if os.path.exists(custom_dir):
        for item in sorted(os.listdir(custom_dir)):
            if os.path.isdir(os.path.join(custom_dir, item)):
                config["custom"].append(item)

    # scan test/formal/performance
    perf_dir = os.path.join("test", "formal", "performance")
    if os.path.exists(perf_dir):
        for item in sorted(os.listdir(perf_dir)):
            if os.path.isdir(os.path.join(perf_dir, item)):
                formal_config["performance"].append(item)

    # scan test/formal/basic/functional
    func_dir = os.path.join("test", "formal", "basic", "functional")
    if os.path.exists(func_dir):
        for item in sorted(os.listdir(func_dir)):
            if os.path.isdir(os.path.join(func_dir, item)):
                basic_config["functional"].append(item)

    # scan test/formal/basic/h_functional
    hfunc_dir = os.path.join("test", "formal", "basic", "h_functional")
    if os.path.exists(hfunc_dir):
        for item in sorted(os.listdir(hfunc_dir)):
            if os.path.isdir(os.path.join(hfunc_dir, item)):
                basic_config["h_functional"].append(item)

    # scan test/formal/extra
    extra_dir = os.path.join("test", "formal", "extra")
    if os.path.exists(extra_dir):
        for item in sorted(os.listdir(extra_dir)):
            if os.path.isdir(os.path.join(extra_dir, item)):
                formal_config.setdefault("extra", []).append(item)

    # combine them
    config["formal"] = formal_config
    config["formal"]["basic"] = basic_config

    os.makedirs(os.path.dirname(output_file), exist_ok=True)
    with open(output_file, "w") as f:
        toml.dump(config, f)

    print(f"Generated full config at: {output_file}")
    print(f"Total test cases: {len(config['custom']) + len(formal_config['performance']) + len(basic_config['functional']) + len(basic_config['h_functional'])}")


def main():
    parser = argparse.ArgumentParser(description="Compiler Test Runner")
    parser.add_argument("-f", "--file", default=DEFAULT_TOML, help=f"TOML config file (default: {DEFAULT_TOML})")
    parser.add_argument("-c", "--case", help="Run single test case directory")
    parser.add_argument("--generate-config", action="store_true", help="Generate full config.toml with all test cases")
    parser.add_argument("--verbose", action="store_true", help="Enable verbose output")
    parser.add_argument("--compiler", default=DEFAULT_COMPILER, help=f"Compiler path (default: {DEFAULT_COMPILER})")
    parser.add_argument("--emulator", default=DEFAULT_EMULATOR, help=f"Emulator path (default: {DEFAULT_EMULATOR})")
    parser.add_argument("--clean", action="store_true", help="Clean up generated files")
    parser.add_argument("--ir", action="store_true", help="Open IR Level Output")
    parser.add_argument("--ir_exec", action="store_true", help="Execute IR Level Output")
    parser.add_argument("--token", action="store_true", help="Open Token Level Output")
    parser.add_argument("--ast", action="store_true", help="Open AST Level Output")
    parser.add_argument("--ir_diff", action="store_true", help="Diff IR Level Output with Stdout")
    parser.add_argument("--llvm-ir", action="store_true", help="Generate LLVM IR output")
    parser.add_argument("--llvm-test", action="store_true", help="Test LLVM IR using LLVM toolchain (requires llc and clang)")
    parser.add_argument("--llc-riscv", action="store_true", help="Use llc to generate riscv64 .s from LLVM IR and run")
    parser.add_argument("--time", action="store_true", help="Show total cc time from .time files")
    parser.add_argument("-n", "--repeat", type=int, default=1, help="Number of times to repeat each test case")
    parser.add_argument("--save-log", action="store_true", help="Save timing and pass information to timestamped log directory")
    parser.add_argument("--log-name", type=str, help="Custom name for log directory (default: timestamp)")

    args = parser.parse_args()

    init(autoreset=True)

    # log_file = open("test/summary.log", "w")
    log_file = open(os.path.join(ROOT_DIR, "test/summary.log"), "w")

    # Clean Cache Files
    if args.clean:
        extensions = [".ir", ".ir_res", ".json", ".tk", ".s", ".log", ".res", ".exe", ".time", ".ll", ".o", "_llvm.exe", "_llvm.res"]
        print("Cleaning up generated files...")
        for root, _, files in os.walk("test"):
            # Skip cleaning test/log directory and its subdirectories
            if "test/log" in root or root.endswith("test/log"):
                continue
            for file in files:
                if any(file.endswith(ext) for ext in extensions):
                    filepath = os.path.join(root, file)
                    try:
                        os.remove(filepath)
                        # print(f"Removed: {filepath}")
                    except Exception as e:
                        print(f"Error removing {filepath}: {e}")
        print("Cleanup completed.")
        print()
        if len(sys.argv) == 2 and sys.argv[1] == "--clean":
            return

    # --generate-config
    if args.generate_config:
        generate_full_config(args.file)
        return

    # get test cases
    test_dirs = []
    case_groups = {}
    if args.case:
        test_dirs = [os.path.abspath(args.case)]
        # For single case, try to determine group from path
        case_name = os.path.basename(args.case)
        if "performance" in args.case:
            case_groups[case_name] = "performance"
        elif "h_functional" in args.case:
            case_groups[case_name] = "h_functional"
        elif "functional" in args.case:
            case_groups[case_name] = "functional"
        elif "custom" in args.case:
            case_groups[case_name] = "custom"
        elif "extra" in args.case:
            case_groups[case_name] = "extra"
        else:
            case_groups[case_name] = "unknown"
    else:
        if not os.path.exists(args.file):
            print(f"Error: Config file not found: {args.file}")
            sys.exit(1)
        test_cases, case_groups = load_test_cases_from_toml(args.file)
        test_dirs = [os.path.join(ROOT_DIR, d) for d in test_cases]

    # time all
    total_time = time.time()

    total_cc_time_us = 0  # 累计所有cc时间（微秒）
    # run all test cases
    status_list = []
    results = []

    # Get log directory name (custom or timestamp)
    if args.log_name:
        log_dir_name = args.log_name
    else:
        log_dir_name = datetime.now().strftime("%Y%m%d_%H%M%S")

    # Parse debug.h passes if save-log is enabled
    passes_info = {}
    if args.save_log:
        debug_h_path = os.path.join(ROOT_DIR, "debug.h")
        passes_info = parse_debug_passes(debug_h_path)

    for test_dir in test_dirs:
        if not os.path.isdir(test_dir):
            warning_msg = f"\033[33m[WARNING] Missing test directory: {test_dir}\033[0m"
            print(warning_msg)
            if log_file:
                log_file.write(warning_msg + "\n")
            continue

        # Run test multiple times if -n is specified
        test_times = []
        test_cc_times = []
        test_success = False
        test_message = ""

        for repeat_idx in range(args.repeat):
            start = time.time()
            success, message = run_compiler_test(
                test_dir,
                args.compiler,
                args.emulator,
                verbose=args.verbose,
                ir=args.ir,
                ir_exec=args.ir_exec,
                token=args.token,
                ast=args.ast,
                ir_diff=args.ir_diff,
                llvm_ir=getattr(args, 'llvm_ir', False) or getattr(args, 'llc_riscv', False),
                llvm_test=getattr(args, 'llvm_test', False),
                llc_riscv=getattr(args, 'llc_riscv', False),
                execonly=(repeat_idx != 0)
            )
            run_time = time.time() - start
            test_times.append(run_time)

            if repeat_idx == 0:  # Use first run result for overall success
                test_success = success
                test_message = message

            # Parse cc time for this run
            dir_name = test_dir.split("/")[-1]
            if os.path.exists(os.path.join(test_dir, f"{dir_name}.time")):
                with open(os.path.join(test_dir, f"{dir_name}.time"), "r") as f:
                    lines = f.readlines()
                    for line in lines:
                        if line.startswith("TOTAL:"):
                            try:
                                parts = line.strip().split(": ")[-1]
                                cc_time_us = parse_time_to_microseconds(parts)
                                test_cc_times.append(cc_time_us)
                                break
                            except Exception:
                                pass

        # Calculate average times
        avg_run_time = sum(test_times) / len(test_times)
        avg_cc_time_us = sum(test_cc_times) / len(test_cc_times) if test_cc_times else 0
        total_cc_time_us += avg_cc_time_us

        if test_success:
            status = f"{Fore.GREEN}PASS{Style.RESET_ALL}"
            status_list.append(1)
        else:
            status = f"{Fore.RED}FAIL{Style.RESET_ALL}"
            status_list.append(0)
            if log_file:
                log_file.write(f"[FAIL] {os.path.basename(test_dir)}\n")
                log_file.write(test_message + "\n\n")

        result_data = {
            'name': os.path.basename(test_dir),
            'status': status,
            'time': f"{avg_run_time * 1000:.2f} ms",
            'success': test_success,
            'message': test_message,
            'group': case_groups.get(os.path.basename(test_dir), 'unknown'),
            'cc_time_us': int(avg_cc_time_us)
        }
        results.append(result_data)

        # Display timing info
        time_of_cc = ""
        if avg_cc_time_us > 0:
            h = int(avg_cc_time_us // 3600000000)
            m = int((avg_cc_time_us % 3600000000) // 60000000)
            s = int((avg_cc_time_us % 60000000) // 1000000)
            us = int(avg_cc_time_us % 1000000)
            time_of_cc = f"{h}H-{m}M-{s}S-{us}us"

        # Show repeat info if multiple runs
        repeat_info = f" (x{args.repeat})" if args.repeat > 1 else ""
        print(f"[{status}] {os.path.basename(test_dir):<30} {f'{avg_run_time * 1000:.8f} ms':>30} {f'{time_of_cc}':>30}{repeat_info}")
        if test_message and args.verbose and test_success:
            print(f"  {test_message}")

    # Save timing and pass logs if requested
    if args.save_log:
        log_dir = os.path.join(ROOT_DIR, "test", "log")
        # Reconstruct command arguments (excluding script name)
        command_args = sys.argv[1:]
        save_timing_log(results, passes_info, log_dir, log_dir_name, command_args)

    # Sort results by runtime and write to log
    if log_file:
        sorted_results = sorted(results, key=lambda x: float(x['time'].replace(' ms', '')), reverse=True)
        log_file.write("\n=== Test Results Sorted by Runtime (High to Low) ===\n")
        for res in sorted_results:
            status_text = "PASS" if res['success'] else "FAIL"
            log_file.write(f"[{status_text}] {res['name']:<30} {res['time']:>15} ({res['group']})\n")
        log_file.close()

    # print table
    def print_table(results):
        from shutil import get_terminal_size
        term_width = get_terminal_size().columns

        max_name_len = max(len(res['name']) for res in results)
        padding = 4
        col_width = max_name_len + padding
        cols_per_line = max(1, term_width // col_width)

        total_items = len(results)

        idx = 0
        while idx < total_items:
            line = ""
            for col in range(cols_per_line):
                if idx >= total_items:
                    break
                res = results[idx]
                name = res['name']
                color = Fore.GREEN if res['success'] else Fore.RED
                line += f"{color}{name.ljust(col_width)}{Style.RESET_ALL}"
                idx += 1
            print(line)

    # print("\n\n>>> Test Results <<<")
    print("\n\n" + Fore.YELLOW + ">>> Results Overview <<<".center(get_terminal_size().columns) + Style.RESET_ALL)
    print()
    print_table(results)
    print()

    # print summary
    print("\n@ Test Summary")
    print(f"  Total:  {len(status_list)}")
    print(f"  {Fore.GREEN}Passed: {sum(status_list)}{Style.RESET_ALL}")
    print(f"  {Fore.RED}Failed: {len(status_list) - sum(status_list)}{Style.RESET_ALL}")

    total_time = time.time() - total_time
    print(f"  Time of Whole Test:   {total_time:.2f} seconds")
    # 输出累计cc时间（仅--time时）
    if args.time:
        print(f"  Total cc time: {total_cc_time_us / 1e6:.3f} seconds")

    sys.exit(0 if sum(status_list) == len(status_list) else 1)


if __name__ == "__main__":
    main()
