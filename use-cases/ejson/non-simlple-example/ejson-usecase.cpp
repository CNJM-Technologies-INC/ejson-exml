// ejson-usecase.cpp
// Comprehensive demonstration of the e-json library.

#include "e-json.h"
#include <iostream>
#include <cassert>

void demonstrate_creation_and_access() {
    std::cout << "--- 1. Creation and Basic Access ---\n";
    
    // Create using the explicit ejson::object and ejson::array helpers to avoid ambiguity
    ejson::JSON doc = ejson::object({
        {"user", "Camresh James"},
        {"project", "e-json"},
        {"active", true},
        {"version", 1.0},
        {"dependencies", ejson::array({"string", "vector", "map"})},
        {"details", ejson::object({
            {"year", 2025},
            {"company", "CNJM TECHNOLOGIES INC"}
        })}
    });

    // Accessing values
    assert(doc["user"].as_string() == "Camresh James");
    assert(doc["version"].as_int() == 1);
    assert(doc["active"].as_bool() == true);
    //Use [0u] to specify an unsigned index and resolve ambiguity
    assert(doc["dependencies"][0].as_string() == "string");
    
    // Modify values
    doc["version"] = 1.1;
    doc["dependencies"].push_back("variant");
    
    std::cout << "Initial document (modified):\n" << doc.dump_pretty(2) << "\n\n";
}

void demonstrate_path_operations() {
    std::cout << "--- 2. JSON Path Operations ---\n";
    
    ejson::JSON doc; // Start with a null JSON object
    
    // Use set_path to build the structure from nothing
    doc.set_path("user.profile.name", "John Doe");
    doc.set_path("user.profile.age", 30);
    doc.set_path("user.contacts[0].type", "email");
    doc.set_path("user.contacts[0].value", "john.doe@example.com");
    doc.set_path("user.contacts[1].type", "phone");
    
    std::cout << "Document built with set_path:\n" << doc.dump_pretty(2) << "\n";
    
    // Use at_path for safe reading
    std::string name = doc.at_path("user.profile.name").as_string("Unknown");
    int age = doc.at_path("user.profile.age").as_int();
    std::string first_contact_type = doc.at_path("user.contacts[0].type").as_string();

    assert(name == "John Doe");
    assert(age == 30);
    assert(first_contact_type == "email");
    
    // Check non-existent paths
    assert(doc.has_path("user.profile.city") == false);
    assert(doc.at_path("user.contacts[2]").is_null());
    
    std::cout << "Successfully read data using at_path.\n\n";
}

void demonstrate_advanced_features() {
    std::cout << "--- 3. Advanced Features ---\n";
    
    ejson::JSON obj1 = ejson::object({{"a", 1}, {"b", 2}});
    ejson::JSON obj2 = ejson::object({{"b", 3}, {"c", 4}});
    
    // Merge obj2 into obj1. Key "b" will be overwritten.
    obj1.merge(obj2);
    assert(obj1["a"].as_int() == 1);
    assert(obj1["b"].as_int() == 3);
    assert(obj1["c"].as_int() == 4);
    std::cout << "Merged object: " << obj1 << "\n";
    
    // Flatten a complex object
    ejson::JSON to_flatten = ejson::object({
        {"user", ejson::object({
            {"name", "Alice"},
            {"roles", ejson::array({"admin", "editor"})}
        })}
    });
    ejson::JSON flat = to_flatten.flattened();
    assert(flat["user.name"].as_string() == "Alice");
    assert(flat["user.roles[0]"].as_string() == "admin");
    std::cout << "Flattened object: " << flat << "\n";

    // Iteration over an array
    ejson::JSON scores = ejson::array({10, 20, 30});
    int sum = 0;
    std::cout << "Iterating over array: ";
    for (const auto& score : scores) {
        std::cout << score.as_int() << " ";
        sum += score.as_int();
    }
    std::cout << "\n";
    assert(sum == 60);

    // Iteration over an object
    ejson::JSON user_data = ejson::object({{"name", "Bob"}, {"id", 123}});
    std::cout << "Iterating over object:\n";
    for (auto it = user_data.begin(); it != user_data.end(); ++it) {
        std::cout << "  Key: " << it.key() << ", Value: " << *it << "\n";
    }
    std::cout << "\n";
}

void demonstrate_parsing_and_edge_cases() {
    std::cout << "--- 4. Parsing and Edge Cases ---\n";

    // Test with all data types and Unicode, including surrogate pairs for emojis
    std::string json_str = R"({
        "greeting": "Hello, World!",
        "number": -1.23e-4,
        "is_valid": true,
        "nothing": null,
        "nested_array": [1, [2, 3]],
        "escaped_chars": "\n\t\"\\",
        "emoji": "\uD83D\uDE00" 
    })";

    ejson::JSON doc = ejson::JSON::parse(json_str);
    std::cout << "Parsed complex document:\n" << doc.dump_pretty(2) << "\n";
    
    assert(doc["emoji"].as_string() == "😀"); // Check if surrogate pair was decoded correctly
    std::cout << "Successfully parsed surrogate pair for emoji.\n";
    
    // Test validation
    assert(ejson::JSON::is_valid(json_str) == true);
    assert(ejson::JSON::is_valid(R"({"key": "value",})") == false); // Trailing comma
    
    // Test error handling
    try {
        ejson::JSON::parse(R"({"key": unquoted_string})");
    } catch (const ejson::JSONParseError& e) {
        std::cout << "Caught expected parse error: " << e.what() << "\n";
    }
    
    try {
        ejson::JSON j = ejson::object({{"key", "value"}});
        // FIX: Use [0u] to force the size_t overload and trigger the runtime error
        int val = j[0u].as_int(); // Type error
    } catch (const ejson::JSONParseError& e) {
        std::cout << "Caught expected type error: " << e.what() << "\n\n";
    }
}

void demonstrate_file_io() {
    std::cout << "--- 5. File I/O ---\n";
    
    const std::string filename = "ejson_test.json";
    
    ejson::JSON data_to_save = ejson::object({
        {"id", 42},
        {"message", "Data saved to file"}
    });
    
    // Save to file
    data_to_save.to_file(filename, true);
    std::cout << "Saved data to " << filename << "\n";
    
    // Load from file
    ejson::JSON loaded_data = ejson::JSON::from_file(filename);
    assert(loaded_data["id"].as_int() == 42);
    std::cout << "Loaded message from file: \"" << loaded_data["message"].as_string() << "\"\n\n";
}

int main() {
    try {
        demonstrate_creation_and_access();
        demonstrate_path_operations();
        demonstrate_advanced_features();
        demonstrate_parsing_and_edge_cases();
        demonstrate_file_io();
        
        std::cout << "e-json use case demonstration completed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}