link_directories(${PROJECT_SOURCE_DIR}/addr2Symbol/release/build/)
link_libraries(addr2Symbol)

string(TOLOWER ${CMAKE_SYSTEM_NAME} OS)
include_directories(${PROJECT_SOURCE_DIR}/addr2Symbol/src/main/)
include_directories($ENV{JAVA_HOME}/include/${OS}/)
include_directories($ENV{JAVA_HOME}/include/)

include_directories(cpp/)

file(GLOB_RECURSE SOURCES_CPP cpp/*.cpp)

set(NAME "agent")

find_package(Protobuf REQUIRED)
find_package(absl REQUIRED)
find_package(utf8_range REQUIRED)
file(GLOB_RECURSE ALL_PROTO_SOURCES "protos/**/*.pb.cc")
list(FILTER ALL_PROTO_SOURCES EXCLUDE REGEX ".*grpc\\.pb\\.cc$")

add_library(perfetto_protos STATIC ${ALL_PROTO_SOURCES})
target_link_libraries(perfetto_protos PUBLIC protobuf::libprotobuf)
target_include_directories(perfetto_protos PUBLIC ./)
target_compile_options(perfetto_protos PRIVATE "-fPIC" "-Wno-deprecated-declarations")

add_library(${NAME} SHARED ${SOURCES_CPP})
target_compile_options(${NAME} PRIVATE
        "-O3" "-Werror" "-Wall" "-Wpedantic" "-Wextra" "-Wshadow" "-fexceptions" "-Wnon-virtual-dtor" "-Wno-unused-parameter" "-Wno-gnu" "-Wno-deprecated-declarations"
        "$<$<CONFIG:DEBUG>:-Og;>"
)

if (DEFINED ENV{ADDRESS_SANITIZER})
    add_compile_options("-fsanitize=address;-fsanitize=undefined;")
    add_link_options("-fsanitize=address;-fsanitize=undefined;")
ENDIF()

if (DEFINED ENV{THREAD_SANITIZER} AND NOT APPLE)
    add_compile_options("-fsanitize=thread;-fsanitize=undefined;-Wno-error=tsan")
    add_link_options("-fsanitize=thread;-fsanitize=undefined;-Wno-error=tsan")
ENDIF()

if (NOT APPLE)
    link_libraries("unwind")
endif(NOT APPLE)

target_link_libraries(${NAME} PUBLIC
        addr2Symbol
        perfetto_protos
        protobuf::libprotobuf
        absl::log
        absl::log_internal_check_op
        absl::log_internal_message
        absl::cord
        absl::cordz_info
        absl::status
        absl::statusor
        absl::strings
        absl::str_format
        absl::raw_hash_set
        absl::hash
        absl::base
        absl::synchronization
        utf8_range::utf8_range
        utf8_range::utf8_validity
)

set_property(GLOBAL PROPERTY AGENT_PATH "${CMAKE_CURRENT_BINARY_DIR}/lib${NAME}${CMAKE_SHARED_LIBRARY_SUFFIX}")