#include "../ops/state.hpp"
#include "../ops/pstate.hpp"
#include <gtest/gtest.h>
#include <random>

using U = unsigned;

template<typename C>
class ConcurentClientsTest : public ::testing::Test
{
protected:
  std::vector<C> datas;
  std::vector<State<C>> states;
  typename C::value_type value = 0; // Values for testing. It does not affect the flow, just a little helper.
  std::mt19937 gen;

  void init(unsigned clients, unsigned initialSize)
  {
    datas.resize(clients + 1);
    for (auto& d : datas)
      d.reserve(initialSize);
    //std::random_device rd;
    //gen.seed(rd());
    //std::uniform_int_distribution<C::value_type> dis;
    unsigned start = std::max(100U, initialSize);
    for (unsigned i = 0; i < initialSize; i++) {
      for (auto& d : datas)
        d.push_back(start+i);
    }
    for (auto& d : datas)
      states.emplace_back(d);
  }

  void checkStates(unsigned round = 0)
  {
    for (unsigned cid = 1; cid < datas.size(); cid++) {
      EXPECT_TRUE(StateEq(datas.front(), datas[cid]));
    }
  }

  void makeLocalOps(std::vector<OpPack<U>>& packs, unsigned r, unsigned opsPerRound)
  {
    unsigned rev = r * opsPerRound;

    auto put = [&](auto op)
    {
      packs[op.cid] << op; // Own client's pack.
      packs.front() << op; // Send to server :)
    };
    std::uniform_int_distribution<short> typ(1, 3);
    for (unsigned cid = 1; cid < datas.size(); cid++) {
      for (unsigned i = 0; i < opsPerRound; i++) {
        std::uniform_int_distribution<unsigned> pos(0, (unsigned)datas[cid].size());
        auto p = pos(gen);
        auto t = (OpType)typ(gen);
        if (datas[cid].empty())
          t = OpType::insert;
        if (p >= datas[cid].size())
          p = t == OpType::insert && rev % 2 ? (unsigned)datas[cid].size() : 0;
        OpDescriptor<U> op(t, p, value++, cid, rev + i);
        put(op);
      }
    }
  }

  // N.B. This code test particular combination of subsequent ops. Convert it to concurrent to reproduce the problem.
  void makeLocalOps1(std::vector<OpPack<U>>& packs, unsigned r, unsigned opsPerRound)
  {
    unsigned rev = r * opsPerRound;

    auto put = [&](auto op)
    {
      packs[op.cid] << op; // Own client's pack.
      packs.front() << op; // Send to server :)
    };
    unsigned cid = 1;
    put(OpDescriptor<U>{ OpType::insert, 0, value++, cid, 0 });
    put(OpDescriptor<U>{ OpType::update, 3, value++, cid, 1 });
    cid = 2;
    put(OpDescriptor<U>{ OpType::insert, 1, value++, cid, 0 });
    put(OpDescriptor<U>{ OpType::insert, 4, value++, cid, 1 });
  }

  void round(unsigned r, unsigned opsPerRound)
  {
    std::vector<OpPack<U>> packs(states.size());
    makeLocalOps(packs, r, opsPerRound);
    OpPack<U> temp;
    states.front().applyFromOthers(0, temp, packs.front());
    for (unsigned cid = 1; cid < states.size(); cid++) {
      states[cid].apply(packs[cid]);
      states[cid].applyFromOthers(cid, packs[cid], packs.front());
/*
      auto equal = StateEq(datas.front(), datas[cid]);
      if (!equal) {
        std::cout << "round " + std::to_string(r) + ": 0 vs " + std::to_string(cid) << "\n";
        std::cout << states.front().str() << "\n";
        std::cout << states[cid].str() << "\n";
      }
*/
    }
  }

  void run(unsigned clients, unsigned initialSize, unsigned rounds, unsigned opsPerRound, bool checkEachRound = false)
  {
    init(clients, initialSize);
    gen.seed(0); // Make fixed sequence for now.
    for (unsigned r = 0; r < rounds; r++) {
      round(r, opsPerRound);
      if (checkEachRound)
        checkStates(r);
    }
  }

  void runFixed(unsigned clients, unsigned initialSize, unsigned opsPerRound)
  {
    init(clients, initialSize);
    gen.seed(0); // Make fixed sequence for now.
    std::vector<OpPack<U>> packs(states.size());
    makeLocalOps1(packs, 1, opsPerRound);
    states.front().apply(packs.front());
    for (unsigned cid = 1; cid < states.size(); cid++) {
      states[cid].apply(packs[cid]);
      states[cid].applyFromOthers(cid, packs[cid], packs.front());
    }
  }
};

using ConcurentClientsMergeBy1Test = ConcurentClientsTest<std::vector<U>>;

TEST_F(ConcurentClientsMergeBy1Test, DISABLED_Pain) {
  // N.B. This code test particular combination of subsequent ops. Convert it to concurrent to reproduce the problem.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  runFixed(2, 5, 2);
  checkStates();
}

TEST_F(ConcurentClientsMergeBy1Test, Trivial) {
  // Simplest conditions.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(2, 10, 10, 1, true);
  checkStates();
}

TEST_F(ConcurentClientsMergeBy1Test, Fixed1) {
  run(2, 10, 10, 1);
  checkStates();
}

TEST_F(ConcurentClientsMergeBy1Test, Continious) {
  // 20 clients 1000 steps. 1by1.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(20, 10, 1000, 1);
  //run(9, 10, 1, 1);
  checkStates();
}

TEST_F(ConcurentClientsMergeBy1Test, ConcurentPackets) {
  // 20 clients sending packets with 10 ops. 100 rounds.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(20, 100000, 100, 10);
  checkStates();
}

using ConcurentClientsIgushTest = ConcurentClientsTest<IgushArray<U>>;

TEST_F(ConcurentClientsIgushTest, DISABLED_Real) {
  // Conditions more or less from the task.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(20, 10*1000*1000, 400, 10);
  checkStates();
}
