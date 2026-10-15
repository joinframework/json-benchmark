#pragma once

#include <memory>
#include <optional>
#include <ostream>
#include <streambuf>
#include <string>
#include <string_view>

#include <json/json.h>

#include "common/stat.hpp"

namespace jsoncpp_detail
{

    inline void gen_stat (Stat& stat, const Json::Value& v)
    {
        switch (v.type ())
        {
            case Json::objectValue:
                stat.object_count++;
                for (auto it = v.begin (); it != v.end (); ++it)
                {
                    char const* end = nullptr;
                    char const* begin = it.memberName (&end);
                    ++stat.member_count;
                    ++stat.string_count;
                    stat.string_length += static_cast<size_t> (end - begin);
                    gen_stat (stat, *it);
                }
                break;

            case Json::arrayValue:
                stat.array_count++;
                for (auto const& elem : v)
                {
                    ++stat.element_count;
                    gen_stat (stat, elem);
                }
                break;

            case Json::stringValue:
                {
                    char const* begin = nullptr;
                    char const* end = nullptr;
                    v.getString (&begin, &end);
                    stat.string_count++;
                    stat.string_length += static_cast<size_t> (end - begin);
                    break;
                }

            case Json::intValue:
            case Json::uintValue:
            case Json::realValue:
                stat.number_count++;
                break;

            case Json::booleanValue:
                if (v.asBool ())
                    stat.true_count++;
                else
                    stat.false_count++;
                break;

            case Json::nullValue:
                stat.null_count++;
                break;
        }
    }

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

}

struct JsonCppAdapter
{
    static constexpr std::string_view name = "jsoncpp";

    using Document = Json::Value;

    static std::optional<Document> parse (std::string_view json)
    {
        static const std::unique_ptr<Json::CharReader> reader = [] {
            Json::CharReaderBuilder b;
            b["collectComments"] = false;
            return std::unique_ptr<Json::CharReader> (b.newCharReader ());
        }();
        Document doc;
        std::string errors;
        if (!reader->parse (json.data (), json.data () + json.size (), &doc, &errors))
            return std::nullopt;
        return doc;
    }

    static std::string stringify (const Document& doc)
    {
        static const std::unique_ptr<Json::StreamWriter> writer = [] {
            Json::StreamWriterBuilder b;
            b["indentation"] = "";
            b["commentStyle"] = "None";
            b["emitUTF8"] = true;
            return std::unique_ptr<Json::StreamWriter> (b.newStreamWriter ());
        }();
        jsoncpp_detail::StringStreambuf buf;
        std::ostream os (&buf);
        writer->write (doc, &os);
        return std::move (buf.str);
    }

    static std::string prettify (const Document& doc, int = 4)
    {
        static const std::unique_ptr<Json::StreamWriter> writer = [] {
            Json::StreamWriterBuilder b;
            b["indentation"] = "    ";
            b["commentStyle"] = "None";
            b["emitUTF8"] = true;
            return std::unique_ptr<Json::StreamWriter> (b.newStreamWriter ());
        }();
        jsoncpp_detail::StringStreambuf buf;
        std::ostream os (&buf);
        writer->write (doc, &os);
        return std::move (buf.str);
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        jsoncpp_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        return parse (json).has_value ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->isArray () || doc->empty () || !(*doc)[0].isNumeric ())
            return std::nullopt;
        return (*doc)[0].asDouble ();
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->isArray () || doc->empty () || !(*doc)[0].isString ())
            return std::nullopt;
        return (*doc)[0].asString ();
    }
};
