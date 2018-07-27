#!/bin/bash

if [[ "$NDK" = "" ]]; then
  echo "\$NDK is empty"
  exit 1
fi

SRC_DIR=$(pwd)
API=21

mkdir -p toolchain build

function build {
  ANDROID_TOOLCHAIN=$SRC_DIR/toolchain/$API
  if [[ ! -d $ANDROID_TOOLCHAIN/sysroot ]]; then
    $NDK/build/tools/make-standalone-toolchain.sh \
      --arch=arm \
      --stl=libc++ \
      --install-dir=$ANDROID_TOOLCHAIN \
      --platform=android-$API \
      --force
  fi
  export CC=$ANDROID_TOOLCHAIN/bin/arm-linux-androideabi-gcc
  export CXX=$ANDROID_TOOLCHAIN/bin/arm-linux-androideabi-g++
  export LINK=$ANDROID_TOOLCHAIN/bin/arm-linux-androideabi-g++
  export AR=$ANDROID_TOOLCHAIN/bin/arm-linux-androideabi-ar
  export PLATFORM=android
  export CFLAGS="-D__ANDROID_API__=$API"

  cmake \
    -DCMAKE_ANDROID_STANDALONE_TOOLCHAIN=$ANDROID_TOOLCHAIN \
    -DCMAKE_SYSTEM_NAME=Android \
    -DCMAKE_SYSTEM_VERSION=$API \
    -DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a \
    -DCMAKE_CROSSCOMPILING=TRUE \
    -DBUILD_RELEASE=TRUE \
    ..
  make -j8
}

cd build && rm -rf * && build
