package com.mobileai.examples.optimization

import com.mobileai.inference.ModelManager
import com.mobileai.optimization.OptimizationType
import com.mobileai.optimization.QuantizationMode
import com.mobileai.optimization.ModelOptimizer

class ModelOptimizationExample {
    fun optimizeModel() {
        try {
            // Create model optimizer
            val optimizer = ModelOptimizer()

            // Configure optimization
            val config = OptimizationConfig().apply {
                type = OptimizationType.QUANTIZATION
                quantConfig = QuantizationConfig().apply {
                    mode = QuantizationMode.INT8
                    calibrate = true
                    numCalibrationSamples = 100
                    scaleFactor = 1.0f
                    preserveActivation = true
                    
                    // Configure layer-specific quantization
                    layerConfig = mapOf(
                        "conv1" to true,
                        "conv2" to true,
                        "fc1" to false
                    )
                }
            }

            // Initialize optimizer
            if (!optimizer.initialize(config)) {
                throw RuntimeException("Failed to initialize optimizer")
            }

            // Get model path from assets
            val modelPath = context.getExternalFilesDir(null)?.absolutePath + "/model.tflite"

            // Apply optimization
            if (!optimizer.optimizeModel(modelPath)) {
                throw RuntimeException("Failed to optimize model")
            }

            // Get optimization results
            val compressionRatio = optimizer.compressionRatio
            val memoryUsage = optimizer.memoryUsage
            val accuracyImpact = optimizer.accuracyDelta

            println("""
                Optimization completed successfully!
                Compression ratio: $compressionRatio
                Memory usage (MB): $memoryUsage
                Accuracy impact (%): $accuracyImpact
            """.trimIndent())

        } catch (e: Exception) {
            e.printStackTrace()
        }
    }

    companion object {
        init {
            System.loadLibrary("mobileai")
        }
    }
} 