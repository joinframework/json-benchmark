#pragma once

#include <memory>
#include <optional>
#include <sstream>
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
                for (auto const& key : v.getMemberNames ())
                {
                    ++stat.member_count;
                    ++stat.string_count;
                    stat.string_length += key.size ();
                    gen_stat (stat, v[key]);
                }
                break;

            case Json::arrayValue:
                stat.array_count++;
                for (Json::ArrayIndex i = 0; i < v.size (); ++i)
                {
                    ++stat.element_count;
                    gen_stat (stat, v[i]);
                }
                break;

            case Json::stringValue:
                stat.string_count++;
                stat.string_length += v.asString ().size ();
                break;

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
            return std::unique_ptr<Json::StreamWriter> (b.newStreamWriter ());
        }();
        std::ostringstream oss;
        writer->write (doc, &oss);
        return oss.str ();
    }

    static std::string prettify (const Document& doc, int indent = 4)
    {
        Json::StreamWriterBuilder builder;
        builder["indentation"] = std::string (static_cast<size_t> (indent), ' ');
        builder["commentStyle"] = "None";
        return Json::writeString (builder, doc);
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
