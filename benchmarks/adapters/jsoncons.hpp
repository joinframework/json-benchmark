#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <jsoncons/json.hpp>

#include "common/stat.hpp"

namespace jsoncons_detail
{
    STAT_WALK inline void gen_stat (Stat& stat, const jsoncons::json& v)
    {
        switch (v.type ())
        {
            case jsoncons::json_type::object:
                stat.object_count++;
                for (auto& kv : v.object_range ())
                {
                    ++stat.member_count;
                    ++stat.string_count;
                    stat.string_length += kv.key ().size ();
                    gen_stat (stat, kv.value ());
                }
                break;
            case jsoncons::json_type::array:
                stat.array_count++;
                for (auto& elem : v.array_range ())
                {
                    ++stat.element_count;
                    gen_stat (stat, elem);
                }
                break;
            case jsoncons::json_type::string:
                if (v.is_number ())
                {
                    stat.number_count++;
                    break;
                }
                stat.string_count++;
                stat.string_length += v.as_string_view ().size ();
                break;
            case jsoncons::json_type::int64:
            case jsoncons::json_type::uint64:
            case jsoncons::json_type::float16:
            case jsoncons::json_type::float64:
                stat.number_count++;
                break;
            case jsoncons::json_type::boolean:
                if (v.as<bool> ())
                {
                    stat.true_count++;
                }
                else
                {
                    stat.false_count++;
                }
                break;
            case jsoncons::json_type::null:
                stat.null_count++;
                break;
            default:
                break;
        }
    }
}

template <bool Lossless>
struct JsonConsBase
{
    using Document = jsoncons::json;

    static std::optional<Document> parse (std::string_view json)
    {
        try
        {
            jsoncons::json_options options;
            options.lossless_number (Lossless);
            return jsoncons::json::parse (jsoncons::string_view (json.data (), json.size ()), options);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    static std::string stringify (const Document& doc)
    {
        std::string s;
        if constexpr (Lossless)
        {
            jsoncons::json_options options;
            options.bignum_format (jsoncons::bignum_format_kind::number);
            doc.dump (s, options);
        }
        else
        {
            doc.dump (s);
        }
        return s;
    }

    static std::string prettify (const Document& doc, int indent = 4)
    {
        jsoncons::json_options options;
        options.indent_size (static_cast<uint8_t> (indent));
        if constexpr (Lossless)
        {
            options.bignum_format (jsoncons::bignum_format_kind::number);
        }
        std::string s;
        doc.dump_pretty (s, options);
        return s;
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        jsoncons_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        return parse (json).has_value ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->is_array () || doc->empty ())
        {
            return std::nullopt;
        }
        const auto& elem = (*doc)[0];
        if (!elem.is_number ())
        {
            return std::nullopt;
        }
        return elem.as<double> ();
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->is_array () || doc->empty ())
        {
            return std::nullopt;
        }
        const auto& elem = (*doc)[0];
        if (!elem.is_string ())
        {
            return std::nullopt;
        }
        return std::string (elem.as_string_view ());
    }
};

struct JsonConsAdapter : JsonConsBase<false>
{
    static constexpr std::string_view name = "jsoncons";
};

struct JsonConsLosslessAdapter : JsonConsBase<true>
{
    static constexpr std::string_view name = "jsoncons/lossless";
};
