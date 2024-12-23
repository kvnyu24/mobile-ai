#pragma once

#include "../hardware/hardware_accelerator.h"
#include <memory>
#include <string>
#include <vector>

namespace mobileai {
namespace inference {

enum class ModelFormat {
    TFLITE,
    PYTORCH,
    ONNX
};

class ModelEngine {
public:
    ModelEngine();
    ~ModelEngine();

    // Initialize with specific hardware accelerator
    bool Initialize(std::unique_ptr<hardware::HardwareAccelerator> accelerator);

    // Load model from file
    bool LoadModel(const std::string& model_path, ModelFormat format);

    // Run inference
    bool RunInference(const std::vector<float>& input, std::vector<float>& output);

    // Get model information
    std::string GetModelInfo() const;
    
    // Set thread count for CPU fallback
    void SetNumThreads(int num_threads);

    // Enable/disable hardware acceleration
    void EnableHardwareAcceleration(bool enable);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace inference
} // namespace mobileai 