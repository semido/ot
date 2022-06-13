### Op Transform demo project

Implement concurrent merge algo, which makes sure that actions of several users on a common document 
(with a simple data structure = array of elements) result in all of them eventually seeing the same result locally.

- In the initial state Server holds RGA of 10,000,000 elements. It should still work with an element size 4..64 bytes.
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

#### Progress

* Support N clients + server. It creates 1 op per client (N) during a single round. 
  Do exchange/sync through the server.

* Generate M ops on N clients. All synced using same package via the server.

* Create Client & Server classes with simple comm interface.

- Use threads.

- Make design analisys.

- Implement offline, i.e. each client works with own update freq.
