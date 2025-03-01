#ifndef TENSORFLOW_INCLUDES_H_
#define TENSORFLOW_INCLUDES_H_

// Include our dummy implementations first when TensorFlow Lite is disabled
#include "cmake_dummy.h"

// Common TensorFlow Lite includes
#ifdef DISABLE_TENSORFLOW_LITE
// When TensorFlow Lite is disabled, we just use the stubs
#else
// When TensorFlow Lite is enabled, include the real headers
#include "tensorflow/lite/interpreter.h"
#include "tensorflow/lite/model.h"
#include "tensorflow/lite/kernels/register.h"
#endif

// Both real and dummy implementations should provide these classes:
// - tflite::Interpreter
// - tflite::Model
// - tflite::ErrorReporter

#endif // TENSORFLOW_INCLUDES_H_ 