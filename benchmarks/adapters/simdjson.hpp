#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <simdjson.h>

#include "common/stat.hpp"

namespace simdjson_detail
{

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

    using Document = simdjson::dom::document;

    static std::optional<Document> parse (std::string_view json)
    {
        static simdjson::dom::parser parser;
        simdjson::padded_string padded (json.data (), json.size ());
        Document doc;
        if (parser.parse_into_document (doc, padded).error () != simdjson::SUCCESS)
            return std::nullopt;
        return doc;
    }

    static std::string stringify (const Document& doc)
    {
        return simdjson::to_string (doc.root ());
    }

    static std::string prettify (const Document& doc, int = 4)
    {
        return simdjson::prettify (doc.root ());
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        simdjson_detail::gen_stat (stat, doc.root ());
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
        if (doc->root ().get (arr) != simdjson::SUCCESS)
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
        if (doc->root ().get (arr) != simdjson::SUCCESS)
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
