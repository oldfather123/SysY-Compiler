import os
import re

TEST_DIR = "tests/gen-ll-asm"

testcase_list =[ i.split(".")[0]  for i in os.listdir(TEST_DIR) if i.endswith('.s') ]

all_insts = set()


for testcase in testcase_list:
    with open(os.path.join(TEST_DIR, testcase+".s")) as f:
        all_lines = [line.replace("\t", " ").replace("\n", " ").strip() for line in f.readlines()]
        all_lines = [line for line in all_lines if line]
        for line in all_lines:
            if line.strip().startswith(".") or line.strip().startswith("#") or ":" in line:
                continue
            inst = re.match("\s*([\w\.]+)\s*(.*)", line).group(1)
            all_insts.add(inst)
 
            
with open("asminsts.txt", "w") as f:
    f.write("\n".join(sorted(all_insts)))
                