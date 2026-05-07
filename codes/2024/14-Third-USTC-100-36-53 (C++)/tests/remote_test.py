from datetime import datetime
import os
import subprocess
import sys
from pathlib import Path
import re
import json

# 设置限制和超时
timeout = 3600

def usage():
    print(f"Usage: {sys.argv[1]} [testcase-set-name] [compiler]")
    print("testcase-set-name: 'functional', 'hidden_functional', 'performance' or 'final_performance', the name of the testcase set")
    print("compiler: 'cminusfc', 'clang' or 'g++', the compiler to use")
    sys.exit(0)

# 检查参数
if len(sys.argv) < 3 or sys.argv[1] not in ["functional", "hidden_functional", "performance", "final_performance"] \
    or sys.argv[2] not in ["cminusfc", "clang", "g++"]:
    usage()

testcase_name = sys.argv[1]
test_dir = Path(sys.argv[1])

compiler = sys.argv[2]

if not test_dir.exists():
    print(f"Unable to access to '{test_dir}'")
    sys.exit(1)
    
# 目录配置
project_dir = Path(__file__).parent.resolve().parent

output_dir = Path("output")
output_dir.mkdir(exist_ok=True)
output_dir = output_dir / testcase_name 
output_dir.mkdir(exist_ok=True)
for file in output_dir.glob("*"):
    file.unlink()

log_dir = Path("log")
log_dir.mkdir(exist_ok=True)
log_dir = log_dir / (datetime.now().strftime("%Y%m%d-%H%M%S") + "-" + testcase_name + "-" + compiler)
log_dir.mkdir(exist_ok=True)

suffix = "sy"
log_file = log_dir / "test.log"

remote_host = "jianmu_test1@202.38.75.246"
remote_dir = "/home/jianmu_test1/tests/"
remote_output_dir = remote_dir + "output/" + testcase_name

# clear remote output dir
result = subprocess.run(f"ssh {remote_host} 'find {remote_output_dir} -mindepth 1 -delete'", shell=True, text=True, capture_output=True)

with open(log_file, "w") as log:
    log.write("[info] Start testing, using testcase dir: {}\n".format(test_dir))
    print("[info] Start testing, using testcase dir: {}".format(test_dir))

    log_json_file = log_dir / "test.json"
    log_json = {}

    for case_path in sorted(test_dir.glob(f"*.{suffix}")):
        case_base_name = case_path.stem
        asm_file = output_dir / f"{case_base_name}.s"
        log_json[case_base_name] = {}
        # if(case_base_name.find("21") == -1):
            # continue

        log.write(f"[info] Testing {case_base_name}\n")
        print(f"[info] Testing {case_base_name}")
        if compiler == "g++":
            c_file = output_dir / f"{case_base_name}.c"
            result = subprocess.run(f"cp {case_path} {c_file}", shell=True, text=True, capture_output=True)
            result = subprocess.run(f"sed -i '1i #include \"/home/zox/compiler/2023ustc-jianmu-compiler/src/sylib/sylib.c\"' {c_file}", shell=True, text=True, capture_output=True)
            result = subprocess.run(f"sed -i '1i #include \"/home/zox/compiler/2023ustc-jianmu-compiler/src/sylib/sylib.h\"' {c_file}", shell=True, text=True, capture_output=True)

            if(case_base_name.find("conv") == -1):
                result = subprocess.run(f"timeout {timeout} loongarch64-unknown-linux-gnu-g++ -O2 -static -S {c_file} -o {asm_file} >&2", shell=True, text=True, capture_output=True)
            else :
                result = subprocess.run(f"timeout {timeout} loongarch64-unknown-linux-gnu-gcc -O2 -static -S {c_file} -o {asm_file} >&2", shell=True, text=True, capture_output=True)
            print(result.stderr)
        elif compiler == "cminusfc":
            result = subprocess.run(f"timeout {timeout} cminusfc -S -mem2reg {case_path} -o {asm_file} >&2", shell=True, text=True, capture_output=True)
        elif compiler == "clang":
            c_file = output_dir / f"{case_base_name}.c"
            result = subprocess.run(f"cp {case_path} {c_file}", shell=True, text=True, capture_output=True)
            result = subprocess.run(f"sed -i '1i #include \"/home/zox/compiler/2023ustc-jianmu-compiler/src/sylib/sylib.c\"' {c_file}", shell=True, text=True, capture_output=True)
            result = subprocess.run(f"sed -i '1i #include \"/home/zox/compiler/2023ustc-jianmu-compiler/src/sylib/sylib.h\"' {c_file}", shell=True, text=True, capture_output=True)

            result = subprocess.run(f"timeout {timeout} clang -static -S --target=loongarch64 {c_file} -o {asm_file} -O0 >&2", shell=True, text=True, capture_output=True)
            print(result.stderr)
        if result.returncode != 0:
            # 检查是否因为超时
            if result.returncode == 124:
                log_json[case_base_name]["compile"] = "timeout"
                error_msg = "[error] Compile failed: timeout\n"
            else:
                log_json[case_base_name]["compile"] = "failed"
                error_msg = "[error] Compile failed: no output file\n"
            
            # 将错误信息写入日志文件
            log.write(error_msg + result.stderr)
            print(error_msg)
            continue
            
        log_json[case_base_name]["compile"] = "success"

        # use remote host as and ld to compile the asm file

        # copy asm file to remote host
        result = subprocess.run(f"scp {asm_file} {remote_host}:{remote_output_dir}/{case_base_name}.s >&2", shell=True, text=True, capture_output=True)
        if result.returncode != 0:
            log_json[case_base_name]["run"] = "failed"
            error_msg = "[error] Copy asm file to remote host failed\n"
            log.write(error_msg + result.stderr)
            print(error_msg)
            continue
        
        # compile and run the asm file on remote host
        result = subprocess.run(f"ssh {remote_host} 'cd {remote_dir} && sh ./remote.sh {testcase_name} {case_base_name}'", shell=True, text=True, capture_output=True)
        if result.returncode != 0:
            log_json[case_base_name]["run"] = "failed"
            error_msg = "[error] Run failed\n"
            log.write(error_msg + result.stderr)
            print(error_msg + result.stdout)
            continue        
        
        log_json[case_base_name]["run"] = "success"

        test_point_pattern = re.compile(r'Timer@\d+-\d+: (\d+)H-(\d+)M-(\d+)S-(\d+)us', re.DOTALL)
        test_point_match = test_point_pattern.search(result.stderr)
        time = 0
        if test_point_match:
            hour, minute, second, microsecond = test_point_match.groups()
            # log_json[case_base_name]["time"] = int(hour) * 3600 + int(minute) * 60 + int(second) + int(microsecond) / 1000000
            time = int(hour) * 3600 + int(minute) * 60 + int(second) + int(microsecond) / 1000000
        

        log_json[case_base_name]["time"] = time 
        log.write(f"[info] Time: {time}\n")
        log.write(result.stderr)
        print(f"[info] Time: {time}")
        print(result.stdout)
        
    
    with open(log_json_file, "w") as json_file:
        json.dump(log_json, json_file, indent=4)

    log.write("[info] Testing finished\n")

            
        
        
            
        
        






        
