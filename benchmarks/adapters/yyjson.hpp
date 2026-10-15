#pragma once

#include <cstdlib>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <yyjson.h>

#include "common/stat.hpp"

namespace yyjson_detail
{

    struct DocDeleter
    {
        void operator() (yyjson_doc* doc) const noexcept
        {
            yyjson_doc_free (doc);
        }
    };

    inline void gen_stat (Stat& stat, yyjson_val* v)
    {
        switch (yyjson_get_type (v))
        {
            case YYJSON_TYPE_OBJ:
                {
                    stat.object_count++;
                    yyjson_obj_iter it = yyjson_obj_iter_with (v);
                    yyjson_val* key;
                    while ((key = yyjson_obj_iter_next (&it)))
                    {
                        ++stat.member_count;
                        ++stat.string_count;
                        stat.string_length += yyjson_get_len (key);
                        gen_stat (stat, yyjson_obj_iter_get_val (key));
                    }
                    break;
                }
            case YYJSON_TYPE_ARR:
                {
                    stat.array_count++;
                    yyjson_arr_iter it = yyjson_arr_iter_with (v);
                    yyjson_val* elem;
                    while ((elem = yyjson_arr_iter_next (&it)))
                    {
                        ++stat.element_count;
                        gen_stat (stat, elem);
                    }
                    break;
                }
            case YYJSON_TYPE_STR:
                stat.string_count++;
                stat.string_length += yyjson_get_len (v);
                break;

            case YYJSON_TYPE_NUM:
                stat.number_count++;
                break;

            case YYJSON_TYPE_BOOL:
                if (yyjson_get_bool (v))
                    stat.true_count++;
                else
                    stat.false_count++;
                break;

            case YYJSON_TYPE_NULL:
                stat.null_count++;
                break;

            default:
                break;
        }
    }

    struct FreeDeleter
    {
        void operator() (char* json) const noexcept
        {
            std::free (json);
        }
    };

    struct Output
    {
        std::unique_ptr<char, FreeDeleter> json;
        size_t size = 0;
    };

    inline Output write (const yyjson_doc* doc, yyjson_write_flag flags)
    {
        Output out;
        out.json.reset (yyjson_write (doc, flags, &out.size));
        return out;
    }

}

struct YyjsonAdapter
{
    static constexpr std::string_view name = "yyjson";

    using Document = std::unique_ptr<yyjson_doc, yyjson_detail::DocDeleter>;

    static std::optional<Document> parse (std::string_view json)
    {
        Document doc (yyjson_read (json.data (), json.size (), YYJSON_READ_NOFLAG));
        if (!doc)
            return std::nullopt;
        return doc;
    }

    static yyjson_detail::Output stringify (const Document& doc)
    {
        return yyjson_detail::write (doc.get (), YYJSON_WRITE_NOFLAG);
    }

    static yyjson_detail::Output prettify (const Document& doc, int = 4)
    {
        return yyjson_detail::write (doc.get (), YYJSON_WRITE_PRETTY);
    }

    static std::string_view view (const yyjson_detail::Output& out)
    {
        return {out.json.get (), out.size};
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        yyjson_detail::gen_stat (stat, yyjson_doc_get_root (doc.get ()));
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
        yyjson_val* elem = yyjson_arr_get_first (yyjson_doc_get_root (doc->get ()));
        if (!yyjson_is_num (elem))
            return std::nullopt;
        return yyjson_get_num (elem);
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc)
            return std::nullopt;
        yyjson_val* elem = yyjson_arr_get_first (yyjson_doc_get_root (doc->get ()));
        if (!yyjson_is_str (elem))
            return std::nullopt;
        return std::string (yyjson_get_str (elem), yyjson_get_len (elem));
    }
};
