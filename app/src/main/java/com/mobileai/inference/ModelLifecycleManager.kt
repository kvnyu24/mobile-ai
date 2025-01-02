package com.mobileai.inference

import android.util.Log
import com.mobileai.hardware.HardwareLifecycleManager
import com.mobileai.security.SecurityManager
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.collect
import kotlinx.coroutines.launch
import java.util.concurrent.ConcurrentHashMap
import java.util.concurrent.atomic.AtomicBoolean

class ModelLifecycleManager(
    private val hardwareManager: HardwareLifecycleManager,
    private val securityManager: SecurityManager
) {
    companion object {
        private const val TAG = "ModelLifecycleManager"
        private const val MAX_CONCURRENT_MODELS = 3
        private const val PERFORMANCE_CHECK_INTERVAL = 1000L // 1 second
    }

    private val scope = CoroutineScope(Job() + Dispatchers.Default)
    private val activeModels = ConcurrentHashMap<String, ModelState>()
    private val isMonitoring = AtomicBoolean(false)

    private val _modelStates = MutableStateFlow<Map<String, ModelState>>(emptyMap())
    val modelStates: StateFlow<Map<String, ModelState>> = _modelStates

    data class ModelState(
        val modelId: String,
        val status: Status,
        val lastUsedTimestamp: Long = System.currentTimeMillis(),
        val performanceMetrics: PerformanceMetrics = PerformanceMetrics(),
        val error: String? = null
    )

    data class PerformanceMetrics(
        val averageInferenceTime: Float = 0f,
        val memoryUsage: Long = 0L,
        val powerUsage: Float = 0f,
        val totalInferences: Long = 0L
    )

    enum class Status {
        LOADING,
        READY,
        RUNNING,
        PAUSED,
        ERROR,
        UNLOADED
    }

    init {
        observeHardwareState()
    }

    private fun observeHardwareState() {
        scope.launch {
            hardwareManager.hardwareState.collect { hwState ->
                if (hwState.shouldThrottlePerformance()) {
                    handlePerformanceThrottling()
                }
            }
        }
    }

    fun loadModel(modelId: String, modelData: ByteArray): Boolean {
        if (activeModels.size >= MAX_CONCURRENT_MODELS) {
            unloadLeastRecentlyUsedModel()
        }

        return try {
            updateModelState(modelId, Status.LOADING)
            
            // Encrypt model data before storing
            val (encryptedData, iv) = securityManager.encryptModel(modelData) 
                ?: throw SecurityException("Failed to encrypt model data")
            
            // Store model metadata
            val metadata = mapOf(
                "model_id" to modelId,
                "timestamp" to System.currentTimeMillis().toString(),
                "size" to modelData.size.toString()
            )
            securityManager.securelyStoreModelMetadata(metadata)

            // Initialize model in native layer
            val success = initializeModelNative(modelId, encryptedData, iv)
            
            if (success) {
                updateModelState(modelId, Status.READY)
                startModelMonitoring(modelId)
            } else {
                updateModelState(modelId, Status.ERROR, "Failed to initialize model")
            }
            
            success
        } catch (e: Exception) {
            Log.e(TAG, "Error loading model $modelId", e)
            updateModelState(modelId, Status.ERROR, e.message)
            false
        }
    }

    fun unloadModel(modelId: String) {
        try {
            val model = activeModels[modelId] ?: return
            updateModelState(modelId, Status.UNLOADED)
            releaseModelNative(modelId)
            activeModels.remove(modelId)
            _modelStates.value = activeModels.toMap()
        } catch (e: Exception) {
            Log.e(TAG, "Error unloading model $modelId", e)
        }
    }

    fun startInference(modelId: String): Boolean {
        val model = activeModels[modelId] ?: return false
        
        return try {
            if (model.status != Status.READY && model.status != Status.PAUSED) {
                return false
            }

            updateModelState(modelId, Status.RUNNING)
            true
        } catch (e: Exception) {
            Log.e(TAG, "Error starting inference for model $modelId", e)
            updateModelState(modelId, Status.ERROR, e.message)
            false
        }
    }

    fun pauseInference(modelId: String) {
        val model = activeModels[modelId] ?: return
        
        try {
            if (model.status == Status.RUNNING) {
                updateModelState(modelId, Status.PAUSED)
            }
        } catch (e: Exception) {
            Log.e(TAG, "Error pausing inference for model $modelId", e)
            updateModelState(modelId, Status.ERROR, e.message)
        }
    }

    private fun startModelMonitoring(modelId: String) {
        scope.launch {
            while (isMonitoring.get() && activeModels.containsKey(modelId)) {
                updateModelMetrics(modelId)
                kotlinx.coroutines.delay(PERFORMANCE_CHECK_INTERVAL)
            }
        }
    }

    private fun updateModelMetrics(modelId: String) {
        val model = activeModels[modelId] ?: return
        val metrics = getModelMetricsNative(modelId)
        
        if (metrics != null) {
            val updatedState = model.copy(
                performanceMetrics = metrics,
                lastUsedTimestamp = System.currentTimeMillis()
            )
            activeModels[modelId] = updatedState
            _modelStates.value = activeModels.toMap()
        }
    }

    private fun handlePerformanceThrottling() {
        val runningModels = activeModels.values.filter { it.status == Status.RUNNING }
        
        for (model in runningModels) {
            if (shouldThrottleModel(model)) {
                pauseInference(model.modelId)
            }
        }
    }

    private fun shouldThrottleModel(model: ModelState): Boolean {
        val hwUtilization = hardwareManager.getHardwareUtilization()
        val modelMetrics = model.performanceMetrics
        
        return hwUtilization > 0.8f || 
               modelMetrics.powerUsage > 0.7f ||
               modelMetrics.memoryUsage > 512 * 1024 * 1024 // 512MB
    }

    private fun unloadLeastRecentlyUsedModel() {
        activeModels.values
            .minByOrNull { it.lastUsedTimestamp }
            ?.let { unloadModel(it.modelId) }
    }

    private fun updateModelState(modelId: String, status: Status, error: String? = null) {
        val currentState = activeModels[modelId]
        val newState = if (currentState != null) {
            currentState.copy(status = status, error = error)
        } else {
            ModelState(modelId = modelId, status = status, error = error)
        }
        
        activeModels[modelId] = newState
        _modelStates.value = activeModels.toMap()
    }

    // Native method declarations
    private external fun initializeModelNative(modelId: String, encryptedData: ByteArray, iv: ByteArray): Boolean
    private external fun releaseModelNative(modelId: String)
    private external fun getModelMetricsNative(modelId: String): PerformanceMetrics?

    fun cleanup() {
        isMonitoring.set(false)
        activeModels.keys.toList().forEach { unloadModel(it) }
    }
} 