#include "performance_monitor.h"
#include <android/log.h>
#include <thread>
#include <mutex>
#include <fstream>
#include <json/json.h>
#include <numeric>
#include <sys/sysinfo.h>
#include <sys/stat.h>
#include <fstream>
#include <string>
#include <vector>

namespace mobileai {
namespace monitoring {

class PerformanceMonitor::Impl {
public:
    Impl() : monitoring_(false), 
             cpu_threshold_(80.0),
             memory_threshold_(90.0),
             temperature_threshold_(80.0) {}

    ~Impl() {
        StopMonitoring();
    }

    void StartMonitoring(const MonitoringConfig& config) {
        if (monitoring_) return;

        config_ = config;
        monitoring_ = true;
        monitor_thread_ = std::thread(&Impl::MonitoringLoop, this);
    }

    void StopMonitoring() {
        if (!monitoring_) return;

        monitoring_ = false;
        if (monitor_thread_.joinable()) {
            monitor_thread_.join();
        }
    }

    bool IsMonitoring() const {
        return monitoring_;
    }

    PerformanceMetrics GetCurrentMetrics() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return current_metrics_;
    }

    std::vector<PerformanceMetrics> GetMetricsHistory() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return metrics_history_;
    }

    double GetAverageCPUUsage() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_history_.empty()) return 0.0;
        
        double sum = std::accumulate(metrics_history_.begin(),
                                   metrics_history_.end(),
                                   0.0,
                                   [](double acc, const PerformanceMetrics& m) {
                                       return acc + m.cpu_usage;
                                   });
        return sum / metrics_history_.size();
    }

    double GetPeakMemoryUsage() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_history_.empty()) return 0.0;
        
        return std::max_element(metrics_history_.begin(),
                              metrics_history_.end(),
                              [](const PerformanceMetrics& a, const PerformanceMetrics& b) {
                                  return a.memory_usage < b.memory_usage;
                              })->memory_usage;
    }

    double GetAveragePowerConsumption() const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (metrics_history_.empty()) return 0.0;
        
        double sum = std::accumulate(metrics_history_.begin(),
                                   metrics_history_.end(),
                                   0.0,
                                   [](double acc, const PerformanceMetrics& m) {
                                       return acc + m.power_consumption;
                                   });
        return sum / metrics_history_.size();
    }

    std::string GeneratePerformanceReport() const {
        Json::Value report;
        report["average_cpu_usage"] = GetAverageCPUUsage();
        report["peak_memory_usage"] = GetPeakMemoryUsage();
        report["average_power_consumption"] = GetAveragePowerConsumption();
        report["total_samples"] = static_cast<int>(metrics_history_.size());
        
        Json::StreamWriterBuilder writer;
        return Json::writeString(writer, report);
    }

    void SetCPUThreshold(double threshold) {
        cpu_threshold_ = threshold;
    }

    void SetMemoryThreshold(double threshold) {
        memory_threshold_ = threshold;
    }

    void SetTemperatureThreshold(double threshold) {
        temperature_threshold_ = threshold;
    }

    void RegisterAlertCallback(std::function<void(const std::string&)> callback) {
        std::lock_guard<std::mutex> lock(mutex_);
        alert_callbacks_.push_back(callback);
    }

    bool ExportMetrics(const std::string& path) const {
        std::lock_guard<std::mutex> lock(mutex_);
        
        try {
            Json::Value root(Json::arrayValue);
            for (const auto& metrics : metrics_history_) {
                Json::Value entry;
                entry["cpu_usage"] = metrics.cpu_usage;
                entry["memory_usage"] = metrics.memory_usage;
                entry["gpu_usage"] = metrics.gpu_usage;
                entry["power_consumption"] = metrics.power_consumption;
                entry["temperature"] = metrics.temperature;
                entry["network_bandwidth"] = metrics.network_bandwidth;
                entry["timestamp"] = std::chrono::system_clock::to_time_t(metrics.timestamp);
                root.append(entry);
            }

            Json::StreamWriterBuilder writer;
            std::ofstream file(path);
            return file.is_open() ? (file << Json::writeString(writer, root), true) : false;
        } catch (...) {
            return false;
        }
    }

private:
    void MonitoringLoop() {
        while (monitoring_) {
            UpdateMetrics();
            CheckThresholds();
            
            std::this_thread::sleep_for(config_.sampling_interval);
        }
    }

    void UpdateMetrics() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        PerformanceMetrics metrics;
        metrics.timestamp = std::chrono::system_clock::now();
        
        if (config_.monitor_cpu)
            metrics.cpu_usage = MeasureCPUUsage();
        if (config_.monitor_memory)
            metrics.memory_usage = MeasureMemoryUsage();
        if (config_.monitor_gpu)
            metrics.gpu_usage = MeasureGPUUsage();
        if (config_.monitor_power)
            metrics.power_consumption = MeasurePowerConsumption();
        if (config_.monitor_temperature)
            metrics.temperature = MeasureTemperature();
        if (config_.monitor_network)
            metrics.network_bandwidth = MeasureNetworkBandwidth();

        current_metrics_ = metrics;
        metrics_history_.push_back(metrics);
    }

    void CheckThresholds() {
        std::lock_guard<std::mutex> lock(mutex_);
        
        if (current_metrics_.cpu_usage > cpu_threshold_)
            TriggerAlert("CPU usage above threshold");
        if (current_metrics_.memory_usage > memory_threshold_)
            TriggerAlert("Memory usage above threshold");
        if (current_metrics_.temperature > temperature_threshold_)
            TriggerAlert("Temperature above threshold");
    }

    void TriggerAlert(const std::string& message) {
        for (const auto& callback : alert_callbacks_) {
            try {
                callback(message);
            } catch (...) {
                // Ignore callback errors
            }
        }
    }

    double MeasureCPUUsage() {
        std::ifstream stat_file("/proc/stat");
        if (!stat_file.is_open()) return 0.0;

        std::string line;
        std::getline(stat_file, line);
        std::vector<long> cpu_times;
        std::istringstream iss(line);
        std::string cpu;
        long value;

        iss >> cpu; // Skip "cpu" label
        while (iss >> value) {
            cpu_times.push_back(value);
        }

        if (cpu_times.size() < 4) return 0.0;

        long idle = cpu_times[3];
        long total = std::accumulate(cpu_times.begin(), cpu_times.end(), 0L);
        
        static long prev_idle = 0;
        static long prev_total = 0;
        
        long diff_idle = idle - prev_idle;
        long diff_total = total - prev_total;
        
        prev_idle = idle;
        prev_total = total;
        
        if (diff_total == 0) return 0.0;
        return 100.0 * (1.0 - static_cast<double>(diff_idle) / diff_total);
    }

    double MeasureMemoryUsage() {
        struct sysinfo si;
        if (sysinfo(&si) != 0) return 0.0;
        
        double total_ram = si.totalram * si.mem_unit;
        double free_ram = si.freeram * si.mem_unit;
        double used_ram = total_ram - free_ram;
        
        return (used_ram / total_ram) * 100.0;
    }

    double MeasureGPUUsage() {
        std::ifstream gpu_file("/sys/class/kgsl/kgsl-3d0/gpu_busy_percentage");
        if (!gpu_file.is_open()) return 0.0;
        
        int usage;
        gpu_file >> usage;
        return static_cast<double>(usage);
    }

    double MeasurePowerConsumption() {
        std::ifstream power_file("/sys/class/power_supply/battery/current_now");
        if (!power_file.is_open()) return 0.0;
        
        int current;
        power_file >> current;
        
        std::ifstream voltage_file("/sys/class/power_supply/battery/voltage_now");
        if (!voltage_file.is_open()) return 0.0;
        
        int voltage;
        voltage_file >> voltage;
        
        // Convert to milliwatts
        return (static_cast<double>(current) * static_cast<double>(voltage)) / 1e9;
    }

    double MeasureTemperature() {
        std::ifstream temp_file("/sys/class/thermal/thermal_zone0/temp");
        if (!temp_file.is_open()) return 0.0;
        
        int temp;
        temp_file >> temp;
        return temp / 1000.0; // Convert to degrees Celsius
    }

    double MeasureNetworkBandwidth() {
        static long prev_bytes = 0;
        static auto prev_time = std::chrono::steady_clock::now();
        
        std::ifstream net_file("/proc/net/dev");
        if (!net_file.is_open()) return 0.0;
        
        std::string line;
        long total_bytes = 0;
        
        // Skip header lines
        std::getline(net_file, line);
        std::getline(net_file, line);
        
        while (std::getline(net_file, line)) {
            std::istringstream iss(line);
            std::string iface;
            long bytes_in, bytes_out;
            
            iss >> iface; // Interface name
            iss >> bytes_in; // Bytes received
            
            // Skip other fields to get to bytes transmitted
            for (int i = 0; i < 8; ++i) {
                iss >> bytes_out;
            }
            
            total_bytes += bytes_in + bytes_out;
        }
        
        auto current_time = std::chrono::steady_clock::now();
        auto time_diff = std::chrono::duration_cast<std::chrono::seconds>(current_time - prev_time).count();
        
        if (time_diff == 0) return 0.0;
        
        double bandwidth = static_cast<double>(total_bytes - prev_bytes) / time_diff;
        
        prev_bytes = total_bytes;
        prev_time = current_time;
        
        return bandwidth / (1024 * 1024); // Convert to MB/s
    }

    bool monitoring_;
    MonitoringConfig config_;
    std::thread monitor_thread_;
    mutable std::mutex mutex_;
    PerformanceMetrics current_metrics_;
    std::vector<PerformanceMetrics> metrics_history_;
    std::vector<std::function<void(const std::string&)>> alert_callbacks_;
    double cpu_threshold_;
    double memory_threshold_;
    double temperature_threshold_;
};

PerformanceMonitor::PerformanceMonitor() : pImpl(std::make_unique<Impl>()) {}
PerformanceMonitor::~PerformanceMonitor() = default;

void PerformanceMonitor::StartMonitoring(const MonitoringConfig& config) {
    pImpl->StartMonitoring(config);
}

void PerformanceMonitor::StopMonitoring() {
    pImpl->StopMonitoring();
}

bool PerformanceMonitor::IsMonitoring() const {
    return pImpl->IsMonitoring();
}

PerformanceMetrics PerformanceMonitor::GetCurrentMetrics() const {
    return pImpl->GetCurrentMetrics();
}

std::vector<PerformanceMetrics> PerformanceMonitor::GetMetricsHistory() const {
    return pImpl->GetMetricsHistory();
}

double PerformanceMonitor::GetAverageCPUUsage() const {
    return pImpl->GetAverageCPUUsage();
}

double PerformanceMonitor::GetPeakMemoryUsage() const {
    return pImpl->GetPeakMemoryUsage();
}

double PerformanceMonitor::GetAveragePowerConsumption() const {
    return pImpl->GetAveragePowerConsumption();
}

std::string PerformanceMonitor::GeneratePerformanceReport() const {
    return pImpl->GeneratePerformanceReport();
}

void PerformanceMonitor::SetCPUThreshold(double threshold) {
    pImpl->SetCPUThreshold(threshold);
}

void PerformanceMonitor::SetMemoryThreshold(double threshold) {
    pImpl->SetMemoryThreshold(threshold);
}

void PerformanceMonitor::SetTemperatureThreshold(double threshold) {
    pImpl->SetTemperatureThreshold(threshold);
}

void PerformanceMonitor::RegisterAlertCallback(std::function<void(const std::string&)> callback) {
    pImpl->RegisterAlertCallback(callback);
}

bool PerformanceMonitor::ExportMetrics(const std::string& path) const {
    return pImpl->ExportMetrics(path);
}

bool PerformanceMonitor::ExportPerformanceReport(const std::string& path) const {
    try {
        std::ofstream file(path);
        if (!file.is_open()) return false;
        file << GeneratePerformanceReport();
        return true;
    } catch (...) {
        return false;
    }
}

} // namespace monitoring
} // namespace mobileai 