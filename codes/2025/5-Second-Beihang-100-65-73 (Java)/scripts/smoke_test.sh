#!/usr/bin/env bash
set -e

# 配置环境
export JAVA_HOME=/opt/jdk-24
export PATH=$JAVA_HOME/bin:$PATH
SHORT_SHA=$(echo "$CI_COMMIT_SHA" | cut -c1-6)

JAR_DIR=/mnt/artifacts/app_"${SHORT_SHA}".jar
BUILD_DIR=build
LIB_DIR=/root/libs
TESTFILE_DIR=/mnt/dataset
COMPILED_DIR=/mnt/res/compiled_"${SHORT_SHA}"
STDOUT_RES_DIR=/mnt/res/stdout_"${SHORT_SHA}"

echo "🛠️ 进入构建目录..."
mkdir -p "$BUILD_DIR"
mkdir -p "$COMPILED_DIR"
mkdir -p "$STDOUT_RES_DIR"
cp "$TESTFILE_DIR/in.sy" "$BUILD_DIR/"
cp "$LIB_DIR/sylib.o" "$BUILD_DIR/"

cd "$BUILD_DIR"

echo "🌀 使用 Java 编译 in.sy 到 LLVM IR..."
java -jar ${JAR_DIR} -S in.sy -ll out.ll -O0 -mid
cp out.ll "$COMPILED_DIR/"
echo "✅ Java 编译完成：生成 out.ll"

if [[ ! -f "out.ll" ]]; then
  echo "❌ 错误：out.ll 未生成，Java 编译失败"
  exit 1
fi

echo "⚙️ Clang 编译 out.ll + sylib.o..."
clang -c out.ll -o out.o
clang out.o sylib.o -o main
echo "✅ 可执行文件生成完成：main"

echo "🚀 运行程序..."
set +e
./main > testoutput.txt
set -e
echo $? >> testoutput.txt
cp testoutput.txt "$STDOUT_RES_DIR/"
echo "✅ 程序运行完毕"
