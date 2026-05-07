#!/usr/bin/env bash
set -e

export JAVA_HOME=/opt/jdk-24
export PATH=$JAVA_HOME/bin:$PATH
SHORT_SHA=$(echo "$CI_COMMIT_SHA" | cut -c1-6)

mkdir -p artifacts

export JAR_DIR=/mnt/artifacts/app_"${SHORT_SHA}".jar
export TESTFILE_DIR=/mnt/dataset/functional
export COMPILED_DIR=${COMPILED_DIR:-/mnt/res/compiled_${SHORT_SHA}}
export LL_DIR="${COMPILED_DIR}/ll"
export LL_ZIP_DIR="artifacts"  # ✅ 设置 zip 存放目录
export CI_COMMIT_SHA="${SHORT_SHA}"

echo "🚀 批量编译 ------ ..."
echo "📁 创建输出目录：$LL_DIR"
mkdir -p "$LL_DIR"

python scripts/compile_sy.py -O1

echo "✅ 完成编译与打包：$LL_ZIP_DIR/compiled_ll.zip"
