#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <functional>
#include <optional>

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
    std::optional<double> thermal_throttling_percent;
    std::optional<double> cpu_utilization_percent;
    std::optional<double> gpu_utilization_percent;
    std::chrono::system_clock::time_point timestamp;
};

struct BenchmarkConfig {
    int num_runs = 100;
    bool warm_up = true;
    int warm_up_runs = 10;
    bool measure_power = true;
    bool measure_memory = true;
    bool measure_thermal = true;
    bool measure_utilization = true;
    std::vector<int> batch_sizes = {1, 4, 8, 16};
    std::optional<std::chrono::milliseconds> timeout_ms;
    std::function<void(const BenchmarkResult&)> progress_callback;
};

class BenchmarkManager {
public:
    BenchmarkManager();
    ~BenchmarkManager();

    // Run comprehensive benchmarks with error handling
    std::vector<BenchmarkResult> RunBenchmark(
        const std::string& model_path,
        const BenchmarkConfig& config,
        std::string* error_msg = nullptr);

    // Individual metrics with error handling
    std::optional<double> MeasureInferenceTime(
        const std::string& model_path, 
        int batch_size,
        std::string* error_msg = nullptr);
    std::optional<double> MeasureMemoryUsage(
        const std::string& model_path,
        std::string* error_msg = nullptr);
    std::optional<double> MeasurePowerConsumption(
        const std::string& model_path,
        std::string* error_msg = nullptr);

    // Compare accelerators with error handling
    std::vector<BenchmarkResult> CompareAccelerators(
        const std::string& model_path,
        const std::vector<std::string>& accelerator_types,
        std::string* error_msg = nullptr);

    // Export results with format options
    bool ExportResults(
        const std::string& output_path,
        const std::vector<BenchmarkResult>& results,
        const std::string& format = "json",
        std::string* error_msg = nullptr);

    // System information with caching
    std::string GetSystemInfo() const;
    std::string GetThermalInfo() const;
    std::string GetPowerInfo() const;

    // Reset internal state
    void Reset();

    // Check if benchmark manager is initialized properly
    bool IsInitialized() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
}; 