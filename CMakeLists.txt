cmake_minimum_required(VERSION 3.20)
project(janguage LANGUAGES CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_C_COMPILER /usrbin/clang-18 CACHE FILEPATH "" FORCE)
set(CMAKE_CXX_COMPILER /usr/bin/clang++-18 CACHE FILEPATH "" FORCE)
add_compile_options(-std=c++20 -stdlib=libc++)
add_link_options(-std=c++20 -stdlib=libc++)

set(shared_source_files
    compile.cpp
    disasm.cpp
    lex.cpp
    memory_pool.cpp
    parse.cpp
    stringify.cpp
    typecheck.cpp
    vm.cpp
    )

add_executable(jang main.cpp ${shared_source_files})
target_include_directories(jang PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# target_compile_options(jang PUBLIC -fsanitize=address)
# target_link_options(jang PUBLIC -fsanitize=address)

target_compile_features(jang PUBLIC cxx_std_20)


#
# Tests
#

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Catch2)
# find_package(Catch2 3 REQUIRED)

add_executable(
    tests
    compile_test.cpp
    integration_tests.cpp
    lex_test.cpp
    parse_test.cpp
    string_util_test.cpp
    typecheck_test.cpp
    vm_test.cpp
    ${shared_source_files})
target_include_directories(tests PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(tests PUBLIC Catch2::Catch2WithMain)
