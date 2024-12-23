#pragma once

#include <memory>
#include <string>
#include <vector>

namespace mobileai {
namespace hardware {

class HardwareAccelerator {
public:
    virtual ~HardwareAccelerator() = default;

    // Initialize the hardware accelerator
    virtual bool Initialize() = 0;

    // Check if the accelerator is available
    virtual bool IsAvailable() const = 0;

    // Run inference on the accelerator
    virtual bool RunInference(const std::vector<float>& input,
                            std::vector<float>& output) = 0;

    // Get the accelerator type
    virtual std::string GetAcceleratorType() const = 0;

    // Get supported operations
    virtual std::vector<std::string> GetSupportedOperations() const = 0;

    // Power management
    virtual bool SetPowerProfile(const std::string& profile) = 0;
};

} // namespace hardware
} // namespace mobileai 