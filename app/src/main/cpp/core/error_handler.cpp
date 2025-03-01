#include "error_handler.h"
#include <android/log.h>
#include <chrono>
#include <fstream>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <mutex>
#include <sstream>
#include <cxxabi.h>
#include <sys/utsname.h>
#include <unwind.h>
#include <dlfcn.h>
#include <thread>
#include <unistd.h>
#include <cstdio>

namespace mobileai {
namespace core {

using namespace std;
using namespace std::chrono;
using json = nlohmann::json;

// Define logging macros consistent with other files
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ErrorHandler", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "ErrorHandler", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ErrorHandler", __VA_ARGS__)

// Helper function for string formatting
template<typename... Args>
string StringFormat(const char* format, Args... args) {
    int size = snprintf(nullptr, 0, format, args...);
    string result(size + 1, '\0');
    snprintf(&result[0], size + 1, format, args...);
    result.pop_back(); // Remove null terminator
    return result;
}

// Constants
constexpr int MAX_STACK_FRAMES = 64;

class ErrorHandler::Impl {
public:
    Impl() : automatic_recovery_(true), max_retries_(3), healthy_(true) {}

    void RegisterErrorCallback(ErrorCallback callback) {
        if (!callback) {
            LOGW("Attempting to register null error callback");
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        callbacks_.push_back(callback);
    }

    void RegisterRecoveryStrategy(ErrorCategory category, RecoveryStrategy strategy) {
        if (!strategy) {
            LOGW("Attempting to register null recovery strategy for category %s", 
                 GetCategoryString(category).c_str());
            return;
        }
        std::lock_guard<std::mutex> lock(mutex_);
        recovery_strategies_[category] = strategy;
    }

    void HandleError(const ErrorContext& context) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        // Log error
        LogError(context);
        
        // Store in history with size limit to prevent memory issues
        const size_t MAX_HISTORY = 1000;
        if (error_history_.size() >= MAX_HISTORY) {
            error_history_.erase(error_history_.begin());
        }
        error_history_.push_back(context);
        
        // Notify callbacks
        for (const auto& callback : callbacks_) {
            try {
                callback(context);
            } catch (const std::exception& e) {
                LOGE("Error callback threw exception: %s", e.what());
            } catch (...) {
                LOGE("Error callback threw unknown exception");
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
        if (message.empty()) {
            LOGW("Empty error message reported");
            return;
        }

        ErrorContext context;
        context.message = message;
        context.severity = severity;
        context.category = category;
        context.component = component.empty() ? "Unknown" : component;
        context.timestamp = std::chrono::system_clock::now().time_since_epoch().count();
        context.stack_trace = CaptureStackTrace();
        context.device_info = GetDeviceInfo();

        HandleError(context);
    }

    bool AttemptRecovery(const ErrorContext& context) {
        std::lock_guard<std::mutex> lock(mutex_);
        
        auto it = recovery_strategies_.find(context.category);
        if (it == recovery_strategies_.end()) {
            LOGI("No recovery strategy found for category %s", 
                 GetCategoryString(context.category).c_str());
            return false;
        }

        int retries = 0;
        while (retries < max_retries_) {
            try {
                LOGI("Attempting recovery (try %d/%d)", retries + 1, max_retries_);
                if (it->second(context)) {
                    LOGI("Recovery successful");
                    return true;
                }
            } catch (const std::exception& e) {
                LOGE("Recovery attempt failed: %s", e.what());
            } catch (...) {
                LOGE("Recovery attempt failed with unknown error");
            }
            retries++;
            
            // Add exponential backoff between retries
            if (retries < max_retries_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100 * (1 << retries)));
            }
        }

        LOGE("Recovery failed after %d attempts", max_retries_);
        return false;
    }

    void SetAutomaticRecovery(bool enabled) {
        automatic_recovery_ = enabled;
        LOGI("Automatic recovery %s", enabled ? "enabled" : "disabled");
    }

    void SetMaxRetries(int retries) {
        if (retries < 0) {
            LOGW("Invalid retry count %d, using default", retries);
            retries = 3;
        }
        max_retries_ = retries;
    }

    std::vector<ErrorContext> GetErrorHistory() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return error_history_;
    }

    void ClearErrorHistory() {
        std::lock_guard<std::mutex> lock(mutex_);
        error_history_.clear();
        LOGI("Error history cleared");
    }

    bool ExportErrorLogs(const std::string& path) const {
        if (path.empty()) {
            LOGE("Empty export path provided");
            return false;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            json root = json::array();
            for (const auto& error : error_history_) {
                json errorObj;
                errorObj["message"] = error.message;
                errorObj["severity"] = static_cast<int>(error.severity);
                errorObj["category"] = static_cast<int>(error.category);
                errorObj["component"] = error.component;
                errorObj["stack_trace"] = error.stack_trace;
                errorObj["device_info"] = error.device_info;
                errorObj["timestamp"] = error.timestamp;
                root.push_back(errorObj);
            }

            std::ofstream file(path);
            if (!file.is_open()) {
                LOGE("Failed to open file for writing: %s", path.c_str());
                return false;
            }
            file << root.dump(4);
            LOGI("Successfully exported error logs to %s", path.c_str());
            return true;
        } catch (const std::exception& e) {
            LOGE("Failed to export error logs: %s", e.what());
            return false;
        } catch (...) {
            LOGE("Failed to export error logs: unknown error");
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
        LOGI("System reset completed");
    }

    void RetryOperation(const std::function<bool()>& operation, int max_retries) {
        int retries = 0;
        while (retries < max_retries) {
            if (operation()) {
                return;
            }
            // Use platform-specific sleep
            usleep(100000 * (1 << retries));  // 100ms * exponential backoff
            retries++;
        }
    }

    std::string GetStackTrace() const {
        std::string stacktrace = "Stack trace:\n";
        
        // Use platform-specific stack trace capture
        void* buffer[MAX_STACK_FRAMES];
        std::ostringstream trace;
        trace << "Stack trace not available on this platform\n";
        return trace.str();
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
                          "[%s] %s: %s\nStack trace:\n%s",
                          context.component.c_str(),
                          GetCategoryString(context.category).c_str(),
                          context.message.c_str(),
                          context.stack_trace.c_str());
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
            LOGE("System health compromised due to critical error");
        }
        
        std::ostringstream status;
        status << "Last error: " << context.message;
        if (!healthy_) {
            status << " (System unhealthy)";
        }
        system_status_ = status.str();
    }

    std::string CaptureStackTrace() {
        std::ostringstream trace;
        trace << "Stack trace not available on this platform\n";
        return trace.str();
    }

    std::string GetDeviceInfo() {
        struct utsname system_info;
        if (uname(&system_info) == -1) {
            LOGE("Failed to get device info: %s", strerror(errno));
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

ErrorHandler::ErrorHandler() : impl_(std::make_unique<Impl>()) {}
ErrorHandler::~ErrorHandler() = default;

void ErrorHandler::RegisterErrorCallback(ErrorCallback callback) {
    impl_->RegisterErrorCallback(callback);
}

void ErrorHandler::RegisterRecoveryStrategy(ErrorCategory category, RecoveryStrategy strategy) {
    impl_->RegisterRecoveryStrategy(category, strategy);
}

void ErrorHandler::HandleError(const ErrorContext& context) {
    impl_->HandleError(context);
}

void ErrorHandler::ReportError(const std::string& message,
                             ErrorSeverity severity,
                             ErrorCategory category,
                             const std::string& component) {
    impl_->ReportError(message, severity, category, component);
}

bool ErrorHandler::AttemptRecovery(const ErrorContext& context) {
    return impl_->AttemptRecovery(context);
}

void ErrorHandler::SetAutomaticRecovery(bool enabled) {
    impl_->SetAutomaticRecovery(enabled);
}

void ErrorHandler::SetMaxRetries(int retries) {
    impl_->SetMaxRetries(retries);
}

std::vector<ErrorContext> ErrorHandler::GetErrorHistory() const {
    return impl_->GetErrorHistory();
}

void ErrorHandler::ClearErrorHistory() {
    impl_->ClearErrorHistory();
}

bool ErrorHandler::ExportErrorLogs(const std::string& path) const {
    return impl_->ExportErrorLogs(path);
}

bool ErrorHandler::IsSystemHealthy() const {
    return impl_->IsSystemHealthy();
}

std::string ErrorHandler::GetSystemStatus() const {
    return impl_->GetSystemStatus();
}

void ErrorHandler::ResetSystem() {
    impl_->ResetSystem();
}

void ErrorHandler::RetryOperation(const std::function<bool()>& operation, int max_retries) {
    impl_->RetryOperation(operation, max_retries);
}

std::string ErrorHandler::GetStackTrace() const {
    return impl_->GetStackTrace();
}

} // namespace core
} // namespace mobileai 