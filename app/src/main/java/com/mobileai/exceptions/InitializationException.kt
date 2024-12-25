package com.mobileai.exceptions

class InitializationException : Exception {
    constructor(message: String) : super(message)
    constructor(message: String, cause: Throwable) : super(message, cause)
    constructor(cause: Throwable) : super(cause)
    
    companion object {
        private const val DEFAULT_MESSAGE = "Initialization failed"
        
        fun fromCause(cause: Throwable): InitializationException {
            return InitializationException("$DEFAULT_MESSAGE: ${cause.message}", cause)
        }
    }
}
