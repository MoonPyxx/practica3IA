#!/bin/bash

BUILD_DOC=OFF # ON or OFF

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_DOC=$BUILD_DOC .. 
make -j8
cd ..

./build/ParchisGame