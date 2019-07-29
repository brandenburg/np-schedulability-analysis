cmake_minimum_required(VERSION 3.8)

project(NP_schedulabiliy_test)

option(USE_TBB_MALLOC "Use the Intel TBB scalable memory allocator" ON)
option(USE_JE_MALLOC "Use the Facebook jemalloc scalable memory allocator" OFF)

option(DEBUG "Enable debugging" OFF)

option(COLLECT_SCHEDULE_GRAPHS "Enable the collection of schedule graphs" OFF)

if (DEBUG)
    set(CMAKE_BUILD_TYPE Debug)
else()
    set(CMAKE_BUILD_TYPE Release)
endif()

include_directories(include)
include_directories(lib/include)

if (COLLECT_SCHEDULE_GRAPHS)
    add_compile_definitions(CONFIG_COLLECT_SCHEDULE_GRAPH)
endif()

find_path(TBB_INCLUDE_DIR NAMES tbb/task_scheduler_init.h)
find_library(TBB_LIB NAMES tbb)

if(USE_JE_MALLOC)
  find_library(ALLOC_LIB NAMES jemalloc)
elseif(USE_TBB_MALLOC)
  find_library(TBB_ALLOC tbbmalloc_proxy)
endif()

set(CORE_LIBS ${TBB_LIB} ${ALLOC_LIB})

include_directories(${TBB_INCLUDE_DIR})

file(GLOB TEST_SOURCES "src/tests/*.cpp")
add_executable(runtests ${TEST_SOURCES} ${SOURCES})

set(NPTEST_SOURCES src/nptest.cpp lib/src/OptionParser.cpp)
add_executable(nptest ${NPTEST_SOURCES})

target_link_libraries(nptest ${CORE_LIBS})
target_link_libraries(runtests ${CORE_LIBS})

target_compile_features(runtests PUBLIC cxx_std_14)
target_compile_features(nptest PUBLIC cxx_std_14)


