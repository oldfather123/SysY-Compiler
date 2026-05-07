# Command-line testing tool that performs functional/architectural tests on the compiler based on different parameters
# It will evaluate the generated LLVM intermediate code and target architecture assembly code

import argparse
import json
import logging
import subprocess
import sys
from pathlib import Path


def setup_logger(file_path: Path) -> logging.Logger:
    logger = logging.getLogger()
    logger.setLevel(logging.DEBUG)
    if logger.hasHandlers():
        logger.handlers.clear()

    console_handler = logging.StreamHandler(sys.stdout)
    console_handler.setLevel(logging.INFO)
    console_formatter = logging.Formatter("%(levelname)s [line:%(lineno)d] %(message)s")
    console_handler.setFormatter(console_formatter)

    file_handler = logging.FileHandler(file_path, encoding="utf-8")
    file_handler.setLevel(logging.WARNING)
    file_formatter = logging.Formatter("%(levelname)s %(message)s")
    file_handler.setFormatter(file_formatter)

    logger.addHandler(console_handler)
    logger.addHandler(file_handler)

    return logger


logger = setup_logger(Path.cwd() / "checker_res.log")
cur_case_name = None


def compile_source_code(
    compiler_path: Path,
    source_code_path: Path,
    output_path: Path,
    assembly: bool,
    llvm_ir: bool,
    opt_level: int = 0,
) -> bool:
    """Compile according to the given compilation options"""
    logger.info("  -> Compiling source code")
    cmd = [str(compiler_path), str(source_code_path)]
    cmd.extend(["-o", str(output_path)])
    if assembly:
        cmd.append("-S")
    if llvm_ir:
        cmd.append("-emit-llvm")
    if opt_level > 0:
        # TODO add option here
        pass
    try:
        result = subprocess.run(cmd, text=True, capture_output=True, check=False, encoding="utf-8", timeout=3000)
        # # 显示编译器的输出
        # if result.stdout:
        #     print("=== Compiler stdout ===")
        #     print(result.stdout)
        # if result.stderr:
        #     print("=== Compiler stderr ===")
        #     print(result.stderr)
        if result.returncode != 0:
            logger.error(
                f"  -> Case {cur_case_name} compilation failed! \n\tstdout:{result.stdout}\n\t stderr:{result.stderr}"
            )
            return False
        logger.info("  -> Compilation successful")
        return True
    except subprocess.TimeoutExpired:
        logger.error(f"  -> Case {cur_case_name} compilation timeout! (>3000s)")
        return False
    except Exception as e:
        logger.error(f"  -> Case {cur_case_name} compilation failed! err: {e}")
        return False


def link_llvm_lib(lib_path: Path, ir_path: Path, output_path: Path) -> bool:
    """Link LLVM IR with standard library"""
    logger.info("  -> Linking llvm libraries")
    cmd = ["llvm-link", str(lib_path), str(ir_path), "-S", "-o", str(output_path)]
    result = subprocess.run(cmd, text=True, capture_output=True, check=False, encoding="utf-8")
    if result.returncode != 0:
        logger.error(f"  -> Case {cur_case_name} link failed! \n\tstdout:{result.stdout}\n\t stderr:{result.stderr}")
        return False
    logger.info("  -> Linking llvm successful")
    return True


def run_lli(llvm_path: Path, stdin_path: Path, timeout: int = 100) -> tuple[bool, str, str]:
    """Run lli, return whether the execution is successful, the return code, and the standard output content"""
    cmd = ["bash", "-c", "ulimit -s unlimited; lli " + str(llvm_path)]
    try:
        if stdin_path.exists():
            with open(stdin_path, encoding="utf-8") as f_in:
                result = subprocess.run(
                    cmd,
                    stdin=f_in,
                    text=True,
                    capture_output=True,
                    check=False,
                    encoding="utf-8",
                    timeout=timeout,
                )
        else:
            result = subprocess.run(cmd, text=True, capture_output=True, check=False, encoding="utf-8", timeout=timeout)
        return True, str(result.returncode), result.stdout
    except subprocess.TimeoutExpired:
        logger.error(f"Case {cur_case_name} lli timeout! (>{timeout}s)")
        return False, "timeout", ""
    except Exception as e:
        logger.error(f"Case {cur_case_name} lli failed! err: {e}")
        return False, "", ""


def link_riscv_lib(lib_path: Path, asm_path: Path, output_path: Path) -> bool:
    """Link riscv assembly code with standard library"""
    logger.info("  -> Linking riscv libraries")
    cmd = [
        "riscv64-linux-gnu-gcc",
        "-march=rv64gc",
        "-mabi=lp64d",
        "-static",
        "-o",
        str(output_path),
        str(asm_path),
        str(lib_path),
    ]
    result = subprocess.run(cmd, text=True, capture_output=True, check=False, encoding="utf-8")
    if result.returncode != 0:
        logger.error(f"  -> Case {cur_case_name} link failed! \n\tstdout:{result.stdout}\n\t stderr:{result.stderr}")
        return False
    logger.info("  -> Linking riscv successful")
    return True


def run_riscv_asm(
    bin_path: Path,
    stdin_path: Path,
    timeout: int = 200,
    gdb_debug: bool = False,  # 新增参数：是否启用 GDB 调试
    gdb_port: int = 3333,  # 新增参数：GDB Server 端口
) -> tuple[bool, str, str]:
    """Run riscv assembly code (支持普通执行或 GDB 调试模式)"""
    if gdb_debug:
        # GDB 调试模式：通过 QEMU 启动 GDB Server
        cmd = ["qemu-riscv64", "-g", str(gdb_port), str(bin_path)]  # QEMU 的 -g 参数指定 GDB 调试端口
    else:
        # 普通执行模式
        cmd = ["qemu-riscv64", str(bin_path)]
    try:
        if stdin_path.exists():
            with open(stdin_path, encoding="utf-8") as f_in:
                result = subprocess.run(
                    cmd, stdin=f_in, text=True, capture_output=True, check=False, encoding="utf-8", timeout=timeout
                )
        else:
            result = subprocess.run(cmd, text=True, capture_output=True, check=False, encoding="utf-8", timeout=timeout)
        return True, str(result.returncode), result.stdout
    except subprocess.TimeoutExpired:
        logger.error(f"Case {cur_case_name} riscv timeout! (>{timeout}s)")
        return False, "timeout", ""
    except Exception as e:
        logger.error(f"Case {cur_case_name} riscv failed! err: {e}")
        return False, "", ""


def run_one_function_case(
    compiler_path: Path,
    llvm_lib_path: Path,
    riscv_lib_path: Path,
    case_dir_path: Path,
) -> bool:
    """Run a single test case and return whether it passed"""
    global cur_case_name
    cur_case_name = case_dir_path.name
    compile_llvm_res_path = Path.cwd() / "compile_res.ll"
    compile_riscv_res_path = Path.cwd() / "compile_res.s"
    link_llvm_res_path = Path.cwd() / "link_llvm_res.ll"
    link_riscv_res_path = Path.cwd() / "link_riscv_res.bin"

    logger.info(f"  -> case {cur_case_name} check start")

    try:
        if not case_dir_path.is_dir():
            logger.warning(f"⚠️ case path {cur_case_name} is not a directory")
            return True  # Skip this case, don't count as failure

        # 1. Compile
        if not compile_source_code(
            compiler_path,
            case_dir_path / "input.txt",
            Path.cwd() / "compile_res",
            True,
            True,
            0,
        ):
            logger.error(f"❌ Case {cur_case_name} Failed")
            return False

        # 2. Link libraries
        if not link_llvm_lib(llvm_lib_path, compile_llvm_res_path, link_llvm_res_path):
            logger.error(f"❌ Case {cur_case_name} Failed")
            return False

        # 3. Run lli
        ok, ret_code, stdout = run_lli(link_llvm_res_path, case_dir_path / "in")
        if not ok:
            logger.error(f"❌ Case {cur_case_name} Failed")
            return False

        # 4. Get expected results
        with open(case_dir_path / "result.json", encoding="utf-8") as f:
            expected_data = json.load(f)
        expected_return_code, expected_stdout = (
            str(expected_data["return"]),
            expected_data["out"],
        )

        # 5. Compare lli execution results
        stdout = stdout.strip().replace("\r\n", "\n")
        expected_stdout = expected_stdout.strip().replace("\r\n", "\n")
        if ret_code != expected_return_code or stdout != expected_stdout:
            logger.error(f"Case {cur_case_name} lli res not match.")
            if ret_code != expected_return_code:
                logger.error(f"Expected return code: {expected_return_code}, but got {ret_code}")
            if stdout != expected_stdout:
                logger.error(f"Expected stdout: {expected_stdout}, but got {stdout}")
            logger.error(f"❌ Case {cur_case_name} Failed")
            return False

        # 6. link riscv assembly code
        if not link_riscv_lib(riscv_lib_path, compile_riscv_res_path, link_riscv_res_path):
            logger.error(f"❌ Case {cur_case_name} Failed")
            return False

        # 7. run riscv assembly code
        ok, ret_code, stdout = run_riscv_asm(link_riscv_res_path, case_dir_path / "in")
        if not ok:
            logger.error(f"❌ Case {cur_case_name} Failed")
            return False

        # 8. compare riscv and lli results
        stdout = stdout.strip().replace("\r\n", "\n")
        expected_stdout = expected_stdout.strip().replace("\r\n", "\n")
        if ret_code != expected_return_code or stdout != expected_stdout:
            logger.error(f"Case {cur_case_name} riscv res not match.")
            if ret_code != expected_return_code:
                logger.error(f"Expected return code: {expected_return_code}, but got {ret_code}")
            if stdout != expected_stdout:
                logger.error(f"Expected stdout: {expected_stdout}, but got {stdout}")
            logger.error(f"❌ Case {cur_case_name} Failed")
            return False

        logger.info(f"✅ Case {cur_case_name} passed")
        return True

    finally:
        # Clean up temporary files
        if compile_llvm_res_path.exists():
            compile_llvm_res_path.unlink()
        if link_llvm_res_path.exists():
            link_llvm_res_path.unlink()
        if compile_riscv_res_path.exists():
            compile_riscv_res_path.unlink()
        if link_riscv_res_path.exists():
            link_riscv_res_path.unlink()


def check_function(args: argparse.Namespace):
    global cur_case_name
    compiler_path: Path = args.compiler.absolute()
    llvm_lib_path: Path = args.llvm_lib.absolute()
    riscv_lib_path: Path = args.riscv_lib.absolute()
    arm_lib_path: Path = args.arm_lib.absolute()
    test_dir_path: Path = args.testdir.absolute()
    case_names: list[str] = args.case
    exclude_names: list[str] = args.exclude
    logger.info("------- Start Function Test -------")
    if not test_dir_path.exists() or not test_dir_path.is_dir():
        logger.error("❌ test dir invalid")
        sys.exit(1)
    if not llvm_lib_path.exists() or not llvm_lib_path.is_file():
        logger.error("❌ llvm lib path invalid")
        sys.exit(1)
    if not riscv_lib_path.exists() or not riscv_lib_path.is_file():
        logger.error("❌ riscv lib path invalid")
        sys.exit(1)
    if not arm_lib_path.exists() or not arm_lib_path.is_file():
        logger.error("❌ arm lib path invalid")
        sys.exit(1)
    if not compiler_path.exists() or not compiler_path.is_file():
        logger.error("❌ compiler path invalid")
        sys.exit(1)

    if len(case_names) > 0:
        for case_name in case_names:
            case_dir = test_dir_path / case_name
            if not case_dir.exists() or not case_dir.is_dir():
                logger.error(f"❌ case {case_name} not found")
                sys.exit(1)

    case_num = 0
    bad_cases = []

    # Determine which cases to run
    if len(case_names) > 0:
        # Run only the specified cases
        case_num = len(case_names)
        for case_name in case_names:
            case_dir_path = test_dir_path / case_name
            if not run_one_function_case(compiler_path, llvm_lib_path, riscv_lib_path, case_dir_path):
                bad_cases.append(case_dir_path.name)
    else:
        # Run all cases
        for case_dir_path in test_dir_path.iterdir():
            if case_dir_path.name in exclude_names:
                continue
            case_num += 1

            # Run single test case
            if not run_one_function_case(compiler_path, llvm_lib_path, riscv_lib_path, case_dir_path):
                bad_cases.append(case_dir_path.name)

    if len(bad_cases) > 0:
        logger.critical(f"Passed {case_num - len(bad_cases)} / {case_num} cases. Bad cases: {bad_cases}")
        sys.exit(1)
    else:
        logger.info(f"Passed all {case_num} cases")


def main() -> None:
    parser = argparse.ArgumentParser(description="Compiler automated testing command-line tool")
    subparser = parser.add_subparsers(dest="command")

    # Function testing
    function_parser = subparser.add_parser("function", help="Function testing")
    function_parser.add_argument(
        "--compiler", "-c", type=Path, default=Path("..") / "build" / "compiler", help="Compiler path"
    )
    function_parser.add_argument(
        "--llvm-lib",
        "-l",
        type=Path,
        default=Path("..") / "lib" / "lib.ll",
        help="LLVM library file path",
    )
    function_parser.add_argument(
        "--riscv-lib",
        type=Path,
        default=Path("..") / "lib" / "lib.riscv.a",
        help="RISC-V library file path",
    )
    function_parser.add_argument(
        "--arm-lib",
        type=Path,
        default=Path("..") / "lib" / "lib.arm.a",
        help="ARM library file path",
    )
    function_parser.add_argument(
        "--testdir",
        "-t",
        type=Path,
        default=Path.cwd() / "cases",
        help="Function test case directory",
    )
    function_parser.add_argument(
        "--case",
        type=str,
        nargs="*",
        default=[],
        help="specific cases to check (can specify multiple)",
    )
    function_parser.add_argument(
        "--exclude",
        type=str,
        nargs="*",
        default=[],
        help="cases to exclude (can specify multiple)",
    )
    function_parser.set_defaults(func=check_function)
    args = parser.parse_args()
    if hasattr(args, "func"):
        args.func(args)
    else:
        parser.print_help()


if __name__ == "__main__":
    main()
