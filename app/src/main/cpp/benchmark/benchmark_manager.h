#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace mobileai {
namespace benchmark {

struct BenchmarkResult {
    double inference_time_ms;
    double memory_usage_mb;
    double power_usage_mw;
    std::string accelerator_type;
    std::string model_format;
    int batch_size;
    std::vector<std::string> enabled_optimizations;
};

struct BenchmarkConfig {
    int num_runs = 100;
    bool warm_up = true;
    int warm_up_runs = 10;
    bool measure_power = true;
    bool measure_memory = true;
    std::vector<int> batch_sizes = {1, 4, 8, 16};
};

class BenchmarkManager {
public:
    BenchmarkManager();
    ~BenchmarkManager();

    // Run comprehensive benchmarks
    std::vector<BenchmarkResult> RunBenchmark(
        const std::string& model_path,
        const BenchmarkConfig& config);

    // Individual metrics
    double MeasureInferenceTime(const std::string& model_path, int batch_size);
    double MeasureMemoryUsage(const std::string& model_path);
    double MeasurePowerConsumption(const std::string& model_path);

    // Compare accelerators
    std::vector<BenchmarkResult> CompareAccelerators(
        const std::string& model_path,
        const std::vector<std::string>& accelerator_types);

    // Export results
    bool ExportResults(const std::string& output_path,
                      const std::vector<BenchmarkResult>& results);

    // System information
    std::string GetSystemInfo() const;
    std::string GetThermalInfo() const;
    std::string GetPowerInfo() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
}; 