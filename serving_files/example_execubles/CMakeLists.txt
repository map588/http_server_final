cmake_minimum_required(VERSION 3.10)
project(MyProject)

# Set the executables output directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../../Executables")

# Add executable targets for each .cpp file
add_executable(alternating_case alternating_case.cpp)
add_executable(ascii_art ascii_art.cpp)
add_executable(char_pyramid char_pyramid.cpp)
add_executable(diamond_chars diamond_chars.cpp)
add_executable(echo_arg print_arg.cpp)
add_executable(repeating_chars repeating_chars.cpp)
add_executable(reverse_string reverse_string.cpp)
add_executable(rotating_chars spiral.cpp)
add_executable(cross_chars wavy.cpp)
