#!/bin/bash

sudo apt-get update
sudo apt-get install -y g++
sudo apt-get install -y make
sudo apt-get install -y cmake
sudo apt-get install -y ninja-build

echo "GCC version:"
g++ --version

echo "Make version:"
make --version

echo "CMake version:"
cmake --version

echo "Ninja version:"
ninja --version

echo "Installation complete!"
