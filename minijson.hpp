#pragma once

#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace minijson {
class JsonValue {
public:
    struct Invalid { };
    struct Null { };
    using Bool = bool;
    using Number = double;
    using String = std::string;
    using Array = std::vector<JsonValue>;
    // map instead of unordered_map, because it supports incomplete value types
    // We need a transparent comparator to enable .find with std::string_view
    using Object = std::map<std::string, JsonValue, std::less<>>;

    enum class Type {
        Invalid = 0,
        Null,
        Bool,
        Number,
        String,
        Array,
        Object,
    };

    JsonValue() : value_(Invalid {}) { }
    JsonValue(Null n) : value_(std::move(n)) { } // better move that empty struct! #highperformance
    JsonValue(bool b) : value_(b) { }
    JsonValue(double v) : value_(v) { }
    JsonValue(String s) : value_(std::move(s)) { }
    JsonValue(Array v) : value_(std::move(v)) { }
    JsonValue(Object m) : value_(std::move(m)) { }

    // This is a bit brittle, but if we are being honest, doing a switch is not super robust either.
    // I have messed that up before too.
    Type type() const { return static_cast<Type>(value_.index()); }

    template <typename T>
    bool is() const
    {
        return std::holds_alternative<T>(value_);
    }

    bool isValid() const { return !is<Invalid>(); }
    bool isNull() const { return is<Null>(); }
    bool isBool() { return is<Bool>(); }
    bool isNumber() const { return is<Number>(); }
    bool isString() const { return is<String>(); }
    bool isArray() const { return is<Array>(); }
    bool isObject() const { return is<Object>(); }

    // The following "as" and "to" methods are weirdly named.
    // I don't know how to call them. I just want it to be ergonomic (short!) and this is what I
    // picked.

    template <typename T>
    const T& as() const
    {
        return std::get<T>(value_);
    }

    const Bool& asBool() const { return as<Bool>(); }
    const Number& asNumber() const { return as<Number>(); }
    const String& asString() const { return as<String>(); }
    const Array& asArray() const { return as<Array>(); }
    const Object& asObject() const { return as<Object>(); }

    template <typename T>
    const T* to() const
    {
        if (is<T>()) {
            return &as<T>();
        }
        return nullptr;
    }

    const Bool* toBool() const { return to<Bool>(); }
    const Number* toNumber() const { return to<Number>(); }
    const String* toString() const { return to<String>(); }
    const Array* toArray() const { return to<Array>(); }
    const Object* toObject() const { return to<Object>(); }

    // 0 for null and invalid, number of elements for array and object, 1 otherwise
    size_t size() const;

    // returns a reference to an invalid JsonValue if the key/index does not exist
    const JsonValue& operator[](std::string_view key) const;
    const JsonValue& operator[](size_t index) const;

    std::string dump(std::string_view indent = "", size_t indentLevel = 0) const;

private:
    static const JsonValue& getNonExistent(); // returns a static invalid JsonValue

    std::variant<Invalid, Null, Bool, Number, String, Array, Object> value_;
};

struct Error {
    size_t cursor;
    std::string message;
};

template <typename T>
class Result {
public:
    Result(T&& t) : value(std::move(t)) { }
    Result(Error e) : value(std::move(e)) { }

    operator bool() const { return value.index() == 0; }
    const T& operator*() const { return std::get<T>(value); }
    T& operator*() { return std::get<T>(value); }
    const T* operator->() const { return &std::get<T>(value); }
    const Error& error() const { return std::get<Error>(value); }

private:
    std::variant<T, Error> value;
};

std::string getContext(std::string_view str, size_t cursor);

Result<JsonValue> parse(std::string_view source);
}
