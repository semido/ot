#pragma once

#include <cassert>
#include <algorithm>
#include <string>
#include <vector>
#include <climits>

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
/*
  // WIP
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
*/
  inline std::string str() const
  {
    const std::string typName[] = { "0", "I", "D", "U" };
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
    this->push_back(op);
    return *this;
  }

  inline void orderPos()
  {
    std::sort(this->begin(), this->end(), [](auto& a, auto& b) {
      return a.pos < b.pos || 
        a.pos == b.pos && a.cid < b.cid || 
        a.pos == b.pos && a.cid == b.cid && a.rev < b.rev;
      });
  }

  inline void orderRev()
  {
    std::sort(this->begin(), this->end(), [](auto& a, auto& b) {
      return a.rev < b.rev || a.rev == b.rev && a.cid < b.cid;
      });
  }

  // Transform new op by previously added and applied ops.
  // Then add newop to the pack to affect future ops.
  inline void transformAndPut(OpDescriptor<T>& newop)
  {
    for (auto& op : *this) {
      newop.trans(op);
    }
    this->push_back(newop);
  }
};
