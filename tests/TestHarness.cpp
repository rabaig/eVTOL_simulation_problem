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

    // Flush immediately. A failing check often leaves the code under test in
    // a state that trips an assert a few lines later, and abort() discards
    // whatever is still sitting in the stdout buffer. Without this you get a
    // core dump and no indication of which check went wrong first, which is
    // exactly the information you need.
    std::fflush(stdout);
}

void runTest(const char* name, void (*fn)()) {
    failureCount = 0;
    ++testsRun;

    // Name goes out before the test runs, and is flushed for the same reason
    // as above: if the test aborts, this line is the only clue as to which
    // one it was.
    std::printf("  %s\n", name);
    std::fflush(stdout);

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
