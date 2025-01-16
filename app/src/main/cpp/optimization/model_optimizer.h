#pragma once

#include <memory>
#include <string>
#include <vector>
#include <map>

namespace mobileai {
namespace optimization {

enum class OptimizationType {
    QUANTIZATION,
    PRUNING,
    LAYER_FUSION,
    MEMORY_OPTIMIZATION
};

enum class QuantizationMode {
    INT8,
    FP16,
    DYNAMIC,
    PER_CHANNEL,
    PER_TENSOR
};

struct QuantizationConfig {
    QuantizationMode mode{QuantizationMode::INT8};
    bool calibrate{true};                    // Whether to run calibration
    size_t num_calibration_samples{100};     // Number of samples for calibration
    float scale_factor{1.0f};                // Global scale factor for quantization
    std::map<std::string, bool> layer_config;// Per-layer quantization configuration
    bool preserve_activation{true};          // Keep activations in higher precision
};

struct OptimizationConfig {
    OptimizationType type;
    float threshold{0.0f};                   // Threshold for pruning/quantization
    bool enable_fp16{false};                 // Enable FP16 optimization
    size_t target_memory_mb{0};             // Target memory usage in MB
    QuantizationConfig quant_config;         // Quantization-specific configuration
};

class ModelOptimizer {
public:
    ModelOptimizer() = default;
    virtual ~ModelOptimizer() = default;

    // Initialize optimizer with specific configuration
    virtual bool Initialize(const OptimizationConfig& config);

    // Apply optimization to the model
    virtual bool OptimizeModel(const std::string& model_path);

    // Get optimization statistics
    virtual float GetCompressionRatio() const;
    virtual size_t GetMemoryUsage() const;
    virtual float GetAccuracyDelta() const;

protected:
    OptimizationConfig config_;
    float compression_ratio_{1.0f};
    size_t memory_usage_{0};
    float accuracy_delta_{0.0f};
    std::string model_path_;

private:
    // Specific optimization methods
    virtual bool ApplyQuantization();
    virtual bool ApplyPruning();
    virtual bool ApplyLayerFusion();
    virtual bool OptimizeMemory();
};

// Factory function to create optimizer instance
std::unique_ptr<ModelOptimizer> CreateModelOptimizer();

} // namespace optimization
} // namespace mobileai 