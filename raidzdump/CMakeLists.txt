cmake_minimum_required(VERSION 3.17)
project(zfs_raidz_striping)

set(CMAKE_CXX_STANDARD 14)
add_compile_options(-Wall -Wextra)

add_executable(raidzdump_exe main.c)
add_library(raidzdump_lib SHARED libraidzdump.c)
target_link_libraries(raidzdump_exe raidzdump_lib)

install(TARGETS raidzdump_exe DESTINATION bin)
install(TARGETS raidzdump_lib DESTINATION bin)

set_target_properties(raidzdump_exe PROPERTIES OUTPUT_NAME "raidzdump.exe")
set_target_properties(raidzdump_lib PROPERTIES OUTPUT_NAME "raidzdump")  # actually creates "libraidzdump.so"

# allow loading ".so" from the same folder
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath='$ORIGIN'")