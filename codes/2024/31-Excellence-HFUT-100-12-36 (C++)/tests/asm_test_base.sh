#!/bin/bash

# 设置待测试程序的路径和测试样例文件夹的路径
compiler_path=$1
test_cases_folder=$2

last_dir_name=$(basename "$test_cases_folder")
time_path="./time"

if [ ! -d "$time_path" ]; then
    mkdir -p time
fi

time_file="${time_path}/${last_dir_name}.time"

last_case="./last_case.sy"

status=0
success=0

> $time_file

> $last_case

# 获取测试样例文件夹中所有的测试样例文件
test_cases=$(ls "${test_cases_folder}"*.sy)

# 遍历每个测试样例文件
for test_case in ${test_cases}; do
    # 提取测试样例的编号和名称
    base_name=$(basename "${test_case}")
    test_name="${base_name%.*}"
    echo $test_name >> $time_file

    # 打印当前正在执行的测试样例

    diff_test_case=$(diff ${test_case} ${last_case})

    if [ -n "${diff_test_case}" ]; then
        cp $test_case $last_case

        echo "Generating asm for test case: ${test_name}"
        # 调用待测试的程序，并将测试样例文件作为输入，将结果输出到文件
        "${compiler_path}" -o "o.S" -O1 -S "${test_case}"

        # 获取程序的执行返回值
        exit_code=$?

        # 判断返回值是否为0
        if [ ${exit_code} -ne 0 ]; then
            # 输出文件名
            echo "Failed: Failed to compile test case"
            echo "Failed: Failed to compile test case" >> $time_file
            # 终止程序
            cat ${test_case}
            exit 1
            status=$((status+1))
            echo "------------------------------------"
            echo "------------------------------------" >> $time_file
            continue
        fi

    #    llvm-as o.ll -o o.bc /home/Looouiiis/compile/extra/jianmu-supplemental/func/sylib.c
    #	llvm-as o.ll -o o.bc
        
        # cp o.S ../sysdeps/loongarch/start.S

        echo "GCC compiling asm for test case: ${test_name}"

        riscv64-linux-gnu-gcc -static o.S testcases/sylib.c -o app

    else
        echo "The test case is identical to the previous one; therefore, we can use the same application directly."
    fi

    # 获取同名的输入文件
    input_file="${test_cases_folder}${test_name}.in"

    echo "Running test case: ${test_name}"

    # 判断输入文件是否存在
    if [ -f "${input_file}" ]; then
        # 将输入文件内容传递给 lli o.bc
         qemu-riscv64 app < "${input_file}" > actual.out 2>> $time_file
    else
        # 否则，直接执行 lli o.bc
         qemu-riscv64 app > actual.out 2>> $time_file
    fi

    # 获取程序的执行返回值
    res=$?

    # is_file_empty=$(wc -c < actual.out)

    # if [ $is_file_empty -ne 0 ]; then
    echo >> actual.out
    # fi

    sed -e :a -e '/^\n*$/{$d;N;ba' -e '}' actual.out > actual.tmp

    mv actual.tmp actual.out

    dos2unix actual.out 2>> /dev/null

    # 获取同名的输出文件
    output_file="${test_cases_folder}${test_name}.out"

    # 判断输出文件是否存在
    if [ -f "${output_file}" ]; then
        # 复制 outfile 到 expected_output，并去掉最后一行
        sed '$d' "${output_file}" > expected_output
        # tac expected_output.tmp | sed '1s/[[:space:]]\+$//' | tac > expected_output
        # mv expected_output.tmp expected_output

        echo >> expected_output

        sed -e :a -e '/^\n*$/{$d;N;ba' -e '}' expected_output > expected_output.tmp

        mv expected_output.tmp expected_output

        # 比较 actual.out 和 expected_output
        diff_output=$(diff actual.out expected_output)

        # 判断是否有差异
        if [ -n "${diff_output}" ]; then

            # 存在部分性能样例最后一行会有多余的空格，所以需要检测这种情况
            sed '$d' "${output_file}" > expected_output.tmp
            tac expected_output.tmp | sed '1s/[[:space:]]\+$//' | tac > expected_output

            echo >> expected_output

            sed -e :a -e '/^\n*$/{$d;N;ba' -e '}' expected_output > expected_output.tmp

            mv expected_output.tmp expected_output

            diff_output=$(diff actual.out expected_output)

            if [ -n "${diff_output}" ]; then

                # 输出警告
                echo "Failed: Output differs in test case"
                echo "Failed: Output differs in test case" >> $time_file
                # echo "res: ${res} ------- ans: $(cat expected_output)"
                exit 1
                status=$((status+1))
                echo "------------------------------------"
                echo "------------------------------------" >> $time_file
                continue
            fi
        fi

        # 比较最后一行
        expected_last_line=$(tail -n 1 "${output_file}")
        if [ ${res} != ${expected_last_line} ]; then
            echo "Failed: Error in test return: ret:: ${res}, expected:: ${expected_last_line}"
            echo "Failed: Error in test return: ret:: ${res}, expected:: ${expected_last_line}" >> $time_file
            exit 1
            status=$((status+1))
            echo "------------------------------------"
            echo "------------------------------------" >> $time_file
            continue
        fi

    fi

    echo ""
    echo "pass"
    echo "pass" >> $time_file

    echo "------------------------------------"
    echo "------------------------------------" >> $time_file

    success=$((success+1))
    # 暂停，等待用户按任意键
    # read -n 1 -s -r -p "Press any key to continue..."
done


echo "${success}/$((success+status))" >> $time_file

exit $status
