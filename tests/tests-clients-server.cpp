#include "../comm/client.hpp"
#include "../igush/igush_array.h"
#include <gtest/gtest.h>
#include <random>

using U = unsigned;

template<typename C>
class ConcurentClientServerTest : public ::testing::Test
{
protected:
  using T = typename C::value_type;
  std::vector<C> datas;
  Server<C> server;
  std::vector<Client<C>> clients;

  typename C::value_type value = 0; // Values for testing. It does not affect the flow, just a little helper.
  std::mt19937 gen;

  inline void init(unsigned clients, unsigned initialSize)
  {
    datas.reserve(clients + 1);
    datas.resize(1);
    datas.front().reserve(initialSize);
    //std::random_device rd;
    //gen.seed(rd());
    //std::uniform_int_distribution<C::value_type> dis;
    unsigned start = std::max(100U, initialSize);
    for (unsigned i = 0; i < initialSize; i++) {
      datas.front().push_back(start+i);
    }
  }

  inline void checkStates()
  {
    for (unsigned cid = 1; cid < datas.size(); cid++) {
      ASSERT_TRUE(StateEq(datas.front(), datas[cid]));
    }
  }

  inline OpPack<T> createLocalOps(const Client<C>& client, unsigned r, unsigned opsPerRound)
  {
    OpPack<T> pack;

    unsigned rev = r * opsPerRound;

    std::uniform_int_distribution<short> typ(1, 3);
    for (unsigned i = 0; i < opsPerRound; i++) {
      std::uniform_int_distribution<unsigned> pos(0, client.size());
      auto p = pos(gen);
      auto t = (OpType)typ(gen);
      if (client.size() == 0)
        t = OpType::insert;
      if (p >= client.size())
        p = t == OpType::insert && rev % 2 ? client.size() : 0;
      OpDescriptor<U> op(t, p, value++, client.id(), rev + i);
      pack << op;
    }
    return pack;
  }

  void run(unsigned nClients, unsigned initialSize, unsigned rounds, unsigned opsPerRound, bool checkEachRound = false)
  {
    init(nClients, initialSize);
    clients.reserve(nClients);
    server.init(&datas);
    //gen.seed(0); // Make fixed sequence for now.
    for (unsigned r = 0; r < rounds; r++) {
      if (clients.size() < nClients) { // Each round add another client.
        clients.push_back({}); // Avoid realloc, because server stores the addr.
        clients.back().connect(server);
      }
      for (auto& c : clients) {
        auto pack = createLocalOps(c, r, opsPerRound);
        c.makeLocalOps(pack);
      }
      server.update();
      if (checkEachRound)
        checkStates();
    }
  }
};

using AConcurentClientServerVTest = ConcurentClientServerTest<std::vector<U>>;

TEST_F(AConcurentClientServerVTest, Trivial) {
  // Simplest conditions.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(2, 10, 10, 1);
  checkStates();
}

TEST_F(AConcurentClientServerVTest, Continious) {
  // 20 clients 1000 steps. 1by1.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(20, 10, 1000, 1);
  //run(9, 10, 1, 1);
  checkStates();
}

TEST_F(AConcurentClientServerVTest, ConcurentPackets) {
  // 20 clients sending packets with 10 ops. 100 rounds.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(20, 100000, 100, 10);
  checkStates();
}

using AConcurentClientServerIgushTest = ConcurentClientServerTest<IgushArray<U>>;

TEST_F(AConcurentClientServerIgushTest, DISABLED_Real3H) {
  // Conditions more or less from the task.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(20, 10 * 1000 * 1000, 3 * 60 * 60, 5);
  checkStates();
}
