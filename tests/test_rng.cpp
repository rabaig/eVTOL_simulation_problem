#include "StubRng.h"
#include "TestHarness.h"
#include "evtol/MersenneRng.h"

#include <vector>

namespace {

using evtol::MersenneRng;
using evtol::test::StubRng;

TEST(uniform01_stays_in_the_half_open_unit_range) {
    MersenneRng rng(1234);

    // Enough draws to have a fair chance of catching a boundary mistake.
    for (int i = 0; i < 10000; ++i) {
        const double value = rng.uniform01();
        CHECK(value >= 0.0);
        CHECK(value < 1.0);
    }
}

TEST(same_seed_gives_the_same_sequence) {
    MersenneRng first(42);
    MersenneRng second(42);

    for (int i = 0; i < 100; ++i) {
        CHECK_EQ(first.uniform01(), second.uniform01());
    }
}

TEST(different_seeds_diverge) {
    MersenneRng first(1);
    MersenneRng second(2);

    // Any single draw could collide by chance, so look for at least one
    // difference across a run rather than asserting on the first value.
    bool sawDifference = false;
    for (int i = 0; i < 100; ++i) {
        if (first.uniform01() != second.uniform01()) {
            sawDifference = true;
            break;
        }
    }
    CHECK(sawDifference);
}

TEST(uniformInt_includes_both_bounds) {
    MersenneRng rng(7);

    bool sawLow = false;
    bool sawHigh = false;

    for (int i = 0; i < 1000; ++i) {
        const int value = rng.uniformInt(0, 4);
        CHECK(value >= 0);
        CHECK(value <= 4);

        if (value == 0) sawLow = true;
        if (value == 4) sawHigh = true;
    }

    // With 1000 draws over 5 values, missing an endpoint means the range is
    // wrong, not that we got unlucky.
    CHECK(sawLow);
    CHECK(sawHigh);
}

TEST(uniformInt_with_a_single_value_range_returns_it) {
    MersenneRng rng(7);

    // The fleet split hits this when four of the five types are already
    // allocated and the remainder has nowhere else to go.
    for (int i = 0; i < 10; ++i) {
        CHECK_EQ(rng.uniformInt(3, 3), 3);
    }
}

TEST(seed_is_readable_after_construction) {
    // Entropy-seeded runs print their seed so an interesting result can be
    // reproduced. That only works if the seed survives construction.
    MersenneRng rng(99);
    CHECK_EQ(rng.seed(), 99u);
}

TEST(stub_returns_the_scripted_sequence) {
    StubRng rng({0.1, 0.2, 0.3});

    CHECK_NEAR(rng.uniform01(), 0.1, 1e-12);
    CHECK_NEAR(rng.uniform01(), 0.2, 1e-12);
    CHECK_NEAR(rng.uniform01(), 0.3, 1e-12);
}

TEST(stub_cycles_once_the_script_runs_out) {
    StubRng rng({0.25, 0.75});

    CHECK_NEAR(rng.uniform01(), 0.25, 1e-12);
    CHECK_NEAR(rng.uniform01(), 0.75, 1e-12);
    CHECK_NEAR(rng.uniform01(), 0.25, 1e-12);
    CHECK_NEAR(rng.uniform01(), 0.75, 1e-12);

    CHECK_EQ(rng.realCalls(), std::size_t{4});
}

TEST(stub_serves_reals_and_ints_from_separate_scripts) {
    // Fleet composition draws ints and the fault model draws reals. If they
    // shared a cursor, adding a fault would shift the fleet split and every
    // test that depends on both would become order sensitive.
    StubRng rng({0.5}, {2, 3});

    CHECK_EQ(rng.uniformInt(0, 4), 2);
    CHECK_NEAR(rng.uniform01(), 0.5, 1e-12);
    CHECK_EQ(rng.uniformInt(0, 4), 3);

    CHECK_EQ(rng.realCalls(), std::size_t{1});
    CHECK_EQ(rng.intCalls(), std::size_t{2});
}

}  // namespace
