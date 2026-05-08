#!/bin/bash

cmake_ver=3.29.3
op_sys=windows-x86_64

#let's check the final destination before we actually do anything
destination="$HOME/.configuration-dependencies/differential-equations"
destination_dep="$destination/cmake-$cmake_ver-$op_sys"

CMAKE="$destination_dep"/bin/cmake.exe
GCC="$destination"/mingw64/bin/gcc.exe

os_type="$(uname -s)"

if [[ "$os_type" == "Linux" ]]; then
	if ! command -v cmake &>/dev/null || ! command -v ninja &>/dev/null; then
		echo "cmake or ninja not found, please run ./configure"
		exit 1
	fi
	echo "Building the project using Ninja..."
	cmake -B"build" -G Ninja
	echo "Finished building the project."
	cd build
	ninja
elif [ -d "$destination_dep" ]; then
	echo "Building the project using Ninja..."
	"$CMAKE" -B"build" -G Ninja
	echo "Finished building the project."
	cd build
	ninja
else
	echo "CMake v$cmake_ver does not exist, please run ./configure"
fi


