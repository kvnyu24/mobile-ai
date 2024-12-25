#!/bin/bash

# Exit on error
set -e

# Check for Android NDK
if [[ "$(uname)" == "Darwin" ]]; then
    # On macOS, prefer Homebrew installation
    if [ -d "/opt/homebrew/share/android-ndk" ]; then
        export ANDROID_NDK="/opt/homebrew/share/android-ndk"
    elif [ -d "/opt/homebrew/share/android-commandlinetools/ndk" ]; then
        export ANDROID_NDK="/opt/homebrew/share/android-commandlinetools/ndk/23.1.7779620"
    elif [ -d "$ANDROID_HOME/ndk-bundle" ]; then
        export ANDROID_NDK="$ANDROID_HOME/ndk-bundle"
    elif [ -d "$ANDROID_HOME/ndk" ]; then
        export ANDROID_NDK="$ANDROID_HOME/ndk/$(ls -v "$ANDROID_HOME/ndk" | tail -n1)"
    fi
else
    # On other platforms, check standard locations
    if [ -z "$ANDROID_NDK" ]; then
        if [ -d "$ANDROID_HOME/ndk-bundle" ]; then
            export ANDROID_NDK="$ANDROID_HOME/ndk-bundle"
        elif [ -d "$ANDROID_HOME/ndk" ]; then
            export ANDROID_NDK="$ANDROID_HOME/ndk/$(ls -v "$ANDROID_HOME/ndk" | tail -n1)"
        fi
    fi
fi

if [ -z "$ANDROID_NDK" ] || [ ! -f "$ANDROID_NDK/build/cmake/android.toolchain.cmake" ]; then
    echo "Error: Could not find a valid Android NDK installation with CMake toolchain."
    echo "Please install Android NDK via Android Studio or Homebrew."
    exit 1
fi

echo "Using Android NDK at: $ANDROID_NDK"

# Set up directories
THIRD_PARTY_DIR="app/src/main/cpp/third_party"
mkdir -p "$THIRD_PARTY_DIR"

# Get number of CPU cores for parallel build
if [[ "$(uname)" == "Darwin" ]]; then
    NUM_CORES=$(sysctl -n hw.ncpu)
else
    NUM_CORES=$(nproc)
fi

# Download and build JsonCpp
echo "Setting up JsonCpp..."
mkdir -p "${THIRD_PARTY_DIR}/jsoncpp/lib/arm64-v8a"
mkdir -p "${THIRD_PARTY_DIR}/jsoncpp/lib/armeabi-v7a"
mkdir -p "${THIRD_PARTY_DIR}/jsoncpp/include"

rm -rf /tmp/jsoncpp
mkdir -p /tmp/jsoncpp
cd /tmp/jsoncpp
curl -L -o jsoncpp.tar.gz https://github.com/open-source-parsers/jsoncpp/archive/refs/tags/1.9.5.tar.gz
tar xzf jsoncpp.tar.gz --strip-components=1
mkdir -p build && cd build

# Build for arm64-v8a
mkdir -p arm64-v8a && cd arm64-v8a
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=ON \
      -DJSONCPP_WITH_TESTS=OFF \
      -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
      -DCMAKE_SYSTEM_NAME=Android \
      -DCMAKE_ANDROID_ARCH_ABI=arm64-v8a \
      -DCMAKE_ANDROID_NDK="$ANDROID_NDK" \
      -DCMAKE_ANDROID_STL=c++_shared \
      -DCMAKE_ANDROID_PLATFORM=android-26 \
      -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
      ../..
make -j${NUM_CORES}
cd ..

# Build for armeabi-v7a
mkdir -p armeabi-v7a && cd armeabi-v7a
cmake -DCMAKE_BUILD_TYPE=Release \
      -DBUILD_SHARED_LIBS=ON \
      -DJSONCPP_WITH_TESTS=OFF \
      -DJSONCPP_WITH_POST_BUILD_UNITTEST=OFF \
      -DCMAKE_SYSTEM_NAME=Android \
      -DCMAKE_ANDROID_ARCH_ABI=armeabi-v7a \
      -DCMAKE_ANDROID_NDK="$ANDROID_NDK" \
      -DCMAKE_ANDROID_STL=c++_shared \
      -DCMAKE_ANDROID_PLATFORM=android-26 \
      -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
      ../..
make -j${NUM_CORES}

# Copy JsonCpp files
cp -r ../../include/* "${THIRD_PARTY_DIR}/jsoncpp/include/"
cp arm64-v8a/lib/libjsoncpp.so "${THIRD_PARTY_DIR}/jsoncpp/lib/arm64-v8a/"
cp armeabi-v7a/lib/libjsoncpp.so "${THIRD_PARTY_DIR}/jsoncpp/lib/armeabi-v7a/"

cd "$OLDPWD"
rm -rf /tmp/jsoncpp

# Download ONNX Runtime
echo "Setting up ONNX Runtime..."
mkdir -p "${THIRD_PARTY_DIR}/onnxruntime/lib/arm64-v8a"
mkdir -p "${THIRD_PARTY_DIR}/onnxruntime/lib/armeabi-v7a"
mkdir -p "${THIRD_PARTY_DIR}/onnxruntime/include"

# Download and extract ONNX Runtime
echo "Downloading ONNX Runtime..."
curl -L -o onnxruntime-android.aar "https://repo1.maven.org/maven2/com/microsoft/onnxruntime/onnxruntime-android/1.15.1/onnxruntime-android-1.15.1.aar"

echo "Extracting ONNX Runtime..."
unzip -o onnxruntime-android.aar -d /tmp/onnx

echo "Copying library files..."
cp /tmp/onnx/jni/arm64-v8a/libonnxruntime.so "${THIRD_PARTY_DIR}/onnxruntime/lib/arm64-v8a/"
cp /tmp/onnx/jni/armeabi-v7a/libonnxruntime.so "${THIRD_PARTY_DIR}/onnxruntime/lib/armeabi-v7a/"

echo "Copying header files..."
cp /tmp/onnx/headers/* "${THIRD_PARTY_DIR}/onnxruntime/include/"

# Clean up
rm -rf /tmp/onnx onnxruntime-android.aar

echo "Dependencies setup completed successfully!"