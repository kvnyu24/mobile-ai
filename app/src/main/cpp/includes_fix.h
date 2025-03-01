#ifndef INCLUDES_FIX_H_
#define INCLUDES_FIX_H_

// This header is intended to fix namespace issues in Android NDK's standard library
// The approach is to provide our own definitions of affected header files
// and properly qualify all types with std:: namespace

// First, prevent the original system_error header from being included
#define _LIBCPP_SYSTEM_ERROR

// Put all our code in the namespace the compiler is expecting
namespace std { inline namespace _LIBCPP_ABI_NAMESPACE {

// Forward declare all the types we need
class error_code;
class error_condition;
class error_category;
class system_error;

// Provide our own base error code and condition enum classes
enum class errc {
    success = 0
};

// Define the minimal implementations needed for system_error
class error_category {
public:
    virtual ~error_category() = default;
    virtual const char* name() const noexcept = 0;
    virtual std::string message(int ev) const = 0;
    
    // Make all the methods virtual and provide default implementations
    virtual bool equivalent(int code, const error_condition& condition) const noexcept;
    virtual bool equivalent(const error_code& code, int condition) const noexcept;
    virtual error_condition default_error_condition(int ev) const noexcept;
};

// Define error_code
class error_code {
public:
    error_code() noexcept = default;
    error_code(int val, const error_category& cat) noexcept {}
    void assign(int val, const error_category& cat) noexcept {}
    void clear() noexcept {}
    int value() const noexcept { return 0; }
    const error_category& category() const noexcept;
    error_condition default_error_condition() const noexcept;
    std::string message() const { return ""; }
    explicit operator bool() const noexcept { return false; }
};

// Define error_condition
class error_condition {
public:
    error_condition() noexcept = default;
    error_condition(int val, const error_category& cat) noexcept {}
    void assign(int val, const error_category& cat) noexcept {}
    void clear() noexcept {}
    int value() const noexcept { return 0; }
    const error_category& category() const noexcept;
    std::string message() const { return ""; }
    explicit operator bool() const noexcept { return false; }
};

// Define system_error, inheriting from runtime_error
class system_error : public runtime_error {
public:
    system_error(error_code ec, const std::string& what_arg)
        : runtime_error(what_arg) {}
    system_error(error_code ec, const char* what_arg)
        : runtime_error(what_arg) {}
    system_error(error_code ec)
        : runtime_error("") {}
    system_error(int ev, const error_category& ecat, const std::string& what_arg)
        : runtime_error(what_arg) {}
    system_error(int ev, const error_category& ecat, const char* what_arg)
        : runtime_error(what_arg) {}
    system_error(int ev, const error_category& ecat)
        : runtime_error("") {}
    
    const error_code& code() const noexcept { return code_; }
    const char* what() const noexcept override { return "system error"; }
    
private:
    error_code code_;
};

}} // end namespace std::_LIBCPP_ABI_NAMESPACE

#endif // INCLUDES_FIX_H_ 