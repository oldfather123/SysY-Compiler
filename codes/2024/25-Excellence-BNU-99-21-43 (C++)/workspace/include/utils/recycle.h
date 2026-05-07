#pragma once
#include <vector>
#include <functional>

namespace utils
{
    struct Recycle
    {
        static Recycle *instance;

        // 使用 pair 存储指针和对应的删除器
        std::vector<std::pair<void*, std::function<void(void*)>>> RecycleBin;

        Recycle() = default;

        static Recycle *getInstance();

        // 清理 RecycleBin 中的所有指针
        static void free();

        static void free(void *ptr, std::function<void(void*)> deleter);

        
    };
}