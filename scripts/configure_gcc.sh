#!/bin/bash

gcc_ver=14.1.0
op_sys=windows-x86_64

#let's check the final destination before we actually do anything
destination="$HOME/.configuration-dependencies/differential-equations"
destination_dep="$destination/mingw64"

if [ -d "$destination_dep" ]; then
	echo "GCC $gcc_ver-$op_sys already exists."
	echo ""
	echo "Testing the download by checking the version: gcc.exe --version, "
	echo ""
	"$destination_dep"/bin/gcc.exe --version
	echo ""
	echo "Finished Testing the download."
else
	echo "Downloading GCC v$gcc_ver"
	# Define the URL of the GCC installer
	url="https://github.com/brechtsanders/winlibs_mingw/releases/download/14.1.0posix-18.1.5-11.0.1-ucrt-r1/winlibs-x86_64-posix-seh-gcc-14.1.0-llvm-18.1.5-mingw-w64ucrt-11.0.1-r1.zip"

	# Define the temporary file to store the downloaded installer
	zip_file=$(mktemp)
	
	# Download the GCC installer
	curl -L -o $zip_file $url

	# Check if the destination directory exists; if not, create it
	if [ -d "$destination" ]; then
		echo "$destination exists"
	else
		echo "making $destination"
		mkdir -p "$destination"
	fi
	echo "Extracting the ZIP archive to the aforementioned destination directory..."
	unzip -q $zip_file -d "$destination"
	echo "GCC ZIP archive extracted to $destination."

	# Clean up the ZIP file
	echo "Cleaning up the downloaded ZIP file" 
	rm $zip_file
	
	#Testing the download
	echo ""
	echo "Testing the download by checking the version: gcc.exe --version, "
	echo ""
	"$destination_dep"/bin/gcc.exe --version
	echo ""
	echo "Finished Testing the download."
fi


