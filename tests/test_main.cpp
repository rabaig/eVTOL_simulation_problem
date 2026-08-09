#include <cstdio>

#include "TestHarness.h"

int main() {
    std::printf("running tests\n\n");
    return evtol::test::runAll();
}
