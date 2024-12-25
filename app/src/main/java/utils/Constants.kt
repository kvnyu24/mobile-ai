package com.mobileai.utils

object Constants {
    // Native library names
    const val CORE_NATIVE_LIB = "mobileai"
    const val INFERENCE_NATIVE_LIB = "mobileai-native"

    // Log tags
    const val MAIN_ACTIVITY_TAG = "MainActivity"
    const val MODEL_MANAGER_TAG = "ModelManager"
    const val INFERENCE_ENGINE_TAG = "InferenceEngine"
    const val MODEL_LOADER_TAG = "ModelLoader"

    // Default configurations
    const val DEFAULT_THREAD_COUNT = 4
    const val DEFAULT_NUM_DELEGATES = 2
    const val DEFAULT_BATCH_SIZE = 1
    const val DEFAULT_TIMEOUT_MS = 30000L // 30 seconds

    // Required model paths
    val REQUIRED_MODELS = listOf(
        "models/model1.tflite",
        "models/model2.pt", 
        "models/model3.onnx"
    )

    // Model file extensions
    const val TFLITE_EXTENSION = ".tflite"
    const val PYTORCH_EXTENSION = ".pt"
    const val ONNX_EXTENSION = ".onnx"

    // Error messages
    const val MODEL_LOAD_ERROR = "Failed to load model"
    const val INFERENCE_ERROR = "Inference failed"
    const val INVALID_INPUT = "Invalid input provided"
}
