#!/bin/bash

cmake_ver=3.29.3
op_sys=windows-x86_64

#let's check the final destination before we actually do anything
destination="$HOME/.configuration-dependencies/differential-equations"
destination_dep="$destination/cmake-$cmake_ver-$op_sys"

if [ -d "$destination_dep" ]; then
	echo "CMake $cmake_ver-$op_sys already exists."
	echo "Testing the download."
	"$destination_dep"/bin/cmake.exe --version
else
	echo "Downloading CMake v$cmake_ver"
	# Define the URL of the CMake installer
	url="https://github.com/Kitware/CMake/releases/download/v$cmake_ver/cmake-$cmake_ver-$op_sys.zip"

	# Define the temporary file to store the downloaded installer
	zip_file=$(mktemp)
	
	# Download the CMake installer
	curl -L -o $zip_file $url

	# Check if the destination directory exists; if not, create it
	if [ -d "$destination" ]; then
		echo "$destination exists"
	else
		echo "making $destination"
		mkdir -p "$destination"
	fi
	echo "Extracting the ZIP archive to the aforementioned destination directory."
	unzip -q $zip_file -d "$destination"
	echo "CMake ZIP archive extracted to $destination."

	# Clean up the ZIP file
	echo "Cleaning up the downloaded ZIP file" 
	rm $zip_file
	
	#Testing the download
	echo "Testing the download."
	"$destination_dep"/bin/cmake.exe --version
fi


