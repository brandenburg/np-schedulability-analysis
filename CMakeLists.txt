cmake_minimum_required(VERSION 3.8)

project(NP_schedulabiliy_test)

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_BUILD_TYPE Release)

include_directories(include)
include_directories(lib/include)

#set(SOURCES "src/schedule_space.cpp")

file(GLOB TEST_SOURCES "src/tests/*.cpp")
add_executable(runtests ${TEST_SOURCES} ${SOURCES})

set(NPTEST_SOURCES src/nptest.cpp lib/src/OptionParser.cpp)
add_executable(nptest ${NPTEST_SOURCES})

target_compile_features(runtests PUBLIC cxx_std_14)
target_compile_features(nptest PUBLIC cxx_std_14)


