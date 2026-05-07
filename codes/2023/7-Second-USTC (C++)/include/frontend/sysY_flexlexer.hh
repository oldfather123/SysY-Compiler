#pragma once

#ifndef YY_DECL
#define YY_DECL                                                         \
    yy::sysY_parser::symbol_type sysY_flexlexer::yylex(sysY_driver& driver)  // 定义宏 YY_DECL，用于声明 yylex 函数
#endif

// 包含 FlexLexer.h 头文件，为了使用 yyFlexLexer 类，同时需要先取消定义 yyFlexLexer 宏
#undef yyFlexLexer
#include "FlexLexer.hh"

// 包含 sysY_parser.hh 头文件，为了使用 sysY_parser 和 sysY_driver 类型
#include "sysY_parser.hh"

// We need this for the yy::location type:
// 包含 location.hh 头文件，为了使用 yy::location 类型
#include "location.hh"

// 定义 sysY_flexlexer 类，继承自 yyFlexLexer 类
class sysY_flexlexer : public yyFlexLexer {
public:
    // 使用父类的构造函数
    using yyFlexLexer::yyFlexLexer;

    // 提供 yylex 函数的接口，由 flex 工具生成实现代码
    yy::sysY_parser::symbol_type yylex(sysY_driver& driver);

    // 将 yy::location 对象放在这里似乎是个合理的选择，而不是将其作为静态变量（在翻译单元范围内具有内部链接，而不是作为类变量）。
    yy::location loc;
};