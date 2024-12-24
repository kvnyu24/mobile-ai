#include "benchmark_manager.h"
#include <android/log.h>
#include <fstream>
#include <json/json.h>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <chrono>
#include <thread>
#include <filesystem>
#include <sys/utsname.h>
#include <sys/types.h>
#include <unistd.h>

namespace mobileai {
namespace benchmark {

class BenchmarkManager::Impl {
public:
    std::vector<BenchmarkResult> RunBenchmark(
        const std::string& model_path,
        const BenchmarkConfig& config) {
        std::vector<BenchmarkResult> results;

        // Perform warm-up runs if enabled
        if (config.warm_up) {
            for (int i = 0; i < config.warm_up_runs; i++) {
                MeasureInferenceTime(model_path, 1);
            }
        }

        // Run benchmarks for each batch size
        for (int batch_size : config.batch_sizes) {
            BenchmarkResult result;
            result.batch_size = batch_size;
            
            // Measure inference time
            double total_time = 0;
            for (int i = 0; i < config.num_runs; i++) {
                total_time += MeasureInferenceTime(model_path, batch_size);
            }
            result.inference_time_ms = total_time / config.num_runs;

            // Measure memory if enabled
            if (config.measure_memory) {
                result.memory_usage_mb = MeasureMemoryUsage(model_path);
            }

            // Measure power if enabled
            if (config.measure_power) {
                result.power_usage_mw = MeasurePowerConsumption(model_path);
            }

            results.push_back(result);
        }

        return results;
    }

    double MeasureInferenceTime(const std::string& model_path, int batch_size) {
        auto start = std::chrono::high_resolution_clock::now();
        
        // Load model and perform inference
        // Note: This is a placeholder implementation - actual inference would use ML framework
        std::ifstream model_file(model_path, std::ios::binary);
        if (model_file.is_open()) {
            model_file.seekg(0, std::ios::end);
            size_t size = model_file.tellg();
            std::vector<char> buffer(size);
            model_file.seekg(0);
            model_file.read(buffer.data(), size);
            
            // Simulate inference time based on model size and batch size
            std::this_thread::sleep_for(std::chrono::milliseconds(size / (1024 * 1024) * batch_size));
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double, std::milli>(end - start).count();
    }

    double MeasureMemoryUsage(const std::string& model_path) {
        // Get process memory info
        std::string status_file = "/proc/self/status";
        std::ifstream status(status_file);
        std::string line;
        double vm_size = 0.0;
        
        while (std::getline(status, line)) {
            if (line.find("VmRSS:") != std::string::npos) {
                std::istringstream iss(line);
                std::string label;
                double kb;
                iss >> label >> kb;
                vm_size = kb / 1024.0; // Convert KB to MB
                break;
            }
        }
        
        return vm_size;
    }

    double MeasurePowerConsumption(const std::string& model_path) {
        // Read battery stats from sysfs
        std::string power_file = "/sys/class/power_supply/battery/power_now";
        std::ifstream power(power_file);
        double power_usage = 0.0;
        
        if (power.is_open()) {
            power >> power_usage;
            power_usage /= 1000.0; // Convert to milliwatts
        }
        
        return power_usage;
    }

    bool ExportResults(const std::string& output_path,
                      const std::vector<BenchmarkResult>& results) {
        Json::Value root;
        root["timestamp"] = Json::Value::Int64(std::time(nullptr));
        root["system_info"] = GetSystemInfo();
        
        Json::Value benchmarks(Json::arrayValue);
        for (const auto& result : results) {
            Json::Value benchmark;
            benchmark["inference_time_ms"] = result.inference_time_ms;
            benchmark["memory_usage_mb"] = result.memory_usage_mb;
            benchmark["power_usage_mw"] = result.power_usage_mw;
            benchmark["accelerator_type"] = result.accelerator_type;
            benchmark["model_format"] = result.model_format;
            benchmark["batch_size"] = result.batch_size;
            
            Json::Value optimizations(Json::arrayValue);
            for (const auto& opt : result.enabled_optimizations) {
                optimizations.append(opt);
            }
            benchmark["enabled_optimizations"] = optimizations;
            
            benchmarks.append(benchmark);
        }
        root["benchmarks"] = benchmarks;

        Json::StreamWriterBuilder writer;
        std::ofstream file(output_path);
        return file.is_open() ? (file << Json::writeString(writer, root), true) : false;
    }

    std::string GetSystemInfo() const {
        struct utsname system_info;
        uname(&system_info);
        
        Json::Value info;
        info["sysname"] = system_info.sysname;
        info["nodename"] = system_info.nodename;
        info["release"] = system_info.release;
        info["version"] = system_info.version;
        info["machine"] = system_info.machine;
        
        struct sysinfo si;
        if (sysinfo(&si) == 0) {
            info["total_ram_mb"] = (si.totalram * si.mem_unit) / (1024 * 1024);
            info["free_ram_mb"] = (si.freeram * si.mem_unit) / (1024 * 1024);
            info["procs"] = si.procs;
        }
        
        Json::StreamWriterBuilder writer;
        return Json::writeString(writer, info);
    }

    std::string GetThermalInfo() const {
        std::string thermal_info;
        const std::string thermal_path = "/sys/class/thermal/";
        
        for (const auto& entry : std::filesystem::directory_iterator(thermal_path)) {
            if (entry.path().filename().string().find("thermal_zone") != std::string::npos) {
                std::ifstream temp_file(entry.path() / "temp");
                std::ifstream type_file(entry.path() / "type");
                std::string temp, type;
                
                if (type_file >> type && temp_file >> temp) {
                    thermal_info += type + ": " + 
                        std::to_string(std::stoi(temp) / 1000.0) + "°C\n";
                }
            }
        }
        
        return thermal_info;
    }

    std::string GetPowerInfo() const {
        Json::Value power_info;
        const std::string power_path = "/sys/class/power_supply/battery/";
        
        std::vector<std::string> stats = {
            "status", "capacity", "voltage_now", "current_now", 
            "temp", "technology", "health"
        };
        
        for (const auto& stat : stats) {
            std::ifstream file(power_path + stat);
            std::string value;
            if (file >> value) {
                power_info[stat] = value;
            }
        }
        
        Json::StreamWriterBuilder writer;
        return Json::writeString(writer, power_info);
    }
};

BenchmarkManager::BenchmarkManager() : pImpl(std::make_unique<Impl>()) {}
BenchmarkManager::~BenchmarkManager() = default;

std::vector<BenchmarkResult> BenchmarkManager::RunBenchmark(
    const std::string& model_path,
    const BenchmarkConfig& config) {
    return pImpl->RunBenchmark(model_path, config);
}

double BenchmarkManager::MeasureInferenceTime(const std::string& model_path, int batch_size) {
    return pImpl->MeasureInferenceTime(model_path, batch_size);
}

double BenchmarkManager::MeasureMemoryUsage(const std::string& model_path) {
    return pImpl->MeasureMemoryUsage(model_path);
}

double BenchmarkManager::MeasurePowerConsumption(const std::string& model_path) {
    return pImpl->MeasurePowerConsumption(model_path);
}

bool BenchmarkManager::ExportResults(const std::string& output_path,
                                   const std::vector<BenchmarkResult>& results) {
    return pImpl->ExportResults(output_path, results);
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

} // namespace benchmark
} // namespace mobileai 