#include <algorithm>
#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

#include "adapters/boost.hpp"
#include "adapters/cjson.hpp"
#include "adapters/join.hpp"
#include "adapters/jsoncons.hpp"
#include "adapters/jsoncpp.hpp"
#include "adapters/nlohmann.hpp"
#include "adapters/poco.hpp"
#include "adapters/rapidjson.hpp"
#include "adapters/simdjson.hpp"
#include "adapters/sonic.hpp"
#include "adapters/taocpp.hpp"
#include "adapters/yyjson.hpp"
#include "common/utils.hpp"

namespace fs = std::filesystem;

struct Result
{
    int checker_pass = 0;
    int checker_fail = 0;
    int double_pass = 0;
    int double_fail = 0;
    int string_pass = 0;
    int string_fail = 0;
    int roundtrip_pass = 0;
    int roundtrip_fail = 0;
};

template <typename Adapter>
void run_checker (Result& r, const std::vector<std::string>& fail_files, const std::vector<std::string>& pass_files,
                  bool verbose)
{
    for (auto& path : fail_files)
    {
        auto json = load_file (path);
        bool accepted = Adapter::accepts (json);
        if (!accepted)
            r.checker_pass++;
        else
        {
            r.checker_fail++;
            if (verbose)
                std::cerr << "  [FAIL] should reject: " << fs::path (path).filename ().string () << "\n";
        }
    }
    for (auto& path : pass_files)
    {
        auto json = load_file (path);
        bool accepted = Adapter::accepts (json);
        if (accepted)
            r.checker_pass++;
        else
        {
            r.checker_fail++;
            if (verbose)
                std::cerr << "  [FAIL] should accept: " << fs::path (path).filename ().string () << "\n";
        }
    }
}

static bool doubles_bitwise_eq (double a, double b)
{
    uint64_t ua, ub;
    std::memcpy (&ua, &a, 8);
    std::memcpy (&ub, &b, 8);
    return ua == ub;
}

template <typename Adapter>
void run_parse_double (Result& r, bool verbose)
{
    auto test = [&] (const char* json, double expected) {
        auto val = Adapter::parse_number (json);
        bool ok = val && doubles_bitwise_eq (*val, expected);
        if (ok)
            r.double_pass++;
        else
        {
            r.double_fail++;
            if (verbose)
                std::cerr << "  [FAIL] parse_double: " << json << "\n";
        }
    };

    test ("[0.0]", 0.0);
    test ("[-0.0]", -0.0);
    test ("[1.0]", 1.0);
    test ("[-1.0]", -1.0);
    test ("[1.5]", 1.5);
    test ("[-1.5]", -1.5);
    test ("[3.1416]", 3.1416);
    test ("[1E10]", 1E10);
    test ("[1e10]", 1e10);
    test ("[1E+10]", 1E+10);
    test ("[1E-10]", 1E-10);
    test ("[-1E10]", -1E10);
    test ("[-1e10]", -1e10);
    test ("[-1E+10]", -1E+10);
    test ("[-1E-10]", -1E-10);
    test ("[1.234E+10]", 1.234E+10);
    test ("[1.234E-10]", 1.234E-10);
    test ("[1.79769e+308]", 1.79769e+308);
    test ("[2.22507e-308]", 2.22507e-308);
    test ("[-1.79769e+308]", -1.79769e+308);
    test ("[-2.22507e-308]", -2.22507e-308);
    test ("[4.9406564584124654e-324]", 4.9406564584124654e-324);
    test ("[2.2250738585072009e-308]", 2.2250738585072009e-308);
    test ("[2.2250738585072014e-308]", 2.2250738585072014e-308);
    test ("[1.7976931348623157e+308]", 1.7976931348623157e+308);
    test ("[1e-10000]", 0.0);
    test ("[18446744073709551616]", 18446744073709551616.0);
    test ("[-9223372036854775809]", -9223372036854775809.0);
    test ("[0.9868011474609375]", 0.9868011474609375);
    test ("[123e34]", 123e34);
    test ("[45913141877270640000.0]", 45913141877270640000.0);
    test ("[2.2250738585072011e-308]", 2.2250738585072011e-308);
    test ("[1e-214748363]", 0.0);
    test ("[1e-214748364]", 0.0);
    test ("[0.017976931348623157e+310]", 1.7976931348623157e+308);
    test ("[2.2250738585072012e-308]", 2.2250738585072014e-308);
    test ("[2.22507385850720113605740979670913197593481954635164564e-308]", 2.2250738585072009e-308);
    test ("[2.22507385850720113605740979670913197593481954635164565e-308]", 2.2250738585072014e-308);
    test ("[0.999999999999999944488848768742172978818416595458984375]", 1.0);
    test ("[0.999999999999999944488848768742172978818416595458984374]", 0.99999999999999989);
    test ("[0.999999999999999944488848768742172978818416595458984376]", 1.0);
    test ("[1.00000000000000011102230246251565404236316680908203125]", 1.0);
    test ("[1.00000000000000011102230246251565404236316680908203124]", 1.0);
    test ("[1.00000000000000011102230246251565404236316680908203126]", 1.00000000000000022);
    test ("[72057594037927928.0]", 72057594037927928.0);
    test ("[72057594037927936.0]", 72057594037927936.0);
    test ("[72057594037927932.0]", 72057594037927936.0);
    test ("[7205759403792793199999e-5]", 72057594037927928.0);
    test ("[7205759403792793200001e-5]", 72057594037927936.0);
    test ("[9223372036854774784.0]", 9223372036854774784.0);
    test ("[9223372036854775808.0]", 9223372036854775808.0);
    test ("[9223372036854775296.0]", 9223372036854775808.0);
    test ("[922337203685477529599999e-5]", 9223372036854774784.0);
    test ("[922337203685477529600001e-5]", 9223372036854775808.0);
    test ("[10141204801825834086073718800384]", 10141204801825834086073718800384.0);
    test ("[10141204801825835211973625643008]", 10141204801825835211973625643008.0);
    test ("[10141204801825834649023672221696]", 10141204801825835211973625643008.0);
    test ("[1014120480182583464902367222169599999e-5]", 10141204801825834086073718800384.0);
    test ("[1014120480182583464902367222169600001e-5]", 10141204801825835211973625643008.0);
    test ("[5708990770823838890407843763683279797179383808]", 5708990770823838890407843763683279797179383808.0);
    test ("[5708990770823839524233143877797980545530986496]", 5708990770823839524233143877797980545530986496.0);
    test ("[5708990770823839207320493820740630171355185152]", 5708990770823839524233143877797980545530986496.0);
    test ("[5708990770823839207320493820740630171355185151999e-3]", 5708990770823838890407843763683279797179383808.0);
    test ("[5708990770823839207320493820740630171355185152001e-3]", 5708990770823839524233143877797980545530986496.0);

    char n1e308[312];
    n1e308[0] = '[';
    n1e308[1] = '1';
    for (int j = 2; j < 310; j++)
        n1e308[j] = '0';
    n1e308[310] = ']';
    n1e308[311] = '\0';
    test (n1e308, 1E308);

    test (
        "[2.22507385850720113605740979670913197593481954635164564802342610972482222202107694551652952390813508"
        "7914149158913039621106870086438694594645527657207407820621743379988141063267329253552286881372149012"
        "9811224514518898490572223072852551331557550159143974763979834118019993239625482890171070818506906306"
        "6665599493827577257201576306269066333264756530000924588831643303777979186961204949739037782970490505"
        "1080609940730262937128958950003583799967207254304360284078895771796150945516748243471030702609144621"
        "5722898802581825451803257070188608721131280795122334262883686223215037756666225039825343359745688844"
        "2390026549819838548794829220689472168983109969836584681402285424333066033985088644580400103493397042"
        "7567186443383770486037861622771738545623065874679014086723327636718751234567890123456789012345678901"
        "e-308]",
        2.2250738585072014e-308);
}

template <typename Adapter>
void run_parse_string (Result& r, bool verbose)
{
    auto test = [&] (const char* json, const char* expected, size_t expected_len) {
        auto val = Adapter::parse_string (json);
        bool ok = val && val->size () == expected_len && std::memcmp (val->data (), expected, expected_len) == 0;
        if (ok)
            r.string_pass++;
        else
        {
            r.string_fail++;
            if (verbose)
                std::cerr << "  [FAIL] parse_string: " << json << "\n";
        }
    };

    test ("[\"\"]", "", 0);
    test ("[\"Hello\"]", "Hello", 5);
    test ("[\"Hello\\nWorld\"]", "Hello\nWorld", 11);
    test ("[\"Hello\\u0000World\"]", "Hello\0World", 11);
    test ("[\"\\\"\\\\/\\b\\f\\n\\r\\t\"]", "\"\\/\b\f\n\r\t", 8);
    test ("[\"\\u0024\"]", "\x24", 1);
    test ("[\"\\u00A2\"]", "\xC2\xA2", 2);
    test ("[\"\\u20AC\"]", "\xE2\x82\xAC", 3);
    test ("[\"\\uD834\\uDD1E\"]", "\xF0\x9D\x84\x9E", 4);
}

template <typename Adapter, typename Output>
std::string_view output_view (const Output& output)
{
    if constexpr (std::is_convertible_v<const Output&, std::string_view>)
        return output;
    else
        return Adapter::view (output);
}

template <typename Adapter>
void run_roundtrip (Result& r, const std::vector<std::string>& files, bool verbose)
{
    for (auto& path : files)
    {
        auto json = load_file (path);
        auto doc = Adapter::parse (json);
        if (!doc)
        {
            r.roundtrip_fail++;
            if (verbose)
                std::cerr << "  [FAIL] parse error: " << fs::path (path).filename ().string () << "\n";
            continue;
        }
        auto result = Adapter::stringify (*doc);
        auto output = output_view<Adapter> (result);
        if (output == json)
        {
            r.roundtrip_pass++;
        }
        else
        {
            r.roundtrip_fail++;
            if (verbose)
            {
                std::cerr << "  [FAIL] roundtrip mismatch: " << fs::path (path).filename ().string () << "\n";
                std::cerr << "    expected: " << json << "\n";
                std::cerr << "    got:      " << output << "\n";
            }
        }
    }
}

template <typename A, typename = void>
struct has_prettify : std::false_type
{
};

template <typename A>
struct has_prettify<A, std::void_t<decltype (A::prettify (std::declval<const typename A::Document&> ()))>>
: std::true_type
{
};

template <typename A, typename = void>
struct is_lossy : std::false_type
{
};

template <typename A>
struct is_lossy<A, std::void_t<decltype (A::lossy_numbers)>> : std::bool_constant<A::lossy_numbers>
{
};

static bool same_value (yyjson_val* a, yyjson_val* b)
{
    if (yyjson_is_num (a) && yyjson_is_num (b))
        return yyjson_get_num (a) == yyjson_get_num (b);
    if (yyjson_get_type (a) != yyjson_get_type (b))
        return false;
    switch (yyjson_get_type (a))
    {
        case YYJSON_TYPE_OBJ:
            {
                if (yyjson_obj_size (a) != yyjson_obj_size (b))
                    return false;
                yyjson_obj_iter it = yyjson_obj_iter_with (a);
                yyjson_val* key;
                while ((key = yyjson_obj_iter_next (&it)))
                {
                    yyjson_val* other = yyjson_obj_getn (b, yyjson_get_str (key), yyjson_get_len (key));
                    if (!other || !same_value (yyjson_obj_iter_get_val (key), other))
                        return false;
                }
                return true;
            }
        case YYJSON_TYPE_ARR:
            {
                if (yyjson_arr_size (a) != yyjson_arr_size (b))
                    return false;
                yyjson_arr_iter ia = yyjson_arr_iter_with (a);
                yyjson_arr_iter ib = yyjson_arr_iter_with (b);
                yyjson_val* va;
                while ((va = yyjson_arr_iter_next (&ia)))
                {
                    if (!same_value (va, yyjson_arr_iter_next (&ib)))
                        return false;
                }
                return true;
            }
        case YYJSON_TYPE_STR:
            return yyjson_get_len (a) == yyjson_get_len (b) &&
                   std::memcmp (yyjson_get_str (a), yyjson_get_str (b), yyjson_get_len (a)) == 0;
        default:
            return yyjson_equals (a, b);
    }
}

template <typename Adapter>
bool run_output_check (const std::vector<std::string>& files, size_t first_dataset,
                       std::vector<std::optional<Stat>>& reference)
{
    bool ok = true;
    auto fail = [&] (const std::string& path, const char* what) {
        ok = false;
        std::cerr << "  [FAIL] " << Adapter::name << " " << what << ": " << fs::path (path).filename ().string ()
                  << "\n";
    };
    for (size_t i = 0; i < files.size (); ++i)
    {
        auto json = load_file (files[i]);
        auto doc = Adapter::parse (json);
        if (!doc)
        {
            fail (files[i], "parse");
            continue;
        }
        auto stat = Adapter::statistics (*doc);
        if (!reference[i])
            reference[i] = stat;
        else if (!(stat == *reference[i]))
            fail (files[i], "statistics");
        YyjsonAdapter::Document input (yyjson_read (json.data (), json.size (), YYJSON_READ_NOFLAG));
        auto check = [&] (const char* what, std::string_view output) {
            auto reparsed = Adapter::parse (output);
            if (!reparsed || !(Adapter::statistics (*reparsed) == stat))
                fail (files[i], what);
            else if constexpr (!is_lossy<Adapter>::value)
            {
                if (i >= first_dataset)
                {
                    YyjsonAdapter::Document actual (yyjson_read (output.data (), output.size (), YYJSON_READ_NOFLAG));
                    if (!actual ||
                        !same_value (yyjson_doc_get_root (input.get ()), yyjson_doc_get_root (actual.get ())))
                        fail (files[i], what);
                }
            }
        };
        check ("stringify", output_view<Adapter> (Adapter::stringify (*doc)));
        if constexpr (has_prettify<Adapter>::value)
            check ("prettify", output_view<Adapter> (Adapter::prettify (*doc)));
    }
    return ok;
}

static std::vector<std::string> collect_sorted (const fs::path& dir, const std::string& prefix)
{
    std::vector<std::string> files;
    for (auto& entry : fs::directory_iterator (dir))
    {
        if (entry.path ().extension () == ".json" && entry.path ().filename ().string ().rfind (prefix, 0) == 0)
            files.push_back (entry.path ().string ());
    }
    std::sort (files.begin (), files.end ());
    return files;
}

static void print_header ()
{
    std::cout << std::left << std::setw (21) << "Library" << std::right << std::setw (10) << "Checker" << std::setw (12)
              << "Dbl Parse" << std::setw (12) << "Str Parse" << std::setw (12) << "Roundtrip"
              << "\n"
              << std::string (67, '-') << "\n";
}

static void print_row (const std::string& name, const Result& r)
{
    auto fmt = [] (int pass, int total) {
        return std::to_string (pass) + "/" + std::to_string (total);
    };
    std::cout << std::left << std::setw (21) << name << std::right << std::setw (10)
              << fmt (r.checker_pass, r.checker_pass + r.checker_fail) << std::setw (12)
              << fmt (r.double_pass, r.double_pass + r.double_fail) << std::setw (12)
              << fmt (r.string_pass, r.string_pass + r.string_fail) << std::setw (12)
              << fmt (r.roundtrip_pass, r.roundtrip_pass + r.roundtrip_fail) << "\n";
}

static void write_json (const std::string& output_path, const std::vector<std::pair<std::string, Result>>& all)
{
    std::ofstream out (output_path);
    out << "{\n  \"results\": [\n";
    for (size_t i = 0; i < all.size (); ++i)
    {
        auto& [name, r] = all[i];
        auto kv = [&] (const char* key, int pass, int fail) {
            out << "      \"" << key << "\": {\"pass\": " << pass << ", \"total\": " << (pass + fail) << "}";
        };
        out << "    {\n"
            << "      \"library\": \"" << name << "\",\n";
        kv ("parse_validation", r.checker_pass, r.checker_fail);
        out << ",\n";
        kv ("parse_double", r.double_pass, r.double_fail);
        out << ",\n";
        kv ("parse_string", r.string_pass, r.string_fail);
        out << ",\n";
        kv ("roundtrip", r.roundtrip_pass, r.roundtrip_fail);
        out << "\n";
        out << "    }";
        if (i + 1 < all.size ())
            out << ",";
        out << "\n";
    }
    out << "  ]\n}\n";
}

int main (int argc, char* argv[])
{
    bool verbose = false;
    std::string output_path;

    for (int i = 1; i < argc; ++i)
    {
        std::string arg (argv[i]);
        if (arg == "-v")
            verbose = true;
        else if (arg == "--output" && i + 1 < argc)
            output_path = argv[++i];
    }

    const fs::path data_dir = DATA_DIR;
    const fs::path checker_dir = data_dir / "jsonchecker";
    const fs::path roundtrip_dir = data_dir / "roundtrip";

    auto fail_files = collect_sorted (checker_dir, "fail");
    auto pass_files = collect_sorted (checker_dir, "pass");
    auto roundtrip_files = collect_sorted (roundtrip_dir, "roundtrip");

    auto check_files = pass_files;
    for (const char* name : {"canada.json", "citm_catalog.json", "twitter.json"})
        check_files.push_back ((data_dir / name).string ());
    std::vector<std::optional<Stat>> reference (check_files.size ());
    bool outputs_ok = true;

    std::cout << "\n=== JSON Conformance ===\n\n";
    print_header ();

    std::vector<std::pair<std::string, Result>> all_results;

    auto run = [&] (auto adapter) {
        using Adapter = decltype (adapter);
        std::string name{Adapter::name};
        if (verbose)
            std::cout << "\n[" << name << "]\n";

        Result r;
        run_checker<Adapter> (r, fail_files, pass_files, verbose);
        run_parse_double<Adapter> (r, verbose);
        run_parse_string<Adapter> (r, verbose);
        run_roundtrip<Adapter> (r, roundtrip_files, verbose);
        outputs_ok &= run_output_check<Adapter> (check_files, pass_files.size (), reference);
        print_row (name, r);
        all_results.emplace_back (name, r);
    };

    run (BoostAdapter{});
    run (BoostFullPrecAdapter{});
    run (CJsonAdapter{});
    run (JoinAdapter{});
    run (JsonConsAdapter{});
    run (JsonConsLosslessAdapter{});
    run (JsonCppAdapter{});
    run (NlohmannAdapter{});
    run (PocoAdapter{});
    run (RapidJsonAutoUtfAdapter{});
    run (RapidJsonAdapter{});
    run (RapidJsonFullPrecAdapter{});
    run (RapidJsonInsituAdapter{});
    run (RapidJsonIterativeAdapter{});
    run (SimdJsonAdapter{});
    run (SonicAdapter{});
    run (TaoCppAdapter{});
    run (YyjsonAdapter{});

    std::cout << "\n";

    if (!output_path.empty ())
        write_json (output_path, all_results);

    return outputs_ok ? 0 : 1;
}
