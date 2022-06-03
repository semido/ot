#include "../ops/state.hpp"
#include "../ops/pstate.hpp"
#include <gtest/gtest.h>
#include <random>

using U = unsigned;

TEST(Transformations, CP1) {
  std::vector<U> data1(10, 0);
  State<std::vector<U>> state1(data1);
  auto data2 = data1;
  State<std::vector<U>> state2(data2);

  unsigned pos = 3;
  U value = 0;
  for (int shift = -2; shift < 3; shift++) {
    for (auto a = OpType::nothing; a < OpType::thelast; ((char&)a)++) {
      for (auto b = OpType::nothing; b < OpType::thelast; ((char&)b)++) {
        // pos, value, optype, cid (must be concurrent/distinct)
        OpDescriptor<U> opA{ pos, value++, a, 1 };
        OpDescriptor<U> opB{ pos + shift, value++, b, 2 };
        state1 << opA << opB(opA);
        state2 << opB << opA(opB);
        ASSERT_TRUE(StateEq(state1.get(), state2.get())) << "States not equal, CP2/TP2 failed: " << opA.str() << " / " << opB.str();
      }
    }
  }
}

TEST(Transformations, CP2) {
  std::vector<U> data1(10, 0);
  State<std::vector<U>> state1(data1);
  auto data2 = data1;
  State<std::vector<U>> state2(data2);

  unsigned pos = 3;
  U value = 0;
  for (int shift = -1; shift < 2; shift++) {
    for (auto a = OpType::nothing; a < OpType::thelast; ((char&)a)++) {
      for (auto b = OpType::nothing; b < OpType::thelast; ((char&)b)++) {
        for (auto c = OpType::insert; c < OpType::thelast; ((char&)c)++) {
          // pos, value, optype, cid (must be concurrent/distinct)
          OpDescriptor<U> opA{ pos, value++, a, 1 };
          OpDescriptor<U> opB{ pos + shift, value++, b, 2 };
          OpDescriptor<U> opC{ pos + shift + 2, value++, c, 3 };
          auto opBA = opB(opA);
          state1 << opA << opBA << opC(opA)(opBA);
          auto opAB = opA(opB);
          state2 << opB << opAB << opC(opB)(opAB);
          ASSERT_TRUE(StateEq(state1.get(), state2.get())) << "States not equal, CP2/TP2 failed: " << opA.str() << " / " << opB.str() << " / " << opC.str();
        }
      }
    }
  }
}

TEST(Transformations, CP2Packer) {
  std::vector<U> data1(10, 0);
  State<std::vector<U>> state1(data1);
  auto data2 = data1;
  State<std::vector<U>> state2(data2);

  unsigned pos = 3;
  U value = 0;
  for (int shift = -1; shift < 2; shift++) {
    for (auto a = OpType::nothing; a < OpType::thelast; ((char&)a)++) {
      for (auto b = OpType::nothing; b < OpType::thelast; ((char&)b)++) {
        for (auto c = OpType::insert; c < OpType::thelast; ((char&)c)++) {
          // pos, value, optype, cid (must be concurrent/distinct)
          OpDescriptor<U> opA{ pos, value++, a, 1 };
          OpDescriptor<U> opB{ pos + shift, value++, b, 2 };
          OpDescriptor<U> opC{ pos + shift + 2, value++, c, 3 };
          OpPack<U> pack1;
          pack1 << opA << opB << opC << OpPack<U>::transform;
          state1 << pack1;
          OpPack<U> pack2;
          pack2 << opB << opA << opC << OpPack<U>::transform;
          state2 << pack2;
          ASSERT_TRUE(StateEq(state1.get(), state2.get())) << "States not equal, CP2/TP2 failed: " << opA.str() << " / " << opB.str() << " / " << opC.str();
        }
      }
    }
  }
}
