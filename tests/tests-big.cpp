#include "../ops/state.hpp"
#include "../ops/pstate.hpp"
#include <gtest/gtest.h>
#include <random>

using U = unsigned;

#if 0
TEST(Transformations, BigVectorStateCP2) {
  std::vector<U> data1(10 * 1024 * 1024);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned> dis1(0, UINT_MAX);
  for (int i = 0; i < data1.size(); i++)
    data1[i] = dis1(gen);

  State<std::vector<U>> state1(data1);

  auto data2 = data1;
  State<std::vector<U>> state2(data2);

  std::uniform_int_distribution<unsigned> dispos(0, data1.size() - 1);
  U value = 0;
  for (int take = 0; take < 10; take++) {
    for (auto a = OpType::insert; a < OpType::thelast; ((char&)a)++) {
      for (auto b = OpType::insert; b < OpType::thelast; ((char&)b)++) {
        for (auto c = OpType::insert; c < OpType::thelast; ((char&)c)++) {
          // pos, value, optype, cid (must be concurrent/distinct)
          OpDescriptor<U> opA{ dispos(gen) % state1.get().size(), value++, a, 1 };
          OpDescriptor<U> opB{ dispos(gen) % state1.get().size(), value++, b, 2 };
          OpDescriptor<U> opC{ dispos(gen) % state1.get().size(), value++, c, 3 };
          OpPack<U> pack1;
          pack1 << opA << opB << opC << OpPack<U>::transform;
          state1 << pack1;
          OpPack<U> pack2;
          pack2 << opB << opA << opC << OpPack<U>::transform;
          state2 << pack2;
//          ASSERT_TRUE(StateEq(state1.get(), state2.get())) << "States not equal, CP2/TP2 failed: " << opA.str() << " / " << opB.str() << " / " << opC.str();
        }
      }
    }
  }
  std::cout << value << '\n';
  ASSERT_TRUE(Eq(state1.get(), state2.get()));
}
#endif

template<typename C>
void BigStateRandom(C& data1, C& data2, unsigned initialSize, unsigned rounds)
{
  data1.reserve(initialSize);
  data2.reserve(initialSize);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<unsigned> dis1(0, UINT_MAX);
  for (unsigned i = 0; i < initialSize; i++) {
    auto x = dis1(gen);
    data1.push_back(x);
    data2.push_back(x);
  }

  State<C> state1(data1);
  State<C> state2(data2);

  std::uniform_int_distribution<unsigned> dispos(0, (unsigned) data1.size());
  std::uniform_int_distribution<short> diso(1, 3);
  U value = 0;
  for (unsigned r = 0; r < rounds; r++) {
    OpPack<U> pack1;
    for (unsigned i = 0; i < 10; i++) {
      // pos, value, optype, cid (must be concurrent/distinct)
      OpDescriptor<U> op{ dispos(gen) % state1.get().size(), value++, (OpType)diso(gen), i+1 };
      pack1 << op;
    }
    OpPack<U> pack2 = pack1;
    pack1 << OpPack<U>::transform;
    pack2 << OpPack<U>::sort_transform;

    state1 << pack1;
    state2 << pack2;
  }
}

TEST(Transformations, DISABLED_BigVectorRandom) {
  std::vector<U> data1, data2;
  const unsigned initialSize = 10 * 1024 * 1024;
  BigStateRandom(data1, data2, initialSize, 1000);
  ASSERT_TRUE(StateEq(data1, data2));
}

TEST(Transformations, DISABLED_BigIgushRandom) {
  IgushArray<U> data1, data2;
  const unsigned initialSize = 10 * 1024 * 1024;
  BigStateRandom(data1, data2, initialSize, 1000);
  ASSERT_TRUE(StateEq(data1, data2));
}
