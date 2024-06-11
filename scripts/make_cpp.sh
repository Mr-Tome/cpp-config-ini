#!/bin/bash

cmake_ver=3.29.3
op_sys=windows-x86_64

#let's check the final destination before we actually do anything
destination="$HOME/.configuration-dependencies/differential-equations"
destination_dep="$destination/cmake-$cmake_ver-$op_sys"

CMAKE="$destination_dep"/bin/cmake.exe
GCC="$destination"/mingw64/bin/gcc.exe

if [ -d "$destination_dep" ]; then
	## Check if the Makefile exists
	#echo "Checking if Makefile exists."
	#if [[ ! -f "Makefile" ]]; then
	#	# If the Makefile does not exist, generate it using CMake
	#	echo "Generating Makefile using CMake..."
	#	#$CMAKE -B"build"
	#	echo "Finished Generating Makefile using CMake."
	#else
	#	echo "Makefile already exists."
	#fi

	# Build the project using make
	echo "Building the project using Ninja..."
	"$CMAKE" -B"build" -G Ninja
	echo "Finished building the project."
	cd build
	ninja
 #$GCC source/main.cpp -o main
else
	echo "CMake v$cmake_ver does not exist, please run ./configure"
fi


