#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <tao/json.hpp>

#include "common/stat.hpp"

namespace taocpp_detail
{

    inline void gen_stat (Stat& stat, const tao::json::value& v)
    {
        switch (v.type ())
        {
            case tao::json::type::OBJECT:
                stat.object_count++;
                for (auto& [key, val] : v.get_object ())
                {
                    ++stat.member_count;
                    ++stat.string_count;
                    stat.string_length += key.size ();
                    gen_stat (stat, val);
                }
                break;

            case tao::json::type::ARRAY:
                stat.array_count++;
                for (auto& elem : v.get_array ())
                {
                    ++stat.element_count;
                    gen_stat (stat, elem);
                }
                break;

            case tao::json::type::STRING:
                stat.string_count++;
                stat.string_length += v.get_string ().size ();
                break;

            case tao::json::type::STRING_VIEW:
                stat.string_count++;
                stat.string_length += v.get_string_view ().size ();
                break;

            case tao::json::type::SIGNED:
            case tao::json::type::UNSIGNED:
            case tao::json::type::DOUBLE:
                stat.number_count++;
                break;

            case tao::json::type::BOOLEAN:
                if (v.get_boolean ())
                    stat.true_count++;
                else
                    stat.false_count++;
                break;

            case tao::json::type::NULL_:
                stat.null_count++;
                break;

            default:
                break;
        }
    }

}

struct TaoCppAdapter
{
    static constexpr std::string_view name = "taocpp";

    using Document = tao::json::value;

    static std::optional<Document> parse (std::string_view json)
    {
        try
        {
            return tao::json::from_string (json);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    static std::string stringify (const Document& doc)
    {
        return tao::json::to_string (doc);
    }

    static std::string prettify (const Document& doc, int indent = 4)
    {
        return tao::json::to_string (doc, static_cast<std::size_t> (indent));
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        taocpp_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        return parse (json).has_value ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->is_array () || doc->get_array ().empty ())
            return std::nullopt;
        try
        {
            return doc->get_array ()[0].as<double> ();
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->is_array () || doc->get_array ().empty ())
            return std::nullopt;
        const auto& elem = doc->get_array ()[0];
        if (!elem.is_string ())
            return std::nullopt;
        if (elem.type () == tao::json::type::STRING)
            return elem.get_string ();
        return std::string (elem.get_string_view ());
    }
};
