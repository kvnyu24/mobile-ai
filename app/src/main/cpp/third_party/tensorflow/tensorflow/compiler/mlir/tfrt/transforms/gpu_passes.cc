/* Copyright 2022 The TensorFlow Authors. All Rights Reserved.

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

#include "tensorflow/compiler/mlir/tfrt/transforms/gpu_passes.h"

#include "mlir/IR/DialectRegistry.h"  // from @llvm-project
#include "mlir/IR/MLIRContext.h"  // from @llvm-project
#include "mlir/IR/PatternMatch.h"  // from @llvm-project
#include "mlir/Transforms/DialectConversion.h"  // from @llvm-project
#include "tensorflow/compiler/mlir/tfrt/ir/gpu_ops.h"

namespace tensorflow {

void RegisterGpuDialects(mlir::DialectRegistry *registry) {
  registry->insert<tfrt::gpu::GpuRuntimeDialect>();
}

void AddGpuTargetDialectAndPatterns(mlir::MLIRContext *context,
                                    mlir::ConversionTarget *target,
                                    mlir::RewritePatternSet *patterns) {
  target->addLegalDialect<tfrt::gpu::GpuRuntimeDialect>();
}

}  // namespace tensorflow