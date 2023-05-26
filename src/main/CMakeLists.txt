add_compile_options(
        "-Wall" "-Wpedantic" "-Wextra" "-Wshadow" "-fexceptions" "-Wnon-virtual-dtor" "-Wno-unused-parameter" "-DDEBUG"
        "$<$<CONFIG:DEBUG>:-fsanitize=address;-DDEBUG>"
)

add_link_options(
        "$<$<CONFIG:DEBUG>:-fsanitize=address>"
)

string(TOLOWER ${CMAKE_SYSTEM_NAME} OS)
include_directories($ENV{JDK_DIR}/include/${OS}/)
include_directories($ENV{JDK_DIR}/include/)

include_directories(cpp/)

file(GLOB_RECURSE SOURCES_CPP cpp/*.cpp)

add_library(agent SHARED ${SOURCES_CPP})