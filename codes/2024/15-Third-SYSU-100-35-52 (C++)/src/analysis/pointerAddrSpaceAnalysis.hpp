#pragma  once
#include "Value.h"
#include "Function.h"
#include "domTreeAnalysis.h"

enum class AddressSpaceType { InternalStack = 1 << 0 };

class PointerAddressSpaceAnalysisResult final {
    std::unordered_map<Value*, AddressSpaceType> mMappings;

public:
    void addTag(Value* ptr, AddressSpaceType space);
    bool isTagged(Value* ptr) const;
    AddressSpaceType getAddressSpace(Value* ptr) const;
    bool mayBe(Value* ptr, AddressSpaceType space) const;
    bool mustBe(Value* ptr, AddressSpaceType space) const;
};


PointerAddressSpaceAnalysisResult runPointerAddrSpaceAnalysis(FunctionPtr func);