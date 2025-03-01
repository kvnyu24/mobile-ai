/*
 * Copyright 2023 The TensorFlow Authors. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package org.tensorflow.lite.gpu;

/**
 * Stub implementation of the GpuDelegateFactory class.
 * This is a placeholder class to satisfy build dependencies when TensorFlow Lite is disabled.
 */
public class GpuDelegateFactory {

  /**
   * Stub implementation of the Options class.
   * This is a placeholder class to satisfy build dependencies when TensorFlow Lite is disabled.
   */
  public static class Options {
    
    /**
     * Enumeration of available GPU backends.
     */
    public enum GpuBackend {
      UNSET,
      OPENCL,
      OPENGL,
      VULKAN;
    }
    
    private GpuBackend gpuBackend = GpuBackend.UNSET;
    
    /** Sets the GPU backend to use. */
    public Options setGpuBackend(GpuBackend backend) {
      this.gpuBackend = backend;
      return this;
    }
    
    /** Gets the currently set GPU backend. */
    public GpuBackend getGpuBackend() {
      return gpuBackend;
    }
  }
} 