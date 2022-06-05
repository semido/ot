#include "../ops/state.hpp"
#include "../ops/pstate.hpp"
#include <gtest/gtest.h>
#include <random>

using U = unsigned;

template<typename C>
class ConcurentClients : public ::testing::Test
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

  void checkStates()
  {
    for (unsigned cid = 1; cid < datas.size(); cid++) {
      ASSERT_TRUE(StateEq(datas.front(), datas[cid]));
    }
  }

  void makeLocalOps(std::vector<OpPack<U>>& packs, unsigned r, unsigned opsPerRound)
  {
    unsigned rev = r * opsPerRound;

    auto put = [&](auto op)
    {
      packs[op.cid] << op; // Own client's pack.
      states[op.cid] << op; // Apply to own state. Avoid re-order.
      packs.front() << op; // Send to server :)
    };
#if 0
    unsigned cid = 1;
    put(OpDescriptor<U>{ 0, value++, OpType::insert, cid, 0 });
    put(OpDescriptor<U>{ 3, value++, OpType::update, cid, 1 });
    cid = 2;
    put(OpDescriptor<U>{ 1, value++, OpType::insert, cid, 0 });
    put(OpDescriptor<U>{ 4, value++, OpType::insert, cid, 1 });
#else
    std::uniform_int_distribution<short> typ(1, 3);
    for (unsigned cid = 1; cid < datas.size(); cid++) {
      for (unsigned i = 0; i < opsPerRound; i++) {
        std::uniform_int_distribution<unsigned> pos(0, (unsigned)datas[cid].size());
        OpDescriptor<U> op(pos(gen), value++, (OpType)typ(gen), cid, rev + i);
        if (datas[cid].empty())
          op.typ = OpType::insert;
        if (op.pos >= datas[cid].size())
          op.pos = op.typ == OpType::insert && rev%2 ? (unsigned) datas[cid].size() : 0;
        put(op);
      }
    }
#endif
  }

  void applyFromOther(const OpPack<U>& srcPack, OpPack<U>& dstPack, unsigned cid)
  {
    for (auto op : srcPack) {
      if (op.cid == cid)
        continue; // Own op already applied and present in pack.
      dstPack.transformAndPut(op);
      states[cid] << op;
    }
  }

  void round(unsigned r, unsigned opsPerRound)
  {
    std::vector<OpPack<U>> packs(states.size());
    makeLocalOps(packs, r, opsPerRound);
    for (unsigned cid = 1; cid < states.size(); cid++)
      applyFromOther(packs.front(), packs[cid], cid);
    packs.front().transAll();
    states.front() << packs.front();
  }

  void run(unsigned clients, unsigned initialSize, unsigned rounds, unsigned opsPerRound, bool checkEachRound = false)
  {
    init(clients, initialSize);
    //gen.seed(0); // Make fixed sequence for now.
    for (unsigned r = 0; r < rounds; r++) {
      round(r, opsPerRound);
      if (checkEachRound)
        checkStates();
    }
  }
};

using ConcurentClientsMergeBy1Test = ConcurentClients<std::vector<U>>;

TEST_F(ConcurentClientsMergeBy1Test, DISABLED_MyPain) {
  // ???
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(2, 5, 1, 2);
  checkStates();
}

TEST_F(ConcurentClientsMergeBy1Test, Trivial) {
  // Simplest conditions.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(2, 10, 10, 1);
  checkStates();
}

TEST_F(ConcurentClientsMergeBy1Test, Continious) {
  // 20 clients 1000 steps. 1by1.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(9, 10, 1, 1, true);
  //checkStates();
}

TEST_F(ConcurentClientsMergeBy1Test, DISABLED_ConcurentPackets) {
  // 20 clients sending packets with 10 ops.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(20, 10, 100, 10);
  checkStates();
}


using ConcurentClientsIgushTest = ConcurentClients<IgushArray<U>>;

TEST_F(ConcurentClientsIgushTest, DISABLED_Real) {
  // Conditions more or less from the task.
  // Run: num of clients, num of elements, rounds, ops created in one round.
  run(20, 10*1000*1000, 20, 10);
  checkStates();
}
