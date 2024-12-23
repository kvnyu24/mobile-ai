package com.mobileai.security

import android.security.keystore.KeyGenParameterSpec
import android.security.keystore.KeyProperties
import java.security.KeyStore
import javax.crypto.Cipher
import javax.crypto.KeyGenerator
import javax.crypto.SecretKey
import javax.crypto.spec.GCMParameterSpec

class SecurityManager {
    companion object {
        private const val ANDROID_KEYSTORE = "AndroidKeyStore"
        private const val KEY_ALIAS = "MobileAIKey"
        private const val TRANSFORMATION = "AES/GCM/NoPadding"
        private const val AUTH_TAG_LENGTH = 128
    }

    private val keyStore: KeyStore = KeyStore.getInstance(ANDROID_KEYSTORE).apply {
        load(null)
    }

    init {
        if (!keyStore.containsAlias(KEY_ALIAS)) {
            generateKey()
        }
    }

    private fun generateKey() {
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
            .setKeySize(256)
            .setUserAuthenticationRequired(false)
            .build()

        keyGenerator.init(keyGenParameterSpec)
        keyGenerator.generateKey()
    }

    fun encryptModel(modelData: ByteArray): Pair<ByteArray, ByteArray> {
        val cipher = Cipher.getInstance(TRANSFORMATION)
        val secretKey = keyStore.getKey(KEY_ALIAS, null) as SecretKey
        cipher.init(Cipher.ENCRYPT_MODE, secretKey)
        
        val encryptedData = cipher.doFinal(modelData)
        return Pair(encryptedData, cipher.iv)
    }

    fun decryptModel(encryptedData: ByteArray, iv: ByteArray): ByteArray {
        val cipher = Cipher.getInstance(TRANSFORMATION)
        val secretKey = keyStore.getKey(KEY_ALIAS, null) as SecretKey
        val gcmSpec = GCMParameterSpec(AUTH_TAG_LENGTH, iv)
        cipher.init(Cipher.DECRYPT_MODE, secretKey, gcmSpec)
        
        return cipher.doFinal(encryptedData)
    }

    fun securelyStoreModelMetadata(metadata: Map<String, String>): Boolean {
        try {
            val metadataString = metadata.entries.joinToString(separator = "\n") { 
                "${it.key}=${it.value}" 
            }
            val encryptedMetadata = encryptModel(metadataString.toByteArray())
            // TODO: Store encrypted metadata and IV in secure storage
            return true
        } catch (e: Exception) {
            return false
        }
    }

    fun retrieveModelMetadata(): Map<String, String>? {
        try {
            // TODO: Retrieve encrypted metadata and IV from secure storage
            // val (encryptedMetadata, iv) = retrieveEncryptedMetadata()
            // val decryptedData = decryptModel(encryptedMetadata, iv)
            // return parseMetadata(String(decryptedData))
            return null
        } catch (e: Exception) {
            return null
        }
    }

    private fun parseMetadata(metadataString: String): Map<String, String> {
        return metadataString.lines()
            .filter { it.contains('=') }
            .map { it.split('=', limit = 2) }
            .associate { it[0] to it[1] }
    }
} 