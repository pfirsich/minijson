cmake_minimum_required(VERSION 3.0.0)

project(minijson)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS on)

add_library(minijson STATIC minijson.cpp)

add_executable(test test.cpp)
target_link_libraries(test minijson)
target_compile_options(test PRIVATE -Wall -Wextra -pedantic -Werror)

add_executable(load-file load_file.cpp)
target_link_libraries(load-file minijson)
target_compile_options(load-file PRIVATE -Wall -Wextra -pedantic -Werror)
