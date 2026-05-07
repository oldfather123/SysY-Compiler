// io.c
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/time.h>
#include <stdbool.h>
#include "io.h"

// 全局标志，跟踪是否已经有输出
static bool has_output = false;

// 在程序退出时检查是否需要换行
void ensure_newline_before_exit() {
    if (has_output) {
        printf("\n");
        has_output = false;
    }
}

// 注册退出处理函数
__attribute__((constructor))
void register_exit_handler() {
    atexit(ensure_newline_before_exit);
}

void putint(int x) {
    printf("%d", x);
    has_output = true;
}

int getint() {
    int x;
    scanf("%d", &x);
    return x;
}

void putch(int ch) {
    putchar(ch);
    has_output = true;
    // 如果输出了换行符，重置标志
    if (ch == '\n' || ch == 10) {
        has_output = false;
    }
}

int getch() {
    return getchar();
}

void putfloat(float f) {
    // 使用十六进制浮点格式输出
    printf("%a", f);
    has_output = true;
}

float getfloat() {
    float f;
    scanf("%f", &f);
    return f;
}

int getarray(int *arr) {
    int n;
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        scanf("%d", &arr[i]);
    }
    return n;
}

void putarray(int n, int *arr) {
    printf("%d:", n);
    for (int i = 0; i < n; i++) {
        printf(" %d", arr[i]);
    }
    has_output = true;
}

int getfarray(float *arr) {
    int n;
    scanf("%d", &n);
    for (int i = 0; i < n; i++) {
        scanf("%f", &arr[i]);
    }
    return n;
}

void putfarray(int n, float *arr) {
    printf("%d:", n);
    for (int i = 0; i < n; i++) {
        // 使用十六进制浮点格式输出
        printf(" %a", arr[i]);
    }
    has_output = true;
}

void putf(char a[], ...) {
    va_list args;
    va_start(args, a);
    vfprintf(stdout, a, args);
    va_end(args);
    has_output = true;
}

/* Timing function implementation */
struct timeval _sysy_start, _sysy_end;
int _sysy_l1[_SYSY_N], _sysy_l2[_SYSY_N];
int _sysy_h[_SYSY_N], _sysy_m[_SYSY_N], _sysy_s[_SYSY_N], _sysy_us[_SYSY_N];
int _sysy_idx;

__attribute__((constructor)) 
void before_main() {
    for(int i = 0; i < _SYSY_N; i++)
        _sysy_h[i] = _sysy_m[i] = _sysy_s[i] = _sysy_us[i] = 0;
    _sysy_idx = 1;
}

__attribute__((destructor)) 
void after_main() {
    for(int i = 1; i < _sysy_idx; i++) {
        fprintf(stderr, "Timer@%04d-%04d: %dH-%dM-%dS-%dus\n",
            _sysy_l1[i], _sysy_l2[i], _sysy_h[i], _sysy_m[i], _sysy_s[i], _sysy_us[i]);
        _sysy_us[0] += _sysy_us[i];
        _sysy_s[0] += _sysy_s[i]; _sysy_us[0] %= 1000000;
        _sysy_m[0] += _sysy_m[i]; _sysy_s[0] %= 60;
        _sysy_h[0] += _sysy_h[i]; _sysy_m[0] %= 60;
    }
    fprintf(stderr, "TOTAL: %dH-%dM-%dS-%dus\n", _sysy_h[0], _sysy_m[0], _sysy_s[0], _sysy_us[0]);
}

void _sysy_starttime(int lineno) {
    _sysy_l1[_sysy_idx] = lineno;
    gettimeofday(&_sysy_start, NULL);
}

void _sysy_stoptime(int lineno) {
    gettimeofday(&_sysy_end, NULL);
    _sysy_l2[_sysy_idx] = lineno;
    _sysy_us[_sysy_idx] += 1000000 * (_sysy_end.tv_sec - _sysy_start.tv_sec) + _sysy_end.tv_usec - _sysy_start.tv_usec;
    _sysy_s[_sysy_idx] += _sysy_us[_sysy_idx] / 1000000; _sysy_us[_sysy_idx] %= 1000000;
    _sysy_m[_sysy_idx] += _sysy_s[_sysy_idx] / 60; _sysy_s[_sysy_idx] %= 60;
    _sysy_h[_sysy_idx] += _sysy_m[_sysy_idx] / 60; _sysy_m[_sysy_idx] %= 60;
    _sysy_idx++;
}

// 兼容性函数
int input() {
    return getint();
}

void output(int x) {
    putint(x);
    printf("\n");
    has_output = false;  // output 函数自带换行，所以重置标志
}

void outputFloat(float f) {
    // 使用十六进制浮点格式输出
    printf("%a\n", f);
    has_output = false;  // outputFloat 函数自带换行，所以重置标志
}

void neg_idx_except() {
    fprintf(stderr, "Error: negative array index\n");
    exit(1);
}