#!/bin/bash
# xsh.compile-all.+x - Compile all C files in plugins/ to +x/ executable directory

echo "Compiling TPM Orchestration System Modules..."

# Create +x/ directory if it doesn't exist
mkdir -p +x

echo "Compiling plugins from plugins/core directory..."
# Loop through all .c files in the plugins/core directory
for file in plugins/core/*.c; do
    # Check if any .c files exist
    if [ ! -f "$file" ]; then
        echo "No .c files found in plugins/core directory."
        break
    fi

    # Extract the filename without the .c extension and path
    basename=$(basename "$file" .c)

    # Remove any trailing .+x from basename to avoid double extensions like .+x.+x
    clean_basename=${basename%.+x}
    
    # Append .+x to the executable name and put it in +x/ directory
    executable_name="+x/${clean_basename}.+x"

    # Compile the .c file into an executable in the +x/ directory with .+x suffix
    gcc "$file" -o "$executable_name" -lm -lssl -lcrypto

    # Check the exit status of gcc
    if [ $? -eq 0 ]; then
        echo "Successfully compiled $file into $executable_name"
    else
        echo "Error compiling $file"
    fi
done

echo "Compiling plugins from plugins/ui directory..."
# Loop through all .c files in the plugins/ui directory
for file in plugins/ui/*.c; do
    # Check if any .c files exist
    if [ ! -f "$file" ]; then
        echo "No .c files found in plugins/ui directory."
        break
    fi

    # Extract the filename without the .c extension and path
    basename=$(basename "$file" .c)

    # Remove any trailing .+x from basename to avoid double extensions like .+x.+x
    clean_basename=${basename%.+x}
    
    # Append .+x to the executable name and put it in +x/ directory
    executable_name="+x/${clean_basename}.+x"

    # Compile the .c file into an executable in the +x/ directory with .+x suffix
    gcc "$file" -o "$executable_name" -lm -lssl -lcrypto

    # Check the exit status of gcc
    if [ $? -eq 0 ]; then
        echo "Successfully compiled $file into $executable_name"
    else
        echo "Error compiling $file"
    fi
done

echo "Compiling main orchestrator..."
# Compile the main orchestrator
gcc orchestrator.c -o orchestrator.exe -lm -lssl -lcrypto
if [ $? -eq 0 ]; then
    echo "Successfully compiled orchestrator.c into orchestrator.exe"
else
    echo "Error compiling orchestrator.c"
fi

echo "Compilation complete!"

# Run basic functionality test
echo "Testing module executables..."
for exe in +x/*.+x; do
    if [ -f "$exe" ] && [ -x "$exe" ]; then
        echo "Module $(basename "$exe") is executable"
    fi
done