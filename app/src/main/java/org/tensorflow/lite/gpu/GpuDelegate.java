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

import java.io.Closeable;

/**
 * Stub implementation of GpuDelegate to satisfy build dependencies when TensorFlow Lite is disabled.
 * This is a dummy class that doesn't do any real GPU acceleration.
 */
public class GpuDelegate implements Closeable {
  
  /**
   * Creates a new GPU Delegate instance with default options.
   */
  public GpuDelegate() {
    // No-op constructor
  }
  
  /**
   * Creates a new GPU Delegate instance with the specified options.
   *
   * @param options Options for delegate creation.
   */
  public GpuDelegate(GpuDelegateFactory.Options options) {
    // No-op constructor with options
  }
  
  /**
   * Frees TFLite resources allocated for the delegate.
   */
  @Override
  public void close() {
    // No-op implementation
  }
} 