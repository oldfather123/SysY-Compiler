#include "../include/util.h"

#include <stdio.h>
#include <stdlib.h>

#include "../include/lexer.h"
#include "../include/sysy.h"
void *sy_malloc(size_t size) {
    void *ptr = malloc(size);
    memset(ptr, 0, size);
    if (!ptr) sy_error("out of memory");
    return ptr;
}

void sy_error(const char *fmt, ...) {
    char    buf[1024 * 4];
    va_list ap;
    va_start(ap, fmt);
    vsprintf(buf, fmt, ap);
    fprintf(stderr, "compiler:error:line %d:%s", yylineno, buf);
    exit(-1);
}