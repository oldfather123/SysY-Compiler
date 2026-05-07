#include "Common.h"

NullStream null_out;

String DataType::toString() const {
    String str = "";
    for (int i = 0; i < this->arrayDimensionCount(); ++i) {
        auto arraySize = this->arraySizes()[i];
        str += "[";
        if (arraySize != UNSPECIFIED_ARRAY_SIZE) {
            str += std::to_string(arraySize) + " x ";
        }
    }
    switch (_baseDataType) {
        case PrimaryDataType::Unknown: {
            str += "<unknown_data_type>";
            break;
        }
        case PrimaryDataType::Void: {
            str += "void";
            break;
        }
        case PrimaryDataType::Int: {
            str += _is64Bit ? "i64" : "i32";
            break;
        }
        case PrimaryDataType::Float: {
            str += "f32";
            break;
        }
        case PrimaryDataType::Bool: {
            str += "i1";
            break;
        }
        case PrimaryDataType::String: {
            str += "str";
            break;
        }
        default: {
            break;
        }
    }
    for (int i = 0; i < this->arrayDimensionCount(); ++i) {
        str += "]";
    }
    if (this->_isPointer) {
        str += "*";
    }
    return str;
}
