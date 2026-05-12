#pragma once

#include "codegen/Defs.h"

#include <iosfwd>

namespace codegen {

class IRDumper {
public:
    explicit IRDumper(std::ostream &out);

    void dump(const Region &region);

private:
    std::ostream &out_;
};

} // namespace codegen
