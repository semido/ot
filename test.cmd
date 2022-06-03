cmake -S . -B .\build\win
cmake --build .\build\win --config Release
start /b /wait .\bin\release\tests.exe