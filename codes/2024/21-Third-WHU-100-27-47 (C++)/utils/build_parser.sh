clear

# Start from the current directory
dir="$(pwd)"

# Traverse up the directory tree until we find the CMakeLists.txt file
while [ ! -f "$dir/CMakeLists.txt" ]; do
    # If we reach the root directory without finding the file, exit with an error
    if [ "$dir" == "/" ]; then
        echo "Error: CMakeLists.txt not found in any ancestor directory."
        exit 1
    fi
    
    # Move up one directory
    dir="$(dirname "$dir")"
done

# Set the current working directory to the ancestor directory with CMakeLists.txt
cd "$dir"

echo Building parser...
echo

cd src/frontend

java -jar /usr/local/lib/antlr-4.12.0-complete.jar Sysy.g4 -Dlanguage=Cpp -listener -visitor -o generated/

exitcode=$?

if [ $exitcode -ne 0 ]; then
    echo
    echo Build parser failed.
else
    echo
    echo Build parser successful.
fi

exit $exitcode
