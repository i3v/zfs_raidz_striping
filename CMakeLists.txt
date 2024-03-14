cmake_minimum_required(VERSION 3.17)
project(zfs_raidz_striping)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra)
add_executable(raidzdump main.cpp)
