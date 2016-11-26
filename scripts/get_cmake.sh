#!/bin/bash
set -e

SCRIPT_PATH=$(cd $(dirname $0); pwd -P)

CMAKE_DIR="$SCRIPT_PATH/../external/cmake"
HOST_OS=$(uname -s)
CMAKE_PREFIX=cmake-3.7.0-$HOST_OS-x86_64

if [ ! -f "$CMAKE_DIR/cmake" ]; then
    if [ "$HOST_OS" == "Darwin" ]; then
        CMAKE="$CMAKE_DIR/$CMAKE_PREFIX/CMake.app/Contents/bin/cmake"
    else
        CMAKE="$CMAKE_DIR/$CMAKE_PREFIX/bin/cmake"
    fi

    echo CMake not found at $CMAKE, retrieving...
    mkdir -p "$CMAKE_DIR"

    CMAKE_ARCHIVE="$CMAKE_PREFIX.tar.gz"
    rm -f "$CMAKE_DIR/$CMAKE_ARCHIVE"
    CMAKE_URL=https://cmake.org/files/v3.7/$CMAKE_ARCHIVE

    if [ "$HOST_OS" == "Darwin" ]; then
        curl -L -o "$CMAKE_DIR/$CMAKE_ARCHIVE" $CMAKE_URL
    else
        wget --no-check-certificate -P "$CMAKE_DIR" $CMAKE_URL
    fi

    tar xzf "$CMAKE_DIR/$CMAKE_ARCHIVE" -C "$CMAKE_DIR"
    rm "$CMAKE_DIR/$CMAKE_ARCHIVE"
    ln -s "$CMAKE" "$CMAKE_DIR/cmake"
fi

