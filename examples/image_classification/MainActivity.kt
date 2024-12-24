package com.mobileai.examples.imageclassification

import android.graphics.Bitmap
import android.os.Bundle
import android.util.Log
import androidx.appcompat.app.AppCompatActivity
import com.mobileai.core.HardwareManager
import com.mobileai.inference.ModelManager
import com.mobileai.inference.ModelFormat
import com.mobileai.security.SecurityManager
import android.graphics.Matrix
import java.nio.ByteBuffer
import java.nio.ByteOrder

class ImageClassificationActivity : AppCompatActivity() {
    companion object {
        private const val TAG = "ImageClassification"
        private const val MODEL_PATH = "models/mobilenet_v2_quantized.tflite"
        private const val NUM_THREADS = 4
        private const val INPUT_SIZE = 224 // Standard MobileNet input size
        private const val PIXEL_SIZE = 3 // RGB channels
        private const val BATCH_SIZE = 1
        private val LABELS = arrayOf(
            "airplane", "automobile", "bird", "cat", "deer",
            "dog", "frog", "horse", "ship", "truck"
        ) // Example labels, replace with actual model labels
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
        // Scale the bitmap to required input size
        val scaledBitmap = Bitmap.createScaledBitmap(bitmap, INPUT_SIZE, INPUT_SIZE, true)
        
        // Allocate ByteBuffer for input data
        val inputBuffer = ByteBuffer.allocateDirect(BATCH_SIZE * INPUT_SIZE * INPUT_SIZE * PIXEL_SIZE * 4)
        inputBuffer.order(ByteOrder.nativeOrder())
        
        // Convert bitmap to float values normalized between -1 and 1
        val pixels = IntArray(INPUT_SIZE * INPUT_SIZE)
        scaledBitmap.getPixels(pixels, 0, INPUT_SIZE, 0, 0, INPUT_SIZE, INPUT_SIZE)
        
        val floatArray = FloatArray(BATCH_SIZE * INPUT_SIZE * INPUT_SIZE * PIXEL_SIZE)
        var pixelIndex = 0
        for (i in 0 until INPUT_SIZE) {
            for (j in 0 until INPUT_SIZE) {
                val pixel = pixels[i * INPUT_SIZE + j]
                // Extract RGB values and normalize to [-1, 1]
                floatArray[pixelIndex++] = ((pixel shr 16 and 0xFF) - 128) / 128.0f
                floatArray[pixelIndex++] = ((pixel shr 8 and 0xFF) - 128) / 128.0f
                floatArray[pixelIndex++] = ((pixel and 0xFF) - 128) / 128.0f
            }
        }
        
        return floatArray
    }

    private fun postprocessResults(output: FloatArray): List<Pair<String, Float>> {
        // Convert output array to list of label-confidence pairs
        val results = mutableListOf<Pair<String, Float>>()
        
        // Apply softmax to get probabilities
        val sum = output.map { Math.exp(it.toDouble()) }.sum()
        val probabilities = output.map { (Math.exp(it.toDouble()) / sum).toFloat() }
        
        // Create pairs of labels and probabilities
        for (i in LABELS.indices) {
            if (i < probabilities.size) {
                results.add(Pair(LABELS[i], probabilities[i]))
            }
        }
        
        // Sort by confidence in descending order
        return results.sortedByDescending { it.second }
    }

    override fun onDestroy() {
        super.onDestroy()
        modelManager.release()
        hardwareManager.release()
    }
}