#!/bin/bash

ver=1.83.0
op_sys=windows-x86_64

#let's check the final destination before we actually do anything
destination="$HOME/.configuration-dependencies/differential-equations"
destination_dep="$destination/boost_$ver"

if [ -d "$destination_dep" ]; then
	echo "Boost $ver already exists."
else
	echo "Downloading CMake v$ver"
	# Define the URL of the CMake installer
	url="https://archives.boost.io/release/1.83.0/source/boost_1_83_0.zip"

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
	
	
fi


