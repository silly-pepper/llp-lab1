cmake_minimum_required(VERSION 3.22)
project(llp_lab1 C)

set(CMAKE_C_STANDARD 17)

include_directories(include)

add_executable(llp_lab1_test test/test.c
        include/internals.h
        include/types.h
        include/allocator.h
        src/allocator.c
        include/database.h
        src/database.c
        include/database_iterator.h
        src/database_iterator.c
        test/utils.h
        src/list.c)

add_executable(llp_lab1_benchmark test/benchmark.c
        include/internals.h
        include/types.h
        include/allocator.h
        src/allocator.c
        include/database.h
        src/database.c
        include/database_iterator.h
        src/database_iterator.c
        test/utils.h
        src/list.c)
