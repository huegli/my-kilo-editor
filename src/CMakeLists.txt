add_library(editor STATIC row.cpp)
target_include_directories(editor PUBLIC .)

add_executable(kilo main.cpp)
target_link_libraries(kilo PRIVATE editor)
