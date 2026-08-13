#include "test_rng.h"

#include "TestHarness.h"
#include "evtol/Rng.h"

namespace evtol::test {

void uniform01_stays_between_zero_and_one() {
    Rng rng(1234);

    // Enough draws to have a fair chance of catching a boundary mistake.
    for (int i = 0; i < 10000; ++i) {
        const double value = rng.uniform01();
        CHECK(value >= 0.0);
        CHECK(value < 1.0);
    }
}

void same_seed_gives_the_same_sequence() {
    // This is the property the whole reproducibility story rests on. If it
    // ever stops holding, the sample run in the README stops being checkable.
    Rng first(42);
    Rng second(42);

    for (int i = 0; i < 100; ++i) {
        CHECK_EQ(first.uniform01(), second.uniform01());
    }
}

void different_seeds_give_different_sequences() {
    Rng first(1);
    Rng second(2);

    // Any single draw could match by chance, so look for one difference
    // across a run rather than asserting on the first value.
    bool sawDifference = false;
    for (int i = 0; i < 100; ++i) {
        if (first.uniform01() != second.uniform01()) {
            sawDifference = true;
            break;
        }
    }
    CHECK(sawDifference);
}

void uniformInt_can_return_both_ends_of_the_range() {
    Rng rng(7);

    bool sawLow = false;
    bool sawHigh = false;

    for (int i = 0; i < 1000; ++i) {
        const int value = rng.uniformInt(0, 4);
        CHECK(value >= 0);
        CHECK(value <= 4);

        if (value == 0) sawLow = true;
        if (value == 4) sawHigh = true;
    }

    // 1000 draws over 5 values. Missing an endpoint means the range is wrong,
    // not that we got unlucky.
    CHECK(sawLow);
    CHECK(sawHigh);
}

void uniformInt_handles_a_range_of_one() {
    Rng rng(7);

    // Not reached by the fleet split, which always rolls over the full range
    // of five types. Here because a degenerate range is the obvious place for
    // uniform_int_distribution to be misused.
    for (int i = 0; i < 10; ++i) {
        CHECK_EQ(rng.uniformInt(3, 3), 3);
    }
}

void seed_survives_construction() {
    // Entropy-seeded runs print their seed so a good result can be repeated.
    // That only works if the seed is still readable afterwards.
    Rng rng(99);
    CHECK_EQ(rng.seed(), 99u);
}

}  // namespace evtol::test
