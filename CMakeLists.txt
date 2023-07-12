cmake_minimum_required(VERSION 3.10)
project(kotlin-tracer)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(SOURCES_DIR ${PROJECT_SOURCE_DIR}/src/main/cpp)

link_directories($ENV{JAVA_HOME}/lib/server/)
link_libraries(jvm)

enable_testing()
add_subdirectory(src/main ./agent/)
add_subdirectory(src/test)