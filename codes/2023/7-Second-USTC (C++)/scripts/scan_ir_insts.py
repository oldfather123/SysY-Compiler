import os
import re

TEST_DIR = "tests/gen-ll"

testcase_list =[ i.split(".")[0]  for i in os.listdir(TEST_DIR) if i.endswith('.ll') ]

all_insts = set()

scan_begin = False

for testcase in testcase_list:
    with open(os.path.join(TEST_DIR, testcase+".ll")) as f:
        for line in f.readlines():
            if "{" in line:
                scan_begin = True
                continue
            elif "}" in line:
                scan_begin = False
                continue
            if not scan_begin:
                continue
            
            if(line.strip().startswith("%")): 
                inst = re.match("(.*?)=\s+(\w+)\s+(.*)", line).group(2)
                if inst == "i32":
                    continue
                all_insts.add(inst)
            elif ":" not in line:
                inst = re.match("\s+(\w+)\s+(.*)", line).group(1)
                if inst == "i32":
                    continue
                all_insts.add(inst)
                
            
with open("irinsts.txt", "w") as f:
    f.write("\n".join(sorted(all_insts)))
                