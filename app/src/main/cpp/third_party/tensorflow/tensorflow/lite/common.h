#ifndef TENSORFLOW_LITE_COMMON_H_
#define TENSORFLOW_LITE_COMMON_H_

// Include standard types and utilities
#include <vector>
#include <memory>
#include <string>
#include <algorithm>
#include <cstdint>
#include <cstdlib>

// Use proper namespace qualifications directly instead of relying on using declarations
// This will help avoid namespace conflicts with the C++ standard library

namespace tflite {

// Common status codes
enum class StatusCode {
  kOk = 0,
  kError = 1,
  kInvalidArgument = 2,
  kNotFound = 3,
  kInvalidState = 4,
  kUnimplemented = 5
};

// A simple status class
class Status {
 public:
  Status() : code_(StatusCode::kOk) {}
  Status(StatusCode code, const std::string& message = "") 
    : code_(code), message_(message) {}
  
  bool ok() const { return code_ == StatusCode::kOk; }
  StatusCode code() const { return code_; }
  const std::string& message() const { return message_; }
  
 private:
  StatusCode code_;
  std::string message_;
};

// A simple tensor shape class
class TensorShape {
 public:
  TensorShape() = default;
  TensorShape(const std::vector<int>& dims) : dims_(dims) {}
  
  int dims() const { return dims_.size(); }
  int dim_size(int i) const { return (i < dims_.size()) ? dims_[i] : 0; }
  const std::vector<int>& dims_vector() const { return dims_; }
  
 private:
  std::vector<int> dims_;
};

}  // namespace tflite

#endif  // TENSORFLOW_LITE_COMMON_H_ 