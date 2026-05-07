#!/bin/bash
set -e

# 配置
export JAVA_HOME=/opt/jdk-24
export PATH=$JAVA_HOME/bin:$PATH
SHORT_SHA=$(echo "$CI_COMMIT_SHA" | cut -c1-6)

SRC_DIR=src
BUILD_DIR=build
MAIN_CLASS=Compiler

echo "🛠️ 构建准备中..."
mkdir -p "$BUILD_DIR"

echo "🔧 构建开始..."
echo "📦 正在编译 Java 文件（递归查找）..."
find "$SRC_DIR" -name "*.java" > sources.txt
javac -d "$BUILD_DIR" @sources.txt
rm sources.txt

echo "🧾 生成 MANIFEST.MF..."
echo "Main-Class: $MAIN_CLASS" > "$BUILD_DIR"/MANIFEST.MF

echo "📦 打包 JAR 文件..."
jar cfm "$BUILD_DIR/app.jar" "$BUILD_DIR/MANIFEST.MF" -C "$BUILD_DIR" .

echo "🔧 移动并重命名 .jar 文件"
cp build/app.jar /mnt/artifacts/app_"${SHORT_SHA}".jar

echo "✅ build完成"