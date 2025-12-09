cmake_minimum_required(VERSION 3.20)
project(316Project2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(316Project2 main.cpp
        variant_test.cpp)
target_compile_features(316Project2 PRIVATE cxx_std_20)