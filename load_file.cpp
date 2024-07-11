#include <cassert>
#include <chrono>
#include <cstdio>
#include <iostream>
#include <string_view>
#include <vector>

#include "minijson.hpp"

void printValue(const minijson::JsonValue& value, size_t indent = 0)
{
    std::cout << std::string(4 * indent, ' ');
    switch (value.type()) {
    case minijson::JsonValue::Type::Null:
        std::cout << "null\n";
        break;
    case minijson::JsonValue::Type::Bool:
        std::cout << "bool: " << value.asBool() << "\n";
        break;
    case minijson::JsonValue::Type::Number:
        std::cout << "number: " << value.asNumber() << "\n";
        break;
    case minijson::JsonValue::Type::String:
        std::cout << "string: " << value.asString() << "\n";
        break;
    case minijson::JsonValue::Type::Array:
        std::cout << "array (" << value.size() << ")\n";
        for (const auto& elem : value.asArray()) {
            printValue(elem, indent + 1);
        }
        break;
    case minijson::JsonValue::Type::Object:
        std::cout << "object (" << value.size() << ")\n";
        for (const auto& [key, value] : value.asObject()) {
            std::cout << std::string(4 * (indent + 1), ' ') << "key: " << key << "\n";
            printValue(value, indent + 1);
        }
        break;
    default:
        assert(false && "Invalid JSON value type");
    }
}

int main(int argc, char** argv)
{
    const std::vector<std::string> args(argv + 1, argv + argc);
    if (args.size() < 1) {
        std::cerr << "Usage: load-file <file>" << std::endl;
        return 1;
    }

    const auto readStart = std::chrono::high_resolution_clock::now();

    FILE* f = std::fopen(args[0].c_str(), "rb");
    if (!f) {
        std::cerr << "Could not open file" << std::endl;
        return 1;
    }
    std::fseek(f, 0, SEEK_END);
    const auto size = std::ftell(f);
    if (size < 0) {
        std::cerr << "Error getting file size" << std::endl;
        return 1;
    }
    std::fseek(f, 0, SEEK_SET);
    std::string json;
    json.resize(size);
    const auto readRes = std::fread(json.data(), 1, size, f);
    if (readRes != static_cast<size_t>(size)) {
        std::cerr << "Error reading file: " << readRes << std::endl;
        return 1;
    }

    std::cerr << "Read file: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::high_resolution_clock::now() - readStart)
                     .count()
              << "ms" << std::endl;

    const auto parseStart = std::chrono::high_resolution_clock::now();

    const auto res = minijson::parse(json);
    if (!res) {
        const auto& err = res.error();
        std::cerr << "Could not parse json: " << err.message << " at " << err.cursor << std::endl;
        std::cout << minijson::getContext(json, err.cursor) << std::endl;
        return 1;
    }

    std::cerr << "Parse: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     std::chrono::high_resolution_clock::now() - parseStart)
                     .count()
              << "ms" << std::endl;

    const auto& doc = *res;
    printValue(doc);

    return 0;
}
