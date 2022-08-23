#include "minijson.hpp"

#include <cassert>
#include <charconv>
#include <optional>
#include <unordered_map>

using namespace minijson;

namespace {
Result<std::string> parseString(std::string_view source, size_t& cursor)
{
    assert(cursor < source.size());
    assert(source[cursor] == '"');
    cursor++;
    std::string str;
    str.reserve(32);
    while (cursor < source.size()) {
        if (source[cursor] == '\\') {
            cursor++;
            if (cursor >= source.size()) {
                return Error { cursor, "Incomplete character escape" };
            }
            const auto c = str[cursor];

            if (c == 'u') {
                return Error { cursor, "Unicode escapes are not implemented yet" };
            }

            static const std::unordered_map<char, std::string> escapes {
                { '"', "\"" },
                { '\\', "\\" },
                { '/', "/" },
                { 'b', "\b" },
                { 'f', "\f" },
                { 'n', "\n" },
                { 'r', "\r" },
                { 't', "\t" },
            };
            const auto it = escapes.find(c);
            if (it == escapes.end()) {
                return Error { cursor, "Invalid character escape" };
            }

            str.append(it->second);
            cursor++;
        } else if (source[cursor] == '"') {
            cursor++;
            return str;
        } else {
            str.push_back(source[cursor]);
            cursor++;
        }
    }
    return Error { cursor, "Unterminated string" };
}

bool isWhitespace(char ch)
{
    return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\t';
}

void skipWhitespace(std::string_view source, size_t& cursor)
{
    while (cursor < source.size() && isWhitespace(source[cursor])) {
        cursor++;
    }
}

std::optional<double> parseNumber(std::string_view str)
{
    double value = 0.0;
    const auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    if (ec != std::errc() || ptr != str.data() + str.size()) {
        return std::nullopt;
    }
    return value;
}

bool skipSeparator(std::string_view source, size_t& cursor)
{
    skipWhitespace(source, cursor);
    if (cursor < source.size() && source[cursor] == ',') {
        cursor++;
        skipWhitespace(source, cursor);
        return true;
    }
    return false;
}

Result<JsonValue> parseValue(std::string_view source, size_t& cursor);

Result<JsonValue> parseArray(std::string_view source, size_t& cursor)
{
    JsonValue::Array values;
    while (cursor < source.size()) {
        skipWhitespace(source, cursor);

        if (cursor >= source.size()) {
            return Error { cursor, "Unterminated array" };
        }

        if (source[cursor] == ']') {
            cursor++;
            break;
        }

        auto value = parseValue(source, cursor);
        if (!value) {
            return value.error();
        }
        values.push_back(std::move(*value));

        const auto separatorFound = skipSeparator(source, cursor);
        if (cursor < source.size() && source[cursor] == ']') {
            cursor++;
            break;
        }

        if (!separatorFound) {
            return Error { cursor, "Expected separator" };
        }
    }
    return JsonValue(values);
}

Result<JsonValue> parseObject(std::string_view source, size_t& cursor)
{
    JsonValue::Object obj;
    while (cursor < source.size()) {
        skipWhitespace(source, cursor);

        if (cursor >= source.size()) {
            return Error { cursor, "Unterminated object" };
        }

        if (source[cursor] == '}') {
            cursor++;
            break;
        }

        if (source[cursor] != '"') {
            return Error { cursor, "Expected key" };
        }

        const auto key = parseString(source, cursor);
        if (!key) {
            return key.error();
        }

        skipWhitespace(source, cursor);

        if (cursor >= source.size() || source[cursor] != ':') {
            return Error { cursor, "Expected colon" };
        }
        cursor++;

        skipWhitespace(source, cursor);

        if (cursor >= source.size()) {
            return Error { cursor, "Expected value" };
        }

        auto value = parseValue(source, cursor);
        if (!value) {
            return value.error();
        }
        obj.emplace(std::move(*key), std::move(*value));

        const auto separatorFound = skipSeparator(source, cursor);
        if (cursor < source.size() && source[cursor] == '}') {
            cursor++;
            break;
        }

        if (!separatorFound) {
            return Error { cursor, "Expected separator" };
        }
    }
    return JsonValue(obj);
}

Result<JsonValue> parseValue(std::string_view source, size_t& cursor)
{
    skipWhitespace(source, cursor);
    if (cursor >= source.size()) {
        return Error { cursor, "Expected value" };
        ;
    }

    if (source[cursor] == '{') {
        cursor++;
        auto res = parseObject(source, cursor);
        if (!res) {
            return res.error();
        }
        return res;
    } else if (source[cursor] == '[') {
        cursor++;
        auto res = parseArray(source, cursor);
        if (!res) {
            return res.error();
        }
        return res;
    } else if (source[cursor] == '"') {
        auto res = parseString(source, cursor);
        if (!res) {
            return res.error();
        }
        return JsonValue(std::move(*res));
    } else {
        // null, bool or number
        const auto valueChars = "0123456789abcdefghijlmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.+-";
        auto valueEnd = source.find_first_not_of(valueChars, cursor);
        if (valueEnd == std::string_view::npos) {
            valueEnd = source.size();
        }
        const auto value = source.substr(cursor, valueEnd - cursor);
        if (value.empty()) {
            return Error { cursor, "Value must not be empty" };
            ;
        }

        if (value == "null") {
            cursor += value.size();
            return JsonValue(JsonValue::Null {});
        } else if (value == "true") {
            cursor += value.size();
            return JsonValue(true);
        } else if (value == "false") {
            cursor += value.size();
            return JsonValue(false);
        }

        auto number = parseNumber(value);
        if (!number) {
            return Error { cursor, "Invalid number" };
        }
        cursor += value.size();
        return JsonValue(*number);
    }
}
}

namespace minijson {
size_t JsonValue::size() const
{
    if (!isValid() || isNull()) {
        return 0;
    } else if (isArray()) {
        return std::get<Array>(value_).size();
    } else if (isObject()) {
        return std::get<Object>(value_).size();
    } else {
        return 1;
    }
}

const JsonValue& JsonValue::operator[](std::string_view key) const
{
    if (isObject()) {
        const auto& obj = as<Object>();
        const auto it = obj.find(key);
        if (it == obj.end()) {
            return getNonExistent();
        }
        return it->second;
    } else {
        return getNonExistent();
    }
}

const JsonValue& JsonValue::operator[](size_t index) const
{
    if (isArray() && index < size()) {
        return asArray()[index];
    } else {
        return getNonExistent();
    }
}

std::string JsonValue::dump(std::string_view indent, size_t indentLevel) const
{
    std::string indentStr;
    indentStr.reserve(indent.size() * indentLevel);
    for (size_t i = 0; i < indentLevel; ++i) {
        indentStr.append(indent);
    }

    assert(isValid());
    switch (type()) {
    case Type::Null:
        return "null";
    case Type::Bool:
        return asBool() ? "true" : "false";
    case Type::Number:
        return std::to_string(asNumber());
    case Type::String:
        // TODO: Escape characters
        return "\"" + asString() + "\"";
    case Type::Array: {
        std::string ret = "[\n";
        const auto& arr = asArray();
        for (size_t i = 0; i < arr.size(); ++i) {
            ret.append(indentStr);
            ret.append(indent);
            ret.append(arr[i].dump(indent, indentLevel + 1));
            if (i < arr.size() - 1) {
                ret.append(",");
            }
            ret.append("\n");
        }
        ret.append(indentStr);
        ret.append("]");
        return ret;
    }
    case Type::Object: {
        std::string ret = "{\n";
        size_t idx = 0;
        const auto& obj = asObject();
        for (const auto& [key, value] : obj) {
            ret.append(indentStr);
            ret.append(indent);
            ret.append("\"");
            ret.append(key);
            ret.append("\": ");
            ret.append(value.dump(indent, indentLevel + 1));
            if (idx < obj.size() - 1) {
                ret.append(",");
            }
            idx++;
            ret.append("\n");
        }
        ret.append(indentStr);
        ret.append("}");
        return ret;
    }
    default:
        assert(false && "Invalid JsonValue type");
    }
}

const JsonValue& JsonValue::getNonExistent()
{
    static JsonValue v;
    return v;
}

std::string getContext(std::string_view str, size_t cursor)
{
    size_t lineStart = cursor;
    while (lineStart > 0 && str[lineStart] != '\n')
        lineStart--;
    if (lineStart < cursor && str[lineStart] == '\n')
        lineStart++;

    size_t lineEnd = lineStart;
    while (lineEnd < str.size() && str[lineEnd] != '\n')
        lineEnd++;

    return std::string(str.substr(lineStart, lineEnd - lineStart)) + "\n"
        + std::string(cursor - lineStart, ' ') + "^";
}

Result<JsonValue> parse(std::string_view source)
{
    size_t cursor = 0;
    return parseValue(source, cursor);
}
}
