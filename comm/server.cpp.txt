#include <vector>
#include <atomic>
#include <mutex>
#include <condition_variable>

struct atomic_guard
{
  std::mutex& m;
  std::atomic<int>& c;
  atomic_guard(std::mutex& mutex, std::atomic<int>& cnt) :
    m(mutex),
    c(cnt)
  {
    std::lock_guard lock(m);
    c += 1;
  }
  ~atomic_guard()
  {
    std::lock_guard lock(m);
    c -= 1;
  }
};

template<typename T>
class Server
{
  std::vector<T> state;
  std::list<OpDescriptor<T>> received;
  std::list<OpDescriptor<T>> applied;
  int numClients = 0; // Server cloned its state for new client and assign Id.
  std::atomic<int> cloningThreads;
  std::mutex stateMutex; // Blocks access to cloningThreads and also while updating the state.
  std::condition_variable cv;
public:
  Server()
  {
    cloningThreads = 0;
  }
  int revision() const
  {
    return (int) applied.size();
  }
  auto clone(std::vector<T>& copy)
  {
    // It works in thread of client, so several copies can be done in parallel.
    atomic_guard aguard(stateMutex, cloningThreads); // Wait end of state update.
    copy = state;
    return std::make_tuple(++numClients, revision());
  }
};


template<typename T>
void clientLoop1(Server<T>& server)
{
  std::vector<T> state;
  auto [clientId, revision] = server.clone(state);

}

