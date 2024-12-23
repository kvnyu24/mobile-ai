#pragma once

#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace mobileai {
namespace monitoring {

struct PerformanceMetrics {
    double cpu_usage;
    double memory_usage;
    double gpu_usage;
    double power_consumption;
    double temperature;
    double network_bandwidth;
    std::chrono::system_clock::time_point timestamp;
};

struct MonitoringConfig {
    bool monitor_cpu = true;
    bool monitor_memory = true;
    bool monitor_gpu = true;
    bool monitor_power = true;
    bool monitor_temperature = true;
    bool monitor_network = true;
    std::chrono::milliseconds sampling_interval{1000};
    std::string log_file_path;
};

class PerformanceMonitor {
public:
    PerformanceMonitor();
    ~PerformanceMonitor();

    // Monitoring control
    void StartMonitoring(const MonitoringConfig& config);
    void StopMonitoring();
    bool IsMonitoring() const;

    // Real-time metrics
    PerformanceMetrics GetCurrentMetrics() const;
    std::vector<PerformanceMetrics> GetMetricsHistory() const;

    // Analysis
    double GetAverageCPUUsage() const;
    double GetPeakMemoryUsage() const;
    double GetAveragePowerConsumption() const;
    std::string GeneratePerformanceReport() const;

    // Alerts
    void SetCPUThreshold(double threshold);
    void SetMemoryThreshold(double threshold);
    void SetTemperatureThreshold(double threshold);
    void RegisterAlertCallback(std::function<void(const std::string&)> callback);

    // Export
    bool ExportMetrics(const std::string& path) const;
    bool ExportPerformanceReport(const std::string& path) const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};
}
} 