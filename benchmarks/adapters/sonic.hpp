#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <sonic/sonic.h>

#include "common/stat.hpp"

namespace sonic_detail
{

    template <typename Node>
    inline void gen_stat (Stat& stat, const Node& v)
    {
        if (v.IsObject ())
        {
            stat.object_count++;
            for (auto it = v.MemberBegin (); it != v.MemberEnd (); ++it)
            {
                ++stat.member_count;
                ++stat.string_count;
                stat.string_length += it->name.Size ();
                gen_stat (stat, it->value);
            }
        }
        else if (v.IsArray ())
        {
            stat.array_count++;
            for (auto it = v.Begin (); it != v.End (); ++it)
            {
                ++stat.element_count;
                gen_stat (stat, *it);
            }
        }
        else if (v.IsString ())
        {
            stat.string_count++;
            stat.string_length += v.Size ();
        }
        else if (v.IsNumber ())
            stat.number_count++;
        else if (v.IsTrue ())
            stat.true_count++;
        else if (v.IsFalse ())
            stat.false_count++;
        else if (v.IsNull ())
            stat.null_count++;
    }

}

struct SonicAdapter
{
    static constexpr std::string_view name = "sonic";

    using Document = sonic_json::Document;

    static std::optional<Document> parse (std::string_view json)
    {
        Document doc;
        doc.Parse (json.data (), json.size ());
        if (doc.HasParseError ())
            return std::nullopt;
        return doc;
    }

    static sonic_json::WriteBuffer stringify (const Document& doc)
    {
        sonic_json::WriteBuffer wb;
        doc.Serialize (wb);
        return wb;
    }

    static std::string_view view (const sonic_json::WriteBuffer& wb)
    {
        return {wb.ToString (), wb.Size ()};
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        sonic_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        return parse (json).has_value ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->IsArray () || doc->Empty () || !(*doc)[0].IsNumber ())
            return std::nullopt;
        return (*doc)[0].GetDouble ();
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->IsArray () || doc->Empty () || !(*doc)[0].IsString ())
            return std::nullopt;
        auto sv = (*doc)[0].GetStringView ();
        return std::string (sv.data (), sv.size ());
    }
};
