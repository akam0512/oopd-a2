cmake_minimum_required(VERSION 3.14)
project(BookManagement LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(bookmgmt
    src/Money.cpp
    src/Resource.cpp
    src/Book.cpp
    src/ElectronicResource.cpp
    src/Budget.cpp
    src/Catalog.cpp
    src/Acquisition.cpp
)
target_include_directories(bookmgmt PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(bookmgmt PRIVATE /W4)
else()
    target_compile_options(bookmgmt PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(demo examples/demo.cpp)
target_link_libraries(demo PRIVATE bookmgmt)

enable_testing()
add_executable(bookmgmt_tests tests/test_main.cpp)
target_link_libraries(bookmgmt_tests PRIVATE bookmgmt)
add_test(NAME bookmgmt_tests COMMAND bookmgmt_tests)
