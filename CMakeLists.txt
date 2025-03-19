cmake_minimum_required(VERSION 3.30)

set(PROJECT_NAME your_workbench_lib_test)

project(your_workbench_lib_test)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -static")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static -static-libgcc -static-libstdc++")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 23)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(your_workbench_lib_test ${SOURCES})

include(FetchContent)

FetchContent_Declare(
        your_workbench_lib
        URL https://github.com/YiRanMushroom/Your_Workbench_Lib/archive/master.zip
)

FetchContent_MakeAvailable(your_workbench_lib)

#[[
# Catch2
FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.5.3
        OVERRIDE_FIND_PACKAGE
)

# 确保 Catch2 使用正确的标准库
set(CATCH_CONFIG_NO_POSIX_SIGNALS ON)
set(CATCH_CONFIG_USE_ASYNC ON)

FetchContent_MakeAvailable(Catch2)
]]

file (GLOB_RECURSE Module_Files "src/*.cppm")

# 为测试目标设置选项
target_compile_options(your_workbench_lib_test PRIVATE -stdlib=libc++)
target_link_options(your_workbench_lib_test PRIVATE -stdlib=libc++ -lc++abi)
target_link_libraries(your_workbench_lib_test your_workbench_lib)

target_sources(your_workbench_lib_test
        PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES
        FILES ${Module_Files})