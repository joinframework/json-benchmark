#pragma once

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <new>
#include <streambuf>
#include <string_view>

/// output of the libraries that only write to a std::ostream (Join, JsonCpp, POCO).
struct StreamOutput
{
    struct FreeDeleter
    {
        void operator() (char* data) const noexcept
        {
            std::free (data);
        }
    };

    std::unique_ptr<char, FreeDeleter> data;
    size_t size = 0;

    std::string_view view () const
    {
        return {data.get (), size};
    }
};

/// stream buffer shared by every std::ostream based adapter, so they all write into the same sink.
struct OutputStreambuf : std::streambuf
{
    StreamOutput out;
    size_t capacity = 0;

    explicit OutputStreambuf (size_t reserve = 256)
    {
        grow (reserve);
    }

    void grow (size_t n)
    {
        if (out.size + n <= capacity)
            return;
        size_t cap = std::max (capacity + (capacity + 1) / 2, out.size + n);
        char* data = static_cast<char*> (std::realloc (out.data.get (), cap));
        if (data == nullptr)
            throw std::bad_alloc ();
        out.data.release ();
        out.data.reset (data);
        capacity = cap;
    }

    int_type overflow (int_type c) override
    {
        if (c != EOF)
        {
            grow (1);
            out.data.get ()[out.size++] = static_cast<char> (c);
        }
        return c;
    }

    std::streamsize xsputn (const char* s, std::streamsize n) override
    {
        grow (static_cast<size_t> (n));
        std::memcpy (out.data.get () + out.size, s, static_cast<size_t> (n));
        out.size += static_cast<size_t> (n);
        return n;
    }
};
