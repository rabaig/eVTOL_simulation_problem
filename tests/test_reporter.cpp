#include "test_reporter.h"

#include <string>

#include "TestHarness.h"
#include "evtol/Reporter.h"

// formatReport returns a string rather than printing, which is what makes any
// of this testable without capturing stdout.

namespace evtol::test {
namespace {

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

struct Run {
    SimulationConfig config;
    std::array<TypeStatistics, kCompanyCount> stats;
    std::string text;
};

Run report(std::uint32_t seed, Hours duration = 3.0, int fleetSize = 20) {
    Run run;
    run.config.duration = duration;
    run.config.fleetSize = fleetSize;

    Rng rng(seed);
    Simulation sim(run.config, rng);
    sim.run();

    run.stats = collectStatistics(sim.fleet(), duration);
    run.text = formatReport(run.stats, run.config, seed);

    return run;
}

}  // namespace

void the_report_lists_every_company() {
    const Run run = report(42);

    // Every type gets a row even when the random split gave it no vehicles.
    // Silently dropping empty rows would leave a reader wondering whether the
    // type was absent or the program forgot about it.
    for (const Aircraft& spec : allAircraft()) {
        CHECK(contains(run.text, spec.name()));
    }

    CHECK(contains(run.text, "Total"));
    CHECK(contains(run.text, "Passenger-miles"));
}

void the_report_records_the_seed() {
    const Run run = report(31337);

    // Without the seed in the output, a run pasted into a README is a claim
    // nobody can check.
    CHECK(contains(run.text, "31337"));
    CHECK(contains(run.text, "--seed 31337"));
}

void an_absent_type_prints_a_dash_not_a_zero() {
    // One vehicle leaves four types empty.
    const Run run = report(12, 3.0, 1);

    int absent = 0;
    for (const TypeStatistics& row : run.stats) {
        if (row.vehicleCount == 0) {
            ++absent;
        }
    }
    CHECK_EQ(absent, 4);

    // A dash reads as "nothing to report". A 0.0000 in an average column
    // reads as a measurement, and would be a lie.
    CHECK(contains(run.text, "-"));
    CHECK(!contains(run.text, "0.0000"));
}

void the_numbers_in_the_report_match_the_statistics() {
    const Run run = report(7);

    // Spot-check that the table is rendering the values it was handed rather
    // than recomputing anything. Formatting bugs are easy and silent.
    for (const TypeStatistics& row : run.stats) {
        if (row.completedFlights == 0) {
            continue;
        }

        char buffer[64];

        std::snprintf(buffer, sizeof(buffer), "%.4f", *row.averageFlightTime());
        CHECK(contains(run.text, buffer));

        std::snprintf(buffer, sizeof(buffer), "%.2f", *row.averageDistancePerFlight());
        CHECK(contains(run.text, buffer));

        std::snprintf(buffer, sizeof(buffer), "%.1f", row.totalPassengerMiles);
        CHECK(contains(run.text, buffer));
    }
}

void the_report_is_identical_for_the_same_seed() {
    // The end-to-end version of reproducibility: not just matching totals,
    // but byte-for-byte identical output. This is the property that lets the
    // sample run in the README be checked by anyone who clones the repo.
    CHECK(report(2024).text == report(2024).text);
    CHECK(report(2024).text != report(2025).text);
}

}  // namespace evtol::test
