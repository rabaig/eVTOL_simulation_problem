#ifndef EVTOL_TEST_HARNESS_H
#define EVTOL_TEST_HARNESS_H

#include <cmath>
#include <cstdio>
#include <string>

/// Assertions for the unit tests.
///
/// The problem says tests don't need to run in a framework, so this is just
/// three checks and a counter. A failure prints the file and line it came
/// from, and the run exits non-zero if anything failed.
///
/// Tests are plain functions. test_main.cpp calls them.

namespace evtol::test {

/// Failures seen so far. runTest resets this per test.
extern int failureCount;

void reportFailure(const char* file, int line, const std::string& what);

/// Runs one test function and prints whether it passed.
void runTest(const char* name, void (*fn)());

/// Exit code for main: 0 if every test passed.
int summary();

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
