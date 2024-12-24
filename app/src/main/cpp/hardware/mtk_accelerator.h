#pragma once

#include "hardware_accelerator.h"
#include <memory>
#include <string>
#include <vector>

namespace mobileai {
namespace hardware {

class MTKAccelerator : public HardwareAccelerator {
public:
    MTKAccelerator();
    ~MTKAccelerator() override;

    // Core functionality
    bool Initialize() override;
    bool IsAvailable() const override;
    bool RunInference(const std::vector<float>& input,
                     std::vector<float>& output) override;

    // Configuration and capabilities                     
    std::string GetAcceleratorType() const override;
    std::vector<std::string> GetSupportedOperations() const override;
    bool SetPowerProfile(const std::string& profile) override;

    // Enhanced functionality
    bool SetThreadCount(int thread_count);
    bool SetPreferredMemoryType(const std::string& memory_type);
    float GetLastInferenceTime() const;
    bool EnableProfiling(bool enable);
    
    // Resource management
    void ReleaseResources();
    bool ReloadModel();

    // Error handling
    std::string GetLastErrorMessage() const;
    int GetLastErrorCode() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;

    // Prevent copying
    MTKAccelerator(const MTKAccelerator&) = delete;
    MTKAccelerator& operator=(const MTKAccelerator&) = delete;
};

} // namespace hardware
} // namespace mobileai 