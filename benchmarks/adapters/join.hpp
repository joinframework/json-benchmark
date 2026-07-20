#pragma once

#include <optional>
#include <streambuf>
#include <string>
#include <string_view>

#include <join/json.hpp>

#include "common/stat.hpp"

struct StringStreambuf : std::streambuf
{
    std::string str;

    explicit StringStreambuf (size_t reserve = 256)
    {
        str.reserve (reserve);
    }

    int_type overflow (int_type c) override
    {
        if (c != EOF)
            str.push_back (static_cast<char> (c));
        return c;
    }

    std::streamsize xsputn (const char* s, std::streamsize n) override
    {
        str.append (s, static_cast<size_t> (n));
        return n;
    }
};

namespace join_detail
{

    inline void gen_stat (Stat& stat, const join::Value& v)
    {
        switch (v.index ())
        {
            case join::Value::ArrayValue:
                stat.array_count++;
                for (auto& elem : v.getArray ())
                {
                    ++stat.element_count;
                    gen_stat (stat, elem);
                }
                break;

            case join::Value::ObjectValue:
                stat.object_count++;
                for (auto& [key, val] : v.getObject ())
                {
                    ++stat.member_count;
                    ++stat.string_count;
                    stat.string_length += key.size ();
                    gen_stat (stat, val);
                }
                break;

            case join::Value::String:
                stat.string_count++;
                stat.string_length += v.size ();
                break;

            case join::Value::Integer:
            case join::Value::Unsigned:
            case join::Value::Integer64:
            case join::Value::Unsigned64:
            case join::Value::Real:
                stat.number_count++;
                break;

            case join::Value::Boolean:
                if (v.isTrue ())
                    stat.true_count++;
                else
                    stat.false_count++;
                break;

            case join::Value::Null:
                stat.null_count++;
                break;
        }
    }

}

struct JoinAdapter
{
    static constexpr std::string_view name = "join";

    using Document = join::Value;

    static std::optional<Document> parse (std::string_view json)
    {
        Document doc;
        if (doc.deserialize<join::JsonReader> (json.data (), json.size ()) == -1)
            return std::nullopt;
        return doc;
    }

    static std::string stringify (const Document& doc)
    {
        StringStreambuf buf;
        std::ostream os (&buf);
        doc.serialize<join::JsonWriter> (os);
        return std::move (buf.str);
    }

    static std::string prettify (const Document& doc, int indent = 4)
    {
        StringStreambuf buf;
        std::ostream os (&buf);
        join::JsonWriter writer (os, static_cast<size_t> (indent));
        writer.serialize (doc);
        return std::move (buf.str);
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        join_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        Document doc;
        return doc.deserialize<join::JsonReader> (json.data (), json.size ()) != -1;
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->isArray () || doc->getArray ().empty ())
            return std::nullopt;
        const auto& elem = doc->getArray ()[0];
        if (!elem.isNumber ())
            return std::nullopt;
        return elem.getDouble ();
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->isArray () || doc->getArray ().empty ())
            return std::nullopt;
        const auto& elem = doc->getArray ()[0];
        if (!elem.isString ())
            return std::nullopt;
        return elem.getString ();
    }
};
