cmake_minimum_required(VERSION 3.30)
project(Cpp_Lessons)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Подключение зависимостей
include(FetchContent)

# cpp-httplib
FetchContent_Declare(
        cpp-httplib
        GIT_REPOSITORY https://github.com/yhirose/cpp-httplib
        GIT_TAG        v0.14.0
)
FetchContent_MakeAvailable(cpp-httplib)

# nlohmann/json
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json
        GIT_TAG        v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# HowardHinnant/date
FetchContent_Declare(
        hinnant_date
        GIT_REPOSITORY https://github.com/HowardHinnant/date
        GIT_TAG        v3.0.1
)
FetchContent_MakeAvailable(hinnant_date)

add_executable(Cpp_Lessons main.cpp)

# Подключаем зависимости с явным указанием путей
target_include_directories(Cpp_Lessons PRIVATE
        ${cpp-httplib_SOURCE_DIR}
        ${nlohmann_json_SOURCE_DIR}/include  # Новый путь для json
        ${hinnant_date_SOURCE_DIR}/include
)

# Линковка с системными библиотеками
find_package(Threads REQUIRED)
target_link_libraries(Cpp_Lessons PRIVATE
        Threads::Threads
        ${CMAKE_THREAD_LIBS_INIT}
)

# Для Windows
if(WIN32)
    target_link_libraries(Cpp_Lessons PRIVATE ws2_32 wsock32)
endif()