#include <cstdio>

#include "TestHarness.h"
#include "test_rng.h"

// Every test is listed here by hand. Adding one means writing the function,
// declaring it in the matching header, and adding a line below. Slightly more
// typing than a framework, and in exchange there's no magic to explain.

using namespace evtol::test;

#define RUN(fn) runTest(#fn, fn)

int main() {
    std::printf("running tests\n\n");

    RUN(uniform01_stays_between_zero_and_one);
    RUN(same_seed_gives_the_same_sequence);
    RUN(different_seeds_give_different_sequences);
    RUN(uniformInt_can_return_both_ends_of_the_range);
    RUN(uniformInt_handles_a_range_of_one);
    RUN(seed_survives_construction);

    return summary();
}
