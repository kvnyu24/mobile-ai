package com.mobileai.examples.imageclassification

import android.graphics.Bitmap
import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.mobileai.core.HardwareManager
import com.mobileai.inference.ModelManager
import com.mobileai.inference.ModelFormat
import com.mobileai.security.SecurityManager

class ImageClassificationActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "ImageClassification"
        private const val MODEL_PATH = "models/mobilenet_v2_quantized.tflite"
        private const val NUM_THREADS = 4
    }

    private lateinit var hardwareManager: HardwareManager
    private lateinit var modelManager: ModelManager
    private lateinit var securityManager: SecurityManager

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_image_classification)

        initializeManagers()
        setupModel()
    }

    private fun initializeManagers() {
        try {
            hardwareManager = HardwareManager()
            modelManager = ModelManager(hardwareManager)
            securityManager = SecurityManager()

            // Initialize hardware acceleration
            if (hardwareManager.initialize()) {
                Log.i(TAG, "Hardware acceleration initialized successfully")
                val accelerators = hardwareManager.getAvailableAccelerators()
                Log.i(TAG, "Available accelerators: $accelerators")

                // Prefer MTK APU if available
                if (accelerators.contains(HardwareManager.ACCELERATOR_MTK)) {
                    hardwareManager.setPreferredAccelerator(HardwareManager.ACCELERATOR_MTK)
                }
            }

            modelManager.setNumThreads(NUM_THREADS)
            modelManager.enableHardwareAcceleration(true)

        } catch (e: Exception) {
            Log.e(TAG, "Error initializing managers", e)
        }
    }

    private fun setupModel() {
        try {
            // Load and decrypt model
            val encryptedModel = assets.open("$MODEL_PATH.encrypted").readBytes()
            val iv = assets.open("$MODEL_PATH.iv").readBytes()
            val modelData = securityManager.decryptModel(encryptedModel, iv)

            // Load model into memory
            val tempFile = createTempFile("model", ".tflite")
            tempFile.writeBytes(modelData)
            
            if (modelManager.loadModel(tempFile.absolutePath, ModelFormat.TFLITE)) {
                Log.i(TAG, "Model loaded successfully")
                Log.i(TAG, "Model info: ${modelManager.getModelInfo()}")
            } else {
                Log.e(TAG, "Failed to load model")
            }
            
            tempFile.delete()
        } catch (e: Exception) {
            Log.e(TAG, "Error setting up model", e)
        }
    }

    fun classifyImage(bitmap: Bitmap): List<Pair<String, Float>> {
        try {
            // Preprocess image
            val input = preprocessImage(bitmap)
            
            // Run inference
            val output = modelManager.runInference(input)
            
            // Post-process results
            return postprocessResults(output)
        } catch (e: Exception) {
            Log.e(TAG, "Error classifying image", e)
            return emptyList()
        }
    }

    private fun preprocessImage(bitmap: Bitmap): FloatArray {
        // TODO: Implement image preprocessing
        // - Resize to model input size
        // - Normalize pixel values
        // - Convert to float array
        return FloatArray(0)
    }

    private fun postprocessResults(output: FloatArray): List<Pair<String, Float>> {
        // TODO: Implement result post-processing
        // - Convert output to probabilities
        // - Map to class labels
        // - Sort by confidence
        return emptyList()
    }

    override fun onDestroy() {
        super.onDestroy()
        modelManager.release()
        hardwareManager.release()
    }
} 