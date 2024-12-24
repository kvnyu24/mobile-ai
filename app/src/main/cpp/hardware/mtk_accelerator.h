#pragma once

#include "hardware_accelerator.h"
#include <memory>
#include <string>
#include <vector>
#include <neuropilot/neuropilot.h>

namespace mobileai {
namespace hardware {

class MTKAccelerator : public HardwareAccelerator {
public:
    MTKAccelerator();
    ~MTKAccelerator() override;

    // Core functionality
    ErrorCode Initialize() override;
    bool IsAvailable() const override;
    ErrorCode RunInference(const std::vector<float>& input,
                          std::vector<float>& output,
                          PerformanceMetrics* metrics = nullptr) override;

    // Configuration and capabilities                     
    std::string GetAcceleratorType() const override;
    std::vector<std::string> GetSupportedOperations() const override;
    bool SupportsOperation(const std::string& operation) const override;
    ErrorCode SetPowerProfile(PowerProfile profile) override;
    PowerProfile GetCurrentPowerProfile() const override;
    PerformanceMetrics GetPerformanceMetrics() const override;

    // Resource management
    void ReleaseResources() override;
    bool ResetState() override;

    // Version information
    std::string GetDriverVersion() const override;
    std::string GetFirmwareVersion() const override;

    // Enhanced functionality (MTK-specific extensions)
    bool SetThreadCount(int thread_count);
    bool SetPreferredMemoryType(const std::string& memory_type);
    float GetLastInferenceTime() const;
    bool EnableProfiling(bool enable);
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