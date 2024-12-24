#include "mtk_accelerator.h"
#include <android/log.h>
#include <chrono>
#include <algorithm>
#include "core/error_handler.h"

namespace mobileai {
namespace hardware {

class MTKAccelerator::Impl {
public:
    Impl() : current_power_profile_("balanced"),
             last_inference_time_ms_(0.0f),
             thread_count_(1),
             profiling_enabled_(false),
             neuropilot_handle_(nullptr),
             error_handler_(std::make_unique<core::ErrorHandler>()) {}
             
    ~Impl() {
        if (neuropilot_handle_) {
            NeuroPilotClose(neuropilot_handle_);
        }
    }

    HardwareAccelerator::ErrorCode Initialize() {
        NeuroPilotConfig config{};
        config.version = NEUROPILOT_VERSION_1;
        config.flags = NEUROPILOT_FLAG_NONE;
        
        auto status = NeuroPilotInit(&config, &neuropilot_handle_);
        if (status != NEUROPILOT_NO_ERROR) {
            error_handler_->ReportError(
                "Failed to initialize NeuroPilot",
                core::ErrorSeverity::Critical,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
            return HardwareAccelerator::ErrorCode::INITIALIZATION_FAILED;
        }
        return HardwareAccelerator::ErrorCode::SUCCESS;
    }

    bool IsAvailable() const {
        if (!neuropilot_handle_) {
            return false;
        }
        
        NeuroPilotHardwareInfo hw_info{};
        auto status = NeuroPilotGetHardwareInfo(neuropilot_handle_, &hw_info);
        if (status != NEUROPILOT_NO_ERROR) {
            error_handler_->ReportError(
                "Failed to get hardware info",
                core::ErrorSeverity::Warning,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
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

    HardwareAccelerator::ErrorCode RunInference(const std::vector<float>& input,
                                              std::vector<float>& output,
                                              HardwareAccelerator::PerformanceMetrics* metrics = nullptr) {
        if (!neuropilot_handle_) {
            error_handler_->ReportError(
                "NeuroPilot handle is null",
                core::ErrorSeverity::Error,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
            return HardwareAccelerator::ErrorCode::HARDWARE_ERROR;
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
            error_handler_->ReportError(
                "Inference execution failed",
                core::ErrorSeverity::Error,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
            return HardwareAccelerator::ErrorCode::HARDWARE_ERROR;
        }

        auto end = std::chrono::high_resolution_clock::now();
        last_inference_time_ms_ = std::chrono::duration<float, std::milli>(end - start).count();

        if (metrics) {
            metrics->inferenceTimeMs = last_inference_time_ms_;
            metrics->powerConsumptionMw = GetPowerConsumption();
            metrics->utilizationPercent = GetUtilization();
        }

        return HardwareAccelerator::ErrorCode::SUCCESS;
    }

    HardwareAccelerator::ErrorCode SetPowerProfile(HardwareAccelerator::PowerProfile profile) {
        if (!neuropilot_handle_) {
            error_handler_->ReportError(
                "Cannot set power profile - NeuroPilot not initialized",
                core::ErrorSeverity::Warning,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
            return HardwareAccelerator::ErrorCode::HARDWARE_ERROR;
        }

        NeuroPilotPowerConfig power_config{};
        switch (profile) {
            case HardwareAccelerator::PowerProfile::LOW_POWER:
                power_config.performance_mode = NEUROPILOT_POWER_SAVE;
                current_power_profile_ = "low_power";
                break;
            case HardwareAccelerator::PowerProfile::BALANCED:
                power_config.performance_mode = NEUROPILOT_BALANCED;
                current_power_profile_ = "balanced";
                break;
            case HardwareAccelerator::PowerProfile::HIGH_PERFORMANCE:
                power_config.performance_mode = NEUROPILOT_PERFORMANCE;
                current_power_profile_ = "high_performance";
                break;
            default:
                error_handler_->ReportError(
                    "Invalid power profile specified",
                    core::ErrorSeverity::Warning,
                    core::ErrorCategory::Hardware,
                    "MTKAccelerator"
                );
                return HardwareAccelerator::ErrorCode::INVALID_INPUT;
        }

        auto status = NeuroPilotSetPowerConfig(neuropilot_handle_, &power_config);
        if (status != NEUROPILOT_NO_ERROR) {
            error_handler_->ReportError(
                "Failed to set power config",
                core::ErrorSeverity::Warning,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
            return HardwareAccelerator::ErrorCode::HARDWARE_ERROR;
        }

        return HardwareAccelerator::ErrorCode::SUCCESS;
    }

    std::string GetCurrentPowerProfile() const {
        return current_power_profile_;
    }

    float GetLastInferenceTime() const {
        return last_inference_time_ms_;
    }

    bool SetThreadCount(int count) {
        if (count <= 0) {
            error_handler_->ReportError(
                "Invalid thread count specified",
                core::ErrorSeverity::Warning,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
            return false;
        }
        thread_count_ = count;
        return true;
    }

    bool EnableProfiling(bool enable) {
        profiling_enabled_ = enable;
        if (!neuropilot_handle_) {
            error_handler_->ReportError(
                "Cannot enable profiling - NeuroPilot not initialized",
                core::ErrorSeverity::Warning,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
            return false;
        }
        
        auto status = NeuroPilotSetProfiling(neuropilot_handle_, 
                                    enable ? NEUROPILOT_PROFILE_ENABLE : NEUROPILOT_PROFILE_DISABLE);
        if (status != NEUROPILOT_NO_ERROR) {
            error_handler_->ReportError(
                "Failed to set profiling mode",
                core::ErrorSeverity::Warning,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
            return false;
        }
        return true;
    }

    std::string GetLastErrorMessage() const {
        return error_handler_->GetSystemStatus();
    }

    int GetLastErrorCode() const {
        return last_error_code_;
    }

private:
    float GetPowerConsumption() const {
        if (!neuropilot_handle_) {
            return 0.0f;
        }

        NeuroPilotPowerStats power_stats{};
        auto status = NeuroPilotGetPowerStats(neuropilot_handle_, &power_stats);
        if (status != NEUROPILOT_NO_ERROR) {
            error_handler_->ReportError(
                "Failed to get power stats",
                core::ErrorSeverity::Warning,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
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
            error_handler_->ReportError(
                "Failed to get performance stats",
                core::ErrorSeverity::Warning,
                core::ErrorCategory::Hardware,
                "MTKAccelerator"
            );
            return 0.0f;
        }
        return perf_stats.utilization;
    }

    std::string current_power_profile_;
    float last_inference_time_ms_;
    int thread_count_;
    bool profiling_enabled_;
    NeuroPilotHandle neuropilot_handle_;
    std::unique_ptr<core::ErrorHandler> error_handler_;
    int last_error_code_;
};

MTKAccelerator::MTKAccelerator() : pImpl(std::make_unique<Impl>()) {}
MTKAccelerator::~MTKAccelerator() = default;

HardwareAccelerator::ErrorCode MTKAccelerator::Initialize() {
    return pImpl->Initialize();
}

bool MTKAccelerator::IsAvailable() const {
    return pImpl->IsAvailable();
}

HardwareAccelerator::ErrorCode MTKAccelerator::RunInference(
    const std::vector<float>& input,
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

HardwareAccelerator::ErrorCode MTKAccelerator::SetPowerProfile(PowerProfile profile) {
    return pImpl->SetPowerProfile(profile);
}

bool MTKAccelerator::SetThreadCount(int thread_count) {
    return pImpl->SetThreadCount(thread_count);
}

float MTKAccelerator::GetLastInferenceTime() const {
    return pImpl->GetLastInferenceTime();
}

bool MTKAccelerator::EnableProfiling(bool enable) {
    return pImpl->EnableProfiling(enable);
}

std::string MTKAccelerator::GetLastErrorMessage() const {
    return pImpl->GetLastErrorMessage();
}

int MTKAccelerator::GetLastErrorCode() const {
    return pImpl->GetLastErrorCode();
}

} // namespace hardware
} // namespace mobileai