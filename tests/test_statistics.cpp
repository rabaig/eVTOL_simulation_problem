#include "test_statistics.h"

#include <vector>

#include "TestHarness.h"
#include "evtol/Simulation.h"
#include "evtol/Statistics.h"

// There is an unusually strong invariant available here, and most of these
// tests lean on it.
//
// Every vehicle flies until its battery is flat, so every completed flight of
// a given type lasts exactly the same time and covers exactly the same
// distance. The average is therefore not an approximation of anything — it
// has to come out equal to the type's endurance and range, to the last
// decimal place. Charging is the same: a fixed duration per type.
//
// Anything that quietly mixes types, double counts a flight, or lets a
// truncated flight into the average breaks that equality immediately.

namespace evtol::test {
namespace {

constexpr double kTolerance = 1e-9;

std::array<TypeStatistics, kCompanyCount> runAndCollect(std::uint32_t seed,
                                                       Hours duration = 3.0,
                                                       int fleetSize = 20) {
    SimulationConfig config;
    config.duration = duration;
    config.fleetSize = fleetSize;

    Rng rng(seed);
    Simulation sim(config, rng);
    sim.run();

    return collectStatistics(sim.fleet(), duration);
}

}  // namespace

void an_absent_type_reports_nothing_rather_than_zero() {
    // A one-vehicle fleet leaves four of the five types empty. Reporting zero
    // for those would look like a measurement; an empty optional says there
    // was nothing to measure and makes the caller deal with it.
    const auto stats = runAndCollect(12, 3.0, 1);

    int empty = 0;
    for (const TypeStatistics& row : stats) {
        if (row.vehicleCount > 0) {
            continue;
        }

        ++empty;
        CHECK(!row.averageFlightTime().has_value());
        CHECK(!row.averageDistancePerFlight().has_value());
        CHECK(!row.averageChargeTime().has_value());
        CHECK_EQ(row.completedFlights, 0);
        CHECK_NEAR(row.totalPassengerMiles, 0.0, kTolerance);
    }

    CHECK_EQ(empty, 4);
}

void average_flight_time_equals_the_type_endurance() {
    for (std::uint32_t seed = 1; seed <= 12; ++seed) {
        const auto stats = runAndCollect(seed);

        for (const TypeStatistics& row : stats) {
            if (row.completedFlights == 0) {
                continue;
            }

            const Aircraft& spec = aircraftFor(row.company);

            CHECK(row.averageFlightTime().has_value());
            CHECK_NEAR(*row.averageFlightTime(), spec.flightTimeHours(), kTolerance);
        }
    }
}

void average_distance_equals_the_type_range() {
    for (std::uint32_t seed = 1; seed <= 12; ++seed) {
        const auto stats = runAndCollect(seed);

        for (const TypeStatistics& row : stats) {
            if (row.completedFlights == 0) {
                continue;
            }

            const Aircraft& spec = aircraftFor(row.company);

            CHECK(row.averageDistancePerFlight().has_value());
            CHECK_NEAR(*row.averageDistancePerFlight(), spec.rangeMiles(), kTolerance);
        }
    }
}

void average_charge_time_equals_the_type_charge_time() {
    // The problem says a charger fills the battery in the type's listed time
    // regardless of how empty it is, so this one is fixed by definition.
    for (std::uint32_t seed = 1; seed <= 12; ++seed) {
        const auto stats = runAndCollect(seed, 8.0);

        for (const TypeStatistics& row : stats) {
            if (row.completedCharges == 0) {
                continue;
            }

            const Aircraft& spec = aircraftFor(row.company);

            CHECK(row.averageChargeTime().has_value());
            CHECK_NEAR(*row.averageChargeTime(), spec.chargeTimeHours(), kTolerance);
        }
    }
}

void averages_ignore_a_flight_still_in_the_air() {
    // Alpha flies 1h40m, so at the 1 hour mark every Alpha is airborne with
    // nothing finished. If a truncated flight were counted, the average would
    // come out as 1.0 rather than as nothing at all.
    const auto stats = runAndCollect(3, 1.0);
    const TypeStatistics& alpha = stats[static_cast<std::size_t>(Company::Alpha)];

    CHECK(alpha.vehicleCount > 0);
    CHECK_EQ(alpha.completedFlights, 0);
    CHECK(!alpha.averageFlightTime().has_value());
    CHECK(!alpha.averageDistancePerFlight().has_value());
}

void passenger_miles_credit_a_flight_still_in_the_air() {
    // Same run as above: nothing has landed, so the distance average has
    // nothing to report, but real miles have been flown and they must show up.
    const auto stats = runAndCollect(3, 1.0);
    const TypeStatistics& alpha = stats[static_cast<std::size_t>(Company::Alpha)];

    const Aircraft& spec = aircraftFor(Company::Alpha);

    // One hour at 120 mph with 4 seats, times however many Alphas there are.
    const double expected = 1.0 * spec.cruiseSpeedMph() * spec.passengerCount() *
                            alpha.vehicleCount;

    CHECK_NEAR(alpha.totalPassengerMiles, expected, 1e-6);
    CHECK(alpha.totalPassengerMiles > 0.0);
}

void passenger_miles_reproduce_the_worked_example() {
    // From the problem statement: two vehicles carrying four passengers,
    // cruising one hour at 100 mph, is 800 passenger miles.
    //
    // Built by hand rather than from a run, so the arithmetic is checked
    // against the stated answer with nothing else in the way.
    const Aircraft example(Company::Bravo, "Example",
                           100.0,  // mph
                           150.0,  // kWh, exactly one hour at this drain
                           0.5,
                           1.5,    // kWh/mile
                           4,      // passengers
                           0.0);

    std::vector<Vehicle> pair;
    pair.emplace_back(0, example, 0.0);
    pair.emplace_back(1, example, 0.0);

    // Both land exactly on the hour, so both flights are complete.
    pair[0].startQueueing(1.0);
    pair[1].startQueueing(1.0);

    const auto stats = collectStatistics(pair, 1.0);
    const TypeStatistics& row = stats[static_cast<std::size_t>(Company::Bravo)];

    CHECK_EQ(row.completedFlights, 2);
    CHECK_NEAR(row.totalPassengerMiles, 800.0, kTolerance);
    CHECK_NEAR(*row.averageDistancePerFlight(), 100.0, kTolerance);
}

void totals_add_back_up_to_the_whole_fleet() {
    SimulationConfig config;
    Rng rng(2718);
    Simulation sim(config, rng);
    sim.run();

    const auto stats = collectStatistics(sim.fleet(), config.duration);

    int vehicles = 0;
    int flights = 0;
    int faults = 0;
    Hours flightHours = 0.0;

    for (const TypeStatistics& row : stats) {
        vehicles += row.vehicleCount;
        flights += row.completedFlights;
        faults += row.totalFaults;
        flightHours += row.totalFlightHours;
    }

    // Nothing lost and nothing counted twice on the way through the grouping.
    int expectedFlights = 0;
    int expectedFaults = 0;
    Hours expectedHours = 0.0;

    for (const Vehicle& v : sim.fleet()) {
        expectedFlights += v.completedFlights();
        expectedFaults += v.faults();
        expectedHours += v.totalFlightHours();
    }

    CHECK_EQ(vehicles, static_cast<int>(sim.fleet().size()));
    CHECK_EQ(flights, expectedFlights);
    CHECK_EQ(faults, expectedFaults);
    CHECK_NEAR(flightHours, expectedHours, kTolerance);
}

}  // namespace evtol::test
