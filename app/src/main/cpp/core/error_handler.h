#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>

namespace mobileai {
namespace core {

enum class ErrorSeverity {
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

enum class ErrorCategory {
    HARDWARE,
    MODEL,
    MEMORY,
    SYSTEM,
    SECURITY,
    NETWORK
};

struct ErrorContext {
    std::string message;
    ErrorSeverity severity;
    ErrorCategory category;
    std::string component;
    std::string stack_trace;
    std::string device_info;
    long long timestamp;
};

using ErrorCallback = std::function<void(const ErrorContext&)>;
using RecoveryStrategy = std::function<bool(const ErrorContext&)>;

class ErrorHandler {
public:
    ErrorHandler();
    ~ErrorHandler();

    // Error registration and handling
    void RegisterErrorCallback(ErrorCallback callback);
    void RegisterRecoveryStrategy(ErrorCategory category, RecoveryStrategy strategy);
    void HandleError(const ErrorContext& context);

    // Error reporting
    void ReportError(const std::string& message,
                    ErrorSeverity severity,
                    ErrorCategory category,
                    const std::string& component);

    // Recovery management
    bool AttemptRecovery(const ErrorContext& context);
    void SetAutomaticRecovery(bool enabled);
    void SetMaxRetries(int retries);

    // Error history
    std::vector<ErrorContext> GetErrorHistory() const;
    void ClearErrorHistory();
    bool ExportErrorLogs(const std::string& path) const;

    // System health
    bool IsSystemHealthy() const;
    std::string GetSystemStatus() const;
    void ResetSystem();

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
}
} 