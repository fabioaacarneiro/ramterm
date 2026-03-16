#!/bin/bash

# simple build script for RamTerm

if [ -d "build" ]; then
    rm -rf build
fi

mkdir build
cd build
cmake ..
make