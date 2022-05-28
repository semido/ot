#include <iostream>
#include <string>

int main(int argc, const char* argv[])
{
#ifdef _DEBUG
  __debugbreak();
#endif
  std::setlocale(LC_ALL, ".UTF-8");
  std::cout << "Hi, балбесик ☻\n";
}
