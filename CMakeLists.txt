CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
project(qb)

include("cmake/subproject.cmake")

SET_PROPERTY(GLOBAL APPEND PROPERTY ALLOW_DUPLICATE_CUSTOM_TARGETS true)

set(GLHCK_BUILD_EXAMPLES OFF CACHE BOOL "Skip GLHCK examples")
SET(GLFW_BUILD_EXAMPLES 0 CACHE BOOL "Don't build examples for GLFW")
SET(GLFW_BUILD_TESTS 0 CACHE BOOL "Don't build tests for GLFW")

if (PANDORA)
   SET(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${qb_SOURCE_DIR}/lib/glhck/CMake/modules)
   FIND_PACKAGE(EGL REQUIRED)
   FIND_PACKAGE(GLESv1 REQUIRED)
   SET(GLFW_USE_EGL 1 CACHE BOOL "Use EGL with GLFW")
   SET(GLFW_CLIENT_LIBRARY "glesv1" CACHE STRING "Link EGL against glesv1")
endif()

add_subdirectory(lib)

include_directories(
  ${kazmath_SOURCE_DIR}/src
  ${glhck_SOURCE_DIR}/include
  ${GLFW_SOURCE_DIR}/include
  ${gas_SOURCE_DIR}/include
)

file(GLOB SOURCES src/*.cpp src/**/*.cpp )
add_definitions(-DGLHCK_KAZMATH_FLOAT -DUSE_SINGLE_PRECISION)
list(APPEND CMAKE_CXX_FLAGS -std=c++11)

add_executable(qb ${SOURCES})

target_link_libraries(qb glfw glhck gas ${GLFW_LIBRARIES})

file(COPY model DESTINATION .)
file(COPY levels DESTINATION .)
