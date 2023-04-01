find_package(fmt CONFIG REQUIRED)
# find_package(spdlog CONFIG REQUIRED)
# find_package(CLI11 CONFIG REQUIRED)

# Generic test that uses conan libs
add_executable(my-kilo main.cpp)
target_link_libraries(
  my-kilo
  PUBLIC project_options project_warnings)
  #  PRIVATE CLI11::CLI11 fmt::fmt spdlog::spdlog)

target_include_directories(my-kilo PRIVATE "${CMAKE_BINARY_DIR}/configured_files/include")
