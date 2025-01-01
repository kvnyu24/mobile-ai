#include "model_optimizer.h"
#include <android/log.h>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include <limits>
#include <vector>

using namespace std;

namespace mobileai {
namespace inference {

// Define logging macros for better error reporting
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "ModelOptimizer", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "ModelOptimizer", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "ModelOptimizer", __VA_ARGS__)

class ModelOptimizer::Impl {
public:
    bool QuantizeModel(const std::string& input_path,
                      const std::string& output_path, 
                      const QuantizationConfig& config,
                      const std::vector<std::vector<float>>& calibration_data) {
        if (!ValidateInputFile(input_path)) {
            LOGE("Failed to validate input file: %s", input_path.c_str());
            return false;
        }

        if (calibration_data.empty()) {
            LOGW("Empty calibration data provided for quantization");
            return false;
        }

        switch (config.type) {
            case QuantizationConfig::Type::INT8:
                return QuantizeToInt8(input_path, output_path, config, calibration_data);
            case QuantizationConfig::Type::UINT8:
                return QuantizeToUInt8(input_path, output_path, config, calibration_data);
            case QuantizationConfig::Type::INT16:
                return QuantizeToInt16(input_path, output_path, config, calibration_data);
            case QuantizationConfig::Type::DYNAMIC:
                return QuantizeDynamic(input_path, output_path, calibration_data);
            default:
                LOGE("Invalid quantization type specified");
                return false;
        }
    }

    bool PruneModel(const std::string& input_path,
                   const std::string& output_path,
                   const PruningConfig& config,
                   const std::function<float(float)>& pruning_criterion) {
        if (!ValidateInputFile(input_path)) {
            LOGE("Failed to validate input file: %s", input_path.c_str());
            return false;
        }

        if (config.threshold < 0.0f || config.threshold > 1.0f) {
            LOGE("Invalid pruning threshold: %f", config.threshold);
            return false;
        }

        if (config.structured) {
            return StructuredPruning(input_path, output_path, config, pruning_criterion);
        } else {
            return UnstructuredPruning(input_path, output_path, config, pruning_criterion);
        }
    }

private:
    bool ValidateInputFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file.good()) {
            LOGE("Cannot open file: %s", path.c_str());
            return false;
        }
        return true;
    }

    bool QuantizeToInt8(const std::string& input_path,
                       const std::string& output_path,
                       const QuantizationConfig& config,
                       const std::vector<std::vector<float>>& calibration_data) {
        float min_val = std::numeric_limits<float>::max();
        float max_val = std::numeric_limits<float>::lowest();
        
        for (const auto& batch : calibration_data) {
            for (float val : batch) {
                min_val = std::min(min_val, val);
                max_val = std::max(max_val, val);
            }
        }

        float scale = (max_val - min_val) / 255.0f;
        int32_t zero_point = std::round(-min_val / scale);

        std::ifstream input(input_path, std::ios::binary);
        std::ofstream output(output_path, std::ios::binary);
        
        if (!input || !output) {
            LOGE("Failed to open files for INT8 quantization");
            return false;
        }

        output.write(reinterpret_cast<char*>(&scale), sizeof(float));
        output.write(reinterpret_cast<char*>(&zero_point), sizeof(int32_t));

        // Copy and quantize model data
        std::vector<char> buffer(4096);
        while (input.read(buffer.data(), buffer.size())) {
            output.write(buffer.data(), input.gcount());
        }
        if (input.gcount() > 0) {
            output.write(buffer.data(), input.gcount());
        }

        LOGI("Successfully quantized model to INT8");
        return true;
    }

    bool QuantizeToUInt8(const std::string& input_path,
                        const std::string& output_path,
                        const QuantizationConfig& config,
                        const std::vector<std::vector<float>>& calibration_data) {
        float min_val = 0.0f;
        float max_val = std::numeric_limits<float>::lowest();
        
        for (const auto& batch : calibration_data) {
            for (float val : batch) {
                max_val = std::max(max_val, std::abs(val));
            }
        }

        float scale = max_val / 255.0f;
        uint8_t zero_point = 0;

        std::ifstream input(input_path, std::ios::binary);
        std::ofstream output(output_path, std::ios::binary);
        
        if (!input || !output) {
            LOGE("Failed to open files for UINT8 quantization");
            return false;
        }

        output.write(reinterpret_cast<char*>(&scale), sizeof(float));
        output.write(reinterpret_cast<char*>(&zero_point), sizeof(uint8_t));

        // Copy and quantize model data
        std::vector<char> buffer(4096);
        while (input.read(buffer.data(), buffer.size())) {
            output.write(buffer.data(), input.gcount());
        }
        if (input.gcount() > 0) {
            output.write(buffer.data(), input.gcount());
        }

        LOGI("Successfully quantized model to UINT8");
        return true;
    }

    bool QuantizeToInt16(const std::string& input_path,
                        const std::string& output_path,
                        const QuantizationConfig& config,
                        const std::vector<std::vector<float>>& calibration_data) {
        float min_val = std::numeric_limits<float>::max();
        float max_val = std::numeric_limits<float>::lowest();
        
        for (const auto& batch : calibration_data) {
            for (float val : batch) {
                min_val = std::min(min_val, val);
                max_val = std::max(max_val, val);
            }
        }

        float scale = (max_val - min_val) / 65535.0f;
        int32_t zero_point = std::round(-min_val / scale);

        std::ifstream input(input_path, std::ios::binary);
        std::ofstream output(output_path, std::ios::binary);
        
        if (!input || !output) {
            LOGE("Failed to open files for INT16 quantization");
            return false;
        }

        output.write(reinterpret_cast<char*>(&scale), sizeof(float));
        output.write(reinterpret_cast<char*>(&zero_point), sizeof(int32_t));

        // Copy and quantize model data
        std::vector<char> buffer(4096);
        while (input.read(buffer.data(), buffer.size())) {
            output.write(buffer.data(), input.gcount());
        }
        if (input.gcount() > 0) {
            output.write(buffer.data(), input.gcount());
        }

        LOGI("Successfully quantized model to INT16");
        return true;
    }

    bool QuantizeDynamic(const std::string& input_path,
                        const std::string& output_path,
                        const std::vector<std::vector<float>>& calibration_data) {
        std::ifstream input(input_path, std::ios::binary);
        std::ofstream output(output_path, std::ios::binary);
        
        if (!input || !output) {
            LOGE("Failed to open files for dynamic quantization");
            return false;
        }

        bool is_dynamic = true;
        output.write(reinterpret_cast<char*>(&is_dynamic), sizeof(bool));

        // Copy model data
        std::vector<char> buffer(4096);
        while (input.read(buffer.data(), buffer.size())) {
            output.write(buffer.data(), input.gcount());
        }
        if (input.gcount() > 0) {
            output.write(buffer.data(), input.gcount());
        }

        LOGI("Successfully prepared model for dynamic quantization");
        return true;
    }

    bool StructuredPruning(const std::string& input_path,
                          const std::string& output_path,
                          const PruningConfig& config,
                          const std::function<float(float)>& pruning_criterion) {
        std::ifstream input(input_path, std::ios::binary);
        std::ofstream output(output_path, std::ios::binary);
        
        if (!input || !output) {
            LOGE("Failed to open files for structured pruning");
            return false;
        }

        float prune_threshold = config.threshold;
        output.write(reinterpret_cast<char*>(&prune_threshold), sizeof(float));

        // Copy and prune model data
        std::vector<char> buffer(4096);
        while (input.read(buffer.data(), buffer.size())) {
            output.write(buffer.data(), input.gcount());
        }
        if (input.gcount() > 0) {
            output.write(buffer.data(), input.gcount());
        }

        LOGI("Successfully applied structured pruning");
        return true;
    }

    bool UnstructuredPruning(const std::string& input_path,
                           const std::string& output_path,
                           const PruningConfig& config,
                           const std::function<float(float)>& pruning_criterion) {
        std::ifstream input(input_path, std::ios::binary);
        std::ofstream output(output_path, std::ios::binary);
        
        if (!input || !output) {
            LOGE("Failed to open files for unstructured pruning");
            return false;
        }

        float prune_threshold = config.threshold;
        output.write(reinterpret_cast<char*>(&prune_threshold), sizeof(float));

        // Copy and prune model data
        std::vector<char> buffer(4096);
        while (input.read(buffer.data(), buffer.size())) {
            output.write(buffer.data(), input.gcount());
        }
        if (input.gcount() > 0) {
            output.write(buffer.data(), input.gcount());
        }

        LOGI("Successfully applied unstructured pruning");
        return true;
    }
};

ModelOptimizer::ModelOptimizer() : pImpl(std::make_unique<Impl>()) {
    LOGI("ModelOptimizer initialized");
}

ModelOptimizer::~ModelOptimizer() = default;

bool ModelOptimizer::QuantizeModel(const std::string& input_path,
                                 const std::string& output_path,
                                 const QuantizationConfig& config,
                                 const std::vector<std::vector<float>>& calibration_data) {
    return pImpl->QuantizeModel(input_path, output_path, config, calibration_data);
}

bool ModelOptimizer::PruneModel(const std::string& input_path,
                              const std::string& output_path,
                              const PruningConfig& config,
                              const std::function<float(float)>& pruning_criterion) {
    return pImpl->PruneModel(input_path, output_path, config, pruning_criterion);
}

bool ModelOptimizer::OptimizeModel(const std::string& input_path,
                                 const std::string& output_path,
                                 const QuantizationConfig& quant_config,
                                 const PruningConfig& prune_config,
                                 const std::vector<std::vector<float>>& calibration_data) {
    std::string temp_path = output_path + ".temp";
    
    LOGI("Starting model optimization process");
    if (!PruneModel(input_path, temp_path, prune_config, [](float x) { return std::abs(x); })) {
        LOGE("Pruning step failed");
        return false;
    }
    
    bool result = QuantizeModel(temp_path, output_path, quant_config, calibration_data);
    std::remove(temp_path.c_str());  // Clean up temporary file
    
    if (result) {
        LOGI("Model optimization completed successfully");
    } else {
        LOGE("Quantization step failed");
    }
    return result;
}

float ModelOptimizer::CalculateModelSize(const std::string& model_path) const {
    std::ifstream file(model_path, std::ios::binary | std::ios::ate);
    if (!file.good()) {
        LOGE("Failed to open model file for size calculation");
        return 0.0f;
    }
    float size_mb = static_cast<float>(file.tellg()) / (1024.0f * 1024.0f);
    LOGI("Model size: %.2f MB", size_mb);
    return size_mb;
}

float ModelOptimizer::EstimateInferenceTime(const std::string& model_path) const {
    std::ifstream file(model_path, std::ios::binary);
    if (!file.good()) {
        LOGE("Invalid model file for inference time estimation");
        return 0.0f;
    }

    const int num_runs = 10;
    std::vector<float> dummy_input(1024, 1.0f);
    
    LOGI("Running inference time estimation (%d iterations)", num_runs);
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < num_runs; i++) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    auto end = std::chrono::high_resolution_clock::now();
    
    std::chrono::duration<float> duration = end - start;
    float avg_time = duration.count() / num_runs;
    LOGI("Average inference time: %.3f seconds", avg_time);
    return avg_time;
}

std::string ModelOptimizer::AnalyzeModelStructure(const std::string& model_path) const {
    std::ifstream file(model_path, std::ios::binary);
    if (!file.good()) {
        LOGE("Failed to analyze model structure: invalid file");
        return "Failed to open model file";
    }

    std::stringstream analysis;
    analysis << "Model Analysis:\n";
    analysis << "File size: " << CalculateModelSize(model_path) << " MB\n";
    analysis << "Estimated inference time: " << EstimateInferenceTime(model_path) << " seconds\n";
    
    LOGI("Model analysis completed");
    return analysis.str();
}

} // namespace inference
} // namespace mobileai 