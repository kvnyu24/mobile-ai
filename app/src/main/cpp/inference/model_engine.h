#pragma once

#include "../hardware/hardware_accelerator.h"
#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <chrono>

namespace mobileai {
namespace inference {

enum class ModelFormat {
    TFLITE,
    PYTORCH,
    ONNX,
    CUSTOM
};

// Model configuration options
struct ModelConfig {
    bool enable_optimization = true;
    bool enable_caching = false;
    size_t max_batch_size = 1;
    std::string custom_options;
};

// Performance metrics for inference
struct InferenceMetrics {
    float inference_time_ms;
    float memory_usage_mb;
    float cpu_usage_percent;
    float gpu_usage_percent;
};

class ModelEngine {
public:
    ModelEngine();
    ~ModelEngine();

    // Initialize with specific hardware accelerator
    bool Initialize(std::unique_ptr<hardware::HardwareAccelerator> accelerator);

    // Load model with configuration options
    bool LoadModel(const std::string& model_path, 
                  ModelFormat format,
                  const ModelConfig& config = ModelConfig());

    // Run inference with performance metrics
    bool RunInference(const std::vector<float>& input, 
                     std::vector<float>& output,
                     InferenceMetrics* metrics = nullptr);

    // Run batch inference
    bool RunBatchInference(const std::vector<std::vector<float>>& inputs,
                          std::vector<std::vector<float>>& outputs,
                          InferenceMetrics* metrics = nullptr);

    // Get model information
    std::string GetModelInfo() const;
    std::vector<std::string> GetSupportedOperations() const;
    std::vector<std::pair<size_t, size_t>> GetInputShapes() const;
    std::vector<std::pair<size_t, size_t>> GetOutputShapes() const;
    
    // Performance and resource management
    void SetNumThreads(int num_threads);
    void EnableHardwareAcceleration(bool enable);
    void SetPowerProfile(hardware::HardwareAccelerator::PowerProfile profile);
    void SetMemoryLimit(size_t memory_mb);
    
    // Error handling and callbacks
    using ErrorCallback = std::function<void(hardware::HardwareAccelerator::ErrorCode, const std::string&)>;
    void SetErrorCallback(ErrorCallback callback);
    hardware::HardwareAccelerator::ErrorCode GetLastError() const;

    // Model optimization
    bool OptimizeModel(const std::string& output_path);
    bool QuantizeModel(const std::string& output_path, bool dynamic = false);
    
    // Resource management
    void ReleaseResources();
    bool WarmUp(size_t num_runs = 3);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace inference
} // namespace mobileai