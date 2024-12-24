#include "error_handler.h"
#include <android/log.h>
#include <chrono>
#include <fstream>
#include <json/json.h>
#include <unordered_map>
#include <mutex>
#include <sstream>
#include <execinfo.h>
#include <cxxabi.h>
#include <sys/utsname.h>

namespace mobileai {
namespace core {

class ErrorHandler::Impl {
public:
    Impl() : automatic_recovery_(true), max_retries_(3), healthy_(true) {}

    void RegisterErrorCallback(ErrorCallback callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_.push_back(callback);
    }

    void RegisterRecoveryStrategy(ErrorCategory category, RecoveryStrategy strategy) {
        std::lock_guard<std::mutex> lock(mutex_);
        recovery_strategies_[category] = strategy;
    }

    void HandleError(const ErrorContext& context) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Log error
        LogError(context);
        
        // Store in history
        error_history_.push_back(context);
        
        // Notify callbacks
        for (const auto& callback : callbacks_) {
            try {
                callback(context);
            } catch (...) {
                // Ignore callback errors
            }
        }

        // Attempt recovery if enabled
        if (automatic_recovery_) {
            AttemptRecovery(context);
        }

        // Update system health
        UpdateSystemHealth(context);
    }

    void ReportError(const std::string& message,
                    ErrorSeverity severity,
                    ErrorCategory category,
                    const std::string& component) {
        ErrorContext context;
        context.message = message;
        context.severity = severity;
        context.category = category;
        context.component = component;
        context.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        context.stack_trace = CaptureStackTrace();
        context.device_info = GetDeviceInfo();

        HandleError(context);
    }

    bool AttemptRecovery(const ErrorContext& context) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = recovery_strategies_.find(context.category);
        if (it == recovery_strategies_.end()) {
            return false;
        }

        int retries = 0;
        while (retries < max_retries_) {
            try {
                if (it->second(context)) {
                    return true;
                }
            } catch (...) {
                // Ignore recovery errors
            }
            retries++;
        }

        return false;
    }

    void SetAutomaticRecovery(bool enabled) {
        automatic_recovery_ = enabled;
    }

    void SetMaxRetries(int retries) {
        max_retries_ = retries;
    }

    std::vector<ErrorContext> GetErrorHistory() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return error_history_;
    }

    void ClearErrorHistory() {
        std::lock_guard<std::mutex> lock(mutex_);
        error_history_.clear();
    }

    bool ExportErrorLogs(const std::string& path) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            Json::Value root(Json::arrayValue);
            for (const auto& error : error_history_) {
                Json::Value errorObj;
                errorObj["message"] = error.message;
                errorObj["severity"] = static_cast<int>(error.severity);
                errorObj["category"] = static_cast<int>(error.category);
                errorObj["component"] = error.component;
                errorObj["stack_trace"] = error.stack_trace;
                errorObj["device_info"] = error.device_info;
                errorObj["timestamp"] = Json::Value::Int64(error.timestamp);
                root.append(errorObj);
            }

            Json::StreamWriterBuilder writer;
            std::ofstream file(path);
            return file.is_open() ? (file << Json::writeString(writer, root), true) : false;
        } catch (...) {
            return false;
        }
    }

    bool IsSystemHealthy() const {
        return healthy_;
    }

    std::string GetSystemStatus() const {
        return system_status_;
    }

    void ResetSystem() {
        std::lock_guard<std::mutex> lock(mutex_);
        error_history_.clear();
        healthy_ = true;
        system_status_ = "System reset successfully";
    }

private:
    void LogError(const ErrorContext& context) {
        int priority;
        switch (context.severity) {
            case ErrorSeverity::INFO:
                priority = ANDROID_LOG_INFO;
                break;
            case ErrorSeverity::WARNING:
                priority = ANDROID_LOG_WARN;
                break;
            case ErrorSeverity::ERROR:
                priority = ANDROID_LOG_ERROR;
                break;
            case ErrorSeverity::CRITICAL:
                priority = ANDROID_LOG_FATAL;
                break;
            default:
                priority = ANDROID_LOG_INFO;
        }

        __android_log_print(priority, "ErrorHandler",
                          "[%s] %s: %s",
                          context.component.c_str(),
                          GetCategoryString(context.category).c_str(),
                          context.message.c_str());
    }

    std::string GetCategoryString(ErrorCategory category) const {
        switch (category) {
            case ErrorCategory::HARDWARE: return "Hardware";
            case ErrorCategory::MODEL: return "Model";
            case ErrorCategory::MEMORY: return "Memory";
            case ErrorCategory::SYSTEM: return "System";
            case ErrorCategory::SECURITY: return "Security";
            case ErrorCategory::NETWORK: return "Network";
            default: return "Unknown";
        }
    }

    void UpdateSystemHealth(const ErrorContext& context) {
        if (context.severity == ErrorSeverity::CRITICAL) {
            healthy_ = false;
        }
        system_status_ = "Last error: " + context.message;
    }

    std::string CaptureStackTrace() {
        const int max_frames = 32;
        void* callstack[max_frames];
        int frames = backtrace(callstack, max_frames);
        char** symbols = backtrace_symbols(callstack, frames);
        
        std::ostringstream trace;
        for (int i = 0; i < frames; i++) {
            std::string symbol(symbols[i]);
            
            // Try to demangle C++ symbols
            size_t begin = symbol.find('(');
            size_t end = symbol.find('+', begin);
            if (begin != std::string::npos && end != std::string::npos) {
                std::string mangled = symbol.substr(begin + 1, end - begin - 1);
                int status;
                char* demangled = abi::__cxa_demangle(mangled.c_str(), nullptr, nullptr, &status);
                if (status == 0 && demangled) {
                    symbol = symbol.substr(0, begin + 1) + demangled + symbol.substr(end);
                    free(demangled);
                }
            }
            
            trace << "#" << i << ": " << symbol << "\n";
        }
        
        free(symbols);
        return trace.str();
    }

    std::string GetDeviceInfo() {
        struct utsname system_info;
        if (uname(&system_info) == -1) {
            return "Failed to get device info";
        }

        std::ostringstream info;
        info << "System: " << system_info.sysname << "\n"
             << "Node: " << system_info.nodename << "\n"
             << "Release: " << system_info.release << "\n"
             << "Version: " << system_info.version << "\n"
             << "Machine: " << system_info.machine;

        return info.str();
    }

    bool automatic_recovery_;
    int max_retries_;
    bool healthy_;
    std::string system_status_;
    std::vector<ErrorCallback> callbacks_;
    std::unordered_map<ErrorCategory, RecoveryStrategy> recovery_strategies_;
    std::vector<ErrorContext> error_history_;
    mutable std::mutex mutex_;
};

ErrorHandler::ErrorHandler() : pImpl(std::make_unique<Impl>()) {}
ErrorHandler::~ErrorHandler() = default;

void ErrorHandler::RegisterErrorCallback(ErrorCallback callback) {
    pImpl->RegisterErrorCallback(callback);
}

void ErrorHandler::RegisterRecoveryStrategy(ErrorCategory category, RecoveryStrategy strategy) {
    pImpl->RegisterRecoveryStrategy(category, strategy);
}

void ErrorHandler::HandleError(const ErrorContext& context) {
    pImpl->HandleError(context);
}

void ErrorHandler::ReportError(const std::string& message,
                             ErrorSeverity severity,
                             ErrorCategory category,
                             const std::string& component) {
    pImpl->ReportError(message, severity, category, component);
}

bool ErrorHandler::AttemptRecovery(const ErrorContext& context) {
    return pImpl->AttemptRecovery(context);
}

void ErrorHandler::SetAutomaticRecovery(bool enabled) {
    pImpl->SetAutomaticRecovery(enabled);
}

void ErrorHandler::SetMaxRetries(int retries) {
    pImpl->SetMaxRetries(retries);
}

std::vector<ErrorContext> ErrorHandler::GetErrorHistory() const {
    return pImpl->GetErrorHistory();
}

void ErrorHandler::ClearErrorHistory() {
    pImpl->ClearErrorHistory();
}

bool ErrorHandler::ExportErrorLogs(const std::string& path) const {
    return pImpl->ExportErrorLogs(path);
}

bool ErrorHandler::IsSystemHealthy() const {
    return pImpl->IsSystemHealthy();
}

std::string ErrorHandler::GetSystemStatus() const {
    return pImpl->GetSystemStatus();
}

void ErrorHandler::ResetSystem() {
    pImpl->ResetSystem();
}

} // namespace core
} // namespace mobileai 