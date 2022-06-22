#pragma once

#include "../ops/state.hpp"
#include <cassert>
#include <vector>

template<typename C>
class Client;

// Trivial centralized structure holding connections with state modifiers (clients).
// It updates all states by sending the same package of concurrent ops.
// Each state transforms received pack considering local changes.
template<typename C>
class Server {
  using T = typename C::value_type;
  std::vector<C>* datas = nullptr; // Real containers live together to simplify the test.
  State<C> state; // Server' state applier.
  OpPack<T> accum; // Accumulate from clients before common update.
  std::vector<Client<C>*> clients; // Store refs to send updates.
  unsigned rev = 0;

public:

  void init(std::vector<C>* d)
  {
    assert(!datas);
    datas = d;
    state.set(&(*datas)[0]);
  }

  // Initiate server state update followed by all client updates.
  inline void update()
  {
    OpPack<T> temp; // For debugging.
    state.applyFromOthers(0, temp, accum);
    rev++;
    for (auto pc : clients)
      pc->updateFromServer(rev, accum);
    accum.clear();
  }

  inline void clear()
  {
    datas = nullptr;
    clients.clear();
    rev = 0;
  }

  inline std::string str() const { return state.str(); }

protected:
  friend class Client<C>;

  inline auto clone(Client<C>* pc)
  {
    assert(datas);
    clients.push_back(pc); // Store ref to send updates.
    datas->emplace_back(datas->front()); // N.B. Will cause a failure without proper reserve, because state stores the address.
    return std::make_tuple((unsigned)datas->size() - 1, rev, &datas->back());
  }

  // Client pushes its local ops.
  inline void push(unsigned r, const OpPack<T>& pack)
  {
    assert(r == rev);
    accum.insert(accum.end(), allof(pack));
    // TODO apply to server state here
  }
};
