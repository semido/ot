cmake_minimum_required(VERSION 3.9)

# External sort demo project.
project(ot CXX)

# Put all targets in the same directory rather than per project as by default.
include(GNUInstallDirs)
set(stageDir ${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_INSTALL_BINDIR})
message("stageDir=${stageDir}")
if(NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
  set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${stageDir})
endif()
if(NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
  set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${stageDir})
endif()
if(NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
  set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${stageDir})
endif()

# Put sources into appropriate subfolders in vcxproj-es.
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(MSVC)
  set(CMAKE_CXX_STANDARD 17)
#  add_compile_options("-Zc:__cplusplus")
else()
  add_compile_options("-std=c++17")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG -pthread")
endif()

add_executable(ot main.cpp)

target_sources(ot PRIVATE readme.md)

add_subdirectory(ops)
