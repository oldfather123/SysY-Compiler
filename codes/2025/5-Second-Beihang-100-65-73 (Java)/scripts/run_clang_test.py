#!/usr/bin/env python3
import os
import sys
import subprocess
import concurrent.futures
from tqdm import tqdm
import shutil
import zipfile
import time

# 参数配置
optimize = False
running_time = 90
max_workers = 28

# 环境变量路径
test_dir = os.environ.get("TESTFILE_DIR", "./testdata2024")
ll_dir = os.environ.get("LL_DIR", "./compiled_out")
sylib_o_path = os.environ.get("SYLIB_O_PATH", "./sylib.o")

# 标识符（CI 或时间戳）
short_sha = os.environ.get("CI_COMMIT_SHA", time.strftime("%Y%m%d%H%M%S"))[:6]

# 输出目录和文件名
artifact_dir = "run_results"
fail_dir = os.path.join(artifact_dir, f"failed_cases_{short_sha}")
log_file_name = "log4clangTest.txt"
log_output_path = os.path.join(artifact_dir, log_file_name)
diff_summary_path = os.path.join(artifact_dir, "diff_summary.txt")
zip_output_path = os.path.join(artifact_dir, f"failed_cases_{short_sha}.zip")

# 初始化目录
os.makedirs(artifact_dir, exist_ok=True)
os.makedirs(fail_dir, exist_ok=True)
open(log_file_name, 'w').close()

# 收集测试用例
def collect_tests(test_dir):
    tests = []
    for f in os.listdir(test_dir):
        if f.endswith(".out"):
            base = f[:-4]
            out_path = os.path.join(test_dir, base + ".out")
            in_path = os.path.join(test_dir, base + ".in")  # optional
            ll_path = os.path.join(ll_dir, base + ".ll")
            if os.path.exists(out_path) and os.path.exists(ll_path):
                tests.append((base, ll_path, in_path, out_path))
    return sorted(tests)

# 单个测试逻辑
def run_single_test(base, ll_path, in_path, out_path):
    logger_lines = []
    cur_success = True

    exe_path = os.path.join(test_dir, base + ".exe")
    run_output_path = os.path.join(test_dir, base + ".ans.out")
    stderr_output_path = os.path.join(fail_dir, base + ".stderr.txt")

    # 编译
    compile_cmd = f"clang {ll_path} {sylib_o_path} -o {exe_path}"
    logger_lines.append(f"🔧 Compiling: {compile_cmd}")
    try:
        result = subprocess.run(compile_cmd, shell=True, capture_output=True, text=True, timeout=running_time)
        if result.returncode != 0:
            logger_lines.append("❌ Compile Failed!")
            logger_lines.append(result.stderr)
            return base, False, "\n".join(logger_lines), f"{base}: Compile Failed!"
    except subprocess.TimeoutExpired:
        logger_lines.append("❌ Compile Timeout!")
        return base, False, "\n".join(logger_lines), f"{base}: Compile Timeout!"

    # 运行
    run_cmd = f"{exe_path}"
    if os.path.exists(in_path):
        run_cmd += f" < {in_path}"
    run_cmd += f" > {run_output_path}"

    logger_lines.append(f"🚀 Running: {run_cmd}")
    try:
        result = subprocess.run(run_cmd, shell=True, capture_output=True, text=True, timeout=running_time)
        if result.stderr:
            with open(stderr_output_path, "w") as errlog:
                errlog.write(result.stderr)
    except subprocess.TimeoutExpired:
        logger_lines.append("❌ Runtime Timeout!")
        return base, False, "\n".join(logger_lines), f"{base}: Runtime Timeout!"

    # 对比输出
    with open(out_path, 'r') as f:
        std_lines = f.readlines()
    while std_lines and not std_lines[-1].strip():
        std_lines.pop()
    std_return = int(std_lines.pop().strip())
    std_output = std_lines

    with open(run_output_path, 'r') as f:
        act_output = f.readlines()

    logger_lines.append(f"✔️ Expected return: {std_return}")
    logger_lines.append(f"✔️ Actual return: {result.returncode}")
    if result.returncode != std_return:
        logger_lines.append("❌ Return value mismatch!")
        cur_success = False

    if len(std_output) != len(act_output):
        logger_lines.append("❌ Output length mismatch!")
        cur_success = False
    else:
        for i in range(len(std_output)):
            if std_output[i].strip() != act_output[i].strip():
                logger_lines.append(f"❌ Mismatch at line {i}:")
                logger_lines.append(f"   Expected: {std_output[i].strip()}")
                logger_lines.append(f"   Actual:   {act_output[i].strip()}")
                cur_success = False

    logger_lines.append("✅ AC!" if cur_success else "❌ WA!")

    try:
        if os.path.exists(exe_path):
            os.remove(exe_path)
    except Exception as e:
        logger_lines.append(f"⚠️ 清理失败：{e}")

    diff_line = None
    if not cur_success:
        diff_line = f"{base}: Output mismatch or return mismatch"
    return base, cur_success, "\n".join(logger_lines), diff_line

# 命令行参数解析
for i in range(len(sys.argv)):
    if sys.argv[i] == "-O1":
        optimize = True
    if sys.argv[i] == "-O0":
        optimize = False
    if i == 1 and not sys.argv[i].startswith("-"):
        test_dir = sys.argv[i]
if test_dir.endswith('/'):
    test_dir = test_dir[:-1]

# 执行测试
tests = collect_tests(test_dir)
success_count = 0
diff_lines = []
failed_bases = set()

with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as executor:
    future_to_test = {
        executor.submit(run_single_test, base, ll_path, in_path, out_path): base
        for base, ll_path, in_path, out_path in tests
    }

    with open(log_file_name, 'a') as log, tqdm(total=len(tests)) as pbar:
        for future in concurrent.futures.as_completed(future_to_test):
            base, passed, log_text, diff_line = future.result()
            if passed:
                success_count += 1
            else:
                failed_bases.add(base)
                if diff_line:
                    diff_lines.append(diff_line)
                else:
                    for line in log_text.splitlines():
                        if "Mismatch at line" in line or "Return value mismatch" in line:
                            diff_lines.append(f"{base}: {line.strip()}")
                # 收集失败样例所有相关文件
                for ext, src_dir in [
                    (".sy", test_dir),
                    (".in", test_dir),
                    (".out", test_dir),
                    (".ans.out", test_dir),
                    (".ll", ll_dir),
                ]:
                    src = os.path.join(src_dir, base + ext)
                    if os.path.exists(src):
                        shutil.copy(src, os.path.join(fail_dir, base + ext))
            log.write(log_text + "\n\n")
            pbar.update(1)

# 写入 diff & 日志
with open(diff_summary_path, "w") as f:
    f.write("\n".join(diff_lines) if diff_lines else "No output mismatch found.\n")
shutil.copy(log_file_name, log_output_path)

# 打包失败样例
with zipfile.ZipFile(zip_output_path, 'w') as z:
    for base in failed_bases:
        for ext in [".sy", ".in", ".out", ".ans.out", ".ll", ".stderr.txt"]:
            path = os.path.join(fail_dir, base + ext)
            if os.path.exists(path):
                z.write(path, arcname=os.path.join(os.path.basename(fail_dir), base + ext))
    z.write(diff_summary_path, arcname="diff_summary.txt")
    z.write(log_output_path, arcname="log4clangTest.txt")

# 输出结果
print(f"✅ 测试完成：{success_count}/{len(tests)} 通过")
print(f"📄 差异报告：{diff_summary_path}")
print(f"📦 打包输出：{zip_output_path}")
sys.exit(0 if success_count == len(tests) else 1)
