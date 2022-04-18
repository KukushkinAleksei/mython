cmake_minimum_required(VERSION 3.20)

project(Mython CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB Mython_SRC CONFIGURE_DEPENDS "src/*.h" "src/*.cpp")

add_executable(mython ${Mython_SRC})
