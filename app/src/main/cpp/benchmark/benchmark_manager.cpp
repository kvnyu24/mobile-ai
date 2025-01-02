#include "benchmark_manager.h"
#include <android/log.h>
#include <fstream>
#include <chrono>
#include <thread>
#include <sstream>
#include <filesystem>
#include <json/json.h>
#include "../inference/model_engine.h"
#include "../hardware/hardware_accelerator.h"

using json = nlohmann::json;

namespace mobileai {
namespace benchmark {

// Define logging macros
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, "BenchmarkManager", __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, "BenchmarkManager", __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, "BenchmarkManager", __VA_ARGS__)

class BenchmarkManager::Impl {
public:
    Impl() : initialized_(false) {
        Initialize();
    }

    bool Initialize() {
        try {
            model_engine_ = std::make_unique<inference::ModelEngine>();
            initialized_ = true;
            return true;
        } catch (const std::exception& e) {
            LOGE("Failed to initialize: %s", e.what());
            return false;
        }
    }

    std::vector<BenchmarkResult> RunBenchmark(
            const std::string& model_path,
            const BenchmarkConfig& config,
            std::string* error_msg) {
        if (!initialized_) {
            if (error_msg) *error_msg = "BenchmarkManager not initialized";
            return {};
        }

        std::vector<BenchmarkResult> results;
        
        // Perform warm-up runs if enabled
        if (config.warm_up) {
            LOGI("Performing %d warm-up runs", config.warm_up_runs);
            for (int i = 0; i < config.warm_up_runs; i++) {
                RunSingleBenchmark(model_path, 1);
            }
        }

        // Run benchmarks for each batch size
        for (int batch_size : config.batch_sizes) {
            LOGI("Running benchmark with batch size %d", batch_size);
            
            for (int run = 0; run < config.num_runs; run++) {
                BenchmarkResult result;
                result.batch_size = batch_size;
                result.timestamp = std::chrono::system_clock::now();

                // Measure metrics
                if (auto time = MeasureInferenceTime(model_path, batch_size)) {
                    result.inference_time_ms = *time;
                }
                
                if (config.measure_memory) {
                    if (auto memory = MeasureMemoryUsage(model_path)) {
                        result.memory_usage_mb = *memory;
                    }
                }
                
                if (config.measure_power) {
                    if (auto power = MeasurePowerConsumption(model_path)) {
                        result.power_usage_mw = *power;
                    }
                }

                if (config.measure_thermal) {
                    result.thermal_throttling_percent = GetThermalThrottling();
                }

                if (config.measure_utilization) {
                    result.cpu_utilization_percent = GetCPUUtilization();
                    result.gpu_utilization_percent = GetGPUUtilization();
                }

                results.push_back(result);

                // Call progress callback if provided
                if (config.progress_callback) {
                    config.progress_callback(result);
                }

                // Check timeout
                if (config.timeout_ms) {
                    auto elapsed = std::chrono::system_clock::now() - result.timestamp;
                    if (elapsed > *config.timeout_ms) {
                        LOGW("Benchmark timed out after %d runs", run + 1);
                        break;
                    }
                }
            }
        }

        return results;
    }

    std::optional<double> MeasureInferenceTime(
            const std::string& model_path,
            int batch_size,
            std::string* error_msg) {
        try {
            auto start = std::chrono::high_resolution_clock::now();
            RunSingleBenchmark(model_path, batch_size);
            auto end = std::chrono::high_resolution_clock::now();
            
            std::chrono::duration<double, std::milli> duration = end - start;
            return duration.count();
        } catch (const std::exception& e) {
            if (error_msg) *error_msg = e.what();
            return std::nullopt;
        }
    }

    std::optional<double> MeasureMemoryUsage(
            const std::string& model_path,
            std::string* error_msg) {
        try {
            // Read /proc/self/status for memory info
            std::ifstream status("/proc/self/status");
            std::string line;
            double vm_size_kb = 0.0;
            
            while (std::getline(status, line)) {
                if (line.find("VmSize:") != std::string::npos) {
                    std::stringstream ss(line);
                    std::string dummy;
                    ss >> dummy >> vm_size_kb;
                    break;
                }
            }
            
            return vm_size_kb / 1024.0; // Convert to MB
        } catch (const std::exception& e) {
            if (error_msg) *error_msg = e.what();
            return std::nullopt;
        }
    }

    std::optional<double> MeasurePowerConsumption(
            const std::string& model_path,
            std::string* error_msg) {
        try {
            // Read from battery stats
            std::ifstream power_supply("/sys/class/power_supply/battery/power_now");
            double power_uw = 0.0;
            if (power_supply >> power_uw) {
                return power_uw / 1000.0; // Convert to mW
            }
            throw std::runtime_error("Could not read power consumption");
        } catch (const std::exception& e) {
            if (error_msg) *error_msg = e.what();
            return std::nullopt;
        }
    }

    std::vector<BenchmarkResult> CompareAccelerators(
            const std::string& model_path,
            const std::vector<std::string>& accelerator_types,
            std::string* error_msg) {
        std::vector<BenchmarkResult> results;
        
        for (const auto& acc_type : accelerator_types) {
            BenchmarkConfig config;
            config.num_runs = 10; // Reduced runs for comparison
            
            auto acc_results = RunBenchmark(model_path, config);
            for (auto& result : acc_results) {
                result.accelerator_type = acc_type;
                results.push_back(result);
            }
        }
        
        return results;
    }

    bool ExportResults(
            const std::string& output_path,
            const std::vector<BenchmarkResult>& results,
            const std::string& format,
            std::string* error_msg) {
        try {
            if (format == "json") {
                return ExportToJson(output_path, results);
            } else {
                if (error_msg) *error_msg = "Unsupported export format";
                return false;
            }
        } catch (const std::exception& e) {
            if (error_msg) *error_msg = e.what();
            return false;
        }
    }

    std::string GetSystemInfo() const {
        std::stringstream ss;
        ss << "System Information:\n";
        
        // CPU Info
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos ||
                line.find("processor") != std::string::npos) {
                ss << line << "\n";
            }
        }
        
        // Memory Info
        std::ifstream meminfo("/proc/meminfo");
        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal") != std::string::npos ||
                line.find("MemFree") != std::string::npos) {
                ss << line << "\n";
            }
        }
        
        return ss.str();
    }

    std::string GetThermalInfo() const {
        std::stringstream ss;
        ss << "Thermal Information:\n";
        
        // Read thermal zones
        for (int i = 0; i < 10; i++) {
            std::string path = "/sys/class/thermal/thermal_zone" + std::to_string(i) + "/temp";
            std::ifstream temp_file(path);
            if (!temp_file.good()) break;
            
            int temp;
            temp_file >> temp;
            ss << "Zone " << i << ": " << (temp/1000.0) << "°C\n";
        }
        
        return ss.str();
    }

    std::string GetPowerInfo() const {
        std::stringstream ss;
        ss << "Power Information:\n";
        
        // Battery info
        std::ifstream capacity("/sys/class/power_supply/battery/capacity");
        std::ifstream status("/sys/class/power_supply/battery/status");
        std::ifstream current("/sys/class/power_supply/battery/current_now");
        
        std::string bat_status;
        int bat_capacity, bat_current;
        
        if (capacity >> bat_capacity) {
            ss << "Battery Capacity: " << bat_capacity << "%\n";
        }
        if (status >> bat_status) {
            ss << "Battery Status: " << bat_status << "\n";
        }
        if (current >> bat_current) {
            ss << "Current Draw: " << (bat_current/1000.0) << "mA\n";
        }
        
        return ss.str();
    }

    void Reset() {
        initialized_ = false;
        Initialize();
    }

    bool IsInitialized() const {
        return initialized_;
    }

private:
    bool initialized_;
    std::unique_ptr<inference::ModelEngine> model_engine_;

    void RunSingleBenchmark(const std::string& model_path, int batch_size) {
        inference::ModelConfig config;
        config.max_batch_size = batch_size;
        
        if (!model_engine_->LoadModel(model_path, inference::ModelFormat::TFLITE, config)) {
            throw std::runtime_error("Failed to load model");
        }
        
        std::vector<float> input(1024, 1.0f); // Dummy input data
        std::vector<float> output;
        inference::InferenceMetrics metrics;
        
        if (!model_engine_->RunInference(input, output, &metrics)) {
            throw std::runtime_error("Failed to run inference");
        }
    }

    std::optional<double> GetThermalThrottling() {
        try {
            std::ifstream throttling("/sys/class/thermal/thermal_zone0/policy");
            std::string policy;
            if (throttling >> policy) {
                // Simple mapping of thermal policies to throttling percentages
                if (policy == "power_saving") return 75.0;
                if (policy == "balanced") return 25.0;
                if (policy == "performance") return 0.0;
            }
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }

    std::optional<double> GetCPUUtilization() {
        try {
            std::ifstream stat("/proc/stat");
            std::string line;
            std::getline(stat, line);
            
            std::stringstream ss(line);
            std::string cpu;
            long user, nice, system, idle, iowait, irq, softirq;
            ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;
            
            long total = user + nice + system + idle + iowait + irq + softirq;
            long active = total - idle - iowait;
            
            return (active * 100.0) / total;
        } catch (...) {
            return std::nullopt;
        }
    }

    std::optional<double> GetGPUUtilization() {
        try {
            // This is device-specific and may need to be adapted
            std::ifstream gpu_load("/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage");
            int utilization;
            if (gpu_load >> utilization) {
                return static_cast<double>(utilization);
            }
            return std::nullopt;
        } catch (...) {
            return std::nullopt;
        }
    }

    bool ExportToJson(const std::string& output_path,
                     const std::vector<BenchmarkResult>& results) {
        Json::Value root(Json::arrayValue);
        
        for (const auto& result : results) {
            Json::Value result_json;
            result_json["inference_time_ms"] = result.inference_time_ms;
            result_json["memory_usage_mb"] = result.memory_usage_mb;
            result_json["power_usage_mw"] = result.power_usage_mw;
            result_json["batch_size"] = result.batch_size;
            result_json["accelerator_type"] = result.accelerator_type;
            result_json["model_format"] = result.model_format;
            result_json["timestamp"] = static_cast<Json::UInt64>(
                std::chrono::system_clock::to_time_t(result.timestamp));
            
            if (result.thermal_throttling_percent) {
                result_json["thermal_throttling_percent"] = *result.thermal_throttling_percent;
            }
            if (result.cpu_utilization_percent) {
                result_json["cpu_utilization_percent"] = *result.cpu_utilization_percent;
            }
            if (result.gpu_utilization_percent) {
                result_json["gpu_utilization_percent"] = *result.gpu_utilization_percent;
            }
            
            root.append(result_json);
        }

        Json::StreamWriterBuilder writer;
        writer["indentation"] = "    ";
        std::ofstream file(output_path);
        if (!file.is_open()) {
            return false;
        }
        
        std::string document = Json::writeString(writer, root);
        file << document;
        return true;
    }
};

// Implementation of public interface
BenchmarkManager::BenchmarkManager() : pImpl(std::make_unique<Impl>()) {}
BenchmarkManager::~BenchmarkManager() = default;

std::vector<BenchmarkResult> BenchmarkManager::RunBenchmark(
        const std::string& model_path,
        const BenchmarkConfig& config,
        std::string* error_msg) {
    return pImpl->RunBenchmark(model_path, config, error_msg);
}

std::optional<double> BenchmarkManager::MeasureInferenceTime(
        const std::string& model_path,
        int batch_size,
        std::string* error_msg) {
    return pImpl->MeasureInferenceTime(model_path, batch_size, error_msg);
}

std::optional<double> BenchmarkManager::MeasureMemoryUsage(
        const std::string& model_path,
        std::string* error_msg) {
    return pImpl->MeasureMemoryUsage(model_path, error_msg);
}

std::optional<double> BenchmarkManager::MeasurePowerConsumption(
        const std::string& model_path,
        std::string* error_msg) {
    return pImpl->MeasurePowerConsumption(model_path, error_msg);
}

std::vector<BenchmarkResult> BenchmarkManager::CompareAccelerators(
        const std::string& model_path,
        const std::vector<std::string>& accelerator_types,
        std::string* error_msg) {
    return pImpl->CompareAccelerators(model_path, accelerator_types, error_msg);
}

bool BenchmarkManager::ExportResults(
        const std::string& output_path,
        const std::vector<BenchmarkResult>& results,
        const std::string& format,
        std::string* error_msg) {
    return pImpl->ExportResults(output_path, results, format, error_msg);
}

std::string BenchmarkManager::GetSystemInfo() const {
    return pImpl->GetSystemInfo();
}

std::string BenchmarkManager::GetThermalInfo() const {
    return pImpl->GetThermalInfo();
}

std::string BenchmarkManager::GetPowerInfo() const {
    return pImpl->GetPowerInfo();
}

void BenchmarkManager::Reset() {
    pImpl->Reset();
}

bool BenchmarkManager::IsInitialized() const {
    return pImpl->IsInitialized();
}

} // namespace benchmark
} // namespace mobileai
