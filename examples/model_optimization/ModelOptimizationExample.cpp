#include "../../app/src/main/cpp/optimization/model_optimizer.h"
#include <iostream>

using namespace mobileai::optimization;

int main() {
    // Create model optimizer instance
    auto optimizer = CreateModelOptimizer();

    // Configure INT8 quantization
    OptimizationConfig config;
    config.type = OptimizationType::QUANTIZATION;
    
    // Configure quantization parameters
    config.quant_config.mode = QuantizationMode::INT8;
    config.quant_config.calibrate = true;
    config.quant_config.num_calibration_samples = 100;
    config.quant_config.scale_factor = 1.0f;
    
    // Configure specific layers for quantization
    config.quant_config.layer_config["conv1"] = true;
    config.quant_config.layer_config["conv2"] = true;
    config.quant_config.layer_config["fc1"] = false;  // Skip quantization for this layer

    // Initialize optimizer
    if (!optimizer->Initialize(config)) {
        std::cerr << "Failed to initialize optimizer" << std::endl;
        return 1;
    }

    // Apply optimization to model
    const std::string model_path = "path/to/your/model.tflite";
    if (!optimizer->OptimizeModel(model_path)) {
        std::cerr << "Failed to optimize model" << std::endl;
        return 1;
    }

    // Get optimization results
    std::cout << "Optimization completed successfully!" << std::endl;
    std::cout << "Compression ratio: " << optimizer->GetCompressionRatio() << std::endl;
    std::cout << "Memory usage (MB): " << optimizer->GetMemoryUsage() << std::endl;
    std::cout << "Accuracy impact (%): " << optimizer->GetAccuracyDelta() << std::endl;

    return 0;
} 