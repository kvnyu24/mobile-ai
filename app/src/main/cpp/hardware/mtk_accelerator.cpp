#include "mtk_accelerator.h"
#include <android/log.h>
#include <chrono>

namespace mobileai {
namespace hardware {

class MTKAccelerator::Impl {
public:
    Impl() : current_power_profile_(PowerProfile::BALANCED),
             last_inference_time_ms_(0.0f),
             thread_count_(1),
             profiling_enabled_(false),
             neuropilot_handle_(nullptr) {}
    ~Impl() {
        if (neuropilot_handle_) {
            NeuroPilotClose(neuropilot_handle_);
        }
    }

    ErrorCode Initialize() {
        NeuroPilotConfig config{};
        config.version = NEUROPILOT_VERSION_1;
        config.flags = NEUROPILOT_FLAG_NONE;
        
        auto status = NeuroPilotInit(&config, &neuropilot_handle_);
        if (status != NEUROPILOT_NO_ERROR) {
            __android_log_print(ANDROID_LOG_ERROR, "MTKAccelerator", 
                              "Failed to initialize NeuroPilot: %d", status);
            return ErrorCode::INITIALIZATION_FAILED;
        }
        return ErrorCode::SUCCESS;
    }

    bool IsAvailable() const {
        if (!neuropilot_handle_) {
            return false;
        }
        
        NeuroPilotHardwareInfo hw_info{};
        auto status = NeuroPilotGetHardwareInfo(neuropilot_handle_, &hw_info);
        if (status != NEUROPILOT_NO_ERROR) {
            return false;
        }
        
        return hw_info.apu_available;
    }

    std::vector<std::string> GetSupportedOperations() const {
        return {"CONV_2D", "DEPTHWISE_CONV_2D", "FULLY_CONNECTED", 
                "AVERAGE_POOL_2D", "MAX_POOL_2D", "SOFTMAX"};
    }

    bool SupportsOperation(const std::string& operation) const {
        auto ops = GetSupportedOperations();
        return std::find(ops.begin(), ops.end(), operation) != ops.end();
    }

    ErrorCode RunInference(const std::vector<float>& input,
                          std::vector<float>& output,
                          PerformanceMetrics* metrics) {
        if (!neuropilot_handle_) {
            return ErrorCode::NOT_INITIALIZED;
        }

        auto start = std::chrono::high_resolution_clock::now();

        NeuroPilotBuffer input_buffer{};
        input_buffer.data = const_cast<float*>(input.data());
        input_buffer.size = input.size() * sizeof(float);
        
        NeuroPilotBuffer output_buffer{};
        output.resize(GetOutputSize()); // Assuming we know output size
        output_buffer.data = output.data();
        output_buffer.size = output.size() * sizeof(float);

        auto status = NeuroPilotExecute(neuropilot_handle_, 
                                      &input_buffer,
                                      &output_buffer,
                                      thread_count_);
                                      
        if (status != NEUROPILOT_NO_ERROR) {
            __android_log_print(ANDROID_LOG_ERROR, "MTKAccelerator", 
                              "Inference failed: %d", status);
            return ErrorCode::INFERENCE_ERROR;
        }

        auto end = std::chrono::high_resolution_clock::now();
        last_inference_time_ms_ = std::chrono::duration<float, std::milli>(end - start).count();

        if (metrics) {
            metrics->inferenceTimeMs = last_inference_time_ms_;
            metrics->powerConsumptionMw = GetPowerConsumption();
            metrics->utilizationPercent = GetUtilization();
        }

        return ErrorCode::SUCCESS;
    }

    ErrorCode SetPowerProfile(PowerProfile profile) {
        if (!neuropilot_handle_) {
            return ErrorCode::NOT_INITIALIZED;
        }

        NeuroPilotPowerConfig power_config{};
        switch (profile) {
            case PowerProfile::LOW_POWER:
                power_config.performance_mode = NEUROPILOT_POWER_SAVE;
                break;
            case PowerProfile::BALANCED:
                power_config.performance_mode = NEUROPILOT_BALANCED;
                break;
            case PowerProfile::HIGH_PERFORMANCE:
                power_config.performance_mode = NEUROPILOT_PERFORMANCE;
                break;
            default:
                return ErrorCode::INVALID_ARGUMENT;
        }

        auto status = NeuroPilotSetPowerConfig(neuropilot_handle_, &power_config);
        if (status != NEUROPILOT_NO_ERROR) {
            return ErrorCode::HARDWARE_ERROR;
        }

        current_power_profile_ = profile;
        return ErrorCode::SUCCESS;
    }

    PowerProfile GetCurrentPowerProfile() const {
        return current_power_profile_;
    }

    PerformanceMetrics GetPerformanceMetrics() const {
        PerformanceMetrics metrics;
        metrics.inferenceTimeMs = last_inference_time_ms_;
        metrics.powerConsumptionMw = GetPowerConsumption();
        metrics.utilizationPercent = GetUtilization();
        return metrics;
    }

    bool SetThreadCount(int count) {
        if (count > 0) {
            thread_count_ = count;
            return true;
        }
        return false;
    }

    void EnableProfiling(bool enable) {
        profiling_enabled_ = enable;
        if (neuropilot_handle_) {
            NeuroPilotSetProfiling(neuropilot_handle_, 
                                 enable ? NEUROPILOT_PROFILE_ENABLE : NEUROPILOT_PROFILE_DISABLE);
        }
    }

private:
    float GetPowerConsumption() const {
        if (!neuropilot_handle_) {
            return 0.0f;
        }

        NeuroPilotPowerStats power_stats{};
        auto status = NeuroPilotGetPowerStats(neuropilot_handle_, &power_stats);
        if (status != NEUROPILOT_NO_ERROR) {
            return 0.0f;
        }
        return power_stats.power_consumption;
    }

    float GetUtilization() const {
        if (!neuropilot_handle_) {
            return 0.0f;
        }

        NeuroPilotPerformanceStats perf_stats{};
        auto status = NeuroPilotGetPerformanceStats(neuropilot_handle_, &perf_stats);
        if (status != NEUROPILOT_NO_ERROR) {
            return 0.0f;
        }
        return perf_stats.utilization;
    }

    PowerProfile current_power_profile_;
    float last_inference_time_ms_;
    int thread_count_;
    bool profiling_enabled_;
    NeuroPilotHandle neuropilot_handle_;
};

MTKAccelerator::MTKAccelerator() : pImpl(std::make_unique<Impl>()) {}
MTKAccelerator::~MTKAccelerator() = default;

ErrorCode MTKAccelerator::Initialize() {
    return pImpl->Initialize();
}

bool MTKAccelerator::IsAvailable() const {
    return pImpl->IsAvailable();
}

ErrorCode MTKAccelerator::RunInference(const std::vector<float>& input,
                                     std::vector<float>& output,
                                     PerformanceMetrics* metrics) {
    return pImpl->RunInference(input, output, metrics);
}

std::string MTKAccelerator::GetAcceleratorType() const {
    return "MediaTek APU";
}

std::vector<std::string> MTKAccelerator::GetSupportedOperations() const {
    return pImpl->GetSupportedOperations();
}

bool MTKAccelerator::SupportsOperation(const std::string& operation) const {
    return pImpl->SupportsOperation(operation);
}

ErrorCode MTKAccelerator::SetPowerProfile(PowerProfile profile) {
    return pImpl->SetPowerProfile(profile);
}

PowerProfile MTKAccelerator::GetCurrentPowerProfile() const {
    return pImpl->GetCurrentPowerProfile();
}

PerformanceMetrics MTKAccelerator::GetPerformanceMetrics() const {
    return pImpl->GetPerformanceMetrics();
}

bool MTKAccelerator::SetThreadCount(int thread_count) {
    return pImpl->SetThreadCount(thread_count);
}

bool MTKAccelerator::EnableProfiling(bool enable) {
    pImpl->EnableProfiling(enable);
    return true;
}

} // namespace hardware
} // namespace mobileai