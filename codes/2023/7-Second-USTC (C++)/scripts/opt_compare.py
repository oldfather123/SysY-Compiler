import os
import re
import matplotlib.pyplot as plt
import numpy as np

OPT_LOG_ROOT = "optlogs"
TEST_CASE_ROOT = "tests/final_performance"
SUFFIX = ".log"
id2case = {}
opt2scores = {}
colors = ['b', 'g', 'r', 'c', 'm', 'y', 'k', 'tab:blue', 'tab:orange', 'tab:green']

allopts = set()
for opt in os.listdir(OPT_LOG_ROOT):
    allopts.add(opt.split(".")[0])
allopts = list(sorted(allopts))
    
for opt in allopts:
    test = os.path.join(OPT_LOG_ROOT, opt+SUFFIX)
    with open(test, "r") as f:
        for line in f.readlines():
            matches = re.findall(r"\d+.\d+", line)
            if matches:
                if opt in opt2scores:
                    opt2scores[opt].append(float(matches[0]))
                else:
                    opt2scores[opt] = [float(matches[0])]
                    
baseline = opt2scores['baseline']

for opt in opt2scores:
    if opt == 'baseline':
        continue
    for i in range(len(opt2scores[opt])):
        opt2scores[opt][i] = baseline[i] / opt2scores[opt][i]
opt2scores['baseline'] = [1.0] * len(baseline)

allcases = set()    
for case in sorted(os.listdir(TEST_CASE_ROOT)):
    allcases.add(case.split(".")[0])
allcases = list(sorted(allcases))
        
for index, case in enumerate(allcases):
    id2case[index] = case
   
X = list(id2case.keys())

for i, opt in enumerate(opt2scores):
    case_num = len(opt2scores[opt])
    plt.plot(X[0:case_num], opt2scores[opt], color=colors[i%len(colors)], label=opt) 

plt.xticks(np.arange(0, len(allcases), 1))
plt.xlabel('Testcase')
plt.ylabel('Opt method')
plt.title('Opt Compare')
plt.legend()
plt.show()
                    


                    
    


        
        

    
