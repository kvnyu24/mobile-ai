package com.mobileai

import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.mobileai.core.HardwareManager
import com.mobileai.inference.ModelManager

class MainActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "MainActivity"
        
        init {
            System.loadLibrary("mobileai")
        }
    }

    private lateinit var hardwareManager: HardwareManager
    private lateinit var modelManager: ModelManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_main)

        initializeManagers()
    }

    private fun initializeManagers() {
        try {
            hardwareManager = HardwareManager()
            modelManager = ModelManager(hardwareManager)

            // Initialize hardware acceleration
            if (hardwareManager.initialize()) {
                Log.i(TAG, "Hardware acceleration initialized successfully")
                Log.i(TAG, "Available accelerators: ${hardwareManager.getAvailableAccelerators()}")
            } else {
                Log.w(TAG, "Hardware acceleration initialization failed")
            }

        } catch (e: Exception) {
            Log.e(TAG, "Error initializing managers", e)
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        hardwareManager.release()
        modelManager.release()
    }
} 