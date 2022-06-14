#pragma once

#include "../ops/state.hpp"
#include "server.hpp"
#include <cassert>
#include <vector>

// The source of concurrent changes (ops) of the shared state.
// It connects to the server to get its current state,
// then push each change to the server and expects accumulated update back from the server.
template<typename C>
class Client
{
  using T = typename C::value_type;
  Server<C>* server = nullptr;
  State<C> state;
  OpPack<T> applied;
  unsigned cid = 0;
  unsigned rev = 0;
  unsigned servRev = 0;

public:

  Client() = default;
  
  inline void connect(Server<C>& s)
  {
    assert(!connected());
    auto [c, r, buf] = s.clone(this);
    server = &s;
    cid = c;
    servRev = r;
    state.set(buf);
  }

  inline bool connected() const { return server != nullptr; }

  inline void makeLocalOps(OpPack<T>& pack)
  {
    assert(connected());
    assert(applied.empty()); // Assume applied is already sent to server and used for update from server!
    for (auto& op : pack) { // Make sure ops are properly signed.
      op.cid = cid;
      op.rev = rev++;
    }
    state.apply(pack); // Ops in pack are concurrent.
    server->push(servRev, pack);
    applied = std::move(pack);
    // N.B. Next call for this client must be update from server!
  }

  inline unsigned size() const { return state.size(); }

  inline unsigned id() const { return cid; }

protected:
  friend class Server<C>;

  inline void updateFromServer(unsigned r, const OpPack<T>& pack)
  {
    assert(connected());
    servRev = r;
    state.applyFromOthers(cid, applied, pack);
    applied.clear();
  }
};
