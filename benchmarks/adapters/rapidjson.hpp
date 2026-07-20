#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <rapidjson/document.h>
#include <rapidjson/encodedstream.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "common/stat.hpp"

namespace rapidjson_detail
{

    inline void gen_stat (Stat& stat, const rapidjson::Value& v)
    {
        switch (v.GetType ())
        {
            case rapidjson::kObjectType:
                stat.object_count++;
                for (auto it = v.MemberBegin (); it != v.MemberEnd (); ++it)
                {
                    ++stat.member_count;
                    ++stat.string_count;
                    stat.string_length += it->name.GetStringLength ();
                    gen_stat (stat, it->value);
                }
                break;

            case rapidjson::kArrayType:
                stat.array_count++;
                for (rapidjson::SizeType i = 0; i < v.Size (); ++i)
                {
                    ++stat.element_count;
                    gen_stat (stat, v[i]);
                }
                break;

            case rapidjson::kStringType:
                stat.string_count++;
                stat.string_length += v.GetStringLength ();
                break;

            case rapidjson::kNumberType:
                stat.number_count++;
                break;
            case rapidjson::kTrueType:
                stat.true_count++;
                break;
            case rapidjson::kFalseType:
                stat.false_count++;
                break;
            case rapidjson::kNullType:
                stat.null_count++;
                break;
        }
    }

    inline std::string do_stringify (const rapidjson::Value& v)
    {
        rapidjson::StringBuffer sb;
        rapidjson::Writer<rapidjson::StringBuffer> writer (sb);
        v.Accept (writer);
        return {sb.GetString (), sb.GetSize ()};
    }

    inline std::string do_prettify (const rapidjson::Value& v, int indent = 4)
    {
        rapidjson::StringBuffer sb;
        rapidjson::PrettyWriter<rapidjson::StringBuffer> writer (sb);
        writer.SetIndent (' ', static_cast<unsigned> (indent));
        v.Accept (writer);
        return {sb.GetString (), sb.GetSize ()};
    }

}

template <unsigned Flags>
struct RapidJsonCopyBase
{
    using Document = rapidjson::Document;

    static std::optional<Document> parse (std::string_view json)
    {
        Document doc;
        doc.Parse<Flags> (json.data (), json.size ());
        if (doc.HasParseError ())
            return std::nullopt;
        return doc;
    }

    static std::string stringify (const Document& d)
    {
        return rapidjson_detail::do_stringify (d);
    }

    static std::string prettify (const Document& d, int i = 4)
    {
        return rapidjson_detail::do_prettify (d, i);
    }

    static Stat statistics (const Document& d)
    {
        Stat stat;
        rapidjson_detail::gen_stat (stat, d);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        Document doc;
        doc.Parse<Flags> (json.data (), json.size ());
        return !doc.HasParseError ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->IsArray () || doc->Size () < 1 || !(*doc)[0].IsNumber ())
            return std::nullopt;
        return (*doc)[0].GetDouble ();
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->IsArray () || doc->Size () < 1 || !(*doc)[0].IsString ())
            return std::nullopt;
        return std::string ((*doc)[0].GetString (), (*doc)[0].GetStringLength ());
    }
};

template <unsigned ExtraFlags = rapidjson::kParseDefaultFlags>
struct RapidJsonInsituBase
{
    struct Document
    {
        std::vector<char> buf;
        rapidjson::Document doc;

        Document () = default;
        Document (Document&&) = default;
        Document& operator= (Document&&) = default;
        Document (const Document&) = delete;
        Document& operator= (const Document&) = delete;
    };

    static std::optional<Document> parse (std::string_view json)
    {
        Document result;
        result.buf.assign (json.begin (), json.end ());
        result.buf.push_back ('\0');
        result.doc.template ParseInsitu<ExtraFlags> (result.buf.data ());
        if (result.doc.HasParseError ())
            return std::nullopt;
        return result;
    }

    static std::string stringify (const Document& d)
    {
        return rapidjson_detail::do_stringify (d.doc);
    }

    static std::string prettify (const Document& d, int i = 4)
    {
        return rapidjson_detail::do_prettify (d.doc, i);
    }

    static Stat statistics (const Document& d)
    {
        Stat stat;
        rapidjson_detail::gen_stat (stat, d.doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        std::vector<char> buf (json.begin (), json.end ());
        buf.push_back ('\0');
        rapidjson::Document doc;
        doc.template ParseInsitu<ExtraFlags> (buf.data ());
        return !doc.HasParseError ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto d = parse (json);
        if (!d || !d->doc.IsArray () || d->doc.Size () < 1 || !d->doc[0].IsNumber ())
            return std::nullopt;
        return d->doc[0].GetDouble ();
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto d = parse (json);
        if (!d || !d->doc.IsArray () || d->doc.Size () < 1 || !d->doc[0].IsString ())
            return std::nullopt;
        return std::string (d->doc[0].GetString (), d->doc[0].GetStringLength ());
    }
};

struct RapidJsonAdapter : RapidJsonCopyBase<rapidjson::kParseDefaultFlags>
{
    static constexpr std::string_view name = "Default";
};

struct RapidJsonFullPrecAdapter : RapidJsonCopyBase<rapidjson::kParseFullPrecisionFlag>
{
    static constexpr std::string_view name = "FullPrec";
};

struct RapidJsonIterativeAdapter : RapidJsonCopyBase<rapidjson::kParseIterativeFlag>
{
    static constexpr std::string_view name = "Iterative";
};

struct RapidJsonInsituAdapter : RapidJsonInsituBase<rapidjson::kParseDefaultFlags>
{
    static constexpr std::string_view name = "Insitu";
};

struct RapidJsonAutoUtfAdapter
{
    static constexpr std::string_view name = "AutoUTF";
    using Document = rapidjson::Document;

    static std::optional<Document> parse (std::string_view json)
    {
        Document doc;
        rapidjson::MemoryStream ms (json.data (), json.size ());
        rapidjson::AutoUTFInputStream<unsigned, rapidjson::MemoryStream> eis (ms);
        doc.ParseStream<rapidjson::kParseDefaultFlags, rapidjson::AutoUTF<unsigned>> (eis);
        if (doc.HasParseError ())
            return std::nullopt;
        return doc;
    }

    static std::string stringify (const Document& d)
    {
        return rapidjson_detail::do_stringify (d);
    }

    static std::string prettify (const Document& d, int i = 4)
    {
        return rapidjson_detail::do_prettify (d, i);
    }

    static Stat statistics (const Document& d)
    {
        Stat stat;
        rapidjson_detail::gen_stat (stat, d);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        Document doc;
        rapidjson::MemoryStream ms (json.data (), json.size ());
        rapidjson::AutoUTFInputStream<unsigned, rapidjson::MemoryStream> eis (ms);
        doc.ParseStream<rapidjson::kParseDefaultFlags, rapidjson::AutoUTF<unsigned>> (eis);
        return !doc.HasParseError ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->IsArray () || doc->Size () < 1 || !(*doc)[0].IsNumber ())
            return std::nullopt;
        return (*doc)[0].GetDouble ();
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->IsArray () || doc->Size () < 1 || !(*doc)[0].IsString ())
            return std::nullopt;
        return std::string ((*doc)[0].GetString (), (*doc)[0].GetStringLength ());
    }
};
