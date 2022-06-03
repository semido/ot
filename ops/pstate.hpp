#include "ops.hpp"
#include "../igush/igush_array.h"

template<typename T>
class PState
{
  IgushArray<T>& data;
public:
  PState(IgushArray<T>& v) : data(v) {}

  const IgushArray<T>& get() const { return data; }

  PState& operator << (const OpDescriptor<T>& op)
  {
    if (apply(op))
      return *this;
    throw std::exception("cannot apply op on state");
  }

  PState& operator << (const OpPack<T>& pack)
  {
    for (int i = 0; i < pack.ops.size(); i++)
      if (!apply(pack.ops[i]))
        throw std::exception("cannot apply op on state");
    return *this;
  }

  bool apply(const OpDescriptor<T>& op)
  {
    if (op.typ == OpType::nothing)
      return true;
    if (op.typ == OpType::insert && op.pos == data.size()) {
      data.push_back(op.value);
      return true;
    }
    if (op.pos >= data.size())
      return false;
    if (op.typ == OpType::insert)
      data.insert(data.begin() + op.pos, op.value);
    else if (op.typ == OpType::remove)
      data.erase(data.begin() + op.pos);
    else if (op.typ == OpType::update)
      data[op.pos] = op.value;
    return true;
  }
};
