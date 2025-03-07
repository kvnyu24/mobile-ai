/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

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
/// \file
///
/// Memory management for TF Lite.
#ifndef TENSORFLOW_COMPILER_MLIR_LITE_ALLOCATION_H_
#define TENSORFLOW_COMPILER_MLIR_LITE_ALLOCATION_H_

namespace tensorflow {
namespace mlir {
namespace lite {

// This is a placeholder to satisfy dependencies
class Allocation {
 public:
  Allocation() = default;
  virtual ~Allocation() = default;
};

}  // namespace lite
}  // namespace mlir
}  // namespace tensorflow

#endif  // TENSORFLOW_COMPILER_MLIR_LITE_ALLOCATION_H_
