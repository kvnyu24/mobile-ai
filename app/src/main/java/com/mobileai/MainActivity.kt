package com.mobileai

import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import androidx.lifecycle.lifecycleScope
import com.mobileai.core.HardwareManager
import com.mobileai.inference.ModelLifecycleManager
import com.mobileai.hardware.HardwareLifecycleManager
import com.mobileai.security.SecurityManager
import com.mobileai.utils.SafetyChecks
import com.mobileai.utils.Constants
import com.mobileai.exceptions.InitializationException
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch

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
    private var hardwareLifecycleManager: HardwareLifecycleManager? = null
    private var modelLifecycleManager: ModelLifecycleManager? = null
    private var securityManager: SecurityManager? = null
    private var isInitialized = false
    private val initLock = Object()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        synchronized(initLock) {
            if (!isInitialized) {
                initializeManagers()
                setupObservers()
            }
        }
    }

    private fun initializeManagers() {
        try {
            SafetyChecks.checkSystemRequirements(this)
            
            // Initialize security manager first
            securityManager = SecurityManager().also { security ->
                Log.i(TAG, "Security manager initialized successfully")
            }

            // Initialize hardware lifecycle manager
            hardwareLifecycleManager = HardwareLifecycleManager(this).also { hwLifecycle ->
                hwLifecycle.startMonitoring()
                Log.i(TAG, "Hardware lifecycle manager initialized successfully")
            }
            
            // Create hardware manager
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

            // Initialize model lifecycle manager
            hardwareLifecycleManager?.let { hwLifecycle ->
                securityManager?.let { security ->
                    modelLifecycleManager = ModelLifecycleManager(hwLifecycle, security).also { modelLifecycle ->
                        Log.i(TAG, "Model lifecycle manager initialized successfully")
                    }
                }
            } ?: throw InitializationException("Required managers were not properly initialized")

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

    private fun setupObservers() {
        lifecycleScope.launch {
            hardwareLifecycleManager?.hardwareState?.collect { hwState ->
                // Update UI or handle hardware state changes
                if (hwState.isLowBattery || hwState.isLowMemory) {
                    Log.w(TAG, "System resources are low: battery=${hwState.batteryLevel}%, " +
                          "memory=${hwState.availableMemory / (1024 * 1024)}MB")
                }
                
                if (hwState.isThermalThrottling) {
                    Log.w(TAG, "System is thermal throttling at ${hwState.cpuTemperature}°C")
                }
            }
        }

        lifecycleScope.launch {
            modelLifecycleManager?.modelStates?.collect { modelStates ->
                // Update UI or handle model state changes
                modelStates.forEach { (modelId, state) ->
                    when (state.status) {
                        ModelLifecycleManager.Status.ERROR -> {
                            Log.e(TAG, "Model $modelId error: ${state.error}")
                        }
                        ModelLifecycleManager.Status.RUNNING -> {
                            val metrics = state.performanceMetrics
                            Log.d(TAG, "Model $modelId metrics: " +
                                  "inference=${metrics.averageInferenceTime}ms, " +
                                  "memory=${metrics.memoryUsage / (1024 * 1024)}MB")
                        }
                        else -> {
                            Log.d(TAG, "Model $modelId state changed to ${state.status}")
                        }
                    }
                }
            }
        }
    }

    private fun cleanup() {
        synchronized(initLock) {
            try {
                modelLifecycleManager?.cleanup()
                hardwareLifecycleManager?.stopMonitoring()
                hardwareManager?.apply {
                    if (isActive()) {
                        release()
                    }
                }
            } catch (e: Exception) {
                Log.e(TAG, "Error during cleanup", e)
            } finally {
                modelLifecycleManager = null
                hardwareLifecycleManager = null
                hardwareManager = null
                securityManager = null
                isInitialized = false
            }
        }
    }

    override fun onDestroy() {
        cleanup()
        super.onDestroy()
    }
}