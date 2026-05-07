#include <map>
#include <memory>
#include <stack>
#include <vector>

#include "Constant.hh"
#include "Value.hh"

class AggregateValue {
 public:
  std::unique_ptr<std::vector<int>> arr_shape;
  std::unique_ptr<std::map<int, Value*>> arr_elems;
  std::unique_ptr<std::vector<int>> arr_width;
  std::unique_ptr<std::stack<int>> depth_stack;

  int depth = 0;
  int ploc = 0;
  int loc = 0;
  int total_size = 0;

  ArrayType* arrayType;

  AggregateValue(ArrayType* arrayType_) : arrayType(arrayType_) {
    arr_shape = std::make_unique<std::vector<int>>();
    Type* elemType = arrayType;
    while (ArrayType* currArrType = dynamic_cast<ArrayType*>(elemType)) {
      arr_shape->push_back(currArrType->getLen());
      elemType = currArrType->getElemType();
    }
    int n = 1;
    int size = arr_shape->size();
    arr_width = std::make_unique<std::vector<int>>(size);
    arr_width->at(size - 1) = 4;
    for (int i = size - 1; i >= 0; i--) {
      n *= arr_shape->at(i);
      if (i == size - 1) {
        arr_width->at(i) = arr_shape->at(i);
      } else {
        arr_width->at(i) = arr_shape->at(i) * arr_width->at(i + 1);
      }
    }

    depth_stack = std::make_unique<std::stack<int>>();
    arr_elems = std::make_unique<std::map<int, Value*>>();
    depth = loc = 0;
    total_size = n;
  }
  void pushValue(Value* v) {
    if (v->isa(VT_INTCONST) && ((IntegerConstant*)v)->getValue() == 0) {
      loc++;
    } else if (v->isa(VT_FLOATCONST) && ((FloatConstant*)v)->getValue() == 0) {
      loc++;
    } else {
      arr_elems->emplace(loc++, v);
    }
  }
  void inBrace() {
    int size = arr_shape->size();
    int nd = depth + 1;
    for (int i = size - 1; i >= depth; i--) {
      if (loc % arr_width->at(i) != 0) {
        nd = i + 1;
        break;
      }
    }
    depth_stack->push(depth);
    depth = nd;
    ploc = loc;
  }
  void outBrace() {
    int size = arr_width->at(0);
    while (loc < size && (loc % arr_width->at(depth) != 0 || ploc == loc)) {
      loc++;
    }
    if (!depth_stack->empty()) {
      depth = depth_stack->top();
      depth_stack->pop();
    }
  }

  ArrayConstant* toArrayConstant() {
    return buildArray(0, 0, total_size, arrayType);
  }

  ArrayConstant* buildArray(int d, int l, int r, ArrayType* type) {
    int md = arr_shape->size();

    ArrayConstant* arrayConstant = new ArrayConstant(type);
    Type* elemType = type->getElemType();
    if (d == md - 1) {
      auto itLow = arr_elems->lower_bound(l);
      auto itUp = arr_elems->upper_bound(r);
      for (auto it = itLow; it != itUp; ++it) {
        arrayConstant->put(it->first - l, it->second);
      }
    } else {
      int width = arr_width->at(d + 1);
      int size = arr_shape->at(d);
      for (int i = 0; i < size; i++) {
        int cl = l + i * width;
        int cr = cl + width;
        ArrayConstant* subArray =
            buildArray(d + 1, cl, cr, (ArrayType*)elemType);
        if (subArray) {
          arrayConstant->put(i, subArray);
        }
      }
    }

    if (d != 0 && arrayConstant->isZeorInit()) {
      delete arrayConstant;
      return nullptr;
    }
    return arrayConstant;
  }
};