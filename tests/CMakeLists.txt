file(GLOB_RECURSE MY_H CONFIGURE_DEPENDS "*.hpp")
file(GLOB_RECURSE MY_SRC CONFIGURE_DEPENDS "*.cpp")

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Headers Files" FILES ${MY_H})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" PREFIX "Source Files" FILES ${MY_SRC})

enable_testing()
add_executable(tests ${MY_SRC})
target_link_libraries(tests PRIVATE gtest gtest_main)
include(GoogleTest)
gtest_discover_tests(tests WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
