package com.mobileai.hardware

import android.content.Context
import android.os.BatteryManager
import android.content.Intent
import android.content.IntentFilter
import android.os.PowerManager
import android.util.Log
import kotlinx.coroutines.CoroutineScope
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.Job
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.launch
import java.util.concurrent.atomic.AtomicBoolean
import android.app.ActivityManager
import android.content.BroadcastReceiver

class HardwareLifecycleManager(private val context: Context) {
    companion object {
        private const val TAG = "HardwareLifecycleManager"
        private const val THERMAL_THROTTLING_TEMP = 39.0f // Celsius
        private const val LOW_MEMORY_THRESHOLD = 0.15f // 15% free memory threshold
        private const val LOW_BATTERY_THRESHOLD = 15
    }

    private val scope = CoroutineScope(Job() + Dispatchers.Default)
    private val powerManager = context.getSystemService(Context.POWER_SERVICE) as PowerManager
    private val activityManager = context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
    private val batteryManager = context.getSystemService(Context.BATTERY_SERVICE) as BatteryManager
    
    private val _hardwareState = MutableStateFlow(HardwareState())
    val hardwareState: StateFlow<HardwareState> = _hardwareState
    
    private val isMonitoring = AtomicBoolean(false)
    private var batteryReceiver: BroadcastReceiver? = null

    data class HardwareState(
        val cpuTemperature: Float = 0f,
        val batteryLevel: Int = 100,
        val isCharging: Boolean = false,
        val availableMemory: Long = 0L,
        val totalMemory: Long = 0L,
        val isThermalThrottling: Boolean = false,
        val isLowMemory: Boolean = false,
        val isLowBattery: Boolean = false,
        val isPowerSaveMode: Boolean = false
    )

    fun startMonitoring() {
        if (isMonitoring.getAndSet(true)) {
            Log.w(TAG, "Hardware monitoring is already active")
            return
        }

        registerBatteryReceiver()
        startStateMonitoring()
    }

    fun stopMonitoring() {
        if (!isMonitoring.getAndSet(false)) {
            return
        }

        unregisterBatteryReceiver()
    }

    private fun registerBatteryReceiver() {
        batteryReceiver = object : BroadcastReceiver() {
            override fun onReceive(context: Context, intent: Intent) {
                when (intent.action) {
                    Intent.ACTION_BATTERY_CHANGED -> updateBatteryState(intent)
                    PowerManager.ACTION_POWER_SAVE_MODE_CHANGED -> updatePowerSaveMode()
                }
            }
        }

        val filter = IntentFilter().apply {
            addAction(Intent.ACTION_BATTERY_CHANGED)
            addAction(PowerManager.ACTION_POWER_SAVE_MODE_CHANGED)
        }
        context.registerReceiver(batteryReceiver, filter)
    }

    private fun unregisterBatteryReceiver() {
        batteryReceiver?.let {
            try {
                context.unregisterReceiver(it)
            } catch (e: Exception) {
                Log.e(TAG, "Error unregistering battery receiver", e)
            }
        }
        batteryReceiver = null
    }

    private fun startStateMonitoring() {
        scope.launch {
            while (isMonitoring.get()) {
                updateHardwareState()
                kotlinx.coroutines.delay(5000) // Update every 5 seconds
            }
        }
    }

    private fun updateHardwareState() {
        val memoryInfo = ActivityManager.MemoryInfo()
        activityManager.getMemoryInfo(memoryInfo)

        val currentState = _hardwareState.value
        _hardwareState.value = currentState.copy(
            cpuTemperature = getCpuTemperature(),
            availableMemory = memoryInfo.availMem,
            totalMemory = memoryInfo.totalMem,
            isLowMemory = memoryInfo.availMem.toFloat() / memoryInfo.totalMem < LOW_MEMORY_THRESHOLD,
            isThermalThrottling = currentState.cpuTemperature > THERMAL_THROTTLING_TEMP
        )
    }

    private fun updateBatteryState(intent: Intent) {
        val level = intent.getIntExtra(BatteryManager.EXTRA_LEVEL, -1)
        val scale = intent.getIntExtra(BatteryManager.EXTRA_SCALE, -1)
        val batteryLevel = (level * 100 / scale.toFloat()).toInt()
        
        val status = intent.getIntExtra(BatteryManager.EXTRA_STATUS, -1)
        val isCharging = status == BatteryManager.BATTERY_STATUS_CHARGING || 
                        status == BatteryManager.BATTERY_STATUS_FULL

        val currentState = _hardwareState.value
        _hardwareState.value = currentState.copy(
            batteryLevel = batteryLevel,
            isCharging = isCharging,
            isLowBattery = batteryLevel <= LOW_BATTERY_THRESHOLD && !isCharging
        )
    }

    private fun updatePowerSaveMode() {
        val currentState = _hardwareState.value
        _hardwareState.value = currentState.copy(
            isPowerSaveMode = powerManager.isPowerSaveMode
        )
    }

    private fun getCpuTemperature(): Float {
        return try {
            val thermalFiles = arrayOf(
                "/sys/class/thermal/thermal_zone0/temp",
                "/sys/devices/virtual/thermal/thermal_zone0/temp"
            )
            
            for (file in thermalFiles) {
                try {
                    val temp = java.io.File(file).readText().trim().toInt()
                    // Convert to Celsius (some devices report in milliCelsius)
                    return if (temp > 1000) temp / 1000f else temp.toFloat()
                } catch (e: Exception) {
                    continue
                }
            }
            0f
        } catch (e: Exception) {
            Log.e(TAG, "Error reading CPU temperature", e)
            0f
        }
    }

    fun shouldThrottlePerformance(): Boolean {
        val state = _hardwareState.value
        return state.isThermalThrottling || 
               state.isLowMemory || 
               state.isLowBattery || 
               state.isPowerSaveMode
    }

    fun getHardwareUtilization(): Float {
        val state = _hardwareState.value
        val memoryUtilization = 1 - (state.availableMemory.toFloat() / state.totalMemory)
        val thermalUtilization = (state.cpuTemperature / THERMAL_THROTTLING_TEMP).coerceIn(0f, 1f)
        val batteryUtilization = 1 - (state.batteryLevel / 100f)

        return ((memoryUtilization + thermalUtilization + batteryUtilization) / 3f)
            .coerceIn(0f, 1f)
    }
} 