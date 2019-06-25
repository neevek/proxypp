#!/bin/bash

if [[ "$NDK" = "" ]]; then
  echo "\$NDK is empty"
  exit 1
fi

abi1=${1:-arm64-v8a}
abi2=
if [[ "$abi1" != "arm64-v8a" ]] && [[ "$abi1" != "arm-v7a" ]]; then
  echo "invalid abi1: $abi1, only arm64-v8a or arm-v7a is supported"
  exit 1
fi

PREFIX1=
PREFIX2=
if [[ "$abi1" = "arm64-v8a" ]]; then
  PREFIX1=aarch64-linux-android
  PREFIX2=aarch64-linux-android21
  abi2=arm64-v8a
elif [[ "$abi1" = "arm-v7a" ]]; then
  PREFIX1=arm-linux-androideabi
  PREFIX2=armv7a-linux-androideabi21
  abi2=armeabi-v7a
fi

SRC_DIR=$(pwd)
API=21

mkdir -p build

function build {
  HOST_TAG=darwin-x86_64
  export TOOLCHAIN=$NDK/toolchains/llvm/prebuilt/$HOST_TAG
  export AR=$TOOLCHAIN/bin/${PREFIX1}-ar
  export AS=$TOOLCHAIN/bin/${PREFIX1}-as
  export LD=$TOOLCHAIN/bin/${PREFIX1}-ld
  export RANLIB=$TOOLCHAIN/bin/${PREFIX1}-ranlib
  export STRIP=$TOOLCHAIN/bin/${PREFIX1}-strip
  export CC=$TOOLCHAIN/bin/${PREFIX2}-clang
  export CXX=$TOOLCHAIN/bin/${PREFIX2}-clang++

  cmake \
    -DCMAKE_TOOLCHAIN_FILE=$NDK/build/cmake/android.toolchain.cmake \
    -DANDROID_STL=c++_shared \
    -DANDROID_TOOLCHAIN=clang \
    -DANDROID_PLATFORM=android-${API} \
    -DANDROID_ABI=$abi2 \
    -DCMAKE_ANDROID_NDK=$NDK \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=$API \
    -DCMAKE_ANDROID_ARCH_ABI=$abi1 \
    -DCMAKE_ANDROID_STL_TYPE=c++_shared \
    -DCMAKE_CROSSCOMPILING=TRUE \
    -DHOST_COMPILER=${PREFIX1} \
    -DBUILD_RELEASE=TRUE \
    ..

  NPROCESSORS=$(getconf NPROCESSORS_ONLN 2>/dev/null || getconf _NPROCESSORS_ONLN 2>/dev/null)
  PROCESSORS=${NPROCESSORS:-1}
  make -j${PROCESSORS}
}

cd build && rm -rf * && build
