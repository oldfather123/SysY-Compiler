import subprocess, os, shutil, signal, time
from typing import Union, Optional
import pandas as pd

BUILD_RELEASE_DIR = "build/release/"
BUILD_DEBUG_DIR = "build/debug/"
FUNCTIONAL_TEST_DIR = "test_cases/functional/"
PERFORMANCE_TEST_DIR = "test_cases/performance/"
CUSTOM_TEST_DIR = "test_cases/custom/"

COMPILE_TIME_LIMIT_DEF = 5  # 编译超时时间（默认）
RUN_TIME_LIMIT_DEF = 5  # 运行超时时间（默认）
COMPILE_TIME_LIMIT_EXT = 60  # 编译超时时间（加长）
RUN_TIME_LIMIT_EXT = 60  # 运行超时时间（加长）

compile_time_limit, run_time_limit = COMPILE_TIME_LIMIT_DEF, RUN_TIME_LIMIT_DEF


class Styles:
    # Foreground colors
    BLACK = "\033[0;30m"
    RED = "\033[0;31m"
    GREEN = "\033[0;32m"
    YELLOW = "\033[0;33m"
    BLUE = "\033[0;34m"
    MAGENTA = "\033[0;35m"
    CYAN = "\033[0;36m"
    WHITE = "\033[0;37m"

    # Bright foreground colors
    BRIGHT_BLACK = "\033[1;30m"
    BRIGHT_RED = "\033[1;31m"
    BRIGHT_GREEN = "\033[1;32m"
    BRIGHT_YELLOW = "\033[1;33m"
    BRIGHT_BLUE = "\033[1;34m"
    BRIGHT_MAGENTA = "\033[1;35m"
    BRIGHT_CYAN = "\033[1;36m"
    BRIGHT_WHITE = "\033[1;37m"

    # Background colors
    BG_BLACK = "\033[0;40m"
    BG_RED = "\033[0;41m"
    BG_GREEN = "\033[0;42m"
    BG_YELLOW = "\033[0;43m"
    BG_BLUE = "\033[0;44m"
    BG_MAGENTA = "\033[0;45m"
    BG_CYAN = "\033[0;46m"
    BG_WHITE = "\033[0;47m"

    # Bright background colors
    BG_BRIGHT_BLACK = "\033[1;40m"
    BG_BRIGHT_RED = "\033[1;41m"
    BG_BRIGHT_GREEN = "\033[1;42m"
    BG_BRIGHT_YELLOW = "\033[1;43m"
    BG_BRIGHT_BLUE = "\033[1;44m"
    BG_BRIGHT_MAGENTA = "\033[1;45m"
    BG_BRIGHT_CYAN = "\033[1;46m"
    BG_WHITE = "\033[1;47m"

    # Other styles
    RESET = "\033[0;0m"
    BOLD = "\033[;1m"
    UNDERLINE = "\033[;4m"
    INVERSE = "\033[;7m"


result_style_dict = {
    "AC": Styles.BRIGHT_GREEN,
    "CTLE": Styles.BRIGHT_YELLOW,
    "RTLE": Styles.BRIGHT_YELLOW,
    "WA": Styles.BRIGHT_RED,
    "RE": Styles.BRIGHT_MAGENTA,
    "CE": Styles.BRIGHT_BLUE,
    "AE": Styles.BRIGHT_CYAN,
}


def execution_time_recorded(func):
    def wrapper(*args, **kwargs):
        start_time = time.time()
        result = func(*args, **kwargs)
        end_time = time.time()
        execution_time = end_time - start_time
        return result, execution_time

    return wrapper


def styled(style: str, text: str):
    return style + text + Styles.RESET


def get_cur_file_dir() -> str:
    """
    获取当前文件所在目录
    """
    return os.path.dirname(os.path.realpath(__file__))


root_dir = None  # 缓存根目录


def get_root_dir() -> str:
    """
    获取 `sysy-compiler` 的根目录
    """
    global root_dir
    if not root_dir:
        root_dir = get_cur_file_dir()
        while os.path.basename(root_dir) != "sysy-compiler":
            root_dir = os.path.dirname(root_dir)
    return root_dir


def get_abs_path(rel_path: str) -> str:
    """
    获取相对于根目录的路径
    """
    return os.path.join(get_root_dir(), rel_path)


def get_file_name(file_path: str) -> str:
    """
    获取文件名，不包括后缀
    """
    return os.path.basename(file_path).split(".")[0]


def get_out_dir() -> str:
    """
    获取输出目录
    """
    return os.path.join(get_cur_file_dir(), "out/")


def get_build_dir(debug_mode: bool = False) -> str:
    """
    获取构建目录
    """
    return get_abs_path(BUILD_RELEASE_DIR if not debug_mode else BUILD_DEBUG_DIR)


def get_functional_test_dir() -> str:
    """
    获取功能测试目录
    """
    return get_abs_path(FUNCTIONAL_TEST_DIR)


def get_performance_test_dir() -> str:
    """
    获取性能测试目录
    """
    return get_abs_path(PERFORMANCE_TEST_DIR)


def get_custom_test_dir() -> str:
    """
    获取自定义测试目录
    """
    return get_abs_path(CUSTOM_TEST_DIR)


def setcwd(rel_path: str) -> None:
    """
    设置当前工作目录
    """
    os.chdir(get_abs_path(rel_path))


def clear_out_dir() -> None:
    """
    清理输出目录
    """
    shutil.rmtree(get_out_dir(), ignore_errors=True)


def truncate_string(
    s: str,
    max_len: "Union[int, None]" = None,
    max_lines: "Union[int, None]" = None,
    trunc_mode="right",
) -> str:
    """
    输出字符串，并根据长度截断
    """

    lines = s.splitlines()
    if max_lines is not None and len(lines) > max_lines:
        max_lines -= 1
        if trunc_mode == "right":
            s = "\n".join(lines[:max_lines])
            if max_len is not None and len(s) > max_len:
                s = s[:max_len] + "..."
            else:
                s += "\n..."
        elif trunc_mode == "middle":
            s1 = "\n".join(lines[: max_lines // 2])
            s2 = "\n".join(lines[-max_lines // 2 :])
            if max_len is not None and len(s1) + len(s2) > max_len:
                overflow = len(s1) + len(s2) - max_len
                s1 = s1[: -overflow // 2] + "..."
                s2 = "..." + s2[overflow // 2 :]
            s = s1 + "\n...\n" + s2
        elif trunc_mode == "left":
            s = "\n".join(lines[-max_lines:])
            if max_len is not None and len(s) > max_len:
                s = "..." + s[-max_len:]
            else:
                s = "...\n" + s
    elif max_len is not None and len(s) > max_len:
        if trunc_mode == "right":
            s = s[:max_len] + "..."
        elif trunc_mode == "middle":
            s = s[: max_len // 2] + "..." + s[-max_len // 2 :]
        elif trunc_mode == "left":
            s = "..." + s[-max_len:]
    return s


def build_project(build_args: list = []) -> None:
    """
    运行 `utils/build.sh`，构建项目
    """
    print("构建项目...")
    setcwd("")
    result = subprocess.run(args=["bash", "utils/build.sh", "--nverbose"] + build_args)
    if result.returncode != 0:
        raise Exception(f"项目构建失败，退出码: {result.returncode}")
    print("项目构建成功。")
    print()


class CompileError(Exception):
    pass


class AssembleError(Exception):
    pass


@execution_time_recorded
def compile_sysy(
    sysy_path: str, debug_mode: bool, opt_level: int = 0, log_level: int = 0
) -> None:
    """
    使用 `${BUILD_DIR}/compiler` 编译文件，并将编译结果保存到相应的 `.s` 文件中
    """
    setcwd("")
    file_name = get_file_name(sysy_path)
    sysy_path = get_abs_path(sysy_path)
    asm_path = os.path.join(get_out_dir(), f"{file_name}.s")
    ir_path = os.path.join(get_out_dir(), f"{file_name}.ll")
    log_path = os.path.join(get_out_dir(), f"{file_name}.log")

    # 使用 subprocess.run 来执行命令并捕获输出
    args = [
        os.path.join(get_build_dir(debug_mode=debug_mode), "compiler"),
        "-S",
        "-o",
        asm_path,
        sysy_path,
        f"-O{opt_level}",
    ]
    if debug_mode:
        args.append("-o-ir")
        args.append(ir_path)
    if log_level >= 1:
        print("编译命令:", styled(Styles.WHITE, " ".join(args)))
    result = subprocess.run(args=args, capture_output=True, text=True)
    if debug_mode:
        if os.path.exists(ir_path):
            print("中间代码文件:", styled(Styles.WHITE, ir_path))
        with open(log_path, "w") as file:
            file.write(result.stderr)
        print("编译输出文件:", styled(Styles.WHITE, log_path))
    if result.returncode != 0:
        if log_level >= 0:
            print("编译输出:")
            print(
                styled(
                    Styles.BLACK,
                    truncate_string(
                        str(result.stderr).strip(),
                        max_len=None if log_level >= 1 else 255,
                        max_lines=None if log_level >= 1 else 9,
                        trunc_mode="left",
                    ),
                )
            )
        raise CompileError(f"编译失败，退出码: {result.returncode}")
    if log_level >= 2:
        print("编译输出:")
        print(styled(Styles.BLACK, str(result.stderr).strip()))

    # 使用 riscv64-linux-gnu-gcc 汇编器将汇编代码编译为二进制文件
    link_path = os.path.join(get_root_dir(), "sysy_runtime")
    result = subprocess.run(
        [
            "riscv64-linux-gnu-gcc",
            asm_path,
            "-o",
            os.path.join(get_out_dir(), f"{file_name}.bin"),
            f"-L{link_path}",
            "-lsysy",
        ],
        capture_output=True,
        text=True,
    )
    if result.returncode != 0:
        if result.stderr:
            print("汇编错误:", styled(Styles.BLACK, result.stderr.strip()), sep="\n")
        raise AssembleError(f"汇编失败，退出码: {result.returncode}")
    print("汇编文件:", styled(Styles.WHITE, asm_path))


@execution_time_recorded
def run_bin(file_name: str, input_str: str, log_level: int = 0) -> str:
    """
    使用 `qemu-riscv64` 模拟器运行二进制文件，并将结果保存到相应的 `.out` 文件中
    """

    # 运行二进制文件
    run_path = os.path.join(get_out_dir(), f"{file_name}.bin")
    result = subprocess.run(
        ["qemu-riscv64", run_path], capture_output=True, text=True, input=input_str
    )

    # 将结果保存到文件中
    out_path = os.path.join(get_out_dir(), f"{file_name}.out")
    print("输出文件:", styled(Styles.WHITE, out_path))
    with open(out_path, "w") as file:
        if result.stderr and result.stderr.strip().find("TOTAL") == -1:
            # 用 stderr 来输出错误信息
            if log_level >= 1:
                print(str(result.stderr).strip())
            raise Exception(str(result.stderr).strip() + "\n")
        else:
            if result.stdout:
                # 输出 stdout 的内容
                file.write(str(result.stdout).strip() + "\n")
            if result.returncode == -11:
                # 段错误
                raise Exception("Segmentation fault")
            # 输出返回代码
            file.write(str(result.returncode) + "\n")
            if result.stderr:
                # 输出运行日志
                if log_level >= 0:
                    print("运行日志:")
                    print(styled(Styles.BLACK, str(result.stderr).strip()))

    return out_path


def compare_output(sysy_path: str, out_path: str, log_level: int = 0) -> bool:
    """
    与正确输出比较输出结果
    """
    # 正确输出路径
    ans_path = sysy_path.replace(".sy", ".out")
    if not os.path.exists(ans_path):
        raise FileNotFoundError("没有找到正确输出文件")
    print("正确输出文件:", styled(Styles.WHITE, ans_path))

    with open(ans_path, "r") as file:
        answer = file.read().strip()
    with open(out_path, "r") as file:
        output = file.read().strip()

    answer_split = answer.strip().splitlines(keepends=False)
    output_split = output.strip().splitlines(keepends=False)
    correct = True

    if answer_split[-1] != output_split[-1]:
        # 返回代码
        correct = False

    if "\n".join(answer_split[:-1]).strip() != "\n".join(output_split[:-1]).strip():
        # stdout
        correct = False

    if log_level >= 0 and not correct:
        print("输出结果:")
        print(
            styled(
                Styles.BLACK,
                truncate_string(output, max_len=500, max_lines=7, trunc_mode="middle"),
            )
        )
        print("正确输出:")
        print(
            styled(
                Styles.BLACK,
                truncate_string(answer, max_len=500, max_lines=7, trunc_mode="middle"),
            )
        )

    return correct


def compile_timeout_signal_handler(signum, frame):
    raise TimeoutError("编译超时")


def run_timeout_signal_handler(signum, frame):
    raise TimeoutError("运行超时")


def test_single_file(
    sysy_path: str, debug_mode: bool, opt_level: int = 1, log_level: int = 0
) -> "tuple[str, float, float]":
    """
    编译、汇编、运行单个文件，并将结果保存到相应的 `.s` `.bin` `.out` 文件中

    Parameters:
        - `log_level`: 日志级别，0 为不输出，1 为输出错误信息，2 为输出所有信息。
    """

    file_name = get_file_name(sysy_path)

    # 创建输出目录
    os.makedirs(get_out_dir(), exist_ok=True)

    compile_time, run_time = None, None

    # 编译、汇编
    try:
        signal.signal(signal.SIGALRM, compile_timeout_signal_handler)
        signal.alarm(compile_time_limit)
        _, compile_time = compile_sysy(
            sysy_path, debug_mode=debug_mode, opt_level=opt_level, log_level=log_level
        )
    except TimeoutError as e:
        compile_time = compile_time_limit
        print(styled(Styles.RED, str(e).strip()))
        return "CTLE", compile_time, run_time
    except CompileError as e:
        print(styled(Styles.RED, str(e).strip()))
        return "CE", compile_time, run_time
    except AssembleError as e:
        print(styled(Styles.RED, str(e).strip()))
        return "AE", compile_time, run_time
    finally:
        signal.alarm(0)

    # 读取输入文件
    in_path = sysy_path.replace(".sy", ".in")
    input_str = None
    if os.path.exists(in_path):
        with open(in_path, "r") as file:
            input_str = file.read()
    print("输入文件:", styled(Styles.WHITE, in_path))

    # 运行
    try:
        signal.signal(signal.SIGALRM, run_timeout_signal_handler)
        signal.alarm(run_time_limit)
        out_path, run_time = run_bin(file_name, input_str, log_level=log_level)
    except TimeoutError as e:
        run_time = run_time_limit
        print(e)
        return "RTLE", compile_time, run_time
    except Exception as e:
        print(styled(Styles.RED, f"运行时错误: {str(e).strip()}"))
        return "RE", compile_time, run_time
    finally:
        signal.alarm(0)

    # 比较输出结果
    result = None
    try:
        result = (
            "AC" if compare_output(sysy_path, out_path, log_level=log_level) else "WA"
        )
    except Exception as e:
        print(styled(Styles.RED, str(e).strip()))
    return result, compile_time, run_time


def main_loop_body():
    global compile_time_limit, run_time_limit
    compile_time_limit, run_time_limit = COMPILE_TIME_LIMIT_DEF, RUN_TIME_LIMIT_DEF

    try:
        os.system("clear")

        # 处理用户输入
        test_case_input = input(
            "─" * 80 + "\n"
            "格式:\t(f|p)[<测试样例名前缀>]\n"
            "说明:\tf - 功能测试，p - 性能测试，c - 自定义测试；前缀为空则测试全部。\n"
            "例:\tf00 - 运行 functional/00_main.sy\n"
            "\tp0 - 运行 performance/0*.sy（以 0 开头的性能测试用例）\n"
            "\tc - 运行 custom/*.sy（所有自定义测试用例）\n" + ""
            "─" * 80 + "\n"
            "请输入: "
        ).strip()
        test_case_group, test_dir = None, None
        if test_case_input[0] == "f":
            test_case_group, test_dir = "functional", get_functional_test_dir()
        elif test_case_input[0] == "p":
            test_case_group, test_dir = "performance", get_performance_test_dir()
        elif test_case_input[0] == "c":
            test_case_group, test_dir = "custom", get_custom_test_dir()
        else:
            raise ValueError("无效的测试类别")
        case_prefix = test_case_input[1:]
        print()

        while True:
            options_input = input(
                "─" * 80 + "\n"
                "格式:\t[d][v][t][o]\n"
                "说明:\td - 调试模式，v - 详细输出，t - 时间限制加长，\n"
                "\to - 开启 O1 优化，r - 重新开始\n" + ""
                "─" * 80 + "\n"
                "请输入选项: "
            ).strip()
            debug_mode, verbose_mode, opt_level = False, False, 0
            if options_input.find("r") != -1:
                return
            if options_input.find("d") != -1:
                debug_mode = True
            if options_input.find("v") != -1:
                verbose_mode = True
            if options_input.find("t") != -1:
                compile_time_limit, run_time_limit = (
                    COMPILE_TIME_LIMIT_EXT,
                    RUN_TIME_LIMIT_EXT,
                )
            if options_input.find("o") != -1:
                opt_level = 1
            break
        print()

        # 构建项目
        try:
            build_args = []
            if debug_mode:
                build_args.append("--debug")
            build_project(build_args=build_args)
        except Exception as e:
            print(e)
            cont = input("是否继续？(Y/n): ")
            if cont.upper() == "Y":
                pass
            else:
                return

        # 清理输出目录
        clear_out_dir()

        # 遍历测试用例并测试
        result_dict = {}
        total_score = 0
        max_score = 0
        total_compile_time = 0.0
        compile_count = 0
        total_run_time = 0.0
        run_count = 0
        file_count = 0
        for file in sorted(os.listdir(test_dir)):
            if file.startswith(case_prefix) and file.endswith(".sy"):
                file_path = os.path.join(test_dir, file)
                print()
                file_count += 1
                print(styled(Styles.BOLD, f"第 {file_count} 个测试点"))
                print("测试文件:", styled(Styles.WHITE, file_path))
                result, compile_time, run_time = None, None, None
                try:
                    result, compile_time, run_time = test_single_file(
                        file_path,
                        debug_mode=debug_mode,
                        opt_level=opt_level,
                        log_level=2 if verbose_mode else 0,
                    )
                except Exception as e:
                    print(styled(Styles.RED, str(e).strip()))
                if compile_time is not None:
                    total_compile_time += compile_time
                    compile_count += 1
                    print(
                        "编译时间:", styled(Styles.WHITE, f"{compile_time*1000:.0f}ms")
                    )
                if run_time is not None:
                    total_run_time += run_time
                    run_count += 1
                    print("运行时间:", styled(Styles.WHITE, f"{run_time*1000:.0f}ms"))
                if result is not None:
                    result_style = result_style_dict[result]
                    print(f"测试结果: {styled(result_style, result)}")
                    total_score += 1 if result == "AC" else 0
                    max_score += 1
                    if result not in result_dict:
                        result_dict[result] = 0
                    result_dict[result] += 1

        # 输出汇总
        print()
        print("评测完成。")
        for key, value in result_dict.items():
            print(
                f"{styled(result_style_dict[key], key)}:",
                styled(Styles.WHITE, str(value)),
            )
        if file_count == 0:
            print("无测试用例。")
        else:
            if compile_count > 0:
                print(
                    "总编译时间:",
                    styled(Styles.WHITE, f"{total_compile_time*1000:.0f}ms"),
                )
            if run_count > 0:
                print(
                    "总运行时间:",
                    styled(Styles.WHITE, f"{total_run_time*1000:.0f}ms"),
                )
            print("总分:", styled(Styles.BRIGHT_WHITE, f"{total_score}/{max_score}"))

    except Exception as e:
        print(e)
    except KeyboardInterrupt:
        pass

    print()
    input("按回车重新开始...\n")


if __name__ == "__main__":
    while True:
        main_loop_body()
