#!/bin/bash

# Create artifacts directory
mkdir -p artifacts

# Build
make

# Find and copy artifacts based on platform
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
  # Linux
  find . -name "websocketserver*.pd_linux" -exec cp {} artifacts/ \;
elif [[ "$OSTYPE" == "darwin"* ]]; then
  # macOS
  find . -name "websocketserver*.pd_darwin" -exec cp {} artifacts/ \;
elif [[ "$OSTYPE" == "msys"* || "$OSTYPE" == "cygwin"* ]]; then
  # Windows
  find . -name "websocketserver*.dll" -exec cp {} artifacts/ \; || true
  find . -name "websocketserver*.m_*" -exec cp {} artifacts/ \; || true
fi

# Copy supporting files
cp -f websocketserver-help.pd artifacts/ || true
cp -f send_receive.html artifacts/ || true
cp -f LICENSE artifacts/ || true
cp -f README.md artifacts/ || true
cp -f CHANGELOG.txt artifacts/ || true

echo "Build artifacts placed in ./artifacts directory"
