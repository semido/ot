#include "../ops/state.hpp"
#include "../ops/pstate.hpp"
#include <gtest/gtest.h>
#include <random>

using U = unsigned;

template<typename C>
void RunConcurentClientsOptimistic(std::vector<C>& datas, unsigned initialSize, unsigned rounds, unsigned opsPerRound)
{
  for (auto& d : datas)
    d.reserve(initialSize);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<C::value_type> dis1;
  for (unsigned i = 0; i < initialSize; i++) {
    const auto x = dis1(gen);
    for (auto& d : datas)
      d.push_back(x);
  }

  for (int cid = 1; cid < datas.size(); cid++)
    ASSERT_TRUE(StateEq(datas.front(), datas[cid]));

  std::vector<State<C>> states(datas.begin(), datas.end());

  std::uniform_int_distribution<short> disot(1, 3);
  U value = 0;
  for (unsigned r = 0; r < rounds; r++) {
    std::vector<OpPack<U>> packs(states.size());
    auto& pack0 = packs.front(); // Server's pack;

    auto put = [&](auto op)
    {
      packs[op.cid] << op; // Own client's pack.
      states[op.cid] << op; // Apply to own state. Avoid re-order.
      pack0 << op;
    };
#if 0
    unsigned cid = 1;
    put(OpDescriptor<U>{ 0, value++, OpType::insert, cid, 0 });
    put(OpDescriptor<U>{ 3, value++, OpType::update, cid, 1 });
    cid = 2;
    put(OpDescriptor<U>{ 1, value++, OpType::insert, cid, 0 });
    put(OpDescriptor<U>{ 4, value++, OpType::insert, cid, 1 });
#else
    unsigned rev = r * opsPerRound;
    for (unsigned cid = 1; cid < states.size(); cid++) {
      for (unsigned i = 0; i < opsPerRound; i++) {
        std::uniform_int_distribution<unsigned> dispos(0, (unsigned) states[cid].get().size());
        // OpDescriptor: pos, value, optype, cid (must be concurrent/distinct), revision
        put(OpDescriptor<U>{ dispos(gen), value++, (OpType)disot(gen), cid, rev + i });
      }
      states[cid] << packs[cid]; // Apply to own state.
    }
#endif
    for (int cid = 1; cid < states.size(); cid++) {
      auto& clientPack = packs[cid];
      for (unsigned i = 0; i < pack0.size(); i++) {
        auto op = pack0[i];
        if (op.cid == cid)
          continue; // Own op already applied and present in pack.
        clientPack.transformAndPut(op);
        states[cid] << op;
      }
    }
    pack0.transAll();
    states.front() << pack0;
  }
}

TEST(ConcurentClients, DISABLED_Vectors) {
  std::vector<std::vector<U>> datas(3);
  const unsigned initialSize = 5;
  RunConcurentClientsOptimistic(datas, initialSize, 1, 2);
  for (int cid = 1; cid < datas.size(); cid++)
    ASSERT_TRUE(StateEq(datas.front(), datas[cid]));
}

TEST(ConcurentClients, DISABLED_Igush) {
  std::vector<IgushArray<U>> datas(20);
  const unsigned initialSize = 10 * 1024 * 1024;
  RunConcurentClientsOptimistic(datas, initialSize, 1, 5);
  for(int cid = 1; cid < datas.size(); cid++)
    ASSERT_TRUE(StateEq(datas.front(), datas[cid]));
}
