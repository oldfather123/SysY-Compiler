#!/usr/bin bash
set -e
SHORT_SHA=$(echo "$CI_COMMIT_SHA" | cut -c1-6)

export COMPILED_DIR=${COMPILED_DIR:-/mnt/res/compiled_${SHORT_SHA}}
export LL_DIR="${COMPILED_DIR}/ll"

export SYLIB_O_PATH=/root/libs/sylib.o

echo "🚀 启动测试 ------ ..."
export TESTFILE_DIR=/mnt/dataset/functional
python scripts/run_clang_test.py


