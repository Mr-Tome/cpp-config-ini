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
    rm -f run
    rm -f run_ex_*
	rm -r build/
	echo "Finished Cleaning ./make files."
else
	bash scripts/make_cpp.sh
	touch run
	chmod +x run
	echo "#!/bin/bash" > run
	echo "./build/configs \"\$@\"" >> run
	
	for cpp_file in examples/**/*.cpp; do
		if [ -f "$cpp_file" ]; then
			name=$(basename "$cpp_file" .cpp)
			{
				echo "#!/bin/bash"
				echo "./build/${name} \"\$@\""
			} > "run_ex_${name}"
			chmod +x "run_ex_${name}"
		fi
	done
	
	
	echo "Finished Making CPP project..."
	echo ""
	echo "1) Now you may execute ./run"
	echo "2) execute one of the folowing examples:"
	echo "3) To remove these dependencies, run ./make clean"
fi
