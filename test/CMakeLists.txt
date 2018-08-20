cmake_minimum_required(VERSION 2.8)

enable_testing()

set(CMAKE_CXX_FLAGS "-g -Wall -std=c++14 -DLOG_VERBOSE -DLOG_TAG_NAME='\"sockspp\"'")
set(SOCKSPP_SRC_DIR ${CMAKE_SOURCE_DIR}/../src)
include_directories(${SOCKSPP_SRC_DIR})

find_package(Threads REQUIRED)


# macro to set library properties
macro(SET_LIBRARY_PROP LIB_NAME DEPENDENCY LIB_FILE)
  # Create a libgmock target to be used as a dependency by test programs
  add_library(${LIB_NAME} IMPORTED STATIC GLOBAL)
  add_dependencies(${LIB_NAME} ${DEPENDENCY})

  # Set libgmock properties
  set_target_properties(${LIB_NAME} PROPERTIES
    "IMPORTED_LOCATION" ${LIB_FILE}
    "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}"
    )
endmacro()


# Enable ExternalProject CMake module
include(ExternalProject)


# add uvcpp dependency
ExternalProject_Add(uvcpp
  PREFIX "${CMAKE_BINARY_DIR}/deps"
  GIT_REPOSITORY "${CMAKE_SOURCE_DIR}/../../uvcpp"
  GIT_TAG "master"
  CMAKE_ARGS -DCMAKE_ANDROID_STANDALONE_TOOLCHAIN=${CMAKE_ANDROID_STANDALONE_TOOLCHAIN}
  #UPDATE_COMMAND ""
  INSTALL_COMMAND ""
  TEST_COMMAND ""
  GIT_SHALLOW 1
  GIT_PROGRESS 1
)

ExternalProject_Get_Property(uvcpp SOURCE_DIR)
set(UVCPP_INCLUDE_DIR ${SOURCE_DIR}/src)
set(NUL_INCLUDE_DIR ${SOURCE_DIR}/../uvcpp-build/nul-prefix/src/nul/src)
set(UV_INCLUDE_DIR ${SOURCE_DIR}/../uvcpp-build/libuv-prefix/src/libuv/include)
set(UV_LIBRARY_DIR ${SOURCE_DIR}/../uvcpp-build/libuv-prefix/src/libuv/.libs)
add_library(uv IMPORTED STATIC GLOBAL)
add_dependencies(uv uvcpp)
set_target_properties(uv PROPERTIES IMPORTED_LOCATION "${UV_LIBRARY_DIR}/libuv.a")


# add GoogleTest
ExternalProject_Add(gtest
  PREFIX "${CMAKE_BINARY_DIR}/deps"
  GIT_REPOSITORY "https://github.com/google/googletest.git"
  GIT_TAG "master"
  CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_BINARY_DIR}/deps/installed
  GIT_SHALLOW 1
  GIT_PROGRESS 1
  UPDATE_DISCONNECTED 1
)
# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest SOURCE_DIR BINARY_DIR)
set(GTEST_INCLUDE_DIR ${SOURCE_DIR})
set(GTEST_LIBRARY_DIR ${BINARY_DIR})
SET_LIBRARY_PROP(libgtest gtest "${GTEST_LIBRARY_DIR}/googlemock/gtest/libgtest.a")
SET_LIBRARY_PROP(libgmock gtest "${GTEST_LIBRARY_DIR}/googlemock/libgmock.a")

include_directories(
  "${GTEST_INCLUDE_DIR}/googletest/include"
  "${GTEST_INCLUDE_DIR}/googlemock/include"
  "${UVCPP_INCLUDE_DIR}"
  "${NUL_INCLUDE_DIR}"
  "${UV_INCLUDE_DIR}"
  ) 

set(COMMON_SRCS
  main.cc
  ${SOCKSPP_SRC_DIR}/socks_server.cc
  ${SOCKSPP_SRC_DIR}/client.cc
  ${SOCKSPP_SRC_DIR}/socks_req_parser.cc
  ${SOCKSPP_SRC_DIR}/socks_resp_parser.cc
  ${SOCKSPP_SRC_DIR}/socks_client.cc
  ${SOCKSPP_SRC_DIR}/sockspp.cc
  )
set(COMMON_LINK_LIBS libgtest libgmock uv)

macro(ADD_SOCKSPP_TEST TEST_NAME TEST_SOURCE)
  add_executable(${TEST_NAME} ${COMMON_SRCS} ${TEST_SOURCE})
  target_link_libraries(${TEST_NAME} ${COMMON_LINK_LIBS})
  add_test(NAME ${TEST_NAME} COMMAND ${TEST_NAME})
endmacro()

ADD_SOCKSPP_TEST(client sockspp/test_server_and_client.cc)