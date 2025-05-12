cmake_minimum_required(VERSION 3.30)
project(Cpp_Lessons)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Threads REQUIRED)

include(FetchContent)

FetchContent_Declare(
        cpp-httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
        GIT_TAG        v0.14.0
)
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG        v3.11.3
)
FetchContent_Declare(
        hinnant_date
        GIT_REPOSITORY https://github.com/HowardHinnant/date
        GIT_TAG        v3.0.1
)
FetchContent_Declare(
        inih
        GIT_REPOSITORY https://github.com/benhoyt/inih.git
        GIT_TAG        r57
)

FetchContent_MakeAvailable(
        cpp-httplib
        nlohmann_json
        hinnant_date
        inih
)

add_executable(Cpp_Lessons main.cpp)

target_include_directories(Cpp_Lessons PRIVATE
        ${cpp-httplib_SOURCE_DIR}
        ${nlohmann_json_SOURCE_DIR}/include
        ${hinnant_date_SOURCE_DIR}/include
        ${inih_SOURCE_DIR}
)

target_link_libraries(Cpp_Lessons PRIVATE
        Threads::Threads
        ${inih_SOURCE_DIR}/ini.c
)

if(WIN32)
    target_link_libraries(Cpp_Lessons PRIVATE ws2_32 wsock32)
endif()