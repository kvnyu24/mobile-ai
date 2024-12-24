package com.mobileai.core

import android.util.Log
import java.io.IOException

class HardwareManager {
    private var nativeHandle: Long = 0
    private var isInitialized: Boolean = false
    private var currentAccelerator: String? = null

    @Throws(IOException::class)
    fun initialize(): Boolean {
        synchronized(this) {
            if (isInitialized) {
                return true
            }
            
            try {
                if (!initializeNative()) {
                    throw IOException("Failed to initialize native hardware manager")
                }
                
                // Verify we have at least one accelerator available
                val accelerators = getAvailableAcceleratorsNative()
                if (accelerators.isEmpty()) {
                    throw IOException("No hardware accelerators available")
                }

                // Set CPU as default accelerator if no other preference
                if (currentAccelerator == null) {
                    setPreferredAccelerator(ACCELERATOR_CPU)
                }
                
                isInitialized = true
                return true
            } catch (e: Exception) {
                Log.e(TAG, "Error initializing hardware manager", e)
                cleanup()
                throw IOException("Failed to initialize hardware manager", e)
            }
        }
    }

    fun getAvailableAccelerators(): List<String> {
        checkInitialized()
        return try {
            getAvailableAcceleratorsNative()
        } catch (e: Exception) {
            Log.e(TAG, "Error getting available accelerators", e)
            emptyList()
        }
    }

    fun setPreferredAccelerator(acceleratorType: String): Boolean {
        checkInitialized()
        if (!SUPPORTED_ACCELERATORS.contains(acceleratorType)) {
            Log.w(TAG, "Unsupported accelerator type: $acceleratorType")
            return false
        }

        return try {
            if (setPreferredAcceleratorNative(acceleratorType)) {
                currentAccelerator = acceleratorType
                true
            } else {
                Log.w(TAG, "Failed to set accelerator: $acceleratorType")
                false
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error setting preferred accelerator", e)
            false
        }
    }

    fun getCurrentAccelerator(): String {
        checkInitialized()
        return currentAccelerator ?: ACCELERATOR_CPU
    }

    fun release() {
        synchronized(this) {
            cleanup()
        }
    }

    private fun cleanup() {
        if (isInitialized) {
            try {
                releaseNative()
            } catch (e: Exception) {
                Log.e(TAG, "Error releasing native resources", e)
            } finally {
                isInitialized = false
                nativeHandle = 0
                currentAccelerator = null
            }
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
            try {
                System.loadLibrary("mobileai-core")
                Log.i(TAG, "Successfully loaded native library")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library", e)
                throw RuntimeException("Native library loading failed", e)
            }
        }
    }
}