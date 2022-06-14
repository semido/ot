#include "comm/client.hpp"
#include "igush/igush_array.h"
#include "utils/timer.hpp"
#include "utils/cmd.hpp"
#include <random>
#include <iostream>
#include <vector>
#include <string>

using U = unsigned;

using C = IgushArray<U>;

class TrivialClientServer
{
  std::vector<C> datas;
  Server<C> server;
  std::vector<Client<C>> clients;

  std::mt19937 gen;

  inline bool checkStates() const
  {
    for (unsigned cid = 1; cid < datas.size(); cid++) {
      if (!StateEq(datas.front(), datas[cid]))
        return false;
    }
    return true;
  }

  inline OpPack<U> createLocalOps(const Client<C>& client, unsigned opsPerRound)
  {
    OpPack<U> pack;

    std::uniform_int_distribution<U> dis;
    std::uniform_int_distribution<short> typ(1, 3);
    for (unsigned i = 0; i < opsPerRound; i++) {
      std::uniform_int_distribution<unsigned> pos(0, client.size());
      auto p = pos(gen);
      auto t = (OpType)typ(gen);
      if (client.size() == 0)
        t = OpType::insert;
      if (p >= client.size())
        p = t == OpType::insert && i % 2 ? client.size() : 0;
      OpDescriptor<U> op(t, p, dis(gen), client.id());
      pack << op;
    }
    return pack;
  }

  inline bool test(const Timer& t, unsigned r, unsigned testPeriod) const
  {
    bool tested = false;
    if (testPeriod > 0 && r % testPeriod == 0 || testPeriod == 0 && r % 1800 == 0) {
      tested = true;
      bool good = checkStates();
      if (good)
        std::cout << t << "sec, " << r << " rounds, state size: " << datas.front().size() << " OK\n";
      else
        return false;
    }
    if (!tested && r % 30 == 0) // Just a time line.
      std::cout << t << "sec, " << r << " rounds\n";
    return true;
  }

public:
  inline TrivialClientServer(unsigned nClients, unsigned initialSize)
  {
    datas.reserve(nClients + 1); // N.B. State stores addr.
    datas.resize(1);
    datas.front().reserve(initialSize);
    std::random_device rd;
    gen.seed(rd());
    std::uniform_int_distribution<U> dis;
    for (unsigned i = 0; i < initialSize; i++) {
      datas.front().push_back(dis(gen));
    }
    server.init(&datas);
    clients.reserve(nClients); // N.B. Server stores addr.
  }

  inline void run(unsigned nClients, unsigned initialSize, unsigned rounds, unsigned opsPerRound, unsigned testPeriod = 0)
  {
    Timer t;
    std::cout << "Initial size: " << initialSize << ". Max " << rounds << " rounds of sync.\n";
    std::cout << nClients << " clients, each makes " << opsPerRound << " ops then sync with server:\n";
    //gen.seed(0); // Make fixed sequence for now.
    for (unsigned r = 0; r <= rounds; r++) {
      if (r % 4 == 0 && clients.size() < nClients) { // Add another client.
        clients.emplace_back();
        clients.back().connect(server);
      }
      for (auto& c : clients) {
        if (!c.connected())
          continue;
        auto pack = createLocalOps(c, opsPerRound);
        c.makeLocalOps(pack);
      }
      server.update(); // Sync everything
      if (!test(t, r, testPeriod))
        break;
    }
    std::cout << "Stopped at " << t << "sec, " << "state size: " << datas.front().size() << "\n";
    if (checkStates())
      std::cout << "OK\n";
    else
      std::cout << "Consistency lost :(\n";
  }
};

int main(int argc, const char* argv[])
{
#ifdef _DEBUG
  __debugbreak();
#endif

  CmdOptions cmd(argc, argv);

  unsigned clients = 20; // Num of concurrent clients.
  if (cmd.exists_option("-c"))
    clients = std::stoi(cmd.get_option("-c"));

  unsigned size = 10 * 1000 * 1000U; // Initial size of the buffer
  if (cmd.exists_option("-s"))
    size = std::stoi(cmd.get_option("-s"));

  unsigned rounds = 24 * 60 * 60U; // Pretend we update once a sec. How long can it run?
  if (cmd.exists_option("-r"))
    rounds = std::stoi(cmd.get_option("-r"));

  unsigned ops = 5; // Ops to make locally before the sync with server.
  if (cmd.exists_option("-o"))
    ops = std::stoi(cmd.get_option("-o"));

  unsigned test = 30; // Test the consistency after a num of rounds.
  if (cmd.exists_option("-t"))
    test = std::stoi(cmd.get_option("-t"));

  TrivialClientServer runner(clients, size);
  runner.run(clients, size, rounds, ops, test);
}
