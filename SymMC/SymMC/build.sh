#!/bin/bash

export CC=gcc-9
export CXX=g++-9

rm -rf ./cmake-build-debug
cmake -Bcmake-build-debug -H.
cmake --build cmake-build-debug --target all
rm -rf ./cmake-build-release
cmake -Bcmake-build-release -H.
cmake --build cmake-build-release --target all
