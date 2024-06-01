#!/bin/bash

echo "Downloading CMake v3.29.3"

# Define the URL of the CMake installer
url="https://github.com/Kitware/CMake/releases/download/v3.29.3/cmake-3.29.3-windows-x86_64.zip"

# Define the temporary file to store the downloaded installer
zip_file=$(mktemp)

# Download the CMake installer
curl -L -o $zip_file $url

destination="$HOME/.configuration-dependencies/differential-equations"

# Check if the destination directory exists; if not, create it
if [[! -d $destination ]]; then
	echo "making $destination"
    mkdir -p $destination
fi

# Extract the ZIP archive to the specified destination directory
unzip -q $zip_file -d $destination

# Clean up the ZIP file
rm $zip_file

echo "CMake ZIP archive extracted to $destination."
