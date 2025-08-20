// e-json.h
// Author: Camresh James, 15 June 2025
// CNJM TECHNOLOGIES INC
// e-json: Easy JSON, header-only, zero-dependency C++ library
// Complete replacement for any JSON library

#pragma once
#include <string>
#include <vector>
#include <map>
#include <variant>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <initializer_list>
#include <iterator>
#include <iomanip>
#include <climits>
#include <type_traits>

namespace ejson {

struct JSONParseError : std::runtime_error {
    JSONParseError(const std::string& msg) : std::runtime_error("JSON Parse Error: " + msg) {}
};

struct JSON;
using JSONValue = std::variant<std::nullptr_t, bool, double, std::string, std::vector<JSON>, std::map<std::string, JSON>>;

struct JSON {
    JSONValue value;

    // ============ CONSTRUCTORS ============
    JSON() : value(nullptr) {}
    JSON(std::nullptr_t) : value(nullptr) {}
    JSON(bool b) : value(b) {}
    // Note: large 64-bit integers (roughly > 2^53 or 9e15) may lose precision
    // as they are stored internally as doubles. This is standard for JSON libraries.
    JSON(int n) : value(double(n)) {}
    JSON(long n) : value(double(n)) {}
    JSON(long long n) : value(double(n)) {}
    JSON(float n) : value(double(n)) {}
    JSON(double n) : value(n) {}
    JSON(const std::string& s) : value(s) {}
    JSON(const char* s) : value(std::string(s)) {}
    JSON(const std::vector<JSON>& a) : value(a) {}
    JSON(const std::map<std::string, JSON>& o) : value(o) {}
    
    // Initializer list constructors for easy creation
    JSON(std::initializer_list<JSON> list) : value(std::vector<JSON>(list)) {}
    JSON(std::initializer_list<std::pair<std::string, JSON>> list) {
        std::map<std::string, JSON> obj;
        for (const auto& pair : list) {
            obj[pair.first] = pair.second;
        }
        value = obj;
    }

    // Copy and move semantics
    JSON(const JSON& other) = default;
    JSON(JSON&& other) noexcept = default;
    JSON& operator=(const JSON& other) = default;
    JSON& operator=(JSON&& other) noexcept = default;

    // ============ TYPE CHECKS ============
    bool is_null() const { return std::holds_alternative<std::nullptr_t>(value); }
    bool is_bool() const { return std::holds_alternative<bool>(value); }
    bool is_number() const { return std::holds_alternative<double>(value); }
    bool is_string() const { return std::holds_alternative<std::string>(value); }
    bool is_array() const { return std::holds_alternative<std::vector<JSON>>(value); }
    bool is_object() const { return std::holds_alternative<std::map<std::string, JSON>>(value); }
    bool is_primitive() const { return is_null() || is_bool() || is_number() || is_string(); }

    // ============ SAFE ACCESS WITH DEFAULTS ============
    bool as_bool(bool default_val = false) const { 
        return is_bool() ? std::get<bool>(value) : default_val; 
    }
    
    double as_number(double default_val = 0.0) const { 
        return is_number() ? std::get<double>(value) : default_val; 
    }
    
    int as_int(int default_val = 0) const {
        return is_number() ? static_cast<int>(std::get<double>(value)) : default_val;
    }
    
    long long as_int64(long long default_val = 0) const {
        return is_number() ? static_cast<long long>(std::get<double>(value)) : default_val;
    }
    
    float as_float(float default_val = 0.0f) const {
        return is_number() ? static_cast<float>(std::get<double>(value)) : default_val;
    }
    
    const std::string& as_string() const { 
        if (!is_string()) throw JSONParseError("Not a string"); 
        return std::get<std::string>(value); 
    }
    
    std::string as_string(const std::string& default_val) const {
        return is_string() ? std::get<std::string>(value) : default_val;
    }
    
    const std::vector<JSON>& as_array() const { 
        if (!is_array()) throw JSONParseError("Not an array"); 
        return std::get<std::vector<JSON>>(value); 
    }
    
    const std::map<std::string, JSON>& as_object() const { 
        if (!is_object()) throw JSONParseError("Not an object"); 
        return std::get<std::map<std::string, JSON>>(value); 
    }

    // ============ ARRAY ACCESS ============
    JSON& operator[](size_t idx) {
        if (is_null()) value = std::vector<JSON>{};
        if (!is_array()) throw JSONParseError("Not an array");
        auto& arr = std::get<std::vector<JSON>>(value);
        if (idx >= arr.size()) arr.resize(idx + 1);
        return arr[idx];
    }

    const JSON& operator[](size_t idx) const {
        if (!is_array()) throw JSONParseError("Not an array");
        const auto& arr = std::get<std::vector<JSON>>(value);
        if (idx >= arr.size()) throw JSONParseError("Array index out of bounds");
        return arr[idx];
    }

    // ============ OBJECT ACCESS ============
    JSON& operator[](const std::string& key) {
        if (is_null()) {
            value = std::map<std::string, JSON>{};
        }
        if (!is_object()) throw JSONParseError("Not an object");
        auto& obj = std::get<std::map<std::string, JSON>>(value);
        return obj[key];
    }

    const JSON& operator[](const std::string& key) const {
        if (!is_object()) throw JSONParseError("Not an object");
        const auto& obj = std::get<std::map<std::string, JSON>>(value);
        auto it = obj.find(key);
        if (it == obj.end()) throw JSONParseError("Key not found: " + key);
        return it->second;
    }

    // SFINAE-enabled template to handle string-like keys (e.g., const char*)
    // This overload is only enabled if T is NOT an integral type.
    // This resolves the ambiguity with operator[](size_t).
    template <typename T,
              typename = std::enable_if_t<!std::is_integral_v<T>>>
    JSON& operator[](T key) {
        return (*this)[std::string(key)];
    }

    template <typename T,
              typename = std::enable_if_t<!std::is_integral_v<T>>>
    const JSON& operator[](T key) const {
        return (*this)[std::string(key)];
    }

    // Safe object access
    JSON at(const std::string& key, const JSON& default_val = JSON()) const {
        if (!is_object()) return default_val;
        const auto& obj = std::get<std::map<std::string, JSON>>(value);
        auto it = obj.find(key);
        return it != obj.end() ? it->second : default_val;
    }

    // Check if object contains key
    bool contains(const std::string& key) const {
        if (!is_object()) return false;
        const auto& obj = std::get<std::map<std::string, JSON>>(value);
        return obj.find(key) != obj.end();
    }

    // ============ SIZE AND EMPTY ============
    size_t size() const {
        if (is_array()) return std::get<std::vector<JSON>>(value).size();
        if (is_object()) return std::get<std::map<std::string, JSON>>(value).size();
        if (is_string()) return std::get<std::string>(value).size();
        return 0;
    }

    bool empty() const { 
        if (is_array()) return std::get<std::vector<JSON>>(value).empty();
        if (is_object()) return std::get<std::map<std::string, JSON>>(value).empty();
        if (is_string()) return std::get<std::string>(value).empty();
        return is_null();
    }

    // ============ ARRAY OPERATIONS ============
    void push_back(const JSON& item) {
        if (is_null()) value = std::vector<JSON>{};
        if (!is_array()) throw JSONParseError("Not an array");
        auto& arr = std::get<std::vector<JSON>>(value);
        arr.push_back(item);
    }

    void push_front(const JSON& item) {
        if (is_null()) value = std::vector<JSON>{};
        if (!is_array()) throw JSONParseError("Not an array");
        auto& arr = std::get<std::vector<JSON>>(value);
        arr.insert(arr.begin(), item);
    }

    void pop_back() {
        if (!is_array()) throw JSONParseError("Not an array");
        auto& arr = std::get<std::vector<JSON>>(value);
        if (arr.empty()) throw JSONParseError("Array is empty");
        arr.pop_back();
    }

    void insert(size_t index, const JSON& item) {
        if (!is_array()) throw JSONParseError("Not an array");
        auto& arr = std::get<std::vector<JSON>>(value);
        if (index > arr.size()) throw JSONParseError("Index out of bounds");
        arr.insert(arr.begin() + index, item);
    }

    void erase(size_t index) {
        if (!is_array()) throw JSONParseError("Not an array");
        auto& arr = std::get<std::vector<JSON>>(value);
        if (index >= arr.size()) throw JSONParseError("Index out of bounds");
        arr.erase(arr.begin() + index);
    }

    // ============ OBJECT OPERATIONS ============
    void erase(const std::string& key) {
        if (!is_object()) throw JSONParseError("Not an object");
        auto& obj = std::get<std::map<std::string, JSON>>(value);
        obj.erase(key);
    }

    std::vector<std::string> keys() const {
        if (!is_object()) return {};
        const auto& obj = std::get<std::map<std::string, JSON>>(value);
        std::vector<std::string> result;
        for (const auto& [key, _] : obj) {
            result.push_back(key);
        }
        return result;
    }

    // ============ CLEAR CONTENT ============
    void clear() {
        if (is_array()) std::get<std::vector<JSON>>(value).clear();
        else if (is_object()) std::get<std::map<std::string, JSON>>(value).clear();
        else value = nullptr;
    }

    // ============ JSON PATH OPERATIONS ============
    JSON at_path(const std::string& path) const {
        const JSON* current = this;
        size_t i = 0;
        while(i < path.size()) {
            if(path[i] == '.') { i++; continue; }
            if(std::isalpha(path[i]) || path[i]=='_') {
                size_t start = i;
                while(i < path.size() && (std::isalnum(path[i]) || path[i]=='_')) i++;
                std::string key = path.substr(start,i-start);
                if(!current->is_object()) return JSON();
                const auto& obj = current->as_object();
                auto it = obj.find(key);
                if (it == obj.end()) return JSON();
                current = &it->second;
            } else if(path[i]=='[') {
                i++;
                size_t start = i;
                while(i < path.size() && std::isdigit(path[i])) i++;
                if(i>=path.size() || path[i]!=']') throw JSONParseError("Expected closing bracket");
                int idx = std::stoi(path.substr(start,i-start));
                if(!current->is_array()) return JSON();
                const auto& arr = current->as_array();
                if (idx < 0 || static_cast<size_t>(idx) >= arr.size()) return JSON();
                current = &arr[idx];
                i++;
            } else {
                throw JSONParseError("Invalid character in path: " + std::string(1,path[i]));
            }
        }
        return *current;
    }

    void set_path(const std::string& path, const JSON& val) {
        JSON* current = this;
        size_t i = 0;
        std::vector<std::pair<std::string, int>> path_parts;
        
        while(i < path.size()) {
            if(path[i] == '.') { i++; continue; }
            if(std::isalpha(path[i]) || path[i]=='_') {
                size_t start = i;
                while(i < path.size() && (std::isalnum(path[i]) || path[i]=='_')) i++;
                path_parts.push_back({path.substr(start,i-start), -1});
            } else if(path[i]=='[') {
                i++;
                size_t start = i;
                while(i < path.size() && std::isdigit(path[i])) i++;
                if(i>=path.size() || path[i]!=']') throw JSONParseError("Expected closing bracket");
                int idx = std::stoi(path.substr(start,i-start));
                path_parts.push_back({"", idx});
                i++;
            } else {
                throw JSONParseError("Invalid character in path: " + std::string(1,path[i]));
            }
        }

        for (size_t j = 0; j < path_parts.size(); ++j) {
            bool is_last = (j == path_parts.size() - 1);
            const auto& [key, index] = path_parts[j];
            
            if (index == -1) {
                if (current->is_null()) *current = std::map<std::string, JSON>{};
                if (!current->is_object()) throw JSONParseError("Expected object in path");
                
                if (is_last) {
                    (*current)[key] = val;
                } else {
                    current = &(*current)[key];
                }
            } else {
                if (current->is_null()) *current = std::vector<JSON>{};
                if (!current->is_array()) throw JSONParseError("Expected array in path");
                
                auto& arr = std::get<std::vector<JSON>>(current->value);
                if (arr.size() <= static_cast<size_t>(index)) {
                    arr.resize(static_cast<size_t>(index) + 1);
                }
                
                if (is_last) {
                    arr[index] = val;
                } else {
                    current = &arr[index];
                }
            }
        }
    }

    bool has_path(const std::string& path) const {
        return !at_path(path).is_null();
    }

    // ============ COMPARISON OPERATORS ============
    bool operator==(const JSON& other) const {
        return value == other.value;
    }
    bool operator!=(const JSON& other) const {
        return !(*this == other);
    }
    
    bool operator<(const JSON& other) const {
        if (value.index() != other.value.index()) {
            return value.index() < other.value.index();
        }
        return std::visit(
            [](const auto& lhs, const auto& rhs) -> bool {
                if constexpr (std::is_same_v<decltype(lhs), decltype(rhs)>) {
                    using T = std::decay_t<decltype(lhs)>;
                    if constexpr (std::is_same_v<T, std::nullptr_t>) {
                        return false;
                    } else {
                        return lhs < rhs;
                    }
                } else {
                    return false;
                }
            },
            value, other.value
        );
    }

    // ============ SERIALIZATION ============
    std::string dump(bool pretty = false, int indent = 0, int indent_size = 2, int max_precision = 6) const {
        std::ostringstream oss;
        
        if (is_null()) { 
            oss << "null"; 
        }
        else if (is_bool()) { 
            oss << (std::get<bool>(value) ? "true" : "false"); 
        }
        else if (is_number()) { 
            double num = std::get<double>(value);
            if (num == static_cast<long long>(num) && num >= LLONG_MIN && num <= LLONG_MAX) {
                oss << static_cast<long long>(num);
            } else {
                oss << std::setprecision(max_precision) << std::noshowpoint << num;
            }
        }
        else if (is_string()) {
            oss << '"';
            for (auto c : std::get<std::string>(value)) {
                switch(c) {
                    case '\"': oss << "\\\""; break;
                    case '\\': oss << "\\\\"; break;
                    case '\b': oss << "\\b"; break;
                    case '\f': oss << "\\f"; break;
                    case '\n': oss << "\\n"; break;
                    case '\r': oss << "\\r"; break;
                    case '\t': oss << "\\t"; break;
                    default: 
                        if (c < 32 || c == 127) {
                            oss << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                        } else {
                            oss << c;
                        }
                }
            }
            oss << '"';
        }
        else if (is_array()) {
            const auto& arr = std::get<std::vector<JSON>>(value);
            oss << "[";
            bool first = true;
            for (const auto& el : arr) {
                if (!first) oss << ",";
                first = false;
                if (pretty) oss << "\n" << std::string(indent + indent_size,' ');
                oss << el.dump(pretty, indent + indent_size, indent_size, max_precision);
            }
            if (pretty && !arr.empty()) oss << "\n" << std::string(indent,' ');
            oss << "]";
        }
        else if (is_object()) {
            const auto& obj = std::get<std::map<std::string, JSON>>(value);
            oss << "{";
            bool first = true;
            for (const auto& [k,v] : obj) {
                if (!first) oss << ",";
                first = false;
                if (pretty) oss << "\n" << std::string(indent + indent_size,' ');
                oss << '"' << k << "\":" << (pretty ? " " : "") << v.dump(pretty, indent + indent_size, indent_size, max_precision);
            }
            if (pretty && !obj.empty()) oss << "\n" << std::string(indent,' ');
            oss << "}";
        }
        return oss.str();
    }

    std::string dump_minified() const { return dump(false); }
    std::string dump_pretty(int indent_size = 2) const { return dump(true, 0, indent_size, 6); }

    // ============ FILE I/O ============
    static JSON from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw JSONParseError("Cannot open file: " + filename);
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return parse(content);
    }

    void to_file(const std::string& filename, bool pretty = true) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw JSONParseError("Cannot write to file: " + filename);
        }
        file << dump(pretty);
    }

    // ============ STREAM OPERATORS ============
    friend std::ostream& operator<<(std::ostream& os, const JSON& json) {
        os << json.dump();
        return os;
    }

    friend std::istream& operator>>(std::istream& is, JSON& json) {
        std::string content((std::istreambuf_iterator<char>(is)), std::istreambuf_iterator<char>());
        json = parse(content);
        return is;
    }

    // ============ MERGE AND FLATTEN ============
    void merge(const JSON& other) {
        if (!is_object() || !other.is_object()) {
            throw JSONParseError("Can only merge objects");
        }
        auto& obj = std::get<std::map<std::string, JSON>>(value);
        const auto& other_obj = other.as_object();
        for (const auto& [key, val] : other_obj) {
            obj[key] = val;
        }
    }

    JSON flattened(const std::string& separator = ".") const {
        JSON result = std::map<std::string, JSON>{};
        flatten_recursive(*this, "", result, separator);
        return result;
    }

    // ============ TYPE CONVERSION HELPERS ============
    template<typename T>
    T get() const {
        if constexpr (std::is_same_v<T, bool>) {
            return as_bool();
        } else if constexpr (std::is_same_v<T, int>) {
            return as_int();
        } else if constexpr (std::is_same_v<T, long long>) {
            return as_int64();
        } else if constexpr (std::is_same_v<T, float>) {
            return as_float();
        } else if constexpr (std::is_same_v<T, double>) {
            return as_number();
        } else if constexpr (std::is_same_v<T, std::string>) {
            return as_string();
        } else {
            static_assert(std::is_same_v<T, void>, "Unsupported type for get()");
        }
    }

    template<typename T>
    T get_or(const T& default_val) const {
        try {
            return get<T>();
        } catch (...) {
            return default_val;
        }
    }

    // ============ ITERATION SUPPORT ============
    class iterator {
        std::variant<
            std::vector<JSON>::iterator,
            std::map<std::string, JSON>::iterator
        > it;
        bool is_array_iter;
        
    public:
        iterator(std::vector<JSON>::iterator arr_it) : it(arr_it), is_array_iter(true) {}
        iterator(std::map<std::string, JSON>::iterator obj_it) : it(obj_it), is_array_iter(false) {}
        
        JSON& operator*() {
            if (is_array_iter) {
                return *std::get<std::vector<JSON>::iterator>(it);
            } else {
                return std::get<std::map<std::string, JSON>::iterator>(it)->second;
            }
        }
        
        iterator& operator++() {
            if (is_array_iter) {
                ++std::get<std::vector<JSON>::iterator>(it);
            } else {
                ++std::get<std::map<std::string, JSON>::iterator>(it);
            }
            return *this;
        }
        
        bool operator!=(const iterator& other) const {
            if (is_array_iter != other.is_array_iter) return true;
            if (is_array_iter) {
                return std::get<std::vector<JSON>::iterator>(it) != std::get<std::vector<JSON>::iterator>(other.it);
            } else {
                return std::get<std::map<std::string, JSON>::iterator>(it) != std::get<std::map<std::string, JSON>::iterator>(other.it);
            }
        }
        
        std::string key() const {
            if (!is_array_iter) {
                return std::get<std::map<std::string, JSON>::iterator>(it)->first;
            }
            throw JSONParseError("Cannot get key from array iterator");
        }
    };

    iterator begin() {
        if (is_array()) {
            return iterator(std::get<std::vector<JSON>>(value).begin());
        } else if (is_object()) {
            return iterator(std::get<std::map<std::string, JSON>>(value).begin());
        }
        throw JSONParseError("Cannot iterate over non-container type");
    }

    iterator end() {
        if (is_array()) {
            return iterator(std::get<std::vector<JSON>>(value).end());
        } else if (is_object()) {
            return iterator(std::get<std::map<std::string, JSON>>(value).end());
        }
        throw JSONParseError("Cannot iterate over non-container type");
    }

    // ============ PARSING WITH ENHANCED ERROR REPORTING ============
    static JSON parse(const std::string& s) {
        size_t idx = 0;
        try {
            JSON result = parse_value(s, idx);
            skip_ws(s, idx);
            if (idx < s.size()) {
                throw JSONParseError("Extra characters after JSON at position " + std::to_string(idx));
            }
            return result;
        } catch (const std::exception& e) {
            throw JSONParseError("Parse error at position " + std::to_string(idx) + ": " + e.what());
        }
    }

    // ============ VALIDATION ============
    static bool is_valid(const std::string& s) {
        try {
            parse(s);
            return true;
        } catch (...) {
            return false;
        }
    }

    // ============ UTILITY FUNCTIONS ============
    JSON deep_copy() const {
        return JSON(*this); // Uses copy constructor
    }

    std::string type_name() const {
        if (is_null()) return "null";
        if (is_bool()) return "boolean";
        if (is_number()) return "number";
        if (is_string()) return "string";
        if (is_array()) return "array";
        if (is_object()) return "object";
        return "unknown";
    }

private:
    // ============ HELPER FUNCTIONS ============
    static void flatten_recursive(const JSON& obj, const std::string& prefix, JSON& result, const std::string& sep) {
        if (obj.is_object()) {
            for (const auto& [key, value] : obj.as_object()) {
                std::string new_key = prefix.empty() ? key : prefix + sep + key;
                if (value.is_object() || value.is_array()) {
                    flatten_recursive(value, new_key, result, sep);
                } else {
                    result[new_key] = value;
                }
            }
        } else if (obj.is_array()) {
            for (size_t i = 0; i < obj.size(); ++i) {
                std::string new_key = prefix + "[" + std::to_string(i) + "]";
                if (obj[i].is_object() || obj[i].is_array()) {
                    flatten_recursive(obj[i], new_key, result, sep);
                } else {
                    result[new_key] = obj[i];
                }
            }
        } else {
            result[prefix] = obj;
        }
    }

    static void skip_ws(const std::string& s, size_t& idx) {
        while(idx < s.size() && std::isspace(s[idx])) idx++;
    }
    
    static void encode_utf8(std::string& res, int codepoint) {
        if (codepoint <= 0x7F) {
            res += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            res += static_cast<char>(0xC0 | (codepoint >> 6));
            res += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else if (codepoint <= 0xFFFF) {
            res += static_cast<char>(0xE0 | (codepoint >> 12));
            res += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            res += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else if (codepoint <= 0x10FFFF) {
            res += static_cast<char>(0xF0 | (codepoint >> 18));
            res += static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F));
            res += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            res += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }


    static JSON parse_value(const std::string& s, size_t& idx) {
        skip_ws(s, idx);
        if(idx >= s.size()) throw JSONParseError("Unexpected end of input");

        char c = s[idx];
        if(c=='n') return parse_null(s, idx);
        else if(c=='t' || c=='f') return parse_bool(s, idx);
        else if(c=='\"') return parse_string(s, idx);
        else if(c=='[') return parse_array(s, idx);
        else if(c=='{') return parse_object(s, idx);
        else if(c=='-' || std::isdigit(c)) return parse_number(s, idx);
        throw JSONParseError(std::string("Unexpected character: ")+c);
    }

    static JSON parse_null(const std::string& s, size_t& idx) {
        if(idx + 4 > s.size() || s.substr(idx,4)!="null") throw JSONParseError("Invalid null");
        idx+=4;
        return JSON(nullptr);
    }

    static JSON parse_bool(const std::string& s, size_t& idx) {
        if(idx + 4 <= s.size() && s.substr(idx,4)=="true") { idx+=4; return JSON(true); }
        if(idx + 5 <= s.size() && s.substr(idx,5)=="false") { idx+=5; return JSON(false); }
        throw JSONParseError("Invalid boolean");
    }

    static JSON parse_number(const std::string& s, size_t& idx) {
        size_t start = idx;
        if(s[idx]=='-') idx++;
        if(idx >= s.size() || !std::isdigit(s[idx])) throw JSONParseError("Invalid number");
        
        if(s[idx] == '0') {
            idx++;
        } else {
            while(idx<s.size() && std::isdigit(s[idx])) idx++;
        }
        
        if(idx<s.size() && s[idx]=='.') { 
            idx++; 
            if(idx >= s.size() || !std::isdigit(s[idx])) throw JSONParseError("Invalid number: missing digits after decimal point");
            while(idx<s.size() && std::isdigit(s[idx])) idx++; 
        }
        
        if(idx<s.size() && (s[idx]=='e' || s[idx]=='E')) {
            idx++;
            if(idx<s.size() && (s[idx]=='+' || s[idx]=='-')) idx++;
            if(idx >= s.size() || !std::isdigit(s[idx])) throw JSONParseError("Invalid number: missing digits in exponent");
            while(idx<s.size() && std::isdigit(s[idx])) idx++;
        }
        
        try {
            double num = std::stod(s.substr(start, idx-start));
            return JSON(num);
        } catch (const std::exception&) {
            throw JSONParseError("Invalid number format");
        }
    }

    static JSON parse_string(const std::string& s, size_t& idx) {
        if(s[idx]!='"') throw JSONParseError("Expected string");
        idx++;
        std::string res;
        while(idx<s.size()) {
            char c = s[idx++];
            if(c=='"') break;
            if(c=='\\') {
                if(idx>=s.size()) throw JSONParseError("Invalid escape: unexpected end of string");
                char esc = s[idx++];
                switch(esc){
                    case '"': res+='"'; break;
                    case '\\': res+='\\'; break;
                    case '/': res+='/'; break;
                    case 'b': res+='\b'; break;
                    case 'f': res+='\f'; break;
                    case 'n': res+='\n'; break;
                    case 'r': res+='\r'; break;
                    case 't': res+='\t'; break;
                    case 'u': {
                        if (idx + 4 > s.size()) throw JSONParseError("Invalid unicode escape");
                        try {
                            int codepoint = std::stoi(s.substr(idx, 4), nullptr, 16);
                            idx += 4;
                            
                            if (codepoint >= 0xD800 && codepoint <= 0xDBFF) { // High surrogate
                                if (idx + 6 > s.size() || s.substr(idx, 2) != "\\u") {
                                    throw JSONParseError("Invalid surrogate pair: high surrogate not followed by low surrogate escape");
                                }
                                int low_surrogate = std::stoi(s.substr(idx + 2, 4), nullptr, 16);
                                idx += 6;
                                
                                if (low_surrogate < 0xDC00 || low_surrogate > 0xDFFF) {
                                    throw JSONParseError("Invalid surrogate pair: high surrogate not followed by a low surrogate");
                                }
                                codepoint = 0x10000 + ((codepoint - 0xD800) << 10 | (low_surrogate - 0xDC00));
                            } else if (codepoint >= 0xDC00 && codepoint <= 0xDFFF) {
                                throw JSONParseError("Invalid surrogate pair: low surrogate without high surrogate");
                            }
                            
                            encode_utf8(res, codepoint);
                        } catch (const std::exception&) {
                            throw JSONParseError("Invalid unicode escape sequence");
                        }
                        break;
                    }
                    default: throw JSONParseError("Unknown escape sequence: \\" + std::string(1, esc));
                }
            } else if (c < 32) {
                throw JSONParseError("Unescaped control character in string");
            } else {
                res+=c;
            }
        }
        if (idx > s.size()) throw JSONParseError("Unterminated string");
        return JSON(res);
    }

    static JSON parse_array(const std::string& s, size_t& idx) {
        if(s[idx]!='[') throw JSONParseError("Expected '['");
        idx++;
        std::vector<JSON> arr;
        skip_ws(s, idx);
        if(idx<s.size() && s[idx]==']') { idx++; return JSON(arr); }
        while(true) {
            arr.push_back(parse_value(s, idx));
            skip_ws(s, idx);
            if(idx>=s.size()) throw JSONParseError("Expected ',' or ']'");
            if(s[idx]==',') { idx++; skip_ws(s, idx); continue; }
            if(s[idx]==']') { idx++; break; }
            throw JSONParseError(std::string("Unexpected character in array: ")+s[idx]);
        }
        return JSON(arr);
    }

    static JSON parse_object(const std::string& s, size_t& idx) {
        if(s[idx]!='{') throw JSONParseError("Expected '{'");
        idx++;
        std::map<std::string, JSON> obj;
        skip_ws(s, idx);
        if(idx < s.size() && s[idx] == '}') { idx++; return JSON(obj); }
        while(true) {
            skip_ws(s, idx);
            if(idx >= s.size() || s[idx] != '"') throw JSONParseError("Expected string key in object");
            JSON key = parse_string(s, idx);
            skip_ws(s, idx);
            if(idx >= s.size() || s[idx] != ':') throw JSONParseError("Expected ':' after key in object");
            idx++;
            JSON val = parse_value(s, idx);
            obj[key.as_string()] = val;
            skip_ws(s, idx);
            if(idx >= s.size()) throw JSONParseError("Expected ',' or '}' in object");
            if(s[idx] == ',') { idx++; skip_ws(s, idx); continue; }
            if(s[idx] == '}') { idx++; break; }
            throw JSONParseError(std::string("Unexpected character in object: ") + s[idx]);
        }
        return JSON(obj);
    }
};

// ============ CONVENIENCE FUNCTIONS ============
inline JSON object(std::initializer_list<std::pair<std::string, JSON>> list) {
    return JSON(list);
}

inline JSON array(std::initializer_list<JSON> list) {
    return JSON(list);
}

// JSON literals support
inline JSON operator""_json(const char* str, size_t) {
    return JSON::parse(str);
}

} // namespace ejson

// ============ CONVENIENCE MACROS ============
#define JSON_OBJECT(...) ejson::object({__VA_ARGS__})
#define JSON_ARRAY(...) ejson::array({__VA_ARGS__})


// coffee 😀  =  +254741593506