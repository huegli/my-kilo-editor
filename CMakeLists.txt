cmake_minimum_required(VERSION 3.20)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)

project(
  my-kilo-editor
  LANGUAGES CXX C)

add_executable(kilo src/main.cpp)
