### Op Transform demo project

Implement concurrent merge algo, which makes sure that actions of several users on a common document 
(with a simple data structure = array of elements) result in all of them eventually seeing the same result locally.

- In the initial state Server holds an ordered array of 10,000,000 elements. It should still work with an element size 4..64 bytes.
- At any time a client process can connect, download the current state and start inserting, updating, or deleting numbers 
  (each time choosing a random position and value of the operation) at a speed of around 5 operations per second.
- There can be up to 20 clients connected at the same time.
- State should be eventual consistent:
  - Change made by any client should be present in every other client's state + the server's state within 1 second. 
    Therefore if everyone stops pushing new operations - everyone's state should be equal within a similar 1 second.

- An efficient implementation of the replication algorithm is expected. 
  The algorithm inside one executable running on multiple threads.
  Or to further minimize amount of boilerplate code - having a single threaded executable where clients' and server 
  functions are called one after another in a loop. But in this case please don't use the fact that every client has 
  the same current time because in real life clients would be on different machines and their local time is not guaranteed 
  to be exactly the same.
- Do care about the efficiency of the implementation - how much data needs to be sent around, how complex the calculations 
  are to reconcile the state e.t.c. Feel free to make trade-offs (CPU time vs RAM vs Network traffic) similar to 
  how you would do it if you had to design such a system in real life in production serving 1mln+ users that can be 
  connected via 3G/4G, not only high speed broadband.
- Good solution description is a bonus:
  - What/why algorithms and data structures have been chosen.
  - What alternatives were considered
  - Where are design tradeoffs, optimizations required
- How would you implement the algorithm if one or several clients can work "offline" for a few hours (e.g. on a plane) 
  and the state needs to be reconciled once they are back online.

#### Build

1. To configure with cmake do one of the following:
   * cmake -S . -B ./build/rel -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
   * cmake -S . -B ./build/win -G "Visual Studio 16 2019"

2. Then build with:
   * cmake --build ./build/${config}
   * cmake --build ./build/${config} --config Release
   * or use ALL_BUILD.vcxproj

#### Executable and Options

Main executable runs on initial state of 10,000,000 elements by default.
Clients are created with little delay, 1 cli in 4 rounds until it makes 20 clients.
It does 24x60x60 rounds of sync between 20 concurrent clients and the communication server.
Each client produces 5 ops before push it to the server.
By default it tests all states once per 30 rounds, which makes notable slowdown.

Options:
  * -c N, Number of clients, def = 20
  * -s N, Number of elements in the initial state, def = 10,000,000
  * -r N, Number of rounds, def = 86,400
  * -o N, Number of ops to make locally before the sync with server, def = 5
  * -t N, Test the consistency after a num of rounds, def = 30
          With -t 0, it does a test in 1,800 rounds.

#### Progress

* Support N clients + server. It creates 1 op per client (N) during a single round. 
  Do exchange/sync through the server.

* Generate M ops on N clients. All synced using same package via the server.

* Create Client & Server classes with simple comm interface.

* To Do

  - Make intermediate design analisys.

  - Use threads.

  - Implement offline, i.e. each client works with own update freq.
