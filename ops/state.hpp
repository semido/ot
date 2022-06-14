#pragma once

#include "ops.hpp"

template<typename V>
inline bool StateEq(const V& v1, const V& v2)
{
  if (v1.size() != v2.size())
    return false;
  for (int i = 0; i != v1.size(); i++)
    if (v1[i] != v2[i])
      return false;
  return true;
}

// Apply ops and packs on data container representing the state.
// Requirements for C: value_type, size, push_back, insert, begin, erase, operator[].
template<typename C>
class State
{
  C* data = nullptr; // Weak ptr to external data container.
  using T = typename C::value_type;
public:
  State() = default;
  State(C& d) : data(&d) {}

  inline void set(C* d) { data = d; }

  inline unsigned size() const { return data ? (unsigned) data->size() : 0; }

  inline std::string str() const
  {
    assert(data);
    std::string s = "[" + std::to_string(size()) + "]";
    for (auto& e : *data)
      s += " " + std::to_string(e);
    return s;
  }

  // Do not transform, just apply subsequent ops. Op must be transformed if there was a concurrence.
  inline State& operator << (const OpDescriptor<T>& op)
  {
    apply(op);
    return *this;
  }

  // Apply concurrent ops of the pack.
  inline void apply(const OpPack<T>& pack)
  {
    OpPack<T> appliedPack;
    for (auto op : pack)
      apply(appliedPack, op);
  }

  // Apply concurrent ops of src pack with consideration of some applied ops.
  inline void applyFromOthers(unsigned cid, OpPack<T>& appliedPack, const OpPack<T>& srcPack)
  {
    for (auto op : srcPack) {
      if (cid == op.cid)
        continue; // Own op. Assume it's already applied and present in pack.
      apply(appliedPack, op);
    }
  }

  // Transform op considering an effect of previous ops from the pack.
  // Apply op to state.
  inline void apply(OpPack<T>& pack, OpDescriptor<T> op)
  {
    pack.transformAndPut(op);
    apply(op);
  }

  inline void apply(const OpDescriptor<T>& op)
  {
    assert(data);
    if (op.typ == OpType::nothing)
      return;
    if(op.typ == OpType::insert && op.posser >= size()) {
      data->push_back(op.value);
      return;
    }
    if (size() == 0)
      return;
    auto pos1 = op.posser; // Bound.
    if(pos1 >= size())
      pos1 = size() - 1;
    if(op.typ == OpType::insert)
      data->insert(data->begin() + pos1, op.value);
    else if (op.typ == OpType::remove)
      data->erase(data->begin() + pos1);
    else if (op.typ == OpType::update)
      (*data)[pos1] = op.value;
    return;
  }
};
