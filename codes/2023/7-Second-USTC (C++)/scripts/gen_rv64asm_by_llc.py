import os
import subprocess

# 指定目录路径
directory = "tests/functional"

# 查找所有.ll文件
for filename in os.listdir(directory):
    if filename.endswith(".ll"):
        # 构造命令行
        input_file = os.path.join(directory, filename)
        output_file = os.path.join(directory, filename[:-3] + ".s")
        command = f"llc -march=riscv64 -mattr=+m,+a,+f,+d,-relax --mcpu=generic-rv64 -O0 {input_file} -o {output_file}"
        # 执行命令行
        subprocess.run(command, shell=True)