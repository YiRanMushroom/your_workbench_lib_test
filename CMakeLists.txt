cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME your_workbench_lib_test)

project(your_workbench_lib_test)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(your_workbench_lib_test ${SOURCES})

include(FetchContent)

FetchContent_Declare(
        your_workbench_lib
        URL https://github.com/YiRanMushroom/Your_Workbench_Lib/archive/master.zip
)

FetchContent_MakeAvailable(your_workbench_lib)

target_link_libraries(your_workbench_lib_test your_workbench_lib)