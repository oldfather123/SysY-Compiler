#include "../include/parallel_runtime.h"
#include <string.h>

// 线程函数包装器
void* thread_wrapper(void* arg) {
    thread_wrapper_args_t* wrapper_arg = (thread_wrapper_args_t*)arg;
    
    // 调用实际的循环函数
    wrapper_arg->func_ptr(&wrapper_arg->thread_args);
    
    return NULL;
}

// 并行化运行时库函数实现
void parallel_loop_execute(parallel_loop_func_t func_ptr, int start, int end, int num_threads) {
    if (num_threads <= 0 || start >= end) {
        return;
    }
    
    // 限制最大线程数
    if (num_threads > 8) {
        num_threads = 8;
    }
    
    // 计算每个线程的工作量
    int total_work = end - start;
    int work_per_thread = total_work / num_threads;
    int remainder = total_work % num_threads;
    
    // 创建线程数组和包装参数数组
    pthread_t* threads = (pthread_t*)malloc(num_threads * sizeof(pthread_t));
    thread_wrapper_args_t* wrapper_args = (thread_wrapper_args_t*)malloc(num_threads * sizeof(thread_wrapper_args_t));
    
    // 创建并启动线程
    for (int i = 0; i < num_threads; i++) {
        // 计算当前线程的工作范围
        int thread_start = start + i * work_per_thread;
        int thread_end = thread_start + work_per_thread;
        
        // 最后一个线程处理剩余的工作
        if (i == num_threads - 1) {
            thread_end += remainder;
        }
        
        // 设置包装参数
        wrapper_args[i].func_ptr = func_ptr;
        wrapper_args[i].thread_args.thread_id = i;
        wrapper_args[i].thread_args.start = thread_start;
        wrapper_args[i].thread_args.end = thread_end;
        
        // 创建线程
        int ret = pthread_create(&threads[i], NULL, thread_wrapper, &wrapper_args[i]);
        if (ret != 0) {
            fprintf(stderr, "创建线程失败: %d\n", ret);
            // 清理已创建的线程
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            free(threads);
            free(wrapper_args);
            return;
        }
    }
    
    // 等待所有线程完成
    for (int i = 0; i < num_threads; i++) {
        pthread_join(threads[i], NULL);
    }
    
    // 清理资源
    free(threads);
    free(wrapper_args);
}