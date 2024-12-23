#pragma once

#include "hardware_accelerator.h"
#include <memory>

namespace mobileai {
namespace hardware {

class MTKAccelerator : public HardwareAccelerator {
public:
    MTKAccelerator();
    ~MTKAccelerator() override;

    bool Initialize() override;
    bool IsAvailable() const override;
    bool RunInference(const std::vector<float>& input,
                     std::vector<float>& output) override;
    std::string GetAcceleratorType() const override;
    std::vector<std::string> GetSupportedOperations() const override;
    bool SetPowerProfile(const std::string& profile) override;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace hardware
} // namespace mobileai 