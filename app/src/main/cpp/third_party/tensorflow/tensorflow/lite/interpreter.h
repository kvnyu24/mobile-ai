/* Copyright 2020 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/
#ifndef TENSORFLOW_LITE_INTERPRETER_H_
#define TENSORFLOW_LITE_INTERPRETER_H_

/// For documentation, see
/// tensorflow/lite/core/interpreter.h.

#include <vector>
#include <memory>
#include <string>

// Include our common header with necessary types
#include "tensorflow/lite/common.h"

namespace tflite {

// Forward declarations
class Tensor;
class Node;
class NodeAndRegistration;
class ErrorReporter;

// A minimal implementation of SignatureRunner
class SignatureRunner {
 public:
  SignatureRunner() = default;
  virtual ~SignatureRunner() = default;
};

// A minimal TFLite interpreter implementation to satisfy dependencies
class Interpreter {
 public:
  Interpreter() = default;
  virtual ~Interpreter() = default;

  // Common methods that might be referenced
  size_t tensors_size() const { return 0; }
  Tensor* tensor(int) { return nullptr; }
  int inputs() const { return 0; }
  int input(int) const { return 0; }
  int outputs() const { return 0; }
  int output(int) const { return 0; }
  
  // These are stubs to make compilation work
  bool AllocateTensors() { return true; }
  bool Invoke() { return true; }
  void SetNumThreads(int) {}
  
  SignatureRunner* GetSignatureRunner(const char*) { return nullptr; }
};

}  // namespace tflite

#endif  // TENSORFLOW_LITE_INTERPRETER_H_
