cmake_minimum_required(VERSION 2.8)

set(CMAKE_CXX_FLAGS "-Wall -std=c++14 -fexceptions -frtti -DLOG_TAG_NAME='\"TR\"'")
if(CMAKE_ANDROID_STANDALONE_TOOLCHAIN)
  message("Building shared sockspp library for Android...")

  set(BUILD_SHARED_LIBRARY TRUE)

  # for executable
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC -fPIE")
  set(CMAKE_C_LINK_FLAGS "")
  set(CMAKE_CXX_LINK_FLAGS "")
ELSE()
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBUILD_CLIENT")
endif()

if(NOT BUILD_RELEASE)
  message("Building for debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLOG_VERBOSE -g")
else()
  message("Building for release")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DLOG_DEBUG -O3")
endif()

find_package(Threads REQUIRED)

include(ExternalProject)
ExternalProject_Add(uvcpp
  PREFIX "${CMAKE_BINARY_DIR}/deps"
  GIT_REPOSITORY "${CMAKE_SOURCE_DIR}/../uvcpp"
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
set(UV_INCLUDE_DIR ${SOURCE_DIR}/../uvcpp-build/libuv-prefix/src/libuv/include)
set(UV_LIBRARY_DIR ${SOURCE_DIR}/../uvcpp-build/libuv-prefix/src/libuv/.libs)
add_library(uv IMPORTED STATIC GLOBAL)
add_dependencies(uv uvcpp)
set_target_properties(uv PROPERTIES IMPORTED_LOCATION "${UV_LIBRARY_DIR}/libuv.a")

include_directories(
  ${UVCPP_INCLUDE_DIR}
  ${UV_INCLUDE_DIR}
  ) 

set(LINK_LIBS uv)
if(CMAKE_ANDROID_STANDALONE_TOOLCHAIN)
  set(LINK_LIBS ${LINK_LIBS} log)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  set(LINK_LIBS ${LINK_LIBS} pthread)
endif()

set(SRCS
  src/buffer_pool.cc
  src/socks_server.cc
  src/client.cc
  src/socks.cc
  src/sockspp.cc
  )

if(BUILD_SHARED_LIBRARY)
  add_library(sockspp SHARED ${SRCS})
  add_dependencies(sockspp uvcpp)
  target_link_libraries(sockspp ${LINK_LIBS})
else()
  add_executable(sockspp ${SRCS})
  add_dependencies(sockspp uvcpp)
  target_link_libraries(sockspp ${LINK_LIBS})
endif()
