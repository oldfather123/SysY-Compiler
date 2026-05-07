#!/bin/bash

# 设置待测试程序的路径和测试样例文件夹的路径
compiler_path=$1
test_cases_folder="./testcases/"
status=0

if [ ! -d "$test_cases_folder" ]; then
    echo "cloning testcases......"
    git clone https://gitlab.eduxiji.net/csc1/nscscc/compiler2023.git
    mkdir -p testcases
    
    mkdir -p testcases/functional
    mkdir -p testcases/performance

    mv compiler2023/公开样例与运行时库/functional/* testcases/functional/
    mv compiler2023/公开样例与运行时库/hidden_functional/* testcases/functional/
    mv compiler2023/公开样例与运行时库/performance/* testcases/performance/
    mv compiler2023/公开样例与运行时库/final_performance/* testcases/performance/
    mv compiler2023/公开样例与运行时库/sylib.c testcases/
    mv compiler2023/公开样例与运行时库/sylib.h testcases/
    # mv compiler2023/公开样例与运行时库/* testcases/
    rm -rf compiler2023
    echo "complete"
fi

test_paths=$(ls -l "$test_cases_folder" | grep '^d' | awk '{print $NF}')

for path in ${test_paths}; do
    test_path="${test_cases_folder}${path}/"
    sh ./asm_test_base.sh $compiler_path $test_path
    exit_code=$?
    if [ ${exit_code} -ne 0 ]; then
        # echo "Test Failed"
        # rm -rf actual.out app expected_output last_case.sy o.S
        exit 1
        status=1
    fi
done

rm -rf actual.out app expected_output last_case.sy o.S
exit $status
