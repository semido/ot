#pragma once

#include <cassert>
#include <algorithm>
#include <string>
#include <vector>

#ifndef allof
#define allof(c) (c).begin(), (c).end()
#endif

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
  unsigned pos = 0; // Concurrent pos
  unsigned posser = UINT_MAX; // Serial pos considering other concurrent ops.
  T value;
  OpType typ = OpType::nothing;
  CID cid = 0;

  OpDescriptor() = default;

  OpDescriptor(OpType t, unsigned p, const T& v, unsigned c, unsigned r=0) :
    typ(t), pos(p), posser(p), value(v), cid((CID)c), rev(r) {}

  inline OpDescriptor operator()(const OpDescriptor<T>& pre) const
  {
    auto newop = *this;
    newop.trans(pre);
    return newop;
  }

  // Transform this op as concurrent to consider an effect of pre op.
  inline void trans(const OpDescriptor<T>& pre)
  {
    if (pos < pre.pos || pre.typ == OpType::nothing)
      return;
    if (pos == pre.pos) {
      //P = check cid; 0 = make nothing; . = as is
      /// i d u (pre)
      //i P . .
      //d + 0 .
      //u + 0 P
      if (pre.typ == OpType::remove) {
        if (typ != OpType::insert)
          typ = OpType::nothing;
        return;
      }
      if (typ == OpType::update && pre.typ == OpType::update && cid > pre.cid) {
        typ = OpType::nothing; // higher priority wins, lower dropped
        return;
      }
      if (typ == OpType::insert && pre.typ == OpType::insert && cid <= pre.cid) {
        return; // higher priority inserts before prev insert
      }
    }
    // pos > pre.pos || pos == pre.pos && pre.typ == insert
    if (pre.typ == OpType::remove)
      posser--;
    if (pre.typ == OpType::insert)
      posser++;
    return;
  }
/* Failed to make it working.

  // Transform this applied op to consider an effect of later subsequent op.
  // That affects new op addition in pack.transformAndPut.
  // Ops with same cid are already subsequent.
  inline void transApplied(const OpDescriptor<T>& next)
  {
    if (pos < next.pos || cid == next.cid || next.typ == OpType::nothing)
      return;
    if (pos == next.pos) {
      if (next.typ != OpType::insert)// || typ == OpType::remove)
        return;
    }
    if (next.typ == OpType::remove)
      pos--;
    if (next.typ == OpType::insert)
      pos++;
    return;
  }

*/

  // Transform this op to exclude an effect of 'pre' op with the same cid,
  // i.e. make this op concurrent to 'pre' instead of being subsequent.
  // This is not applicable to 'nothing' op created from a concurrent state by 'trans' func.
  inline void reverse(const OpDescriptor<T>& pre)
  {
    assert(cid == pre.cid); // It is probably not ready to reverse ops from different clients.
    assert(pos == posser);
    if (pos < pre.pos || pre.typ == OpType::nothing)
      return;
    if (pos == pre.pos) {
      if (pre.typ == OpType::remove) {
        pos++;
        return;
      }
      if (typ == OpType::update && pre.typ == OpType::update)
        ; // TODO
    }
    // pos > pre.pos
    if (pre.typ == OpType::remove)
      pos++;
    if (pre.typ == OpType::insert)
      pos--;
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
    const std::string typName[] = { "n", "I", "D", "U" };
    return typName[(int)typ] + "[" + std::to_string(pos) + "]=" + std::to_string(value) + " c" + std::to_string(cid) + " r" + std::to_string(rev);
  }
};
// Provident communication protocol must skip unused/optional fields depending on 'op'.

template<typename T>
class OpPack : public std::vector<OpDescriptor<T>>
{
  bool transformed = false;
public:

  inline OpPack& operator <<(const OpDescriptor<T>& op)
  {
    push_back(op);
    return *this;
  }

  inline void orderPos()
  {
    std::sort(begin(), end(), [](auto& a, auto& b) {
      return a.value < b.value;
//         a.pos < b.pos || 
//         a.pos == b.pos && a.cid < b.cid || 
//         a.pos == b.pos && a.cid == b.cid && a.rev < b.rev;
      });
  }

  inline void orderRev()
  {
    std::sort(begin(), end(), [](auto& a, auto& b) {
      return a.rev < b.rev || a.rev == b.rev && a.cid < b.cid;
      });
  }

  // Transform new op by previously added and applied ops.
  // Then add newop to the pack to affect future ops.
  // NB op.trans skips same cid.
  inline void transformAndPut(OpDescriptor<T>& newop)
  {
    for (auto& op : *this) {
      newop.trans(op);
    }
    push_back(newop);
  }
/*
  // Making transAll before applying to the state is not working,
  // because previous ops must be applied and only after that transformed on newop.
 
  // Transform all concurrent ops of the pack.
  // It assumes ops added concurrently, but not applied on the state
  // and thus don't know about each other yet.
  inline void transAll(bool sort = true)
  {
    if (transformed)
      return;
    if (sort)
      orderRev();
#if 1
    OpPack newpack;
    for (auto& op : *this)
      newpack.transformAndPut(op);
    *this = std::move(newpack);
#else
    for (auto i = begin() + 1; i < end(); i++)
      for (auto j = begin(); j < i; j++)
        i->trans(*j);
#endif
    transformed = true;
  }
*/
};
