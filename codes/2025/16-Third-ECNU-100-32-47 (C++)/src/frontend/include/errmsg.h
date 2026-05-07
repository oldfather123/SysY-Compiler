#ifndef ERRMSG_H
#define ERRMSG_H

#pragma once

#include <vector>
#include <string>
#include <functional>

class Scanner;

namespace frontend {
namespace err {

#define MAX_ERR_LEN 1024


class ErrMsg {
    friend class ::Scanner;

public:
    ErrMsg():token_pos(0),line_start(1, 1),msgs(0) {}
    void InsertErrAt(int pos, std::string msg, ...);
    void NewLine();
    int getLineOf(int pos);
    int getTokPos();
    bool hasErr();
    void traverse(std::function<void(std::string)> func);

private:
    int token_pos;
    std::vector<int> line_start;
    std::vector<std::string> msgs;
    char buf[MAX_ERR_LEN];
};

}
} // namespace frontend
#endif