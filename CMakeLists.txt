cmake_minimum_required(VERSION 3.10.0)

project(multi-get VERSION 0.1.1)
configure_file(version.h.in version.h)

set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)

set(CMAKE_BUILD_TYPE Release)

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

install(TARGETS multi-get
        COMPONENT applications
        DESTINATION "bin"
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
set(CPACK_GENERATOR "DEB")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_DEBIAN_PACKAGE_NAME ${PROJECT_NAME})
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64")
set(CPACK_PACKAGE_DESCRIPTION "multi-thread download tool")
set(CPACK_PACKAGE_CONTACT "xueyuanzhou@qq.com")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "xyz")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "Optional")
include(CPack)
