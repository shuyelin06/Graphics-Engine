#!/bin/bash

if [ $# -ne 1 ];
then
	echo "usage: $0 <directory name>"
	echo "Export the project into an executable package"
	exit 1
fi

SRC="Graphics Engine"
DEST="$1"

if [ "$SRC" = "$DEST" ];
then
	echo "Cannot be the same as the code directory"
	exit 1
fi
	
# Create my folders
mkdir "$DEST"

# Copy the executable
cp "$SRC/bin/x64/Debug/Graphics Engine.exe" "$DEST/"

# Copy the data/ and shaders/
mkdir "$DEST/bin"
cp -r "$SRC/data/" "$DEST/"
cp -r "$SRC/shaders/" "$DEST/"

# Copy the .dlls
cp "$SRC/zlib.dll" "$DEST/"

exit 0
