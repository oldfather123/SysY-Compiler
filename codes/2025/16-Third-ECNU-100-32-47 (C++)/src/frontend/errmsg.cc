#include "errmsg.h"
#include <cassert>
#include <cstdarg>
#include <cstdio>
#include <functional>

namespace frontend {
namespace err {

void ErrMsg::NewLine() {
    line_start.push_back(token_pos);
}

int ErrMsg::getTokPos() {
    return token_pos;
}

int ErrMsg::getLineOf(int pos) {
    int line = line_start.size() - 1;
    for(;line_start[line] > pos; line --) { }
    return line + 1;
}

void ErrMsg::InsertErrAt(int pos, std::string msg, ...) {
    int line = line_start.size() - 1, col = 0, num = 0;
    va_list ap;

    for(; line_start[line] > pos; line --) { }
    col = pos - line_start[line];
    char *ptr = buf;
    num = sprintf(ptr,"[%d:%d]: ",line + 1,col);
    assert(num >= 0 && "wrong format");
    ptr += num;
    
    va_start(ap, msg);
    num = vsprintf(ptr, msg.c_str(), ap);
    va_end(ap);
    assert(num >= 0 && "wrong format");
    ptr += num;
    *ptr = '\0';
    msgs.push_back(buf);
}

bool ErrMsg::hasErr() {
    return !msgs.empty();
}

void ErrMsg::traverse(std::function<void(std::string)> func) {
    for(auto &err : msgs) {
        func(err);
    }
}

}
}