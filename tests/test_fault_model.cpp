#include "test_fault_model.h"

#include <cmath>
#include <limits>

#include "TestHarness.h"
#include "evtol/FaultModel.h"
#include "evtol/Simulation.h"

// Testing randomness without a stub takes a little care. Two approaches are
// used here.
//
// Where the answer is a pure function of one draw, a second Rng seeded
// identically reproduces the draw, and the expected value can be worked out
// by hand from it. Those tests are exact.
//
// Where the behaviour only exists in aggregate, the assertion is on
// convergence over a large sample. A 3-hour run is nowhere near enough for
// that, so those tests run for far longer than the real simulation does.

namespace evtol::test {
namespace {

constexpr double kTolerance = 1e-12;

}  // namespace

void the_interval_matches_the_inverse_transform_formula() {
    // Two generators on the same seed produce the same sequence, so the
    // second one reveals exactly which u the fault model was handed. That
    // turns a random result into an exact expectation.
    Rng modelRng(20250810);
    Rng mirrorRng(20250810);

    FaultModel faults(modelRng);

    const double lambda = 0.25;  // Alpha

    for (int draw = 0; draw < 50; ++draw) {
        const Hours actual = faults.timeToNextFault(lambda);

        const double u = mirrorRng.uniform01();
        const Hours expected = -std::log(1.0 - u) / lambda;

        CHECK_NEAR(actual, expected, kTolerance);
    }
}

void a_zero_fault_rate_never_faults() {
    Rng rng(1);
    FaultModel faults(rng);

    // A perfectly reliable aircraft isn't in the problem, but it's a
    // reasonable thing to want to model and dividing by zero is not a
    // reasonable way to handle it.
    CHECK(std::isinf(faults.timeToNextFault(0.0)));
}

void intervals_are_never_negative_or_infinite() {
    Rng rng(77);
    FaultModel faults(rng);

    // uniform01() can return exactly zero. Feeding that straight into the
    // logarithm would give negative infinity, which is why the formula uses
    // 1 - u. This is the test that would catch it going back.
    for (int draw = 0; draw < 100000; ++draw) {
        const Hours interval = faults.timeToNextFault(0.61);  // Echo, the worst

        CHECK(interval >= 0.0);
        CHECK(std::isfinite(interval));
    }
}

void the_long_run_mean_interval_converges_on_one_over_lambda() {
    // The defining property of an exponential gap: its mean is 1/lambda.
    // Nothing in a single simulated run is a large enough sample to show
    // this, so it gets its own long draw here.
    Rng rng(5150);
    FaultModel faults(rng);

    for (double lambda : {0.05, 0.25, 0.61}) {  // Charlie, Alpha, Echo
        constexpr int kSamples = 400000;

        double total = 0.0;
        for (int i = 0; i < kSamples; ++i) {
            total += faults.timeToNextFault(lambda);
        }

        const double observedMean = total / kSamples;
        const double expectedMean = 1.0 / lambda;

        // Standard error over 400k samples is about 0.16% of the mean, so 2%
        // is loose enough never to flake and tight enough to catch a formula
        // that is wrong by any interesting amount.
        CHECK_NEAR(observedMean, expectedMean, expectedMean * 0.02);
    }
}

void a_higher_rate_produces_shorter_gaps() {
    Rng rng(31);
    FaultModel faults(rng);

    auto meanGap = [&faults](double lambda) {
        double total = 0.0;
        for (int i = 0; i < 50000; ++i) {
            total += faults.timeToNextFault(lambda);
        }
        return total / 50000.0;
    };

    // Echo faults twelve times as often as Charlie, so its gaps should be
    // roughly a twelfth as long.
    const double echo = meanGap(0.61);
    const double charlie = meanGap(0.05);

    CHECK(echo < charlie);
    CHECK_NEAR(charlie / echo, 0.61 / 0.05, 1.0);
}

void faults_scale_with_flight_hours_not_wall_clock() {
    // The assumption under test: faults accrue only in flight. A vehicle
    // spends a large share of the run queued or charging, so if faults were
    // accruing against wall-clock time this count would come out far higher.
    //
    // Expected faults for a type is lambda multiplied by the hours that type
    // actually spent airborne, and nothing else.
    SimulationConfig config;
    config.duration = 3000.0;  // long enough for the law of large numbers

    Rng rng(8675309);
    Simulation sim(config, rng);
    sim.run();

    double expectedFaults = 0.0;
    double observedFaults = 0.0;
    double flightHours = 0.0;
    double wallClockHours = 0.0;

    for (const Vehicle& v : sim.fleet()) {
        // Only completed flights are in totalFlightHours, and only faults
        // inside those flights were counted, so the two line up.
        expectedFaults += v.type().faultsPerHour() * v.totalFlightHours();
        observedFaults += v.faults();
        flightHours += v.totalFlightHours();
        wallClockHours += config.duration;
    }

    CHECK(observedFaults > 0.0);
    CHECK_NEAR(observedFaults, expectedFaults, expectedFaults * 0.05);

    // The fleet spends a serious fraction of the run on the ground. If that
    // weren't true this test would prove very little, so assert it.
    CHECK(flightHours < wallClockHours * 0.85);
}

void fault_counts_are_reproducible_from_a_seed() {
    Rng firstRng(4242);
    Rng secondRng(4242);

    Simulation first(SimulationConfig{}, firstRng);
    Simulation second(SimulationConfig{}, secondRng);

    first.run();
    second.run();

    // Faults draw from the same generator as the fleet split, so this also
    // checks the two haven't started interleaving differently between runs.
    for (std::size_t i = 0; i < first.fleet().size(); ++i) {
        CHECK_EQ(first.fleet()[i].faults(), second.fleet()[i].faults());
    }
}

}  // namespace evtol::test
