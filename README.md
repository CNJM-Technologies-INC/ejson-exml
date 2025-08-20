# ejson-exml 

### e-json.h: JSON for People Who Have a Job to Do

Look, let's be honest. You need to sling some JSON around in C++. You don't want to link a gargantuan library, you don't want to read a 300-page manual, and you certainly don't want to fight with the build system for three days. You just want to parse a string, modify a value, and get back to writing code that actually matters.

This is e-json. A single, header-only, zero-dependency C++17 library that gets out of your way. It's not a science project. It's a tool. It works. End of story.

Philosophy

Modern C++: Uses std::variant for type-safe storage. No raw pointers, no new/delete, no nonsense.

Intuitive API: If you've ever used JSON in Python or JavaScript, you already know how to use this. doc["user"]["name"] = "John"; just works.

Header-Only: Drop it in your project. Include it. You're done.

Zero-Dependency: All it needs is the C++17 standard library. Nothing else. It won't pollute your project with transitive dependencies.

Quick Start
```
#include "e-json.h"
#include <iostream>

using ejson::JSON;

int main() {
    // Parse a string using the _json literal
    JSON doc = R"({
        "project": "e-json",
        "awesome": true,
        "stars": 9001
    })"_json;

    // Access and modify data like a std::map or std::vector
    doc["stars"] = 9002;
    doc["features"].push_back("Header-only");
    doc["features"].push_back("Zero-dependency");

    // Build objects and arrays on the fly
    doc["author"]["name"] = "Camresh James";
    doc["author"]["company"] = "CNJM TECHNOLOGIES INC";

    // Serialize it back to a string (pretty-printed)
    std::cout << doc.dump_pretty(2) << std::endl;

    // Safe access
    std::string license = doc.at("license", "MIT").as_string();
    std::cout << "License: " << license << std::endl;

    return 0;
}
```
A Word on Ambiguity (The C++ Problem We Solved So You Don't Have To)

So you've got a powerful library and you want to write this:

```
// DON'T DO THIS
JSON ambiguous_obj = {{"key", "value"}};
```

Seems reasonable, right? Is it an array containing one object? Or an object containing one pair? The C++ compiler agrees: it's reasonable, and it's also completely ambiguous. It will refuse to compile, throwing pages of template errors at you.

Other libraries might force you into some bizarre, verbose "builder" pattern. That's garbage. We have a simple, explicit solution. You already know what you're building, so just tell the compiler.

This is how you do it:
```
// DO THIS: Unambiguous and clear.
JSON obj = JSON_OBJECT({{"key", "value"}});
JSON arr = JSON_ARRAY({1, 2, 3});
```
These macros resolve the ambiguity. Use them. They exist for a reason.

Similarly, if you're accessing an array with an integer literal, the compiler can get confused between operator[](size_t) for arrays and operator[](const char*) for objects (because 0 can be a null pointer). We fixed that in the header. You just write my_array[0] and it does the right thing. No need for weird casting or u suffixes. We took care of it.

Features

Yeah, it's got features. All the ones you actually need.

Feature	Example:

Parsing	JSON doc = JSON::parse(str); or auto doc = R"([])"_json;
Serialization	std::string s = doc.dump_pretty(2); or doc.dump_minified();
Object Access	doc["user"]["name"] = "John";
Array Access	doc["scores"][0] = 100;
Array Manipulation	doc["scores"].push_back(95); doc.erase(0);
Type Checking	if (doc["age"].is_number()) { ... }
Safe Access	int age = doc["age"].as_int(18);
Path Operations	doc.set_path("user.address.city", "New York"); auto city = doc.at_path(...);
Iteration	for (auto& item : doc["items"]) { ... } for (auto it : doc["user"]) { ... }
File I/O	doc.to_file("out.json"); JSON loaded = JSON::from_file("in.json");
Unicode Support	Correctly parses and serializes UTF-8, including surrogate pairs (emojis).
Seriously, That's It.

It's a JSON library. It shouldn't be the hardest part of your project. Now stop reading and go write some code.