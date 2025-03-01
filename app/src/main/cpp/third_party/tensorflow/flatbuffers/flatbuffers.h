#ifndef FLATBUFFERS_H_
#define FLATBUFFERS_H_

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <type_traits>

// Add version macros to satisfy dependencies
#define FLATBUFFERS_VERSION_MAJOR 24
#define FLATBUFFERS_VERSION_MINOR 3
#define FLATBUFFERS_VERSION_REVISION 25
#define FLATBUFFERS_NOEXCEPT noexcept

namespace flatbuffers {

// Add missing utility functions
inline bool IsOutRange(int value, int min, int max) { return value < min || value > max; }

// Forward declarations
struct Table;
struct String;
template<typename T> struct Vector;
template<typename T> struct Offset;
struct VectorOfAny;
struct KeyCompare;
class FlatBufferBuilder;
template<typename T> struct NativeTable;

// Verifier class for compatibility
class Verifier {
public:
  Verifier(const uint8_t* buf, size_t len) : buf_(buf), len_(len) {}
  bool Verify() { return true; }
private:
  const uint8_t* buf_;
  size_t len_;
};

// Resolver function types
using resolver_function_t = void* (*)(void*);
using rehasher_function_t = std::size_t (*)(void*);

// A minimal Offset type for resolving compile errors
template<typename T>
struct Offset {
  uintptr_t o;
  Offset() : o(0) {}
  Offset(uintptr_t _o) : o(_o) {}
  bool IsNull() const { return !o; }
};

// A minimal Vector type
template<typename T>
struct Vector {
  const uint8_t *Data() const { return nullptr; }
  size_t size() const { return 0; }
};

// A minimal String type
struct String {
  const char *c_str() const { return ""; }
  size_t size() const { return 0; }
  std::string str() const { return ""; }
};

// A minimal FlatBufferBuilder
class FlatBufferBuilder {
public:
  FlatBufferBuilder() {}
  uint8_t *GetBufferPointer() const { return nullptr; }
  size_t GetSize() const { return 0; }
  
  template<typename T>
  Offset<T> CreateString(const char* str) { return Offset<T>(); }
  
  template<typename T>
  Offset<T> CreateVector(const T* data, size_t size) { return Offset<T>(); }
};

}  // namespace flatbuffers

#endif  // FLATBUFFERS_H_ 