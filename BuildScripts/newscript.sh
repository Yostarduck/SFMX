#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: newscript.sh <lowerCamelCaseName>"
    exit 1
fi

FILE_NAME="$1"

# Uppercase the first character
FIRST_CHAR="${FILE_NAME:0:1}"
REST="${FILE_NAME:1}"
CLASS_NAME="$(echo "$FIRST_CHAR" | tr '[:lower:]' '[:upper:]')$REST"

# Path relative to this script
OUTPUT_DIR="$(dirname "$0")/../Game/resources"

# Create the Lua file
cat > "$OUTPUT_DIR/$FILE_NAME.lua" << EOF
-- File: $FILE_NAME.lua
-- Description:

local $CLASS_NAME = {}

function $CLASS_NAME.onCreated(self)
end

function $CLASS_NAME.onStart(self)
end

function $CLASS_NAME.onUpdate(self, deltaTime)
end

function $CLASS_NAME.onDestroyed(self)
end

return $CLASS_NAME
EOF

echo "Created $FILE_NAME.lua"