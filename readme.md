### Op Transform demo project

#### Build

1. To configure with cmake do one of the following:
   * cmake -S . -B ./build/rel -G "Unix Makefiles" -DCMAKE_BUILD_TYPE=Release
   * cmake -S . -B ./build/win -G "Visual Studio 16 2019"

2. Then build with:
   * cmake --build ./build/${config}
   * cmake --build ./build/${config} --config Release
   * or use ALL_BUILD.vcxproj

