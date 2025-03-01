#ifndef TF_STD_TYPES_H_
#define TF_STD_TYPES_H_

#include <string>
#include <memory>
#include <vector>
#include <map>
#include <unordered_map>
#include <type_traits>
#include <utility>
#include <functional>
#include <algorithm>
#include <exception>

// Import common standard library types to global namespace 
// to resolve namespace issues in the TensorFlow code
using std::string;
using std::false_type;
using std::true_type;
using std::enable_if;
using std::hash;
using std::runtime_error;
using std::unique_ptr;
using std::shared_ptr;
using std::vector;
using std::map;
using std::unordered_map;
using std::pair;
using std::make_pair;
using std::function;
using std::move;
using std::forward;

#endif // TF_STD_TYPES_H_ 