#ifndef _UTIL_H
#define _UTIL_H

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>


/**
 * @brief 发出错误并退出编译程序
 * @param[in] *fmt 错误消息
 */
void  sy_error(const char *fmt, ...);

/**
 * @brief 内存分配
 * @param[in] size
 */
void *sy_malloc(size_t size);

#define sy_free free

#endif