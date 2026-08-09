#ifndef EVTOL_TEST_HARNESS_H
#define EVTOL_TEST_HARNESS_H

#include <sstream>
#include <string>

/// A small test harness.
///
/// The problem statement says tests don't have to run in a framework, and
/// pulling in GoogleTest would mean either a submodule or a FetchContent
/// download for what comes to a few dozen assertions. This covers the two
/// things that actually matter when a test fails: the test names itself, and
/// the failure points at a file and line.
///
/// Tests register themselves at static-init time, so adding one means writing
/// it and adding the file to tests/CMakeLists.txt. Nothing central to update.

namespace evtol::test {

using TestFn = void (*)();

void registerTest(const char* name, TestFn fn);

/// Runs every registered test. Returns a process exit code: 0 if all passed.
int runAll();

void reportFailure(const char* file, int line, const std::string& what);

/// Static-init helper. The TEST macro creates one of these per test.
struct Registrar {
    Registrar(const char* name, TestFn fn) { registerTest(name, fn); }
};

template <typename Actual, typename Expected>
void checkEqual(const char* file, int line, const char* expr,
                const Actual& actual, const Expected& expected) {
    if (!(actual == expected)) {
        std::ostringstream message;
        message << expr << "\n      got      " << actual
                << "\n      expected " << expected;
        reportFailure(file, line, message.str());
    }
}

void checkNear(const char* file, int line, const char* expr,
               double actual, double expected, double tolerance);

}  // namespace evtol::test

#define EVTOL_JOIN_INNER(a, b) a##b
#define EVTOL_JOIN(a, b) EVTOL_JOIN_INNER(a, b)

/// Declares and registers a test. The body follows the macro:
///
///     TEST(uniform01_stays_within_range) {
///         ...
///     }
#define TEST(name)                                                            \
    static void name();                                                       \
    static ::evtol::test::Registrar EVTOL_JOIN(registrar_, name)(#name, name);\
    static void name()

#define CHECK(condition)                                                      \
    do {                                                                      \
        if (!(condition)) {                                                   \
            ::evtol::test::reportFailure(__FILE__, __LINE__, #condition);     \
        }                                                                     \
    } while (false)

#define CHECK_EQ(actual, expected)                                            \
    ::evtol::test::checkEqual(__FILE__, __LINE__, #actual " == " #expected,   \
                              (actual), (expected))

/// Floating point comparison. Everything in this simulation is derived from
/// divisions like 320 / 192, so exact equality is the wrong test.
#define CHECK_NEAR(actual, expected, tolerance)                               \
    ::evtol::test::checkNear(__FILE__, __LINE__, #actual " ~= " #expected,    \
                             (actual), (expected), (tolerance))

#endif  // EVTOL_TEST_HARNESS_H
