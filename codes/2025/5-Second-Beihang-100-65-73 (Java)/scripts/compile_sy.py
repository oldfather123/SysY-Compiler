#!/usr/bin/python3

import os
import sys
import subprocess
import concurrent.futures
from tqdm import tqdm
import zipfile

# 参数配置
optimize = True
running_time = 120
max_workers = 28

# 环境变量读取
testfile_sy_dir = os.environ.get("TESTFILE_DIR", "./testdata")
ll_dir = os.environ.get("LL_DIR", "./compiled_out")
jar_file = os.environ.get("JAR_DIR", "./build/app.jar")
case_name = os.environ.get("CASE_NAME", "A")
short_sha = os.environ.get("CI_COMMIT_SHA", "manual")[:6]
zip_dir = os.environ.get("LL_ZIP_DIR", ll_dir)
zip_name = f"compiled_ll_{case_name}.zip"
zip_output_path = os.path.join(zip_dir, zip_name)

project_dir = os.environ.get("CI_PROJECT_DIR", os.getcwd())

os.makedirs(ll_dir, exist_ok=True)
os.makedirs(zip_dir, exist_ok=True)
log_file_name = "log4geneTest.txt"
open(log_file_name, 'w').close()

# 成功和失败样例收集
success_sy_paths = []
success_ll_paths = []
failed_sy_paths = []

def get_cmd_list():
    cmd_list = []
    for root, dirs, files in os.walk(testfile_sy_dir):
        for f in files:
            if f.endswith(".sy"):
                sy_path = os.path.join(root, f)
                base = os.path.splitext(f)[0]
                ll_path = os.path.join(ll_dir, base + ".ll")
                cmd = f'java -jar "{jar_file}" -S "{sy_path}" -ll "{ll_path}" -mid'
                if optimize:
                    cmd += " -O1"
                cmd_list.append((cmd, sy_path, ll_path))
    return cmd_list

def run_all(cmd_list):
    result = {"success": 0, "error": 0, "timeout": 0}
    with concurrent.futures.ThreadPoolExecutor(max_workers=max_workers) as pool, open(log_file_name, 'w') as log:
        futures = {pool.submit(run_cmd, cmd, sy, ll): (cmd, sy, ll) for cmd, sy, ll in cmd_list}
        for future in tqdm(concurrent.futures.as_completed(futures), total=len(futures)):
            cmd, sy_path, ll_path = futures[future]
            try:
                code, msg = future.result()
                if code == 0 and os.path.exists(ll_path):
                    result["success"] += 1
                    success_sy_paths.append(sy_path)
                    success_ll_paths.append(ll_path)
                elif code == -1:
                    result["timeout"] += 1
                    failed_sy_paths.append(sy_path)
                else:
                    result["error"] += 1
                    failed_sy_paths.append(sy_path)
                print(f"Command: {cmd}\nStatus: {code}\n{msg}\n", file=log)
            except Exception as e:
                print(f"Exception running {cmd}:\n{e}", file=log)
                failed_sy_paths.append(sy_path)
    return result

def run_cmd(cmd, _, __):
    try:
        result = subprocess.run(cmd, shell=True, text=True, capture_output=True, timeout=running_time)
        return result.returncode, result.stderr or result.stdout
    except subprocess.TimeoutExpired:
        return -1, "TLE"

def zip_success():
    with zipfile.ZipFile(zip_output_path, "w") as zf:
        # 成功文件
        for path in success_ll_paths:
            zf.write(path, arcname=os.path.join("success", "ll", os.path.basename(path)))
        for path in success_sy_paths:
            zf.write(path, arcname=os.path.join("success", "sy", os.path.basename(path)))
        # 失败文件
        for path in failed_sy_paths:
            zf.write(path, arcname=os.path.join("failed", "sy", os.path.basename(path)))
    print(f"📦 打包完成：{zip_output_path}")

for i in range(len(sys.argv)):
    if sys.argv[i] == "-O1":
        optimize = True
    if sys.argv[i] == "-O0":
        optimize = False
    if i == 1 and not sys.argv[i].startswith("-"):
        testfile_sy_dir = sys.argv[i]

cmds = get_cmd_list()
result = run_all(cmds)
print("✅ 编译完成：", result)
zip_success()
if result["error"] + result["timeout"] > 0:
    sys.exit(1)
