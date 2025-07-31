#!/usr/bin/env bash

BUILD_DIR="build"

if [ ! -d "$BUILD_DIR" ]; then
  echo "Creating build directory: $BUILD_DIR"
  mkdir -p "$BUILD_DIR"
fi

echo "Configuring CMake for Ninja build..."
cmake -S . -B "${BUILD_DIR}" -G "Ninja" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON

if [ $? -ne 0 ]; then
  echo "CMake configuration failed. Aborting."
  exit 1
fi

echo "Building with Ninja"
ninja -C "${BUILD_DIR}"

if [ $? -ne 0 ]; then
  echo "Ninja build failed. Aborting."
  exit 1
fi

echo "Exporting compile_commands.json"

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
  echo "Exporting compile_commands.json failed. Aborting."
  exit 1
fi

cp "${BUILD_DIR}/compile_commands.json" .

echo "Build complete!"
