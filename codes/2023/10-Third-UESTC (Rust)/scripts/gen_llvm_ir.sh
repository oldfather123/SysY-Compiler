#!/bin/bash
set -e

if (($# >= 1)); then
	pathorfile=$1
	output_path=$2
	if test -d $pathorfile; then
		path=$pathorfile # is path
		for file in $path/*.c; do
			echo "$file"
			source_file_basename=$(basename $file)
			llvm-gcc -emit-llvm -S -O0 -Xclang -disable-O0-optnone $file -o "${output_path}/${source_file_basename%%.c}.ll"
		done
	else
		file=$pathorfile # is file
		source_file_basename=$(basename $file)
		llvm-gcc -emit-llvm -S -O0 -Xclang -disable-O0-optnone $file -o "${output_path}/${source_file_basename%%.c}.ll"
	fi
else
	echo "use it like $: sh ./scripts/gen_llvm_ir.sh test/functional/30_continue.c llvm/functional or \
$: sh ./scripts/gen_llvm_ir.sh test/functional/ llvm/functional"
fi

# for cfile in *.c; do
# llvm-gcc -emit-llvm -S -O0 $cfile -o "${cfile%%.c}.ll"
# done
