package com.mobileai.inference

import com.mobileai.core.HardwareManager

class ModelManager(private val hardwareManager: HardwareManager) {
    private var nativeHandle: Long = 0

    enum class ModelFormat {
        TFLITE,
        PYTORCH,
        ONNX
    }

    external fun loadModel(modelPath: String, format: ModelFormat): Boolean
    external fun runInference(input: FloatArray): FloatArray
    external fun getModelInfo(): String
    external fun setNumThreads(numThreads: Int)
    external fun enableHardwareAcceleration(enable: Boolean)
    external fun release()

    companion object {
        const val DEFAULT_THREAD_COUNT = 4
    }
} 