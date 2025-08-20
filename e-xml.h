// exml.h
// Author: Camresh James, 16 June 2025
// CNJM TECHNOLOGIES INC
// e-xml: Easy XML, header-only, zero-dependency C++ library
// A complete, modern C++ solution for XML handling.


//Works for 99.99% percent of you practical needs

#pragma once
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <sstream>
#include <cctype>
#include <stdexcept>
#include <iostream>
#include <fstream>
#include <algorithm>
#include <initializer_list>

namespace exml {

struct XMLParseError : std::runtime_error {
    XMLParseError(const std::string& msg) : std::runtime_error("XML Parse Error: " + msg) {}
};

struct Node {
    std::string name;
    std::string text_content;
    std::map<std::string, std::string> attributes;
    std::vector<Node> child_nodes;

    // ============ CONSTRUCTORS ============
    Node() = default;
    Node(const std::string& name) : name(name) {}
    Node(const std::string& name, const std::string& text) : name(name), text_content(text) {}

    // Copy and move semantics
    Node(const Node& other) = default;
    Node(Node&& other) noexcept = default;
    Node& operator=(const Node& other) = default;
    Node& operator=(Node&& other) noexcept = default;

    // ============ ATTRIBUTE OPERATIONS ============
    bool has_attribute(const std::string& key) const {
        return attributes.count(key);
    }

    std::optional<std::string> attribute(const std::string& key) const {
        auto it = attributes.find(key);
        if (it != attributes.end()) {
            return it->second;
        }
        return std::nullopt;
    }

    std::string attribute_or(const std::string& key, const std::string& default_val) const {
        return attribute(key).value_or(default_val);
    }

    Node& set_attribute(const std::string& key, const std::string& value) {
        attributes[key] = value;
        return *this;
    }

    Node& remove_attribute(const std::string& key) {
        attributes.erase(key);
        return *this;
    }
    
    // ============ TEXT CONTENT OPERATIONS ============
    const std::string& text() const { return text_content; }
    
    Node& set_text(const std::string& text) {
        text_content = text;
        return *this;
    }
    
    template<typename T>
    T as(T default_val = T{}) const {
        T result = default_val;
        std::istringstream iss(text_content);
        iss >> result;
        return result;
    }
    int as_int(int default_val = 0) const { return as<int>(default_val); }
    double as_double(double default_val = 0.0) const { return as<double>(default_val); }
    bool as_bool(bool default_val = false) const {
        std::string lower_text = text_content;
        std::transform(lower_text.begin(), lower_text.end(), lower_text.begin(), ::tolower);
        if (lower_text == "true" || lower_text == "1") return true;
        if (lower_text == "false" || lower_text == "0") return false;
        return default_val;
    }

    // ============ CHILD NODE OPERATIONS ============
    // Add a child node
    Node& add_child(const Node& child) {
        child_nodes.push_back(child);
        return *this;
    }
    Node& add_child(Node&& child) {
        child_nodes.push_back(std::move(child));
        return *this;
    }

    // Access the *first* child with a given name
    Node& operator[](const std::string& child_name) {
        for (auto& child : child_nodes) {
            if (child.name == child_name) {
                return child;
            }
        }
        child_nodes.emplace_back(child_name);
        return child_nodes.back();
    }

    const Node& operator[](const std::string& child_name) const {
        for (const auto& child : child_nodes) {
            if (child.name == child_name) {
                return child;
            }
        }
        throw XMLParseError("Child node not found: " + child_name);
    }
    
    // Get all children with a given name
    std::vector<Node*> children(const std::string& name) {
        std::vector<Node*> result;
        for (auto& child : child_nodes) {
            if (child.name == name) {
                result.push_back(&child);
            }
        }
        return result;
    }
    
    std::vector<const Node*> children(const std::string& name) const {
        std::vector<const Node*> result;
        for (const auto& child : child_nodes) {
            if (child.name == name) {
                result.push_back(&child);
            }
        }
        return result;
    }

    // Iteration over all children
    auto begin() { return child_nodes.begin(); }
    auto end() { return child_nodes.end(); }
    auto begin() const { return child_nodes.cbegin(); }
    auto end() const { return child_nodes.cend(); }

    void clear() {
        text_content.clear();
        attributes.clear();
        child_nodes.clear();
    }

    // ============ SERIALIZATION ============
    std::string dump(bool pretty = true, int indent_level = 0, int indent_size = 2) const {
        std::ostringstream oss;
        dump_recursive(oss, pretty, indent_level, indent_size);
        return oss.str();
    }

    // ============ PARSING ============
    static Node parse(const std::string& s) {
        size_t idx = 0;
        skip_ws_and_prolog(s, idx);
        Node root = parse_node(s, idx);
        skip_ws(s, idx);
        if (idx < s.size()) {
            throw XMLParseError("Extra characters after root element at position " + std::to_string(idx));
        }
        return root;
    }

    // ============ FILE I/O ============
    static Node from_file(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw XMLParseError("Cannot open file: " + filename);
        }
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return parse(content);
    }

    void to_file(const std::string& filename, bool pretty = true) const {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw XMLParseError("Cannot write to file: " + filename);
        }
        file << dump(pretty);
    }

private:
    // ============ PARSER IMPLEMENTATION ============
    static void skip_ws(const std::string& s, size_t& idx) {
        while (idx < s.size() && std::isspace(s[idx])) idx++;
    }
    
    static void skip_ws_and_prolog(const std::string& s, size_t& idx) {
        while (idx < s.size()) {
            skip_ws(s, idx);
            if (idx + 1 >= s.size() || s[idx] != '<') break;
            if (s[idx+1] == '?' || s[idx+1] == '!') {
                 auto end_pos = s.find('>', idx);
                 if (end_pos == std::string::npos) throw XMLParseError("Unclosed prolog/comment");
                 idx = end_pos + 1;
            } else {
                break;
            }
        }
    }

    static std::string parse_entity(const std::string& entity) {
        if (entity == "lt") return "<";
        if (entity == "gt") return ">";
        if (entity == "amp") return "&";
        if (entity == "quot") return "\"";
        if (entity == "apos") return "'";
        return "&" + entity + ";";
    }
    
    static std::string decode_text(const std::string& text) {
        std::string decoded;
        size_t i = 0;
        while (i < text.length()) {
            if (text[i] == '&') {
                size_t semi_pos = text.find(';', i);
                if (semi_pos != std::string::npos) {
                    std::string entity = text.substr(i + 1, semi_pos - i - 1);
                    decoded += parse_entity(entity);
                    i = semi_pos + 1;
                } else {
                    decoded += '&'; // Malformed entity
                    i++;
                }
            } else {
                decoded += text[i++];
            }
        }
        return decoded;
    }

    static Node parse_node(const std::string& s, size_t& idx) {
        skip_ws(s, idx);
        if (idx >= s.size() || s[idx] != '<') throw XMLParseError("Expected '<' to start a node");
        idx++;

        // Parse tag name
        size_t name_start = idx;
        while (idx < s.size() && (std::isalnum(s[idx]) || s[idx] == '_' || s[idx] == ':' || s[idx] == '-')) idx++;
        Node node(s.substr(name_start, idx - name_start));

        skip_ws(s, idx);

        // Parse attributes
        while (idx < s.size() && s[idx] != '>' && s[idx] != '/') {
            size_t key_start = idx;
            while (idx < s.size() && (std::isalnum(s[idx]) || s[idx] == '_' || s[idx] == ':')) idx++;
            std::string key = s.substr(key_start, idx - key_start);
            skip_ws(s, idx);
            if (idx >= s.size() || s[idx] != '=') throw XMLParseError("Expected '=' after attribute key");
            idx++;
            skip_ws(s, idx);
            char quote = s[idx];
            if (quote != '"' && quote != '\'') throw XMLParseError("Attribute value must be quoted");
            idx++;
            size_t val_start = idx;
            while (idx < s.size() && s[idx] != quote) idx++;
            std::string val = s.substr(val_start, idx - val_start);
            node.attributes[key] = decode_text(val);
            idx++;
            skip_ws(s, idx);
        }

        if (idx >= s.size()) throw XMLParseError("Unclosed tag");
        
        // Self-closing tag or opening tag
        if (s[idx] == '/') {
            idx++;
            if (idx >= s.size() || s[idx] != '>') throw XMLParseError("Expected '>' for self-closing tag");
            idx++;
            return node;
        }
        if (s[idx] != '>') throw XMLParseError("Expected '>' to close tag opening");
        idx++;
        
        // Parse content (text and children)
        size_t content_start = idx;
        while (idx < s.size()) {
            skip_ws(s, idx);
            if (idx + 1 < s.size() && s[idx] == '<' && s[idx+1] == '/') break;
            if (idx < s.size() && s[idx] == '<') {
                // Found a child node
                if(idx > content_start) {
                    node.text_content += decode_text(s.substr(content_start, idx - content_start));
                }
                node.child_nodes.push_back(parse_node(s, idx));
                content_start = idx;
            } else {
                idx++;
            }
        }
        if(idx > content_start) {
             node.text_content += decode_text(s.substr(content_start, idx - content_start));
        }

        // Closing tag
        if (idx + 1 >= s.size() || s[idx] != '<' || s[idx+1] != '/') throw XMLParseError("Expected closing tag");
        idx += 2;
        size_t close_name_start = idx;
        while (idx < s.size() && s[idx] != '>') idx++;
        if (s.substr(close_name_start, idx - close_name_start) != node.name) {
            throw XMLParseError("Mismatched closing tag: expected " + node.name);
        }
        idx++;
        
        return node;
    }
    
    // ============ SERIALIZER IMPLEMENTATION ============
    static std::string encode_text(const std::string& text) {
        std::string encoded;
        for (char c : text) {
            switch (c) {
                case '<': encoded += "&lt;"; break;
                case '>': encoded += "&gt;"; break;
                case '&': encoded += "&amp;"; break;
                case '"': encoded += "&quot;"; break;
                case '\'': encoded += "&apos;"; break;
                default: encoded += c;
            }
        }
        return encoded;
    }

    void dump_recursive(std::ostringstream& oss, bool pretty, int indent_level, int indent_size) const {
        std::string indent = pretty ? std::string(indent_level * indent_size, ' ') : "";
        oss << indent << "<" << name;
        for (const auto& [k, v] : attributes) {
            oss << " " << k << "=\"" << encode_text(v) << "\"";
        }

        bool is_empty = text_content.empty() && child_nodes.empty();
        if (is_empty) {
            oss << " />" << (pretty ? "\n" : "");
            return;
        }

        oss << ">";
        bool has_children = !child_nodes.empty();
        if (pretty && has_children) oss << "\n";
        
        if (!text_content.empty()) {
            oss << (pretty && has_children ? std::string((indent_level+1)*indent_size, ' ') : "") 
                << encode_text(text_content) 
                << (pretty && has_children ? "\n" : "");
        }

        for (const auto& child : child_nodes) {
            child.dump_recursive(oss, pretty, indent_level + 1, indent_size);
        }

        if (pretty && has_children) oss << indent;
        oss << "</" << name << ">" << (pretty ? "\n" : "");
    }
};

} // namespace exml

// ============ USAGE EXAMPLES ============
/*
#include "exml.h"
#include <iostream>

void run_examples() {
    // 1. Building an XML document programmatically
    exml::Node root("users");
    root.set_attribute("version", "1.0");

    exml::Node user1("user");
    user1.set_attribute("id", "101")
         .add_child({"name", "John Doe"})
         .add_child({"email", "john.doe@example.com"});
    
    exml::Node user2("user");
    user2.set_attribute("id", "102");
    user2["name"].set_text("Jane Smith"); // Creates 'name' node if it doesn't exist
    user2["email"].set_text("jane.smith@example.com");
    user2.add_child({"permissions", ""}) // Add a parent for multiple children
         .add_child({"permission", "read"})
         .add_child({"permission", "write"});

    root.add_child(user1).add_child(user2);

    // 2. Serializing to a string (pretty-printed)
    std::string xml_string = root.dump(true, 0, 2);
    std::cout << "Generated XML:\n" << xml_string << std::endl;

    // 3. Parsing from a string
    exml::Node parsed_root = exml::Node::parse(xml_string);
    
    // 4. Accessing data
    std::cout << "Accessing data:" << std::endl;
    std::cout << "Root tag: " << parsed_root.name << std::endl;
    std::cout << "Version attribute: " << parsed_root.attribute_or("version", "N/A") << std::endl;

    // Access first user's name
    std::cout << "User 1 Name: " << parsed_root["user"]["name"].text() << std::endl;
    
    // 5. Iterating over nodes with the same name
    std::cout << "\nAll users:" << std::endl;
    for (const auto* user_node : parsed_root.children("user")) {
        int id = std::stoi(user_node->attribute_or("id", "0"));
        std::string name = (*user_node)["name"].text();
        std::cout << " - ID: " << id << ", Name: " << name << std::endl;
    }

    // 6. Get all permissions for the second user
    std::cout << "\nUser 2 permissions:" << std::endl;
    const auto& second_user = parsed_root.child_nodes[1];
    for (const auto* perm_node : second_user["permissions"].children("permission")) {
        std::cout << " - " << perm_node->text() << std::endl;
    }

    // 7. File I/O
    root.to_file("users.xml");
    exml::Node loaded_from_file = exml::Node::from_file("users.xml");
    std::cout << "\nLoaded User 1 ID from file: " << loaded_from_file["user"].attribute_or("id", "none") << std::endl;
}

int main() {
    try {
        run_examples();
    } catch (const exml::XMLParseError& e) {
        std::cerr << e.what() << std::endl;
        return 1;
    }
    return 0;
}
*/