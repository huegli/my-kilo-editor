add_library(editor STATIC edit.cpp row.cpp syntax.cpp)
target_include_directories(editor PUBLIC .)

add_executable(kilo main.cpp tui.cpp)
target_link_libraries(kilo PRIVATE editor)
