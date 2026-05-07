#!bin/bash

#
# usage:
# sh scripts/ci_functional_test.sh riscv-gnu-toolchain-bin-dir-path qemu-risc64-file-path

compiler_path=target/release/compiler
functional_test_path=test/functional
hidden_functional_test_path=test/hidden_functional
sysylib_path=lib
output_path=ci_output
riscv_gnu_toolchain_prefix=riscv64-linux-gnu-
gcc_path=$riscv_gnu_toolchain_prefix"gcc"
as_path=$riscv_gnu_toolchain_prefix"as"
qemu_riscv64_path=qemu-riscv64-static

rm -rf $output_path
mkdir -p $output_path

# compile lib
lib_obj_file=$output_path/sylib.o
$gcc_path -c $sysylib_path/sylib.c -o $lib_obj_file
if [ $? -ne 0 ]; then
	echo "compile sysylib failed !!!"
	rm -rf $output_path
	exit 1
fi

for file in $functional_test_path/*.sy $hidden_functional_test_path/*.sy; do
	# compile
	file_basename=$(basename $file)
	echo "test $file_basename"
	output_asm_file=$output_path/${file_basename%%.sy}.s

	start_time=$(date +%s%N)

	$compiler_path $file -S -o $output_asm_file
	if [ $? -ne 0 ]; then
		echo "compile $file_basename failed !!!"
		rm -rf $output_path
		exit 1
	fi

	end_time=$(date +%s%N)
	execution_time=$((end_time - start_time))

	hours=$((execution_time / 3600000000000))
	minutes=$(((execution_time % 3600000000000) / 60000000000))
	seconds=$(((execution_time % 60000000000) / 1000000000))
	microseconds=$(((execution_time % 1000000000) / 1000))

	printf "COMPILE: %1dH-%1dM-%1dS-%1dus\n" $hours $minutes $seconds $microseconds

	# assemble
	output_obj_file=$output_path/${file_basename%%.sy}.o
	$as_path -c $output_asm_file -o $output_obj_file
	if [ $? -ne 0 ]; then
		echo "assemble $file_basename failed !!!"
		rm -rf $output_path
		exit 1
	fi
	# link
	output_exe_file=$output_path/${file_basename%%.sy}
	$gcc_path $output_obj_file $lib_obj_file -o $output_exe_file -static
	if [ $? -ne 0 ]; then
		echo "link $file_basename failed !!!"
		rm -rf $output_path
		exit 1
	fi

	# run
	output_stdout_file=$output_path/temp.out
	stdin_file=${file%%.sy}.in
	if [ -f $stdin_file ]; then
		$qemu_riscv64_path $output_exe_file >$output_stdout_file <$stdin_file
	else
		$qemu_riscv64_path $output_exe_file >$output_stdout_file
	fi
	echo -e "\n$?" >>$output_stdout_file

	# compare
	# 确保 IFS 设为默认值，即空格、制表符和换行符
	IFS=$' \t\n'

	# 读取我们自己的输出文件，每一行或空白字符作为一个新的数组元素
	declare -a output_arr
	unset output_arr
	while read -a line; do
		output_arr+=("${line[@]}")
	done <"$output_stdout_file"

	# 和答案文件比较
	i=0
	line_number=0

	answer_file=${file%%.sy}.out
	while IFS= read -r line; do
		IFS=' ' read -ra words <<<"$line"
		for word in "${words[@]}"; do
			if [ "$word" != "${output_arr[$i]}" ]; then
				echo "line $line_number: answer($word) != ${output_arr[$i]}"
				echo "test $file_basename failed !!!"
				echo "$file_basename output:"
				cat $output_stdout_file
				echo "$file_basename answer:"
				cat $answer_file
				rm -rf $output_path
				exit 1
			fi
			i=$((i + 1))
		done
		line_number=$((line_number + 1))
	done <"$answer_file"
done

# clean
rm -rf $output_path
