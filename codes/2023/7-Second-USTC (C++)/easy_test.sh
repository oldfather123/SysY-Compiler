bin="test"
input_option="test.in"

riscv64-linux-gnu-gcc -o $bin -g test.s lib/sylib.c
qemu-riscv64 $bin < $input_option
RESULT=$?
echo ""
echo $RESULT

