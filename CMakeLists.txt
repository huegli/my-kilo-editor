cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
  my-kilo-editor
  LANGUAGES CXX C)

add_executable(kilo src/main.cpp src/row.cpp)

Include(FetchContent)

FetchContent_Declare(
  Catch2
  GIT_REPOSITORY https://github.com/catchorg/Catch2.git
  GIT_TAG        v3.0.1 # or a later release
)

FetchContent_MakeAvailable(Catch2)

add_executable(tests test/tests.cpp)
target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
