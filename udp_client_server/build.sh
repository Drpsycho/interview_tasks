#!/bin/bash

mkdir -p build_linux
pushd build_linux
cmake ..
make