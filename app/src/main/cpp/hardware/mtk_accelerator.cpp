#include "mtk_accelerator.h"
#include <chrono>
#include <algorithm>
#include "core/error_handler.h"

#ifdef PLATFORM_ANDROID
#include <android/log.h>
#define LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "MTKAccelerator", __VA_ARGS__)

// Forward declare NeuroPilot types and functions
namespace neuropilot {
typedef void* Handle;
typedef struct Config {
    int version;
    int flags;
} Config;

typedef struct Buffer {
    void* data;
    size_t size;
} Buffer;

typedef struct HardwareInfo {
    bool apu_available;
} HardwareInfo;

typedef struct PowerConfig {
    int performance_mode;
} PowerConfig;

typedef struct PowerStats {
    float power_consumption;
} PowerStats;

typedef struct PerformanceStats {
    float utilization;
} PerformanceStats;

constexpr int VERSION_1 = 1;
constexpr int FLAG_NONE = 0;
constexpr int NO_ERROR = 0;
constexpr int POWER_SAVE = 0;
constexpr int BALANCED = 1;
constexpr int PERFORMANCE = 2;
constexpr int PROFILE_ENABLE = 1;
constexpr int PROFILE_DISABLE = 0;

// Forward declare NeuroPilot functions
extern "C" {
int Init(const Config* config, Handle* handle);
int Close(Handle handle);
int GetHardwareInfo(Handle handle, HardwareInfo* info);
int Execute(Handle handle, const Buffer* input, Buffer* output, int thread_count);
int SetPowerConfig(Handle handle, const PowerConfig* config);
int SetProfiling(Handle handle, int enable);
int GetPowerStats(Handle handle, PowerStats* stats);
int GetPerformanceStats(Handle handle, PerformanceStats* stats);
}
} // namespace neuropilot
#else
#include <iostream>
#define LOG_ERROR(...) fprintf(stderr, "MTKAccelerator: " __VA_ARGS__)
#endif

namespace mobileai {
namespace hardware {

class MTKAccelerator::Impl {
public:
    Impl() : current_power_profile_(PowerProfile::BALANCED),
             last_inference_time_ms_(0.0f),
             thread_count_(1),
             profiling_enabled_(false),
#ifdef PLATFORM_ANDROID
             neuropilot_handle_(nullptr),
#endif
             error_handler_(std::make_unique<core::ErrorHandler>()),
             last_error_code_(0) {}

    ~Impl() {
#ifdef PLATFORM_ANDROID
        if (neuropilot_handle_) {
            neuropilot::Close(neuropilot_handle_);
        }
#endif
    }

    HardwareAccelerator::ErrorCode Initialize() {
#ifdef PLATFORM_ANDROID
        neuropilot::Config config;
        config.version = neuropilot::VERSION_1;
        config.flags = neuropilot::FLAG_NONE;

        if (neuropilot::Init(&config, &neuropilot_handle_) != neuropilot::NO_ERROR) {
            LOG_ERROR("Failed to initialize NeuroPilot");
            return HardwareAccelerator::ErrorCode::INITIALIZATION_FAILED;
        }
#else
        LOG_ERROR("MTK accelerator not supported on this platform\n");
        return HardwareAccelerator::ErrorCode::INITIALIZATION_FAILED;
#endif
        return HardwareAccelerator::ErrorCode::SUCCESS;
    }

    bool IsAvailable() const {
#ifdef PLATFORM_ANDROID
        if (!neuropilot_handle_) {
            return false;
        }
        neuropilot::HardwareInfo info;
        if (neuropilot::GetHardwareInfo(neuropilot_handle_, &info) != neuropilot::NO_ERROR) {
            return false;
        }
        return info.apu_available;
#else
        return false;
#endif
    }

    std::vector<std::string> GetSupportedOperations() const {
        return {"CONV_2D", "DEPTHWISE_CONV_2D", "FULLY_CONNECTED"};
    }

    bool SupportsOperation(const std::string& operation) const {
        auto supported = GetSupportedOperations();
        return std::find(supported.begin(), supported.end(), operation) != supported.end();
    }

    HardwareAccelerator::ErrorCode RunInference(
        const std::vector<float>& input,
        std::vector<float>& output,
        HardwareAccelerator::PerformanceMetrics* metrics = nullptr) {
#ifdef PLATFORM_ANDROID
        if (!neuropilot_handle_) {
            return HardwareAccelerator::ErrorCode::NOT_INITIALIZED;
        }

        neuropilot::Buffer input_buffer;
        input_buffer.data = const_cast<float*>(input.data());
        input_buffer.size = input.size() * sizeof(float);

        output.resize(input.size()); // Assuming same size for simplicity
        neuropilot::Buffer output_buffer;
        output_buffer.data = output.data();
        output_buffer.size = output.size() * sizeof(float);

        auto start = std::chrono::high_resolution_clock::now();
        
        if (neuropilot::Execute(neuropilot_handle_, &input_buffer, &output_buffer, thread_count_) != neuropilot::NO_ERROR) {
            LOG_ERROR("Failed to execute inference");
            return HardwareAccelerator::ErrorCode::INFERENCE_ERROR;
        }

        auto end = std::chrono::high_resolution_clock::now();
        last_inference_time_ms_ = std::chrono::duration<float, std::milli>(end - start).count();

        if (metrics) {
            metrics->inference_time_ms = last_inference_time_ms_;
            metrics->power_consumption = GetPowerConsumption();
            metrics->utilization = GetUtilization();
        }
#else
        LOG_ERROR("MTK accelerator not supported on this platform\n");
        return HardwareAccelerator::ErrorCode::NOT_SUPPORTED;
#endif
        return HardwareAccelerator::ErrorCode::SUCCESS;
    }

    HardwareAccelerator::ErrorCode SetPowerProfile(HardwareAccelerator::PowerProfile profile) {
#ifdef PLATFORM_ANDROID
        if (!neuropilot_handle_) {
            return HardwareAccelerator::ErrorCode::NOT_INITIALIZED;
        }

        neuropilot::PowerConfig config;
        switch (profile) {
            case PowerProfile::POWER_SAVING:
                config.performance_mode = neuropilot::POWER_SAVE;
                break;
            case PowerProfile::BALANCED:
                config.performance_mode = neuropilot::BALANCED;
                break;
            case PowerProfile::HIGH_PERFORMANCE:
                config.performance_mode = neuropilot::PERFORMANCE;
                break;
            default:
                return HardwareAccelerator::ErrorCode::INVALID_ARGUMENT;
        }

        if (neuropilot::SetPowerConfig(neuropilot_handle_, &config) != neuropilot::NO_ERROR) {
            LOG_ERROR("Failed to set power profile");
            return HardwareAccelerator::ErrorCode::CONFIGURATION_ERROR;
        }

        current_power_profile_ = profile;
#else
        LOG_ERROR("MTK accelerator not supported on this platform\n");
        return HardwareAccelerator::ErrorCode::NOT_SUPPORTED;
#endif
        return HardwareAccelerator::ErrorCode::SUCCESS;
    }

    HardwareAccelerator::PowerProfile GetCurrentPowerProfile() const {
        return current_power_profile_;
    }

    float GetLastInferenceTime() const {
        return last_inference_time_ms_;
    }

    bool SetThreadCount(int count) {
        if (count < 1) {
            return false;
        }
        thread_count_ = count;
        return true;
    }

    bool EnableProfiling(bool enable) {
#ifdef PLATFORM_ANDROID
        if (!neuropilot_handle_) {
            return false;
        }

        if (neuropilot::SetProfiling(neuropilot_handle_, 
                                    enable ? neuropilot::PROFILE_ENABLE : neuropilot::PROFILE_DISABLE) 
            != neuropilot::NO_ERROR) {
            return false;
        }

        profiling_enabled_ = enable;
        return true;
#else
        return false;
#endif
    }

    std::string GetLastErrorMessage() const {
        return error_handler_->GetLastErrorMessage();
    }

    int GetLastErrorCode() const {
        return last_error_code_;
    }

private:
    float GetPowerConsumption() const {
#ifdef PLATFORM_ANDROID
        if (!neuropilot_handle_ || !profiling_enabled_) {
            return 0.0f;
        }

        neuropilot::PowerStats stats;
        if (neuropilot::GetPowerStats(neuropilot_handle_, &stats) != neuropilot::NO_ERROR) {
            return 0.0f;
        }
        return stats.power_consumption;
#else
        return 0.0f;
#endif
    }

    float GetUtilization() const {
#ifdef PLATFORM_ANDROID
        if (!neuropilot_handle_ || !profiling_enabled_) {
            return 0.0f;
        }

        neuropilot::PerformanceStats stats;
        if (neuropilot::GetPerformanceStats(neuropilot_handle_, &stats) != neuropilot::NO_ERROR) {
            return 0.0f;
        }
        return stats.utilization;
#else
        return 0.0f;
#endif
    }

    HardwareAccelerator::PowerProfile current_power_profile_;
    float last_inference_time_ms_;
    int thread_count_;
    bool profiling_enabled_;
#ifdef PLATFORM_ANDROID
    neuropilot::Handle neuropilot_handle_;
#endif
    std::unique_ptr<core::ErrorHandler> error_handler_;
    int last_error_code_;
};

// Public interface implementation
MTKAccelerator::MTKAccelerator() : impl_(std::make_unique<Impl>()) {}
MTKAccelerator::~MTKAccelerator() = default;

HardwareAccelerator::ErrorCode MTKAccelerator::Initialize() {
    return impl_->Initialize();
}

bool MTKAccelerator::IsAvailable() const {
    return impl_->IsAvailable();
}

std::vector<std::string> MTKAccelerator::GetSupportedOperations() const {
    return impl_->GetSupportedOperations();
}

bool MTKAccelerator::SupportsOperation(const std::string& operation) const {
    return impl_->SupportsOperation(operation);
}

HardwareAccelerator::ErrorCode MTKAccelerator::RunInference(
    const std::vector<float>& input,
    std::vector<float>& output,
    PerformanceMetrics* metrics) {
    return impl_->RunInference(input, output, metrics);
}

HardwareAccelerator::ErrorCode MTKAccelerator::SetPowerProfile(PowerProfile profile) {
    return impl_->SetPowerProfile(profile);
}

HardwareAccelerator::PowerProfile MTKAccelerator::GetCurrentPowerProfile() const {
    return impl_->GetCurrentPowerProfile();
}

float MTKAccelerator::GetLastInferenceTime() const {
    return impl_->GetLastInferenceTime();
}

bool MTKAccelerator::SetThreadCount(int thread_count) {
    return impl_->SetThreadCount(thread_count);
}

bool MTKAccelerator::EnableProfiling(bool enable) {
    return impl_->EnableProfiling(enable);
}

std::string MTKAccelerator::GetLastErrorMessage() const {
    return impl_->GetLastErrorMessage();
}

int MTKAccelerator::GetLastErrorCode() const {
    return impl_->GetLastErrorCode();
}

} // namespace hardware
} // namespace mobileai