package com.mobileai.inference

import android.util.Log
import com.mobileai.core.HardwareManager
import java.io.File
import java.io.IOException

class ModelManager(private val hardwareManager: HardwareManager) {
    private var nativeHandle: Long = 0
    private var isModelLoaded: Boolean = false
    private var currentModelPath: String? = null
    private var currentFormat: ModelFormat? = null
    private var isInitialized: Boolean = false

    enum class ModelFormat {
        TFLITE,
        PYTORCH,
        ONNX
    }

    @Throws(IOException::class)
    fun initialize(): Boolean {
        synchronized(this) {
            if (isInitialized) {
                return true
            }

            try {
                if (!hardwareManager.initialize()) {
                    throw IOException("Failed to initialize hardware manager")
                }
                isInitialized = true
                setNumThreads(DEFAULT_THREAD_COUNT)
                return true
            } catch (e: Exception) {
                Log.e(TAG, "Error initializing model manager", e)
                cleanup()
                throw IOException("Failed to initialize model manager", e)
            }
        }
    }

    @Throws(IllegalStateException::class, IllegalArgumentException::class)
    fun loadModel(modelPath: String, format: ModelFormat): Boolean {
        checkInitialized()
        
        if (modelPath.isEmpty()) {
            throw IllegalArgumentException("Model path cannot be empty")
        }

        val modelFile = File(modelPath)
        if (!modelFile.exists()) {
            throw IllegalArgumentException("Model file does not exist at path: $modelPath")
        }

        synchronized(this) {
            if (isModelLoaded) {
                release()
            }
            
            try {
                if (nativeLoadModel(modelPath, format)) {
                    isModelLoaded = true
                    currentModelPath = modelPath
                    currentFormat = format
                    return true
                }
                return false
            } catch (e: Exception) {
                Log.e(TAG, "Error loading model", e)
                cleanup()
                throw IllegalStateException("Failed to load model", e)
            }
        }
    }

    @Throws(IllegalStateException::class)
    fun runInference(input: FloatArray): FloatArray {
        checkInitialized()
        checkModelLoaded()
        
        if (input.isEmpty()) {
            throw IllegalArgumentException("Input array cannot be empty")
        }

        try {
            return nativeRunInference(input)
        } catch (e: Exception) {
            Log.e(TAG, "Error running inference", e)
            throw IllegalStateException("Failed to run inference", e)
        }
    }

    fun getModelInfo(): String {
        checkInitialized()
        checkModelLoaded()
        return try {
            nativeGetModelInfo()
        } catch (e: Exception) {
            Log.e(TAG, "Error getting model info", e)
            "Unknown"
        }
    }

    fun setNumThreads(numThreads: Int) {
        checkInitialized()
        if (numThreads <= 0) {
            throw IllegalArgumentException("Number of threads must be positive")
        }
        try {
            nativeSetNumThreads(numThreads)
        } catch (e: Exception) {
            Log.e(TAG, "Error setting thread count", e)
        }
    }

    fun enableHardwareAcceleration(enable: Boolean) {
        checkInitialized()
        try {
            val accelerators = hardwareManager.getAvailableAccelerators()
            if (accelerators.contains(HardwareManager.ACCELERATOR_GPU) || 
                accelerators.contains(HardwareManager.ACCELERATOR_MTK) ||
                accelerators.contains(HardwareManager.ACCELERATOR_QUALCOMM)) {
                nativeEnableHardwareAcceleration(enable)
            } else {
                Log.w(TAG, "No hardware acceleration available")
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error configuring hardware acceleration", e)
        }
    }

    @Synchronized
    fun release() {
        cleanup()
    }

    private fun cleanup() {
        if (isModelLoaded) {
            try {
                nativeRelease()
            } catch (e: Exception) {
                Log.e(TAG, "Error releasing native resources", e)
            } finally {
                isModelLoaded = false
                currentModelPath = null
                currentFormat = null
                nativeHandle = 0
                isInitialized = false
            }
        }
    }

    private fun checkInitialized() {
        if (!isInitialized) {
            throw IllegalStateException("ModelManager is not initialized")
        }
    }

    private fun checkModelLoaded() {
        if (!isModelLoaded) {
            throw IllegalStateException("No model is loaded")
        }
    }

    private external fun nativeLoadModel(modelPath: String, format: ModelFormat): Boolean
    private external fun nativeRunInference(input: FloatArray): FloatArray
    private external fun nativeGetModelInfo(): String
    private external fun nativeSetNumThreads(numThreads: Int)
    private external fun nativeEnableHardwareAcceleration(enable: Boolean)
    private external fun nativeRelease()

    companion object {
        private const val TAG = "ModelManager"
        const val DEFAULT_THREAD_COUNT = 4

        init {
            try {
                System.loadLibrary("mobileai-native")
                Log.i(TAG, "Successfully loaded native library")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library", e)
                throw RuntimeException("Native library loading failed", e)
            }
        }
    }
}