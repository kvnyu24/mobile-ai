package com.mobileai.security

import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import android.util.Log
import java.io.IOException
import java.security.KeyStore
import java.security.KeyStoreException
import java.security.NoSuchAlgorithmException
import java.security.cert.CertificateException
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec
import android.content.Context
import android.content.SharedPreferences

class SecurityManager {
    companion object {
        private const val TAG = "SecurityManager"
        private const val ANDROID_KEYSTORE = "AndroidKeyStore"
        private const val KEY_ALIAS = "MobileAIKey"
        private const val TRANSFORMATION = "AES/GCM/NoPadding"
        private const val AUTH_TAG_LENGTH = 128
        private const val KEY_SIZE = 256
        private const val IV_SIZE = 12 // GCM recommended IV size
        private const val PREFS_NAME = "SecureModelMetadata"
        private const val ENCRYPTED_METADATA_KEY = "encrypted_metadata"
        private const val METADATA_IV_KEY = "metadata_iv"
    }

    private var keyStore: KeyStore? = null
    private var secretKey: SecretKey? = null
    private lateinit var sharedPreferences: SharedPreferences

    init {
        initializeKeyStore()
    }

    private fun initializeKeyStore() {
        try {
            keyStore = KeyStore.getInstance(ANDROID_KEYSTORE).apply {
                load(null)
            }
            if (!keyStore?.containsAlias(KEY_ALIAS)!!) {
                generateKey()
            }
            secretKey = keyStore?.getKey(KEY_ALIAS, null) as? SecretKey
        } catch (e: KeyStoreException) {
            Log.e(TAG, "KeyStore initialization failed", e)
        } catch (e: CertificateException) {
            Log.e(TAG, "Certificate error during KeyStore initialization", e)
        } catch (e: NoSuchAlgorithmException) {
            Log.e(TAG, "Algorithm not found during KeyStore initialization", e)
        } catch (e: IOException) {
            Log.e(TAG, "I/O error during KeyStore initialization", e)
        } catch (e: Exception) {
            Log.e(TAG, "Unexpected error during KeyStore initialization", e)
        }
    }

    private fun generateKey() {
        try {
            val keyGenerator = KeyGenerator.getInstance(
                KeyProperties.KEY_ALGORITHM_AES,
                ANDROID_KEYSTORE
            )

            val keyGenParameterSpec = KeyGenParameterSpec.Builder(
                KEY_ALIAS,
                KeyProperties.PURPOSE_ENCRYPT or KeyProperties.PURPOSE_DECRYPT
            )
                .setBlockModes(KeyProperties.BLOCK_MODE_GCM)
                .setEncryptionPaddings(KeyProperties.ENCRYPTION_PADDING_NONE)
                .setKeySize(KEY_SIZE)
                .setUserAuthenticationRequired(false)
                .setRandomizedEncryptionRequired(true)
                .build()

            keyGenerator.init(keyGenParameterSpec)
            secretKey = keyGenerator.generateKey()
        } catch (e: Exception) {
            Log.e(TAG, "Key generation failed", e)
            throw SecurityException("Failed to generate encryption key", e)
        }
    }

    fun encryptModel(modelData: ByteArray): Pair<ByteArray, ByteArray>? {
        if (modelData.isEmpty()) {
            Log.w(TAG, "Empty model data provided")
            return null
        }

        return try {
            val cipher = Cipher.getInstance(TRANSFORMATION)
            val key = secretKey ?: run {
                initializeKeyStore()
                secretKey
            } ?: throw SecurityException("Secret key not available")
            
            cipher.init(Cipher.ENCRYPT_MODE, key)
            
            if (cipher.iv.size != IV_SIZE) {
                throw SecurityException("Invalid IV size: ${cipher.iv.size}")
            }
            
            val encryptedData = cipher.doFinal(modelData)
            Pair(encryptedData, cipher.iv)
        } catch (e: Exception) {
            Log.e(TAG, "Encryption failed", e)
            null
        }
    }

    fun decryptModel(encryptedData: ByteArray, iv: ByteArray): ByteArray? {
        if (encryptedData.isEmpty() || iv.isEmpty()) {
            Log.w(TAG, "Empty encrypted data or IV provided")
            return null
        }

        if (iv.size != IV_SIZE) {
            Log.e(TAG, "Invalid IV size: ${iv.size}")
            return null
        }

        return try {
            val cipher = Cipher.getInstance(TRANSFORMATION)
            val key = secretKey ?: run {
                initializeKeyStore()
                secretKey
            } ?: throw SecurityException("Secret key not available")
            
            val gcmSpec = GCMParameterSpec(AUTH_TAG_LENGTH, iv)
            cipher.init(Cipher.DECRYPT_MODE, key, gcmSpec)
            cipher.doFinal(encryptedData)
        } catch (e: Exception) {
            Log.e(TAG, "Decryption failed", e)
            null
        }
    }

    fun securelyStoreModelMetadata(metadata: Map<String, String>): Boolean {
        return try {
            if (metadata.isEmpty()) {
                Log.w(TAG, "Empty metadata provided")
                return false
            }

            val sanitizedMetadata = metadata.mapValues { (_, value) ->
                value.replace("\n", " ").trim()
            }

            val metadataString = sanitizedMetadata.entries.joinToString(separator = "\n") { 
                "${it.key.trim()}=${it.value}" 
            }
            val encryptedMetadata = encryptModel(metadataString.toByteArray(Charsets.UTF_8))
                ?: return false
                
            sharedPreferences.edit().apply {
                putString(ENCRYPTED_METADATA_KEY, android.util.Base64.encodeToString(encryptedMetadata.first, android.util.Base64.NO_WRAP))
                putString(METADATA_IV_KEY, android.util.Base64.encodeToString(encryptedMetadata.second, android.util.Base64.NO_WRAP))
                apply()
            }
            true
        } catch (e: Exception) {
            Log.e(TAG, "Failed to store model metadata", e)
            false
        }
    }

    fun retrieveModelMetadata(): Map<String, String>? {
        return try {
            val encryptedMetadataStr = sharedPreferences.getString(ENCRYPTED_METADATA_KEY, null)
            val ivStr = sharedPreferences.getString(METADATA_IV_KEY, null)
            
            if (encryptedMetadataStr == null || ivStr == null) {
                Log.w(TAG, "No stored metadata found")
                return null
            }
            
            val encryptedMetadata = android.util.Base64.decode(encryptedMetadataStr, android.util.Base64.NO_WRAP)
            val iv = android.util.Base64.decode(ivStr, android.util.Base64.NO_WRAP)
            
            val decryptedData = decryptModel(encryptedMetadata, iv) ?: return null
            parseMetadata(String(decryptedData, Charsets.UTF_8))
        } catch (e: Exception) {
            Log.e(TAG, "Failed to retrieve model metadata", e)
            null
        }
    }

    private fun parseMetadata(metadataString: String): Map<String, String> {
        return try {
            metadataString.lines()
                .asSequence()
                .filter { it.isNotBlank() && it.contains('=') }
                .map { it.split('=', limit = 2) }
                .filter { it.size == 2 }
                .map { (key, value) -> key.trim() to value.trim() }
                .toMap()
        } catch (e: Exception) {
            Log.e(TAG, "Failed to parse metadata", e)
            emptyMap()
        }
    }
}