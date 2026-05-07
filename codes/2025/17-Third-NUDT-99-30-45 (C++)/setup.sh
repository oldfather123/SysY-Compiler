tar -xzf antlr_src.tar.gz
mkdir -p build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug 
cmake --build build -- -j$(nproc)

echo "构建成功。用此命令启动SysY编译器: build/bin/sysyc -h"
echo "后续重新构建命令:"
echo "cmake -B build -S . -DCMAKE_BUILD_TYPE=Debug"
echo "cmake --build build -- -j$(nproc)"