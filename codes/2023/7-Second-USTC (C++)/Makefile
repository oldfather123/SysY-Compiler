.PHONY: all clean

CMAKE_FLAGS := -DCMAKE_BUILD_TYPE=Debug
PORT := 1234
EXE := test
INPUT := test.in

all:
	@cmake $(CMAKE_FLAGS) -B build
	@cmake --build build

gdb_debug:
	qemu-riscv64 -L /usr/riscv64-linux-gnu -g $(PORT) $(EXE) &
	riscv64-unknown-elf-gdb -q \
		-ex "set sysroot /usr/riscv64-linux-gnu" \
		-ex "file $(EXE)" \
		-ex "target remote localhost:$(PORT)" \
		-ex "break main" \
		-ex "continue"

qemu_server:
	qemu-riscv64 -L /usr/riscv64-linux-gnu -s 1024M -g $(PORT) $(EXE) < $(INPUT)

cfg:
	@python3 scripts/cfg_visualize.py

test_asm:
	@python3 scripts/asm_test.py

test_ir:
	@python3 scripts/ir_test.py

gen_asm_by_llc:
	@python3 scripts/gen_rv64asm_by_llc.py

scan_asm_insts:
	@python3 scripts/scan_asm_insts.py

scan_ir_insts:
	@python3 scripts/scan_ir_insts.py

install:
	@cmake --build build --target install

opt_cmp:
	@python3 scripts/opt_compare.py

clean:
	rm -rf build