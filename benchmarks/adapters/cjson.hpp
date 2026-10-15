#pragma once

#include <cstring>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <cJSON.h>

#include "common/stat.hpp"

namespace cjson_detail
{

    struct Deleter
    {
        void operator() (cJSON* item) const noexcept
        {
            cJSON_Delete (item);
        }
    };

    inline void gen_stat (Stat& stat, const cJSON* v)
    {
        if (cJSON_IsObject (v))
        {
            stat.object_count++;
            for (const cJSON* child = v->child; child; child = child->next)
            {
                ++stat.member_count;
                ++stat.string_count;
                stat.string_length += std::strlen (child->string);
                gen_stat (stat, child);
            }
        }
        else if (cJSON_IsArray (v))
        {
            stat.array_count++;
            for (const cJSON* child = v->child; child; child = child->next)
            {
                ++stat.element_count;
                gen_stat (stat, child);
            }
        }
        else if (cJSON_IsString (v))
        {
            stat.string_count++;
            stat.string_length += std::strlen (v->valuestring);
        }
        else if (cJSON_IsNumber (v))
            stat.number_count++;
        else if (cJSON_IsTrue (v))
            stat.true_count++;
        else if (cJSON_IsFalse (v))
            stat.false_count++;
        else if (cJSON_IsNull (v))
            stat.null_count++;
    }

    struct FreeDeleter
    {
        void operator() (char* json) const noexcept
        {
            cJSON_free (json);
        }
    };

}

struct CJsonAdapter
{
    static constexpr std::string_view name = "cjson";

    using Document = std::unique_ptr<cJSON, cjson_detail::Deleter>;

    static std::optional<Document> parse (std::string_view json)
    {
        Document doc (cJSON_ParseWithLength (json.data (), json.size ()));
        if (!doc)
            return std::nullopt;
        return doc;
    }

    static constexpr bool lossy_numbers = true;

    using Output = std::unique_ptr<char, cjson_detail::FreeDeleter>;

    static Output stringify (const Document& doc)
    {
        return Output (cJSON_PrintUnformatted (doc.get ()));
    }

    static std::string_view view (const Output& out)
    {
        return out.get ();
    }

    static Stat statistics (const Document& doc)
    {
        Stat stat;
        cjson_detail::gen_stat (stat, doc.get ());
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
        const cJSON* elem = cJSON_GetArrayItem (doc->get (), 0);
        if (!cJSON_IsNumber (elem))
            return std::nullopt;
        return elem->valuedouble;
    }

    static std::optional<std::string> parse_string (std::string_view json)
    {
        auto doc = parse (json);
        if (!doc)
            return std::nullopt;
        const cJSON* elem = cJSON_GetArrayItem (doc->get (), 0);
        if (!cJSON_IsString (elem))
            return std::nullopt;
        return std::string (elem->valuestring);
    }
};
