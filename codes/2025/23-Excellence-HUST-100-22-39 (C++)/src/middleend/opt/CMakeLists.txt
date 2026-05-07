cmake_minimum_required(VERSION 3.21)

# 设置源文件
set(SOURCE_FILES
    opt.cpp
    LT.cpp
    Mem2Reg.cpp
    ConstantAnalysis.cpp
    InstCombine.cpp
    DCE.cpp
    SimplifyCFG.cpp
    Loop.cpp LoopInit.cpp LICM.cpp LIVS.cpp LoopUnrolling.cpp
    CSE.cpp
    FuncInfo.cpp FuncInline.cpp
    TRE.cpp
)

# 添加 opt 静态库
add_library(opt STATIC ${SOURCE_FILES})

# 添加头文件路径
target_include_directories(opt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# 链接依赖
target_link_libraries(opt PUBLIC ir)