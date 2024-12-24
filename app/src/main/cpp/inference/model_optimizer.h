#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>

namespace mobileai {
namespace inference {

struct QuantizationConfig {
    enum class Type {
        INT8,
        UINT8,
        INT16,
        DYNAMIC
    };

    Type type = Type::INT8;
    bool per_channel = false;
    float scale = 1.0f;
    int zero_point = 0;
};

struct PruningConfig {
    float sparsity_target = 0.5f;
    float threshold = 0.001f;
    bool structured = false;
    std::string pruning_schedule = "polynomial_decay";
};

class ModelOptimizer {
public:
    ModelOptimizer();
    ~ModelOptimizer();

    // Quantization
    bool QuantizeModel(const std::string& input_path,
                      const std::string& output_path,
                      const QuantizationConfig& config,
                      const std::vector<std::vector<float>>& calibration_data);

    // Pruning
    bool PruneModel(const std::string& input_path,
                   const std::string& output_path,
                   const PruningConfig& config,
                   std::function<float(float)> pruning_criterion);

    // Combined optimization
    bool OptimizeModel(const std::string& input_path,
                      const std::string& output_path,
                      const QuantizationConfig& quant_config,
                      const PruningConfig& prune_config,
                      const std::vector<std::vector<float>>& calibration_data);

    // Model analysis
    float CalculateModelSize(const std::string& model_path);
    float EstimateInferenceTime(const std::string& model_path);
    std::string AnalyzeModelStructure(const std::string& model_path);

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
}; 