#ifndef __IO_H__
#define __IO_H__

#include <stdio.h>
#include <stdarg.h>
#include <sys/time.h>

/* Input & output functions */
int getint();
int getch();
float getfloat();
int getarray(int *arr);
int getfarray(float *arr);

void putint(int x);
void putch(int ch);
void putfloat(float f);
void putarray(int n, int *arr);
void putfarray(int n, float *arr);
void putf(char a[], ...);

/* Timing function implementation */
extern struct timeval _sysy_start, _sysy_end;
#define starttime() _sysy_starttime(__LINE__)
#define stoptime()  _sysy_stoptime(__LINE__)
#define _SYSY_N 1024
extern int _sysy_l1[_SYSY_N], _sysy_l2[_SYSY_N];
extern int _sysy_h[_SYSY_N], _sysy_m[_SYSY_N], _sysy_s[_SYSY_N], _sysy_us[_SYSY_N];
extern int _sysy_idx;

__attribute__((constructor)) void before_main();
__attribute__((destructor)) void after_main();
void _sysy_starttime(int lineno);
void _sysy_stoptime(int lineno);

/* 兼容性函数 */
int input();
void output(int x);
void outputFloat(float f);

/* 错误处理 */
void neg_idx_except();

#endif /* __IO_H__ */