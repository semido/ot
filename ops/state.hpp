#include "ops.hpp"

template<typename V>
bool StateEq(const V& v1, const V& v2)
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
  C& data;
  using T = typename C::value_type;
public:
  State(C& v) : data(v) {}

  inline const C& get() const { return data; }

  // Do not transform, just apply subsequent ops. Op must be transformed if there was a concurrence.
  inline State& operator << (const OpDescriptor<T>& op)
  {
    apply(op);
    return *this;
  }

  // Apply concurrent ops of the pack.
  // Ops with same cid assumed sequential, should not be reordered or transformed in its subset.
  inline void apply(const OpPack<T>& pack)
  {
    OpPack<T> appliedPack;
    for (auto op : pack)
      apply(appliedPack, op);
  }

  // Apply concurrent ops of the pack with consideration of some applied ops.
  // Ops with same cid assumed sequential, should not be reordered or transformed in its subset.
  inline void apply(OpPack<T>& appliedPack, const OpPack<T>& srcPack)
  {
    for (auto op : srcPack) {
      if (appliedPack.size() && appliedPack[0].cid == op.cid)
        continue; // Own op. Assume it's already applied and present in pack.
      apply(appliedPack, op);
    }
  }

  // Transform op considering an effect of previous ops from the pack.
  // Also transform the pack to reflect the fact of the state update.
  // Apply op to state.
  inline void apply(OpPack<T>& pack, OpDescriptor<T> op)
  {
    pack.transformAndPut(op);
    apply(op);
  }

  inline void apply(const OpDescriptor<T>& op)
  {
    if (op.typ == OpType::nothing)
      return;
    if(op.typ == OpType::insert && op.pos >= data.size()) {
      data.push_back(op.value);
      return;
    }
    if (data.empty())
      return;
    auto pos1 = op.pos; // Bound.
    if(pos1 >= data.size())
      pos1 = (unsigned) data.size() - 1;
    if(op.typ == OpType::insert)
      data.insert(data.begin() + pos1, op.value);
    else if (op.typ == OpType::remove)
      data.erase(data.begin() + pos1);
    else if (op.typ == OpType::update)
      data[pos1] = op.value;
    return;
  }
};
