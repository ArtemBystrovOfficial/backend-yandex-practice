cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

file(GLOB SOURCES "src/*.cpp" "src/*.h")
get_filename_component(full_path_main_cpp ${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp ABSOLUTE)
list(REMOVE_ITEM SOURCES "${full_path_main_cpp}")
file(GLOB TESTS "tests/*.cpp" "tests/*.h")

add_executable(game_server_tests ${SOURCES} ${TESTS})
target_include_directories(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost)
target_link_libraries(game_server_tests PRIVATE CONAN_PKG::catch2 CONAN_PKG::boost)

add_executable(${PROJECT_NAME} ${SOURCES} src/main.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE CONAN_PKG::boost)
target_link_libraries(${PROJECT_NAME} PRIVATE Threads::Threads CONAN_PKG::boost)
