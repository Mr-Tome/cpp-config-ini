#pragma once
#include <iostream>
#include <functional>
#include <string>
#include <vector>
#include <sstream>
#include <cstdio>

#define REQUIRE(cond) \
    do { if (!(cond)) { \
        std::cerr << "  REQUIRE(" #cond ") FAILED\n" \
                  << "    at " __FILE__ ":" << __LINE__ << "\n"; \
        return false; \
    } } while(false)

#define REQUIRE_EQ(a, b) \
    do { if (!((a) == (b))) { \
        std::cerr << "  REQUIRE_EQ(" #a ", " #b ") FAILED\n" \
                  << "    at " __FILE__ ":" << __LINE__ << "\n"; \
        return false; \
    } } while(false)

#define REQUIRE_THROWS(expr) \
    do { \
        bool _threw = false; \
        try { (void)(expr); } catch (...) { _threw = true; } \
        if (!_threw) { \
            std::cerr << "  REQUIRE_THROWS(" #expr ") did not throw\n" \
                      << "    at " __FILE__ ":" << __LINE__ << "\n"; \
            return false; \
        } \
    } while(false)

#define REQUIRE_NO_THROW(expr) \
    do { \
        try { (void)(expr); } \
        catch (const std::exception& _e) { \
            std::cerr << "  REQUIRE_NO_THROW(" #expr ") threw: " << _e.what() << "\n" \
                      << "    at " __FILE__ ":" << __LINE__ << "\n"; \
            return false; \
        } \
    } while(false)

// RAII guard: deletes a file on construction and again on destruction
struct TempFile {
    std::string path;
    explicit TempFile(const std::string& p) : path(p) { (void)std::remove(p.c_str()); }
    ~TempFile() { (void)std::remove(path.c_str()); }
};

struct TestCase {
    std::string name;
    std::function<bool()> fn;
};

// Redirect std::cout for the duration of the guard (suppresses library noise)
struct SuppressStdout {
    std::streambuf* saved;
    std::ostringstream sink;
    SuppressStdout() : saved(std::cout.rdbuf(sink.rdbuf())) {}
    ~SuppressStdout() { std::cout.rdbuf(saved); }
};

inline int runTests(const std::vector<TestCase>& tests)
{
    int passed = 0, failed = 0;
    for (const auto& tc : tests) {
        std::cout << "[ RUN  ] " << tc.name << "\n";
        bool ok = false;
        try { ok = tc.fn(); }
        catch (const std::exception& e) {
            std::cerr << "  EXCEPTION: " << e.what() << "\n";
            ok = false;
        } catch (...) {
            std::cerr << "  UNKNOWN EXCEPTION\n";
            ok = false;
        }
        if (ok) { std::cout << "[  OK  ] " << tc.name << "\n"; ++passed; }
        else    { std::cerr << "[ FAIL ] " << tc.name << "\n"; ++failed; }
    }
    std::cout << "\n" << passed << " passed, " << failed << " failed.\n";
    return (failed > 0) ? 1 : 0;
}
