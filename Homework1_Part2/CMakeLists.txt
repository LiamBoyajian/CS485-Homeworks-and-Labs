# Specify the minimum version of CMake required
cmake_minimum_required(VERSION 3.10)

# Set the project name and version
project(MyProject VERSION 1.0)

# Specify the C++ standard (e.g., C++17 or C++20)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add the executable
add_executable(Main Main.cpp Assembler.cpp Utility_Methods.cpp)
