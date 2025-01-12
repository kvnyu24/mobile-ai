#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace mobileai {
namespace hardware {

#if defined(__APPLE__)
#define PLATFORM_MACOS
#elif defined(__ANDROID__)
#define PLATFORM_ANDROID
#endif

class HardwareAccelerator {
public:
    // Performance metrics struct
    struct PerformanceMetrics {
        float inferenceTimeMs;      // Inference time in milliseconds
        float powerConsumptionMw;   // Power consumption in milliwatts
        float utilizationPercent;   // Hardware utilization percentage
    };

    // Power profile options
    enum class PowerProfile {
        LOW_POWER,      // Optimize for battery life
        BALANCED,       // Balance performance and power
        HIGH_PERFORMANCE // Maximum performance
    };

    // Error codes
    enum class ErrorCode {
        SUCCESS,
        INITIALIZATION_FAILED,
        UNSUPPORTED_OPERATION,
        INVALID_INPUT,
        HARDWARE_ERROR,
        RESOURCE_EXHAUSTED
    };

    virtual ~HardwareAccelerator() = default;

    // Initialize the hardware accelerator
    virtual ErrorCode Initialize() = 0;

    // Check if the accelerator is available and ready
    virtual bool IsAvailable() const = 0;

    // Run inference on the accelerator with performance metrics
    virtual ErrorCode RunInference(const std::vector<float>& input,
                                 std::vector<float>& output,
                                 PerformanceMetrics* metrics = nullptr) = 0;

    // Get the accelerator type and capabilities
    virtual std::string GetAcceleratorType() const = 0;
    virtual std::vector<std::string> GetSupportedOperations() const = 0;
    virtual bool SupportsOperation(const std::string& operation) const = 0;

    // Power and performance management
    virtual ErrorCode SetPowerProfile(PowerProfile profile) = 0;
    virtual PowerProfile GetCurrentPowerProfile() const = 0;
    virtual PerformanceMetrics GetPerformanceMetrics() const = 0;

    // Resource management
    virtual void ReleaseResources() = 0;
    virtual bool ResetState() = 0;

    // Version information
    virtual std::string GetDriverVersion() const = 0;
    virtual std::string GetFirmwareVersion() const = 0;
};

} // namespace hardware
} // namespace mobileai