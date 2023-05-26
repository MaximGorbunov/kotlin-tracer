cmake_minimum_required(VERSION 3.10)
project(kotlin-tracer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

link_directories($ENV{JDK_DIR}/lib/server/)
link_libraries(jvm)

add_subdirectory(src/main ./agent/)

#Testing
enable_testing()
add_subdirectory(src/test)