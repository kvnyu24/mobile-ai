#ifndef CMAKE_DUMMY_H_
#define CMAKE_DUMMY_H_

// This file provides dummy implementations for disabled components
// to satisfy compiler dependencies

#ifdef DISABLE_TENSORFLOW_LITE

#include <string>
#include <vector>
#include <memory>

// Namespace and class declarations to satisfy dependencies when TFLite is disabled
namespace tensorflow {
namespace mlir {
namespace lite {

// Minimal implementation for allocation.h
class Allocation {
 public:
  Allocation() = default;
  virtual ~Allocation() = default;
};

namespace experimental {
namespace remat {

// Minimal implementation for metadata_util.h
class MetadataUtil {
 public:
  MetadataUtil() = default;
  ~MetadataUtil() = default;
};

} // namespace remat
} // namespace experimental
} // namespace lite
} // namespace mlir
} // namespace tensorflow

namespace tflite {
  // Forward declarations for common TFLite classes
  class Tensor;
  class ErrorReporter;
  
  // SignatureRunner class (interpreter.h)
  class SignatureRunner {
   public:
    SignatureRunner() = default;
    virtual ~SignatureRunner() = default;
  };

  // Dummy Interpreter implementation (interpreter.h)
  class Interpreter {
  public:
    Interpreter() = default;
    virtual ~Interpreter() = default;
    
    // Common methods that might be referenced
    size_t tensors_size() const { return 0; }
    Tensor* tensor(int) { return nullptr; }
    const Tensor* tensor(int) const { return nullptr; }
    int inputs() const { return 0; }
    int input(int) const { return 0; }
    int outputs() const { return 0; }
    int output(int) const { return 0; }
    
    // These are stubs to make compilation work
    bool AllocateTensors() { return true; }
    bool Invoke() { return true; }
    void SetNumThreads(int) {}
    
    SignatureRunner* GetSignatureRunner(const char*) { return nullptr; }
  };
  
  // Dummy Model implementation
  class Model {
  public:
    static const Model* BuildFromFile(const char*) { return nullptr; }
    static const Model* BuildFromBuffer(const char*, size_t) { return nullptr; }
  };
  
  // Common error codes
  enum TfLiteStatus {
    kTfLiteOk = 0,
    kTfLiteError = 1
  };
  
  // Default ErrorReporter implementation
  class StderrReporter : public ErrorReporter {
   public:
    int Report(const char*, ...) override { return 0; }
  };
  
  // Base ErrorReporter class
  class ErrorReporter {
   public:
    virtual ~ErrorReporter() = default;
    virtual int Report(const char*, ...) = 0;
  };
  
  inline ErrorReporter* DefaultErrorReporter() {
    static StderrReporter error_reporter;
    return &error_reporter;
  }
}

// Flatbuffers namespace to satisfy dependencies
namespace flatbuffers {
  // Add minimal implementations as needed
  struct Table {};
  struct String {
    const char* c_str() const { return ""; }
  };
}

#endif // DISABLE_TENSORFLOW_LITE

#endif // CMAKE_DUMMY_H_ 