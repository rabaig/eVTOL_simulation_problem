#ifndef EVTOL_TEST_HARNESS_H
#define EVTOL_TEST_HARNESS_H

#include <cmath>
#include <cstdio>

/// Assertions for the unit tests.
///
/// The problem says tests don't need to run in a framework, so this is three
/// checks and a counter. A failure prints the file and line it came from, and
/// the run exits non-zero if anything failed.
///
/// Header-only, using C++17 inline variables, so the whole harness is one
/// file and there is nothing to link.

namespace evtol::test {

inline int failuresInTest = 0;
inline int testsRun = 0;
inline int testsFailed = 0;

inline void reportFailure(const char* file, int line, const char* what) {
    ++failuresInTest;
    std::printf("      %s:%d  %s\n", file, line, what);

    // Flushed immediately. A failing check often leaves the code under test in
    // a state that trips an assert a few lines later, and abort() discards
    // whatever is still buffered - losing the one line that says what went
    // wrong first.
    std::fflush(stdout);
}

inline void runTest(const char* name, void (*fn)()) {
    failuresInTest = 0;
    ++testsRun;

    std::printf("  %s\n", name);
    std::fflush(stdout);

    fn();

    if (failuresInTest > 0) {
        ++testsFailed;
        std::printf("    FAILED (%d check%s)\n", failuresInTest,
                    failuresInTest == 1 ? "" : "s");
    }
}

/// Exit code for main: 0 if every test passed.
inline int summary() {
    if (testsFailed == 0) {
        std::printf("\n%d tests passed\n", testsRun);
        return 0;
    }

    std::printf("\n%d of %d tests failed\n", testsFailed, testsRun);
    return 1;
}

}  // namespace evtol::test

#define CHECK(condition)                                                  \
    do {                                                                  \
        if (!(condition)) {                                               \
            ::evtol::test::reportFailure(__FILE__, __LINE__, #condition); \
        }                                                                 \
    } while (false)

#define CHECK_EQ(actual, expected)                                        \
    do {                                                                  \
        if (!((actual) == (expected))) {                                  \
            ::evtol::test::reportFailure(__FILE__, __LINE__,              \
                                         #actual " == " #expected);       \
        }                                                                 \
    } while (false)

/// Nearly everything here comes out of a division like 320 / 192, so exact
/// equality is the wrong comparison for doubles.
#define CHECK_NEAR(actual, expected, tolerance)                           \
    do {                                                                  \
        if (std::fabs((actual) - (expected)) > (tolerance)) {             \
            ::evtol::test::reportFailure(__FILE__, __LINE__,              \
                                         #actual " ~= " #expected);       \
        }                                                                 \
    } while (false)

#endif  // EVTOL_TEST_HARNESS_H
