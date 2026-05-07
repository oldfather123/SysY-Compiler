check_return_value() {
	rv=$1
	expected_rv=$2
	fail_msg=$3
	detail=$4
	if [ "$rv" -eq "$expected_rv" ]; then
		return 0
	else
		printf "\033[1;31m%s\033[0m: %s\n" "$fail_msg" "$detail"
        printf "$fail_msg: $detail\n" >&2 
		return 1
	fi
}

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <testcase> <basename>"
    exit 1
fi
testcase="$1"
basename="$2"
asm_file="output/$testcase/$basename.s"
exe_file="output/$testcase/$basename"
obj_file="output/$testcase/$basename.o"
out_file="output/$testcase/$basename.out"

std_out_file="testcases/$testcase/$basename.out"
in_file="testcases/$testcase/$basename.in"
sylib_file="sylib/sylib.a"


# generate objfile using as
as "$asm_file" -o "$obj_file" >&2 
check_return_value $? 0 "AS" "unable to generate obj file" || exit 1

# generate exe file using ld
ld "$obj_file" "$sylib_file" "/usr/lib64/crt1.o" "/usr/lib64/crtn.o" "/usr/lib64/crti.o" -lc -o "$exe_file" >&2 
check_return_value $? 0 "LD" "unable to generate exe file" || exit 1 

# run the exe file
# check if in file exists
if [ ! -f "$in_file" ]; then
    "$exe_file" > "$out_file"
else
    "$exe_file" < "$in_file" > "$out_file"
fi
ret=$?


sed -i "\$s/\x00*$//" "$out_file"
# append return value
# Check if the last character of the output file is not a newline
if [ -s "$out_file" ]; then
	# Check if the last character of the output file is not a newline
	if [ "$(tail -c1 "$out_file" | wc -l)" -eq 0 ]; then
		echo "" >> "$out_file"  
	fi
fi

echo $ret >>"$out_file"

# compare output
diff --ignore-space-change --strip-trailing-cr "$std_out_file" "$out_file" -y >&2 

check_return_value $? 0 "WA" "output differ, check $std_out_file and $out_file" || exit 2

printf "\033[1;32m%s\033[0m: %s\n" "AC" "output matched"
printf "AC: output matched\n" >&2