package com.mobileai

import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.mobileai.core.HardwareManager
import com.mobileai.inference.ModelManager
import com.mobileai.utils.SafetyChecks
import com.mobileai.utils.Constants
import com.mobileai.exceptions.InitializationException

class MainActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "MainActivity"
        
        init {
            try {
                System.loadLibrary("mobileai")
                SafetyChecks.verifyNativeLibraryVersion()
                Log.i(TAG, "Successfully loaded native library")
            } catch (e: UnsatisfiedLinkError) {
                Log.e(TAG, "Failed to load native library", e)
                throw InitializationException("Native library loading failed", e)
            } catch (e: Exception) {
                Log.e(TAG, "Failed to verify native library version", e)
                throw InitializationException("Native library verification failed", e)
            }
        }
    }

    private var hardwareManager: HardwareManager? = null
    private var modelManager: ModelManager? = null
    private var isInitialized = false
    private val initLock = Object()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        synchronized(initLock) {
            if (!isInitialized) {
                initializeManagers()
            }
        }
    }

    private fun initializeManagers() {
        try {
            SafetyChecks.checkSystemRequirements(this)
            
            // Create hardware manager first
            hardwareManager = HardwareManager().also { manager ->
                if (!manager.initialize()) {
                    throw InitializationException("Hardware acceleration initialization failed")
                }
                
                val accelerators = manager.getAvailableAccelerators()
                if (accelerators.isEmpty()) {
                    throw InitializationException("No hardware accelerators available")
                }
                
                Log.i(TAG, "Hardware acceleration initialized successfully")
                Log.i(TAG, "Available accelerators: $accelerators")
            }

            // Create model manager only if hardware manager is properly initialized
            hardwareManager?.let { hw ->
                modelManager = ModelManager(hw).also { model ->
                    if (!model.isCompatible()) {
                        throw InitializationException("Model manager compatibility check failed")
                    }
                    if (!model.loadModels(Constants.REQUIRED_MODELS)) {
                        throw InitializationException("Failed to load required models")
                    }
                    Log.i(TAG, "Model manager initialized successfully")
                }
            } ?: throw InitializationException("Hardware manager was not properly initialized")

            isInitialized = true

        } catch (e: InitializationException) {
            Log.e(TAG, "Error initializing managers: ${e.message}", e)
            cleanup()
            throw e
        } catch (e: Exception) {
            Log.e(TAG, "Unexpected error during initialization", e)
            cleanup()
            throw InitializationException("Failed to initialize essential components", e)
        }
    }

    private fun cleanup() {
        synchronized(initLock) {
            try {
                modelManager?.apply {
                    if (isActive()) {
                        stopAllInferences()
                    }
                    release()
                }
                hardwareManager?.apply {
                    if (isActive()) {
                        release()
                    }
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error during cleanup", e)
            } finally {
                modelManager = null
                hardwareManager = null
                isInitialized = false
            }
        }
    }

    override fun onDestroy() {
        cleanup()
        super.onDestroy()
    }
}