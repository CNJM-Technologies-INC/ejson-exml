// exml-usecase.cpp
// Comprehensive demonstration of the exml library.

#include "exml.h"
#include <iostream>
#include <cassert>

void demonstrate_creation_and_access() {
    std::cout << "--- 1. Creation and Fluent API ---\n";
    
    // Build an XML tree using a fluent, chained interface
    exml::Node root("playlist");
    root.set_attribute("name", "My Favorites")
        .set_attribute("author", "Camresh")
        .add_child(
            exml::Node("track")
                .set_attribute("id", "101")
                .add_child({"title", "C++ Rhapsody"})
                .add_child({"artist", "The Compilers"})
        )
        .add_child(
            exml::Node("track")
                .set_attribute("id", "102")
                ["title"].set_text("Header-Only Blues") // Create and set in one go
        );

    // Add a child with mixed content (text and nodes)
    exml::Node description("description");
    description.set_text("This is a ");
    description.add_child({"b", "great"});
    description.add_child({"i", " playlist!"});
    root.add_child(description);
        
    std::cout << "Programmatically created XML:\n" << root.dump(true, 0, 2) << "\n";

    // Access data
    assert(root.attribute_or("name", "") == "My Favorites");
    assert(root["track"]["title"].text() == "C++ Rhapsody"); // Gets the first track
    
    // Access second track by iterating
    const auto& second_track = root.child_nodes[1];
    assert(second_track.attribute_or("id", "") == "102");
    
    std::cout << "Basic data access successful.\n\n";
}

void demonstrate_querying_and_iteration() {
    std::cout << "--- 2. Querying and Iteration ---\n";

    exml::Node catalog("catalog");
    catalog.add_child({"book", "The C++ Standard Library"})
           .set_attribute("id", "bk101");
    catalog.add_child({"book", "Effective Modern C++"})
           .set_attribute("id", "bk102");
    catalog.add_child({"magazine", "C++ Weekly"})
           .set_attribute("id", "mg101");

    std::cout << "Catalog XML:\n" << catalog.dump(true, 0, 2) << "\n";
           
    // Get all nodes named "book"
    std::vector<const exml::Node*> books = catalog.children("book");
    assert(books.size() == 2);
    std::cout << "Found " << books.size() << " books:\n";
    for(const auto* book : books) {
        std::cout << " - ID: " << book->attribute_or("id", "") << ", Title: " << book->text() << "\n";
    }

    // Standard iteration over all children (books and magazines)
    std::cout << "Iterating over all children in catalog:\n";
    for(const auto& child : catalog) {
        std::cout << " - Node: <" << child.name << " id=\"" << child.attribute_or("id", "") << "\"/>\n";
    }
    std::cout << "\n";
}

void demonstrate_parsing_and_edge_cases() {
    std::cout << "--- 3. Parsing and Edge Cases ---\n";

    // Test with comments, prolog, self-closing tags, and character entities
    std::string xml_str = R"(
        <?xml version="1.0" encoding="UTF-8"?>
        <!-- This is a sample document -->
        <data quality="high">
            <item key="A&amp;B">Text with &lt;entities&gt;.</item>
            <item key="C" /> <!-- Self-closing tag -->
            <config>
                <enabled>true</enabled>
                <retries>5</retries>
            </config>
        </data>
    )";

    exml::Node doc = exml::Node::parse(xml_str);
    std::cout << "Parsed complex document:\n" << doc.dump(true, 0, 2) << "\n";

    // Verify parsed data
    assert(doc.name == "data");
    assert(doc.attribute_or("quality", "") == "high");
    
    const exml::Node& item1 = doc["item"];
    assert(item1.attribute_or("key", "") == "A&B");
    assert(item1.text() == "Text with <entities>.");
    
    const auto& config = doc["config"];
    assert(config["enabled"].as_bool() == true);
    assert(config["retries"].as_int() == 5);
    
    std::cout << "Successfully parsed document with edge cases.\n";

    // Test error handling
    try {
        exml::Node::parse("<root><child></root>"); // Mismatched tag
    } catch (const exml::XMLParseError& e) {
        std::cout << "Caught expected parse error: " << e.what() << "\n\n";
    }
}

void demonstrate_file_io() {
    std::cout << "--- 4. File I/O ---\n";
    
    const std::string filename = "exml_test.xml";
    
    exml::Node data_to_save("root");
    data_to_save.add_child({"status", "OK"});
    
    // Save to file
    data_to_save.to_file(filename, true);
    std::cout << "Saved data to " << filename << "\n";
    
    // Load from file
    exml::Node loaded_data = exml::Node::from_file(filename);
    assert(loaded_data["status"].text() == "OK");
    std::cout << "Loaded status from file: \"" << loaded_data["status"].text() << "\"\n\n";
}


int main() {
    try {
        demonstrate_creation_and_access();
        demonstrate_querying_and_iteration();
        demonstrate_parsing_and_edge_cases();
        demonstrate_file_io();

        std::cout << "e-xml use case demonstration completed successfully!\n";
    } catch (const std::exception& e) {
        std::cerr << "An unexpected error occurred: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}