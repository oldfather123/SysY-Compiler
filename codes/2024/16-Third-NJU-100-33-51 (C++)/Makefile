.IGNORE: llvm-test
.PHONY: clean all 

ANTLR_PATH = $(shell find /usr/local/lib -name "antlr-*-complete.jar")
ANTLR = java -jar $(ANTLR_PATH) -listener -visitor -long-messages
PFILE = $(shell find . -name "SysYParser.g4")

all:
	cmake -DCMAKE_BUILD_TYPE=Debug -B build
	cmake --build build

debug:
	@cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-DDEBUG_MODE" -B build
	@cmake --build build

BIN_DIR = build
IRFile = tests/test.ll
BCFile = $(patsubst %.ll,%.bc,$(IRFile))
ASMFile = $(patsubst %.ll,%.s,$(IRFile))
OBJFile = $(patsubst %.ll,%.o,$(IRFile))
BINFile = $(patsubst %.ll,%,$(IRFile))
run:
	$(BIN_DIR)/compiler -l -o tests/test.ll tests/test.sy

runs:
	$(BIN_DIR)/compiler -S -o tests/test.s tests/test.sy
	riscv64-linux-gnu-gcc-10 -fPIE -c tests/test.s -o tests/test.o
	riscv64-linux-gnu-gcc-10 tests/test.o -Ltests -lrvsysy -o tests/test
	qemu-riscv64 -L /usr/riscv64-linux-gnu -s 1024M tests/test < tests/test.in; echo $$?

runso:
	$(BIN_DIR)/compiler -S -o tests/test.reg.s tests/test.sy -O1
	riscv64-linux-gnu-gcc-10 -fPIE -c tests/test.reg.s -o tests/test.reg.o
	riscv64-linux-gnu-gcc-10 tests/test.reg.o -Ltests -lrvsysy -o tests/test.reg
	qemu-riscv64 -L /usr/riscv64-linux-gnu -s 1024M tests/test.reg < tests/test.in; echo $$?

gcc-riscv:
	riscv64-linux-gnu-gcc-10 -fPIE -c tests/test.s -o tests/test.o
	riscv64-linux-gnu-gcc-10 tests/test.o -Ltests -lrvsysy -o tests/test
	qemu-riscv64 -L /usr/riscv64-linux-gnu -s 1024M tests/test < tests/test.in; echo $$?


gcc-riscvo:
	riscv64-linux-gnu-gcc-10 -fPIE -c tests/test.reg.s -o tests/test.reg.o
	riscv64-linux-gnu-gcc-10 tests/test.reg.o -Ltests -lrvsysy -o tests/test.reg
	qemu-riscv64 -L /usr/riscv64-linux-gnu -s 1024M tests/test.reg < tests/test.in; echo $$?

# Test llvm
# Input: llvm ir
# Output: return value
llvm-test: run
	llvm-as -opaque-pointers $(IRFile) -o $(BCFile)
	llc -opaque-pointers -relocation-model=pic $(BCFile) -o $(ASMFile)
	clang -fPIE $(ASMFile) -L tests/ -lsysy -o $(BINFile) 
	./$(BINFile) < tests/test.in; echo $$?

EXE := tests/test
INPUT := tests/test.in
qemu-run:
	qemu-riscv64 -L /usr/riscv64-linux-gnu -s 1024M $(EXE) < $(INPUT)

PORT := 1237
qemu-debug:
	qemu-riscv64 -cpu sifive-u54 -L /usr/riscv64-linux-gnu -g $(PORT) $(EXE) &
	gdb-multiarch -q \
		-ex "set sysroot /usr/riscv64-linux-gnu" \
		-ex "file $(EXE)" \
		-ex "target remote localhost:$(PORT)" \
		-ex "break main" \
		-ex "continue"

EXEO := tests/test.reg
PORTO := 1238
qemu-debugo:
	qemu-riscv64 -cpu sifive-u54 -L /usr/riscv64-linux-gnu -g $(PORTO) $(EXEO) &
	gdb-multiarch -q \
		-ex "set sysroot /usr/riscv64-linux-gnu" \
		-ex "file $(EXEO)" \
		-ex "target remote localhost:$(PORTO)" \
		-ex "break main" \
		-ex "continue"

clean:
	rm -rf build
	rm -rf ./parser/antlr4/*.cpp
	rm -rf ./parser/antlr4/*.h
	rm -rf ./parser/antlr4/*.tokens
	rm -rf ./parser/antlr4/*.interp

antlr: $(PFILE)
	$(ANTLR) -Dlanguage=Cpp $(PFILE)


# docker test
# https://pan.educg.net/#/s/V2oiq?path=%2F
# `docker load -i riscv_v3.tar` to load image
# docker-test:
# 	docker run --rm -it -v ./:/coursegrader/submitdata cg/compile-x86:rust2ndclang12 bash

# Test in LLVM IR form: make test MODE=LLVM
# Test in RISCV form: make test MODE=RISCV
MODE := LLVM
test:
	rm -rf tmp
	$(shell [ -d tests/compiler2023 ] || git clone https://gitlab.eduxiji.net/csc1/nscscc/compiler2023.git tests/compiler2023)
	python3 tests/scripts/test.py $(MODE)

single-test:
	gdb build/compiler -x init.gdb

PER_DIR := /home/sprooc/compiler2024/antpie/tests/compiler2024/testdata/performance/
per-test:
	$(BIN_DIR)/compiler -S -o tests/test.s $(PER_DIR)$(PFILE).sy -O1
	riscv64-linux-gnu-gcc-10 -fPIE -c tests/test.s -o tests/test.o
	riscv64-linux-gnu-gcc-10 tests/test.o -Ltests -lrvsysy -o tests/test
	qemu-riscv64 -L /usr/riscv64-linux-gnu -s 1024M tests/test < $(PER_DIR)$(PFILE).in; echo $$?
