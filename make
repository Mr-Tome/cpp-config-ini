#!/bin/bash


# Initialize a variable to track if 'clean' is found
found_clean=false

# Loop through all arguments
for arg in "$@"; do
    # Check if the argument contains 'clean'
    if [[ $arg == *"clean"* ]]; then
        found_clean=true
        break
    fi
done

if $found_clean; then
    echo "'clean' argument detected. Cleaning ./make files..."
    rm -r run
	rm -r build/
	echo "Finished Cleaning ./make files."
else
	bash scripts/make_cpp.sh
	touch run
	chmod +x run
	echo "#!/bin/bash" > run
	echo "start build/differential-equations.exe" >> run
	
	echo "Finished Making CPP project..."
	echo ""
	echo "1) Now you may execute ./run"
	echo "2) To remove these dependencies, run ./make clean"
fi