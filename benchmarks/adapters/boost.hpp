#pragma once

#include <optional>
#include <string>
#include <string_view>

#include <boost/json.hpp>
#include <boost/system/error_code.hpp>

#include "common/stat.hpp"

namespace boost_detail
{

    inline void gen_stat (Stat& stat, const boost::json::value& v)
    {
        switch (v.kind ())
        {
            case boost::json::kind::object:
                {
                    auto& obj = v.as_object ();
                    stat.object_count++;
                    for (auto& kv : obj)
                    {
                        ++stat.member_count;
                        ++stat.string_count;
                        stat.string_length += kv.key ().size ();
                        gen_stat (stat, kv.value ());
                    }
                    break;
                }
            case boost::json::kind::array:
                {
                    auto& arr = v.as_array ();
                    stat.array_count++;
                    for (auto& elem : arr)
                    {
                        ++stat.element_count;
                        gen_stat (stat, elem);
                    }
                    break;
                }
            case boost::json::kind::string:
                stat.string_count++;
                stat.string_length += v.as_string ().size ();
                break;

            case boost::json::kind::int64:
            case boost::json::kind::uint64:
            case boost::json::kind::double_:
                stat.number_count++;
                break;

            case boost::json::kind::bool_:
                if (v.as_bool ())
                    stat.true_count++;
                else
                    stat.false_count++;
                break;

            case boost::json::kind::null:
                stat.null_count++;
                break;
        }
    }

}

template <boost::json::number_precision Precision>
struct BoostBase
{
    static constexpr bool lossy_numbers = Precision != boost::json::number_precision::precise;

    using Document = boost::json::value;

    static std::optional<Document> parse (std::string_view json)
    {
        static boost::json::parser parser ({}, [] {
            boost::json::parse_options opts;
            opts.numbers = Precision;
            return opts;
        }());
        boost::system::error_code ec;
        parser.reset ();
        parser.write (json.data (), json.size (), ec);
        if (ec)
            return std::nullopt;
        return parser.release ();
    }

    static std::string stringify (const Document& doc)
    {
        return boost::json::serialize (doc);
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        boost_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        return parse (json).has_value ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->is_array () || doc->as_array ().empty ())
            return std::nullopt;
        const auto& elem = doc->as_array ()[0];
        switch (elem.kind ())
        {
            case boost::json::kind::double_:
                return elem.as_double ();
            case boost::json::kind::int64:
                return static_cast<double> (elem.as_int64 ());
            case boost::json::kind::uint64:
                return static_cast<double> (elem.as_uint64 ());
            default:
                return std::nullopt;
        }
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc || !doc->is_array () || doc->as_array ().empty ())
            return std::nullopt;
        const auto& elem = doc->as_array ()[0];
        if (elem.kind () != boost::json::kind::string)
            return std::nullopt;
        return std::string (elem.as_string ());
    }
};

struct BoostAdapter : BoostBase<boost::json::number_precision::imprecise>
{
    static constexpr std::string_view name = "boost";
};

struct BoostFullPrecAdapter : BoostBase<boost::json::number_precision::precise>
{
    static constexpr std::string_view name = "boost/full-prec";
};
