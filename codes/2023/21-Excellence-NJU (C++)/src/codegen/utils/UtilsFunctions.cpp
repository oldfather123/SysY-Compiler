#include "codegen/utils/UtilFunctions.hpp"
#include <algorithm>
#include <cctype>

namespace codegen_utils {

std::string lower(std::string_view enum_item) {
    std::string ret(enum_item);
    std::transform(ret.begin(), ret.end(), ret.begin(), ::tolower);
    return ret;
}

}