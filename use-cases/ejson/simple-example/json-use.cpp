// ============ USAGE EXAMPLES ============

#include "e-json-v1.h"
#include <iostream>

using namespace ejson;

int main() {
    try {
        // Basic usage:
        JSON json;
        json["name"] = "James";
        json["age"] = 30;
        json["active"] = true;
        json["scores"] = {85, 90, 78};

        // JSON Path operations:
        json.set_path("user.profile.name", "Jane");
        auto user_name = json.at_path("user.profile.name").as_string();
        std::cout << "User name from path: " << user_name << std::endl;

        // File I/O:
        json.to_file("data.json");
        auto loaded = JSON::from_file("data.json");
        std::cout << "Loaded JSON: " << loaded.dump_pretty() << std::endl;

        // Easy object/array creation:
        auto obj = JSON_OBJECT(
            {"name", "Alice"},
            {"age", 25},
            {"hobbies", JSON_ARRAY("reading", "coding", "gaming")}
        );
        std::cout << "Object: " << obj.dump_pretty() << std::endl;

        // String literals:
        auto json_data = R"({"hello": "world", "number": 42})"_json;
        std::cout << "Parsed from literal: " << json_data.dump() << std::endl;

        // Iteration:
        std::cout << "Scores: ";
        for (auto& item : json["scores"]) {
            std::cout << item.as_number() << " ";
        }
        std::cout << std::endl;

        // Safe access with defaults:
        int age = json.at("age", 0).as_int();
        std::string name = json.at("name", "Unknown").as_string();
        std::cout << "Name: " << name << ", Age: " << age << std::endl;

        // Type checking and conversion:
        if (json["age"].is_number()) {
            int user_age = json["age"].get<int>();
            std::cout << "User age (converted): " << user_age << std::endl;
        }
        
        std::cout << "All examples completed successfully!" << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
