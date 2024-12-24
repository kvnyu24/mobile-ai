package com.mobileai.utils

import android.content.Context
import android.util.Log
import android.os.Build
import android.app.ActivityManager
import android.os.StatFs
import com.mobileai.exceptions.InitializationException
import com.mobileai.utils.Constants

object SafetyChecks {
    private const val TAG = "SafetyChecks"
    private const val MIN_SDK_VERSION = Build.VERSION_CODES.N
    private const val MIN_MEMORY_MB = 2048L // 2GB RAM minimum
    private const val MIN_STORAGE_MB = 1024L // 1GB storage minimum
    
    /**
     * Verifies that the loaded native library version matches the expected version
     * @throws InitializationException if version verification fails
     */
    @Throws(InitializationException::class)
    fun verifyNativeLibraryVersion() {
        try {
            // Get version from native code
            val nativeVersion = getNativeLibraryVersion()
            val expectedVersion = BuildConfig.NATIVE_LIB_VERSION
            
            if (nativeVersion != expectedVersion) {
                throw InitializationException(
                    "Native library version mismatch. Expected: $expectedVersion, Found: $nativeVersion"
                )
            }
            Log.i(TAG, "Native library version verified: $nativeVersion")
        } catch (e: Exception) {
            Log.e(TAG, "Failed to verify native library version", e)
            throw InitializationException("Native library version verification failed", e)
        }
    }

    /**
     * Checks if the system meets minimum requirements for running the app
     * @param context Application context
     * @throws InitializationException if system requirements are not met
     */
    @Throws(InitializationException::class) 
    fun checkSystemRequirements(context: Context) {
        try {
            // Check Android version
            if (Build.VERSION.SDK_INT < MIN_SDK_VERSION) {
                throw InitializationException(
                    "Android version ${Build.VERSION.SDK_INT} not supported. Minimum required: $MIN_SDK_VERSION"
                )
            }

            // Check available memory
            val activityManager = context.getSystemService(Context.ACTIVITY_SERVICE) as ActivityManager
            val memInfo = ActivityManager.MemoryInfo()
            activityManager.getMemoryInfo(memInfo)
            
            val totalMemoryMB = memInfo.totalMem / (1024 * 1024)
            if (totalMemoryMB < MIN_MEMORY_MB) {
                throw InitializationException(
                    "Insufficient memory. Required: ${MIN_MEMORY_MB}MB, Available: ${totalMemoryMB}MB"
                )
            }

            // Check storage space
            val stat = StatFs(context.filesDir.path)
            val availableBytes = stat.availableBytes
            val availableMB = availableBytes / (1024 * 1024)
            
            if (availableMB < MIN_STORAGE_MB) {
                throw InitializationException(
                    "Insufficient storage space. Required: ${MIN_STORAGE_MB}MB, Available: ${availableMB}MB"
                )
            }

            // Check for hardware acceleration support
            if (!context.packageManager.hasSystemFeature("android.hardware.opengles.version")) {
                throw InitializationException("Device does not support required graphics capabilities")
            }

            Log.i(TAG, "System requirements verified successfully")
            
        } catch (e: Exception) {
            Log.e(TAG, "System requirements check failed", e)
            throw InitializationException("Failed to verify system requirements", e)
        }
    }

    private external fun getNativeLibraryVersion(): String
}