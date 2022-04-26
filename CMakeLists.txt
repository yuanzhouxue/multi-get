cmake_minimum_required(VERSION 3.10.0)

project(multi-get VERSION 0.1.0)
set(CMAKE_CXX_STANDARD 20)
set(CXX_STANDARD_REQUIRED ON)


include_directories(include)

include(CTest)
enable_testing()

add_executable(multi-get src/main.cpp src/HTTPResponse.cpp src/HTTPConnection.cpp)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
