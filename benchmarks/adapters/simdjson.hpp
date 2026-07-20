#pragma once

#include <cstdio>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>

#include <simdjson.h>

#include "common/stat.hpp"

namespace simdjson_detail
{

    inline std::string escape_json_string (std::string_view s)
    {
        std::string out;
        out.reserve (s.size () + 2);
        out += '"';
        for (unsigned char c : s)
        {
            switch (c)
            {
                case '"':
                    out += "\\\"";
                    break;
                case '\\':
                    out += "\\\\";
                    break;
                case '\b':
                    out += "\\b";
                    break;
                case '\f':
                    out += "\\f";
                    break;
                case '\n':
                    out += "\\n";
                    break;
                case '\r':
                    out += "\\r";
                    break;
                case '\t':
                    out += "\\t";
                    break;
                default:
                    if (c < 0x20)
                    {
                        char buf[7];
                        snprintf (buf, sizeof (buf), "\\u%04x", c);
                        out += buf;
                    }
                    else
                    {
                        out += static_cast<char> (c);
                    }
            }
        }
        out += '"';
        return out;
    }

    inline void pretty_impl (std::string& out, simdjson::dom::element e, int indent, int depth)
    {
        const auto pad_len   = static_cast<size_t> (depth * indent);
        const auto inner_len = static_cast<size_t> ((depth + 1) * indent);

        switch (e.type ())
        {
            case simdjson::dom::element_type::OBJECT:
                {
                    auto obj = e.get_object ().value ();
                    out += '{';
                    bool first = true;
                    for (auto field : obj)
                    {
                        if (!first)
                            out += ',';
                        out += '\n';
                        out.append (inner_len, ' ');
                        out += escape_json_string (field.key);
                        out += ": ";
                        pretty_impl (out, field.value, indent, depth + 1);
                        first = false;
                    }
                    if (!first)
                    {
                        out += '\n';
                        out.append (pad_len, ' ');
                    }
                    out += '}';
                    break;
                }
            case simdjson::dom::element_type::ARRAY:
                {
                    auto arr = e.get_array ().value ();
                    out += '[';
                    bool first = true;
                    for (auto elem : arr)
                    {
                        if (!first)
                            out += ',';
                        out += '\n';
                        out.append (inner_len, ' ');
                        pretty_impl (out, elem, indent, depth + 1);
                        first = false;
                    }
                    if (!first)
                    {
                        out += '\n';
                        out.append (pad_len, ' ');
                    }
                    out += ']';
                    break;
                }
            default:
                {
                    std::ostringstream oss;
                    oss << e;
                    out += oss.str ();
                    break;
                }
        }
    }

    inline void gen_stat (Stat& stat, simdjson::dom::element e)
    {
        switch (e.type ())
        {
            case simdjson::dom::element_type::OBJECT:
                {
                    auto obj = e.get_object ().value ();
                    ++stat.object_count;
                    for (auto field : obj)
                    {
                        ++stat.member_count;
                        ++stat.string_count;
                        stat.string_length += field.key.size ();
                        gen_stat (stat, field.value);
                    }
                    break;
                }
            case simdjson::dom::element_type::ARRAY:
                {
                    auto arr = e.get_array ().value ();
                    ++stat.array_count;
                    for (auto elem : arr)
                    {
                        ++stat.element_count;
                        gen_stat (stat, elem);
                    }
                    break;
                }
            case simdjson::dom::element_type::STRING:
                {
                    auto sv = e.get_string ().value ();
                    ++stat.string_count;
                    stat.string_length += sv.size ();
                    break;
                }
            case simdjson::dom::element_type::INT64:
            case simdjson::dom::element_type::UINT64:
            case simdjson::dom::element_type::DOUBLE:
                ++stat.number_count;
                break;
            case simdjson::dom::element_type::BOOL:
                if (e.get_bool ().value ())
                    ++stat.true_count;
                else
                    ++stat.false_count;
                break;
            case simdjson::dom::element_type::NULL_VALUE:
                ++stat.null_count;
                break;
        }
    }

}

struct SimdJsonAdapter
{
    static constexpr std::string_view name = "simdjson";

    using Document = simdjson::dom::element;

    static std::optional<Document> parse (std::string_view json)
    {
        static simdjson::dom::parser parser;
        simdjson::padded_string padded (json.data (), json.size ());
        simdjson::dom::element elem;
        if (parser.parse (padded).get (elem) != simdjson::SUCCESS)
            return std::nullopt;
        return elem;
    }

    static std::string stringify (const Document& doc)
    {
        std::ostringstream oss;
        oss << doc;
        return oss.str ();
    }

    static std::string prettify (const Document& doc, int indent = 4)
    {
        std::string out;
        out.reserve (4096);
        simdjson_detail::pretty_impl (out, doc, indent, 0);
        return out;
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        simdjson_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        return parse (json).has_value ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc)
            return std::nullopt;
        simdjson::dom::array arr;
        if (doc->get (arr) != simdjson::SUCCESS)
            return std::nullopt;
        auto it = arr.begin ();
        if (it == arr.end ())
            return std::nullopt;
        simdjson::dom::element first = *it;
        double d;
        if (first.get (d) == simdjson::SUCCESS)
            return d;
        int64_t i;
        if (first.get (i) == simdjson::SUCCESS)
            return static_cast<double> (i);
        uint64_t u;
        if (first.get (u) == simdjson::SUCCESS)
            return static_cast<double> (u);
        return std::nullopt;
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc)
            return std::nullopt;
        simdjson::dom::array arr;
        if (doc->get (arr) != simdjson::SUCCESS)
            return std::nullopt;
        auto it = arr.begin ();
        if (it == arr.end ())
            return std::nullopt;
        std::string_view sv;
        if ((*it).get (sv) != simdjson::SUCCESS)
            return std::nullopt;
        return std::string (sv);
    }
};
