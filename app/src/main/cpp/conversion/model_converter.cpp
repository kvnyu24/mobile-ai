#include "model_converter.h"
#include <android/log.h>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace mobileai {
namespace conversion {

class ModelConverter::Impl {
public:
    bool Convert(const std::string& input_path,
                const std::string& output_path,
                ModelFormat source_format,
                ModelFormat target_format,
                const ConversionConfig& config) {
        if (!IsConversionSupported(source_format, target_format)) {
            return false;
        }

        try {
            // Load source model
            auto model_data = LoadModel(input_path, source_format);
            if (model_data.empty()) {
                return false;
            }

            // Convert model
            auto converted_data = ConvertModelFormat(
                model_data, source_format, target_format, config);
            if (converted_data.empty()) {
                return false;
            }

            // Validate if required
            if (config.validate && !ValidateModel(converted_data, target_format)) {
                return false;
            }

            // Save converted model
            return SaveModel(output_path, converted_data);
        } catch (const std::exception& e) {
            __android_log_print(ANDROID_LOG_ERROR, "ModelConverter",
                              "Conversion error: %s", e.what());
            return false;
        }
    }

    bool BatchConvert(const std::vector<std::string>& input_paths,
                     const std::string& output_dir,
                     ModelFormat source_format,
                     ModelFormat target_format,
                     const ConversionConfig& config) {
        bool all_success = true;
        for (const auto& input_path : input_paths) {
            std::filesystem::path path(input_path);
            std::string output_path = std::filesystem::path(output_dir) / 
                                    path.stem().string() + 
                                    GetFormatExtension(target_format);
            
            if (!Convert(input_path, output_path, source_format, target_format, config)) {
                all_success = false;
            }
        }
        return all_success;
    }

    bool RegisterCustomFormat(const CustomFormatSpec& format_spec) {
        if (custom_formats_.find(format_spec.format_name) != custom_formats_.end()) {
            return false;
        }
        custom_formats_[format_spec.format_name] = format_spec;
        return true;
    }

    bool UnregisterCustomFormat(const std::string& format_name) {
        return custom_formats_.erase(format_name) > 0;
    }

    bool ValidateModel(const std::string& model_path, ModelFormat format) {
        try {
            auto model_data = LoadModel(model_path, format);
            return ValidateModel(model_data, format);
        } catch (const std::exception& e) {
            return false;
        }
    }

private:
    std::vector<uint8_t> LoadModel(const std::string& path, ModelFormat format) {
        std::ifstream file(path, std::ios::binary | std::ios::ate);
        if (!file.is_open()) {
            __android_log_print(ANDROID_LOG_ERROR, "ModelConverter", 
                              "Failed to open model file: %s", path.c_str());
            return std::vector<uint8_t>();
        }

        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        std::vector<uint8_t> buffer(size);
        if (!file.read(reinterpret_cast<char*>(buffer.data()), size)) {
            __android_log_print(ANDROID_LOG_ERROR, "ModelConverter",
                              "Failed to read model file");
            return std::vector<uint8_t>();
        }

        return buffer;
    }

    bool SaveModel(const std::string& path, const std::vector<uint8_t>& data) {
        try {
            std::ofstream file(path, std::ios::binary);
            file.write(reinterpret_cast<const char*>(data.data()), data.size());
            return true;
        } catch (const std::exception& e) {
            return false;
        }
    }

    std::vector<uint8_t> ConvertModelFormat(
        const std::vector<uint8_t>& input_data,
        ModelFormat source_format,
        ModelFormat target_format,
        const ConversionConfig& config) {
        
        // Basic format conversion logic
        std::vector<uint8_t> converted_data;

        // Handle format-specific conversions
        switch (source_format) {
            case ModelFormat::ONNX:
                if (target_format == ModelFormat::TFLITE) {
                    converted_data = ConvertONNXToTFLite(input_data, config);
                } else if (target_format == ModelFormat::PYTORCH) {
                    converted_data = ConvertONNXToPyTorch(input_data, config);
                }
                break;

            case ModelFormat::TFLITE:
                if (target_format == ModelFormat::ONNX) {
                    converted_data = ConvertTFLiteToONNX(input_data, config);
                } else if (target_format == ModelFormat::PYTORCH) {
                    converted_data = ConvertTFLiteToPyTorch(input_data, config);
                }
                break;

            case ModelFormat::PYTORCH:
                if (target_format == ModelFormat::ONNX) {
                    converted_data = ConvertPyTorchToONNX(input_data, config);
                } else if (target_format == ModelFormat::TFLITE) {
                    converted_data = ConvertPyTorchToTFLite(input_data, config);
                }
                break;

            case ModelFormat::CUSTOM:
                if (auto it = custom_formats_.find("custom"); it != custom_formats_.end()) {
                    converted_data = it->second.conversion_func(input_data, target_format, config);
                }
                break;

            default:
                __android_log_print(ANDROID_LOG_ERROR, "ModelConverter",
                                  "Unsupported source format");
                break;
        }

        return converted_data;
    }

    bool ValidateModel(const std::vector<uint8_t>& model_data, ModelFormat format) {
        if (model_data.empty()) {
            return false;
        }

        // Basic format validation
        switch (format) {
            case ModelFormat::ONNX:
                return ValidateONNXFormat(model_data);
            case ModelFormat::TFLITE:
                return ValidateTFLiteFormat(model_data);
            case ModelFormat::PYTORCH:
                return ValidatePyTorchFormat(model_data);
            case ModelFormat::CUSTOM:
                if (auto it = custom_formats_.find("custom"); it != custom_formats_.end()) {
                    return it->second.validation_func(model_data);
                }
                return false;
            default:
                return false;
        }
    }

    std::string GetFormatExtension(ModelFormat format) {
        switch (format) {
            case ModelFormat::ONNX: return ".onnx";
            case ModelFormat::TFLITE: return ".tflite";
            case ModelFormat::PYTORCH: return ".pt";
            case ModelFormat::CUSTOM: return ".custom";
            default: return ".model";
        }
    }

    std::unordered_map<std::string, CustomFormatSpec> custom_formats_;
};

ModelConverter::ModelConverter() : pImpl(std::make_unique<Impl>()) {}
ModelConverter::~ModelConverter() = default;

bool ModelConverter::Convert(const std::string& input_path,
                           const std::string& output_path,
                           ModelFormat source_format,
                           ModelFormat target_format,
                           const ConversionConfig& config) {
    return pImpl->Convert(input_path, output_path, source_format, target_format, config);
}

bool ModelConverter::BatchConvert(const std::vector<std::string>& input_paths,
                                const std::string& output_dir,
                                ModelFormat source_format,
                                ModelFormat target_format,
                                const ConversionConfig& config) {
    return pImpl->BatchConvert(input_paths, output_dir, source_format, target_format, config);
}

bool ModelConverter::RegisterCustomFormat(const CustomFormatSpec& format_spec) {
    return pImpl->RegisterCustomFormat(format_spec);
}

bool ModelConverter::UnregisterCustomFormat(const std::string& format_name) {
    return pImpl->UnregisterCustomFormat(format_name);
}

bool ModelConverter::ValidateModel(const std::string& model_path, ModelFormat format) {
    return pImpl->ValidateModel(model_path, format);
}

std::vector<ModelFormat> ModelConverter::GetSupportedFormats() const {
    return {
        ModelFormat::ONNX,
        ModelFormat::TFLITE,
        ModelFormat::PYTORCH,
        ModelFormat::CUSTOM
    };
}

bool ModelConverter::IsFormatSupported(ModelFormat format) const {
    auto formats = GetSupportedFormats();
    return std::find(formats.begin(), formats.end(), format) != formats.end();
}

bool ModelConverter::IsConversionSupported(ModelFormat source, ModelFormat target) const {
    return IsFormatSupported(source) && IsFormatSupported(target);
}

} // namespace conversion
} // namespace mobileai 