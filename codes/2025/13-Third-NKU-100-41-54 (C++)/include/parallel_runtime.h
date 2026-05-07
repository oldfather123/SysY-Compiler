#ifndef PARALLEL_RUNTIME_H
#define PARALLEL_RUNTIME_H

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

// 线程函数类型定义
typedef void (*parallel_loop_func_t)(void* args);

// 线程参数结构体
typedef struct {
    int thread_id;
    int start;
    int end;
} thread_args_t;

// 线程包装参数结构体
typedef struct {
    parallel_loop_func_t func_ptr;
    thread_args_t thread_args;
} thread_wrapper_args_t;

// 并行化运行时库函数声明
void parallel_loop_execute(parallel_loop_func_t func_ptr, int start, int end, int num_threads);

// 线程函数包装器
void* thread_wrapper(void* arg);

#endif // PARALLEL_RUNTIME_H
