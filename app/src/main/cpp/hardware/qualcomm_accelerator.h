#pragma once

#include "hardware_accelerator.h"
#include <memory>
#include <string>
#include <vector>

namespace mobileai {
namespace hardware {

class QualcommAccelerator : public HardwareAccelerator {
public:
    // Power profile options for Qualcomm hardware
    enum class PowerProfile {
        LOW_POWER,
        BALANCED,
        HIGH_PERFORMANCE
    };

    // DSP power levels (0-5, where 0 is lowest power and 5 is highest performance)
    static constexpr int MIN_DSP_POWER_LEVEL = 0;
    static constexpr int MAX_DSP_POWER_LEVEL = 5;

    QualcommAccelerator();
    ~QualcommAccelerator() override;

    // Base class interface implementation
    bool Initialize() override;
    bool IsAvailable() const override;
    bool RunInference(const std::vector<float>& input,
                     std::vector<float>& output) override;
    std::string GetAcceleratorType() const override;
    std::vector<std::string> GetSupportedOperations() const override;
    bool SetPowerProfile(const std::string& profile) override;

    // Qualcomm-specific configuration
    bool SetDSPPowerLevel(int level);
    bool EnableFastRPC(bool enable = true);
    bool ConfigureCache(size_t cache_size);
    
    // Additional Qualcomm-specific features
    bool SetHexagonDSPMode(bool enable);
    bool EnableHVXOptimization(bool enable = true);
    bool SetNumThreads(int num_threads);
    float GetLastInferenceTime() const;
    bool SupportsDynamicBatching() const;
    
    // Resource management
    void ReleaseResources();
    bool ReloadModel();
    
    // Performance monitoring
    struct PerformanceStats {
        float avg_inference_time_ms;
        float peak_memory_mb;
        int active_threads;
        float dsp_utilization_percent;
    };
    PerformanceStats GetPerformanceStats() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
    
    // Prevent copying
    QualcommAccelerator(const QualcommAccelerator&) = delete;
    QualcommAccelerator& operator=(const QualcommAccelerator&) = delete;
};

}  // namespace hardware
}  // namespace mobileai