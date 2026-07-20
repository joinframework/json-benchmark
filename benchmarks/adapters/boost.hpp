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

    inline void pretty_write (const boost::json::value& v, std::string& out, int indent, int depth)
    {
        const auto pad_len   = static_cast<size_t> (depth * indent);
        const auto inner_len = static_cast<size_t> ((depth + 1) * indent);

        switch (v.kind ())
        {
            case boost::json::kind::object:
                {
                    auto& obj = v.as_object ();
                    out += "{\n";
                    bool first = true;
                    for (auto& kv : obj)
                    {
                        if (!first)
                            out += ",\n";
                        out.append (inner_len, ' ');
                        out += '"';
                        out.append (kv.key ().data (), kv.key ().size ());
                        out += "\": ";
                        pretty_write (kv.value (), out, indent, depth + 1);
                        first = false;
                    }
                    if (!obj.empty ())
                        out += '\n';
                    out.append (pad_len, ' ');
                    out += '}';
                    break;
                }
            case boost::json::kind::array:
                {
                    auto& arr = v.as_array ();
                    out += "[\n";
                    bool first = true;
                    for (auto& elem : arr)
                    {
                        if (!first)
                            out += ",\n";
                        out.append (inner_len, ' ');
                        pretty_write (elem, out, indent, depth + 1);
                        first = false;
                    }
                    if (!arr.empty ())
                        out += '\n';
                    out.append (pad_len, ' ');
                    out += ']';
                    break;
                }
            default:
                out += boost::json::serialize (v);
                break;
        }
    }

}

struct BoostAdapter
{
    static constexpr std::string_view name = "boost";

    using Document = boost::json::value;

    static std::optional<Document> parse (std::string_view json)
    {
        boost::system::error_code ec;
        Document doc = boost::json::parse (json, ec);
        if (ec)
            return std::nullopt;
        return doc;
    }

    static std::string stringify (const Document& doc)
    {
        return boost::json::serialize (doc);
    }

    static std::string prettify (const Document& doc, int indent = 4)
    {
        std::string out;
        boost_detail::pretty_write (doc, out, indent, 0);
        return out;
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        boost_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        boost::system::error_code ec;
        boost::json::parse (json, ec);
        return !ec;
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
