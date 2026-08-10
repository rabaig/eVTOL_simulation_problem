#include "TestHarness.h"

namespace evtol::test {
namespace {

int testsRun = 0;
int testsFailed = 0;

}  // namespace

int failureCount = 0;

void reportFailure(const char* file, int line, const std::string& what) {
    ++failureCount;
    std::printf("      %s:%d  %s\n", file, line, what.c_str());
}

void runTest(const char* name, void (*fn)()) {
    failureCount = 0;
    ++testsRun;

    // Name goes out before the test runs, so a crash still tells you which
    // test caused it.
    std::printf("  %s\n", name);
    fn();

    if (failureCount > 0) {
        ++testsFailed;
        std::printf("    FAILED (%d check%s)\n", failureCount,
                    failureCount == 1 ? "" : "s");
    }
}

int summary() {
    if (testsFailed == 0) {
        std::printf("\n%d test%s passed\n", testsRun, testsRun == 1 ? "" : "s");
        return 0;
    }

    std::printf("\n%d of %d tests failed\n", testsFailed, testsRun);
    return 1;
}

}  // namespace evtol::test
