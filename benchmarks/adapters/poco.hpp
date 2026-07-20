#pragma once

#include <optional>
#include <sstream>
#include <streambuf>
#include <string>
#include <string_view>

#include <Poco/Dynamic/Var.h>
#include <Poco/JSON/Array.h>
#include <Poco/JSON/Object.h>
#include <Poco/JSON/Parser.h>
#include <Poco/JSON/Stringifier.h>

#include "common/stat.hpp"

namespace poco_detail
{

    inline void gen_stat (Stat& stat, const Poco::Dynamic::Var& v)
    {
        if (v.type () == typeid (Poco::JSON::Object::Ptr))
        {
            auto obj = v.extract<Poco::JSON::Object::Ptr> ();
            stat.object_count++;
            for (const auto& key : obj->getNames ())
            {
                ++stat.member_count;
                ++stat.string_count;
                stat.string_length += key.size ();
                gen_stat (stat, obj->get (key));
            }
        }
        else if (v.type () == typeid (Poco::JSON::Array::Ptr))
        {
            auto arr = v.extract<Poco::JSON::Array::Ptr> ();
            stat.array_count++;
            for (size_t i = 0; i < arr->size (); ++i)
            {
                ++stat.element_count;
                gen_stat (stat, arr->get (i));
            }
        }
        else if (v.isString ())
        {
            stat.string_count++;
            stat.string_length += v.convert<std::string> ().size ();
        }
        else if (v.isBoolean ())
        {
            if (v.extract<bool> ())
                stat.true_count++;
            else
                stat.false_count++;
        }
        else if (v.isNumeric ())
        {
            stat.number_count++;
        }
        else
        {
            stat.null_count++;
        }
    }

    struct ViewBuf : std::streambuf
    {
        ViewBuf (const char* p, std::size_t n)
        {
            auto pp = const_cast<char*> (p);
            setg (pp, pp, pp + n);
        }
    };

}

struct PocoAdapter
{
    static constexpr std::string_view name = "poco";

    using Document = Poco::Dynamic::Var;

    static std::optional<Document> parse (std::string_view json)
    {
        try
        {
            poco_detail::ViewBuf buf (json.data (), json.size ());
            std::istream is (&buf);
            Poco::JSON::Parser parser;
            return parser.parse (is);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    static std::string stringify (const Document& doc)
    {
        std::ostringstream oss;
        Poco::JSON::Stringifier::stringify (doc, oss);
        return oss.str ();
    }

    static std::string prettify (const Document& doc, int indent = 4)
    {
        std::ostringstream oss;
        Poco::JSON::Stringifier::stringify (doc, oss, static_cast<unsigned int> (indent));
        return oss.str ();
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        poco_detail::gen_stat (stat, doc);
        return stat;
    }

    static bool accepts (std::string_view json)
    {
        return parse (json).has_value ();
    }

    static std::optional<double> parse_number (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc)
            return std::nullopt;
        try
        {
            auto arr = doc->extract<Poco::JSON::Array::Ptr> ();
            if (!arr || arr->size () == 0)
                return std::nullopt;
            return arr->getElement<double> (0);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc)
            return std::nullopt;
        try
        {
            auto arr = doc->extract<Poco::JSON::Array::Ptr> ();
            if (!arr || arr->size () == 0)
                return std::nullopt;
            return arr->getElement<std::string> (0);
        }
        catch (...)
        {
            return std::nullopt;
        }
    }
};
