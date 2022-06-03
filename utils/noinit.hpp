#pragma once

// Helper for std::vector<NoInit<T>>
template<class T>
class NoInit {
public:
  NoInit() {}
  constexpr NoInit(T v) : value(v) {}
  constexpr operator T () const { return value; }
private:
  T value;
};
