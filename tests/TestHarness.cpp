#include "TestHarness.h"

#include <cmath>
#include <cstdio>
#include <vector>

namespace evtol::test {
namespace {

struct Entry {
    const char* name;
    TestFn fn;
};

// Function-local static so registration during static init can't run before
// the container exists. Ordering across translation units isn't defined, and
// every TEST macro registers from a different one.
std::vector<Entry>& registry() {
    static std::vector<Entry> entries;
    return entries;
}

int failuresInCurrentTest = 0;

}  // namespace

void registerTest(const char* name, TestFn fn) {
    registry().push_back({name, fn});
}

void reportFailure(const char* file, int line, const std::string& what) {
    ++failuresInCurrentTest;
    std::printf("      %s:%d: %s\n", file, line, what.c_str());
}

void checkNear(const char* file, int line, const char* expr,
               double actual, double expected, double tolerance) {
    if (std::fabs(actual - expected) > tolerance) {
        std::ostringstream message;
        message << expr << "\n      got      " << actual
                << "\n      expected " << expected
                << " (tolerance " << tolerance << ")";
        reportFailure(file, line, message.str());
    }
}

int runAll() {
    int failedTests = 0;

    for (const Entry& test : registry()) {
        failuresInCurrentTest = 0;

        // Print the name first so a crash or hang tells you which test did it.
        std::printf("  %s\n", test.name);
        test.fn();

        if (failuresInCurrentTest > 0) {
            ++failedTests;
            std::printf("    FAILED (%d check%s)\n", failuresInCurrentTest,
                        failuresInCurrentTest == 1 ? "" : "s");
        }
    }

    const std::size_t total = registry().size();
    if (failedTests == 0) {
        std::printf("\n%zu test%s passed\n", total, total == 1 ? "" : "s");
        return 0;
    }

    std::printf("\n%d of %zu tests failed\n", failedTests, total);
    return 1;
}

}  // namespace evtol::test
