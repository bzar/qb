CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(qb)

include("cmake/subproject.cmake")

SET_PROPERTY(GLOBAL APPEND PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS true)

find_package(Boost REQUIRED system filesystem)

set(GLHCK_BUILD_EXAMPLES OFF CACHE BOOL "Skip GLHCK examples")
add_subdirectory(lib)

include_directories(
  ${kazmath_SOURCE_DIR}/src
  ${glhck_SOURCE_DIR}/include
  ${GLFW_SOURCE_DIR}/include
  ${gas_SOURCE_DIR}/include
  ${Boost_INCLUDE_DIRS}
)

file(GLOB SOURCES src/*.cpp src/**/*.cpp )
add_definitions(-DGLHCK_KAZMATH_FLOAT -DUSE_SINGLE_PRECISION)
list(APPEND CMAKE_CXX_FLAGS -std=c++11)

add_executable(qb ${SOURCES})

target_link_libraries(qb glfw glhck gas ${GLFW_LIBRARIES} ${Boost_LIBRARIES})

