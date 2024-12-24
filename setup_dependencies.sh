#!/bin/bash

# Create third party directory
mkdir -p app/src/main/cpp/third_party

# Download TensorFlow Lite
TFLITE_VERSION="2.14.0"
TFLITE_DIR="app/src/main/cpp/third_party/tensorflow"

echo "Downloading TensorFlow Lite ${TFLITE_VERSION}..."
mkdir -p "${TFLITE_DIR}"

# Download TensorFlow Lite Android AAR
wget -q "https://repo1.maven.org/maven2/org/tensorflow/tensorflow-lite/${TFLITE_VERSION}/tensorflow-lite-${TFLITE_VERSION}.aar" -O tensorflow-lite.aar

# Extract the AAR
unzip -q tensorflow-lite.aar -d tensorflow-lite

# Copy the necessary files
mkdir -p "${TFLITE_DIR}/include"
mkdir -p "${TFLITE_DIR}/lib"

# Copy headers
cp -r tensorflow-lite/headers/tensorflow "${TFLITE_DIR}/include/"

# Copy libraries for each ABI
for abi in armeabi-v7a arm64-v8a x86 x86_64; do
    mkdir -p "${TFLITE_DIR}/lib/${abi}"
    cp "tensorflow-lite/jni/${abi}/libtensorflowlite_jni.so" "${TFLITE_DIR}/lib/${abi}/libtensorflowlite.so"
done

# Cleanup
rm -rf tensorflow-lite.aar tensorflow-lite

echo "TensorFlow Lite setup complete!" 