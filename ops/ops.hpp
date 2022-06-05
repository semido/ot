#pragma once

#include <algorithm>
#include <string>
#include <vector>

enum class OpType : char
{
  nothing = 0,
  insert = 1, // insert into state[pos], shift prev_state[pos] right
  remove = 2,
  update = 3,
  thelast = 4
};

template<typename T, typename CID = unsigned>
struct OpDescriptor
{
  unsigned rev = 0;
  unsigned pos = 0;
  T value;
  OpType typ = OpType::nothing;
  CID cid = 0; // Client drops its own op and do not trans on that later received ops.

  OpDescriptor() = default;

  OpDescriptor(unsigned p, const T& v, OpType t, unsigned c, unsigned r=0) :
    pos(p), value(v), typ(t), cid((CID)c), rev(r) {}

  inline OpDescriptor operator()(const OpDescriptor<T>& pre) const
  {
    auto newop = *this;
    newop.trans(pre);
    return newop;
  }

  inline void trans(const OpDescriptor<T>& pre)
  {
    if (pos < pre.pos || cid == pre.cid || pre.typ == OpType::nothing)
      return;
    if (pos == pre.pos) {
      //P = check cid; 0 = make nothing; . = as is
      /// i d u (pre)
      //i P . .
      //d > 0 .
      //u > 0 P
      if (pre.typ == OpType::remove) {
        if (typ != OpType::insert)
          typ = OpType::nothing;
        return;
      }
      if (typ == OpType::update && pre.typ == OpType::update && cid > pre.cid) {
        typ = OpType::nothing; // higher priority wins, lower dropped
        return;
      }
      if (typ == OpType::insert && pre.typ == OpType::insert && cid < pre.cid) {
        return; // higher priority inserts before prev insert
      }
    }
    // pos > pre.pos
    if (pre.typ == OpType::remove)
      pos--;
    if (pre.typ == OpType::insert)
      pos++;
    return;
  }

  inline bool operator==(const OpDescriptor<T>& op) const
  {
    // Check op before drop from local pack.
    // Position and op type can be changed due to transform.
    return cid == op.cid && rev == op.rev && value == op.value;
  }

  inline std::string str() const
  {
    return std::to_string(pos) + ", " + std::to_string(value) + ", " + std::to_string((int)typ) + ", " + std::to_string(cid) + ", " + std::to_string(rev);
  }
};
// Provident communication protocol must skip unused/optional fields depending on 'op'.

// Order concurrent ops to minimize transformation 
// then transform all.
template<typename T>
class OpPack : public std::vector<OpDescriptor<T>>
{
  bool transformed = false;
public:
  static const bool transform = false;
  static const bool sort_transform = true;

  inline OpPack& operator <<(const OpDescriptor<T>& op)
  {
    push_back(op);
    return *this;
  }

  inline void operator <<(bool sort)
  {
    transAll(sort);
  }

  inline void orderPos()
  {
    std::sort(begin(), end(), [](auto& a, auto& b) {
      return a.pos < b.pos || a.pos == b.pos && a.cid < b.cid;
      });
  }

  inline void orderRev()
  {
    std::sort(begin(), end(), [](auto& a, auto& b) {
      return a.rev < b.rev || a.rev == b.rev && a.cid < b.cid;
      });
  }

  // Transform new op by previously added and applied ops.
  // It skips existing ops with same cid.
  inline void transformAndPut(OpDescriptor<T>& newop)
  {
    OpPack<T> select;
    for (auto& op : *this)
//      if (op.cid != newop.cid && op.pos <= newop.pos)
        select.push_back(op);
    for (auto& op : select)
      newop.trans(op);
    push_back(newop);
  }

  // Transform all concurrent ops of the pack.
  // It assumes ops added concurrently, but not applied on the state
  // and thus don't know about each other yet.
  inline void transAll(bool sort = true)
  {
    if (transformed)
      return;
    if (sort)
      orderRev();
    for (auto i = begin() + 1; i < end(); i++)
      for (auto j = begin(); j < i; j++)
        i->trans(*j);
    transformed = true;
  }

  inline bool drop(const OpDescriptor<T>& op)
  {
    for (auto i = begin(); i < end(); i++)
      if (op == *i) {
        erase(i);
        return true;
      }
    return false;
  }
};
