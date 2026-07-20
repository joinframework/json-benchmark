#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <nlohmann/json.hpp>

#include "common/stat.hpp"

namespace nlohmann_detail
{

    inline void gen_stat (Stat& stat, const nlohmann::json& v)
    {
        switch (v.type ())
        {
            case nlohmann::json::value_t::object:
                stat.object_count++;
                for (auto& [key, val] : v.items ())
                {
                    ++stat.member_count;
                    ++stat.string_count;
                    stat.string_length += key.size ();
                    gen_stat (stat, val);
                }
                break;

            case nlohmann::json::value_t::array:
                stat.array_count++;
                for (auto& elem : v)
                {
                    ++stat.element_count;
                    gen_stat (stat, elem);
                }
                break;

            case nlohmann::json::value_t::string:
                stat.string_count++;
                stat.string_length += v.get_ref<const std::string&> ().size ();
                break;

            case nlohmann::json::value_t::number_integer:
            case nlohmann::json::value_t::number_unsigned:
            case nlohmann::json::value_t::number_float:
                stat.number_count++;
                break;

            case nlohmann::json::value_t::boolean:
                if (v.get<bool> ())
                    stat.true_count++;
                else
                    stat.false_count++;
                break;

            case nlohmann::json::value_t::null:
                stat.null_count++;
                break;

            default:
                break;
        }
    }

}

struct NlohmannAdapter
{
    static constexpr std::string_view name = "nlohmann";

    using Document = nlohmann::json;

    static std::optional<Document> parse (std::string_view json)
    {
        Document doc = Document::parse (json, nullptr, false);
        if (doc.is_discarded ())
            return std::nullopt;
        return doc;
    }

    static std::string stringify (const Document& doc)
    {
        return doc.dump ();
    }

    static std::string prettify (const Document& doc, int indent = 4)
    {
        return doc.dump (indent);
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        nlohmann_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        return Document::accept (json);
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->is_array () || doc->empty () || !(*doc)[0].is_number ())
            return std::nullopt;
        return (*doc)[0].get<double> ();
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->is_array () || doc->empty () || !(*doc)[0].is_string ())
            return std::nullopt;
        return (*doc)[0].get<std::string> ();
    }
};
