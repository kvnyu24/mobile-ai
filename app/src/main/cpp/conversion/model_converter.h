#pragma once

#include <memory>
#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

namespace mobileai {
namespace conversion {

enum class ModelFormat {
    ONNX,
    TFLITE,
    PYTORCH,
    CUSTOM
};

struct ConversionConfig {
    bool optimize = true;
    bool validate = true;
    bool preserve_metadata = true;
    std::vector<std::string> custom_ops;
    std::string target_device;
};

struct CustomFormatSpec {
    std::string format_name;
    std::string version;
    std::function<bool(const std::string&)> validator;
    std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> serializer;
    std::function<std::vector<uint8_t>(const std::vector<uint8_t>&)> deserializer;
};

class ModelConverter {
public:
    ModelConverter();
    ~ModelConverter();

    // Basic conversion
    bool Convert(const std::string& input_path,
                const std::string& output_path,
                ModelFormat source_format,
                ModelFormat target_format,
                const ConversionConfig& config = ConversionConfig());

    // Batch conversion
    bool BatchConvert(const std::vector<std::string>& input_paths,
                     const std::string& output_dir,
                     ModelFormat source_format,
                     ModelFormat target_format,
                     const ConversionConfig& config = ConversionConfig());

    // Custom format support
    bool RegisterCustomFormat(const CustomFormatSpec& format_spec);
    bool UnregisterCustomFormat(const std::string& format_name);

    // Validation
    bool ValidateModel(const std::string& model_path, ModelFormat format);

    // Utilities
    std::vector<ModelFormat> GetSupportedFormats() const;
    bool IsFormatSupported(ModelFormat format) const;
    bool IsConversionSupported(ModelFormat source, ModelFormat target) const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace conversion
} // namespace mobileai