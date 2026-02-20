cmake_minimum_required(VERSION 3.16)

project(MatrixApp C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GTK4 REQUIRED gtk4)

add_executable(matrix_app
    src/gui.c
    src/matrix_operations.c
    src/r-ref.c
)

target_include_directories(matrix_app
    PRIVATE
    include
    ${GTK4_INCLUDE_DIRS}
)

target_compile_options(matrix_app
    PRIVATE
    ${GTK4_CFLAGS_OTHER}
)

target_link_libraries(matrix_app
    PRIVATE
    ${GTK4_LIBRARIES}
    m
)