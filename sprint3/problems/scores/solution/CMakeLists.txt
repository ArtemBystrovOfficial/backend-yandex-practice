cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

include(CTest)
include(${CONAN_BUILD_DIRS_CATCH2}/Catch.cmake) 

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
get_filename_component(full_path_main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ABSOLUTE)
list(REMOVE_ITEM SOURCES "${full_path_main_cpp}")
file(GLOB TESTS "tests/*.cpp" "tests/*.h")

add_library(SourceLib STATIC ${SOURCES})

target_include_directories(SourceLib PUBLIC CONAN_PKG::boost)
target_link_libraries(SourceLib PUBLIC CONAN_PKG::boost)

add_executable(game_server_tests ${TESTS})
add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} PRIVATE SourceLib)

target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 SourceLib)
target_include_directories(game_server_tests PRIVATE CONAN_PKG::catch2)
target_compile_definitions(game_server_tests PRIVATE CMAKE_BIN_PATH="${CMAKE_BINARY_DIR}/bin")
catch_discover_tests(game_server_tests) 