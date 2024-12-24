package com.mobileai.core

import android.util.Log
import java.io.IOException

class HardwareManager {
    private var nativeHandle: Long = 0
    private var isInitialized: Boolean = false

    @Throws(IOException::class)
    fun initialize(): Boolean {
        if (isInitialized) {
            return true
        }
        
        try {
            if (!initializeNative()) {
                throw IOException("Failed to initialize native hardware manager")
            }
            isInitialized = true
            return true
        } catch (e: Exception) {
            Log.e(TAG, "Error initializing hardware manager", e)
            throw IOException("Failed to initialize hardware manager", e)
        }
    }

    fun getAvailableAccelerators(): List<String> {
        checkInitialized()
        return getAvailableAcceleratorsNative()
    }

    fun setPreferredAccelerator(acceleratorType: String): Boolean {
        checkInitialized()
        if (!SUPPORTED_ACCELERATORS.contains(acceleratorType)) {
            Log.w(TAG, "Unsupported accelerator type: $acceleratorType")
            return false
        }
        return setPreferredAcceleratorNative(acceleratorType)
    }

    fun release() {
        if (isInitialized) {
            releaseNative()
            isInitialized = false
            nativeHandle = 0
        }
    }

    private fun checkInitialized() {
        if (!isInitialized) {
            throw IllegalStateException("HardwareManager is not initialized")
        }
    }

    private external fun initializeNative(): Boolean
    private external fun getAvailableAcceleratorsNative(): List<String>
    private external fun setPreferredAcceleratorNative(acceleratorType: String): Boolean
    private external fun releaseNative()

    companion object {
        private const val TAG = "HardwareManager"
        const val ACCELERATOR_MTK = "MediaTek APU"
        const val ACCELERATOR_QUALCOMM = "Qualcomm DSP"
        const val ACCELERATOR_CPU = "CPU"
        const val ACCELERATOR_GPU = "GPU"

        private val SUPPORTED_ACCELERATORS = setOf(
            ACCELERATOR_MTK,
            ACCELERATOR_QUALCOMM, 
            ACCELERATOR_CPU,
            ACCELERATOR_GPU
        )

        init {
            System.loadLibrary("mobileai-core")
        }
    }
}