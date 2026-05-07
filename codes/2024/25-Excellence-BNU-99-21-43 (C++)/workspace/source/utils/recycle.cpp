#include "recycle.h"
#include <iostream>

namespace utils
{
    Recycle* Recycle::instance = nullptr;

    Recycle* Recycle::getInstance()
    {
        if (!instance)
        {
            instance = new Recycle();
        }
        return instance;
    }

    void Recycle::free()
    {
        
        auto instance = getInstance();
        std::cerr << "Recycle free" << ' ' << instance->RecycleBin.size() << std::endl;
        for (auto& item : instance->RecycleBin)
        {
            item.second(item.first); // 调用删除器删除指针
        }
        instance->RecycleBin.clear(); // 清空 RecycleBin
    }

    void Recycle::free(void *ptr, std::function<void(void*)> deleter)
    {
        if (ptr)
        {
            Recycle::getInstance()->RecycleBin.emplace_back(ptr, deleter);
        }
    }
}