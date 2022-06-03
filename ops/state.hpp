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

  inline State& operator << (const OpDescriptor<T>& op)
  {
    apply(op);
    return *this;
  }

  inline State& operator << (const OpPack<T>& pack)
  {
    for (unsigned i = 0; i < pack.size(); i++)
      apply(pack[i]);
    return *this;
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
