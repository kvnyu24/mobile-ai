package com.mobileai.core

class HardwareManager {
    private var nativeHandle: Long = 0

    external fun initialize(): Boolean
    external fun getAvailableAccelerators(): List<String>
    external fun setPreferredAccelerator(acceleratorType: String): Boolean
    external fun release()

    companion object {
        const val ACCELERATOR_MTK = "MediaTek APU"
        const val ACCELERATOR_QUALCOMM = "Qualcomm DSP"
        const val ACCELERATOR_CPU = "CPU"
        const val ACCELERATOR_GPU = "GPU"
    }
} 