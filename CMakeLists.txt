cmake_minimum_required(VERSION 3.10.0)

project(multi-get VERSION 0.1.0)
configure_file(version.h.in version.h)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)


include_directories(include)
include_directories(${PROJECT_BINARY_DIR})

include(CTest)
enable_testing()

find_package(Threads)
find_package(OpenSSL REQUIRED)

aux_source_directory(src srcs)

add_executable(multi-get ${srcs})
target_link_libraries(multi-get ${CMAKE_THREAD_LIBS_INIT} OpenSSL::SSL)
# Clang和GCC需要额外链接stdc++fs，而MSVC不需要
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" OR CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    target_link_libraries(multi-get stdc++fs)
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_SOURCE_GENERATOR "TGZ")
set(CPACK_SOURCE_IGNORE_FILES
    .git
    .vscode
    build
    .clang-format
    .gitignore)
set(CPACK_SOURCE_PACKAGE_FILE_NAME ${PROJECT_NAME}-${PROJECT_VERSION})
include(CPack)
