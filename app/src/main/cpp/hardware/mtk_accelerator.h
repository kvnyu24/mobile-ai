#ifndef MTK_ACCELERATOR_H
#define MTK_ACCELERATOR_H

#include "hardware_accelerator.h"
#include <memory>
#include <string>
#include <vector>

namespace mobileai {
namespace hardware {

#ifdef PLATFORM_ANDROID
typedef void* NeuroPilotHandle;
#endif

class MTKAccelerator : public HardwareAccelerator {
public:
    MTKAccelerator();
    ~MTKAccelerator() override;

    ErrorCode Initialize() override;
    bool IsAvailable() const override;
    std::vector<std::string> GetSupportedOperations() const override;
    bool SupportsOperation(const std::string& operation) const override;
    ErrorCode RunInference(const std::vector<float>& input,
                          std::vector<float>& output,
                          PerformanceMetrics* metrics = nullptr) override;
    ErrorCode SetPowerProfile(PowerProfile profile) override;
    PowerProfile GetCurrentPowerProfile() const override;
    
    // MTK-specific extensions
    float GetLastInferenceTime() const;
    bool SetThreadCount(int thread_count);
    bool EnableProfiling(bool enable);
    std::string GetLastErrorMessage() const;
    int GetLastErrorCode() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace hardware
} // namespace mobileai

#endif // MTK_ACCELERATOR_H 