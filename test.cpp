#include <cassert>
#include <iostream>

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

int main()
{
    const minijson::JsonValue val = minijson::JsonValue::Object {
        { "a", 12.0 },
        { "arr",
            minijson::JsonValue::Array {
                1.0,
                2.0,
                3.0,
            } },
    };
    std::cout << val.dump("  ") << std::endl;

    constexpr std::string_view src = R"(
        {
            "a": 12,
            "b": "hello",
            "c": null,
            "d": true,
            "arr": [
                {"x": 1, "y": 2},
                {"x": 3, "y": 5}
            ],
            "obj": {
                "foo": "bar"
            }
        }
    )";

    const auto res = minijson::parse(src);
    if (!res) {
        const auto& err = res.error();
        std::cerr << "Could not parse json: " << err.message << " at " << err.cursor << std::endl;
        std::cout << minijson::getContext(src, err.cursor) << std::endl;
        return 1;
    }

    const auto& doc = *res;
    printValue(doc);

    std::cout << doc.dump("  ") << std::endl;

    const auto x = doc["arr"][0]["x"].toNumber();
    if (x) {
        std::cout << *x << std::endl;
    }

    const auto& z = doc["arr"][0]["z"];
    if (!z.isValid()) {
        std::cout << "<empty>" << std::endl;
    }

    return 0;
}
