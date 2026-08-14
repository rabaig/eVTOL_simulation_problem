// Unit tests for the eVTOL simulation.
//
// The problem asked for "just a few examples of unit tests". These are the
// ones worth defending: each covers something that could plausibly be wrong
// and that no other test would catch. Tests that only exercised the standard
// library, or restated a fact from the specification table, were deliberately
// left out.
//
// One file, one list at the bottom. Adding a test means writing the function
// and adding a line to main.
//
// Two things make the assertions here unusually exact for a simulation.
//
// First, every vehicle flies until its battery is flat, so every completed
// flight of a type lasts exactly the same time and covers exactly the same
// distance. The reported averages are not approximations - they must equal
// the type's endurance and range to the last decimal.
//
// Second, the run is a pure function of the seed. Where a result depends on
// one random draw, a second generator on the same seed reveals that draw and
// the expected value can be worked out by hand. Where behaviour only exists
// in aggregate, the test asserts convergence over a sample far larger than
// three hours could ever provide.

#include <array>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>
#include <queue>
#include <string>
#include <vector>

#include "TestHarness.h"
#include "evtol/Aircraft.h"
#include "evtol/ChargerPool.h"
#include "evtol/FaultModel.h"
#include "evtol/Reporter.h"
#include "evtol/Rng.h"
#include "evtol/Simulation.h"
#include "evtol/Statistics.h"
#include "evtol/Vehicle.h"

namespace evtol::test {
namespace {

constexpr double kTolerance = 1e-9;

const AircraftSpec& alpha() { return aircraftFor(Company::Alpha); }

struct Run {
    SimulationConfig config;
    std::vector<Vehicle> fleet;
    std::array<TypeStatistics, kCompanyCount> stats;
    std::size_t peakChargers = 0;
};

Run simulate(std::uint32_t seed, Hours duration = 3.0, int fleetSize = 20) {
    Run run;
    run.config.duration = duration;
    run.config.fleetSize = fleetSize;

    Rng rng(seed);
    Simulation sim(run.config, rng);
    sim.run();

    run.fleet = sim.fleet();
    run.stats = collectStatistics(sim.fleet(), duration);
    run.peakChargers = sim.peakChargersInUse();

    return run;
}

bool contains(const std::string& haystack, const std::string& needle) {
    return haystack.find(needle) != std::string::npos;
}

/// The single line of a report starting with the given company name.
///
/// Assertions about one type's row have to be made against that row. The
/// report as a whole always contains a rule of dashes, so searching all of it
/// for "-" is a check that can never fail.
std::string rowFor(const std::string& report, const std::string& company) {
    for (std::size_t start = 0; start < report.size();) {
        const std::size_t end = report.find('\n', start);
        const std::string line = report.substr(start, end - start);

        if (line.rfind(company, 0) == 0) {
            return line;
        }

        if (end == std::string::npos) {
            break;
        }
        start = end + 1;
    }

    return {};
}

// ---------------------------------------------------------------------------
// AircraftSpec: the numbers everything else is derived from
// ---------------------------------------------------------------------------

void specs_match_the_problem_statement_table() {
    // The table is written as eight positional values per row, six of them
    // doubles, so transposing two compiles cleanly. This is the test that
    // catches it. Alpha and Echo are the two extremes.
    const AircraftSpec& a = aircraftFor(Company::Alpha);
    CHECK_NEAR(a.cruiseSpeedMph, 120.0, kTolerance);
    CHECK_NEAR(a.batteryCapacityKwh, 320.0, kTolerance);
    CHECK_NEAR(a.chargeTimeHours, 0.60, kTolerance);
    CHECK_NEAR(a.energyPerMileKwh, 1.6, kTolerance);
    CHECK_EQ(a.passengerCount, 4);
    CHECK_NEAR(a.faultsPerHour, 0.25, kTolerance);

    const AircraftSpec& e = aircraftFor(Company::Echo);
    CHECK_NEAR(e.cruiseSpeedMph, 30.0, kTolerance);
    CHECK_NEAR(e.batteryCapacityKwh, 150.0, kTolerance);
    CHECK_NEAR(e.chargeTimeHours, 0.30, kTolerance);
    CHECK_NEAR(e.energyPerMileKwh, 5.8, kTolerance);
    CHECK_EQ(e.passengerCount, 2);
    CHECK_NEAR(e.faultsPerHour, 0.61, kTolerance);

    CHECK_EQ(allAircraft().size(), kCompanyCount);
}

void endurance_and_range_match_hand_calculation() {
    // Drain is energy per mile times cruise speed; endurance is the battery
    // divided by that; range is endurance times speed. Worked out by hand
    // from the table, not recorded from a run.
    //
    //           drain   endurance          range
    // Alpha     192.0   320/192 = 1.6667   200.0
    // Bravo     150.0   100/150 = 0.6667    66.7
    // Charlie   352.0   220/352 = 0.6250   100.0
    // Delta      72.0   120/72  = 1.6667   150.0
    // Echo      174.0   150/174 = 0.8621    25.9
    CHECK_NEAR(aircraftFor(Company::Alpha).powerDrawKwhPerHour(), 192.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Charlie).powerDrawKwhPerHour(), 352.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Echo).powerDrawKwhPerHour(), 174.0, kTolerance);

    CHECK_NEAR(aircraftFor(Company::Alpha).enduranceHours(), 320.0 / 192.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Bravo).enduranceHours(), 100.0 / 150.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Charlie).enduranceHours(), 0.625, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Delta).enduranceHours(), 120.0 / 72.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Echo).enduranceHours(), 150.0 / 174.0, kTolerance);

    CHECK_NEAR(aircraftFor(Company::Alpha).rangeMiles(), 200.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Charlie).rangeMiles(), 100.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Delta).rangeMiles(), 150.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Echo).rangeMiles(), 750.0 / 29.0, kTolerance);

    // Alpha and Delta reach the same endurance from very different batteries,
    // which looks like a bug until you check it.
    CHECK_NEAR(aircraftFor(Company::Alpha).enduranceHours(),
               aircraftFor(Company::Delta).enduranceHours(), kTolerance);
}

void lookup_returns_the_matching_company() {
    // aircraftFor indexes the spec array with the enum value, so the array
    // order and the enum order have to agree. Reordering either one silently
    // gives every vehicle the wrong aircraft.
    for (const AircraftSpec& spec : allAircraft()) {
        CHECK(aircraftFor(spec.company).name == spec.name);
    }
}

// ---------------------------------------------------------------------------
// Rng: only the property the rest of the suite depends on
// ---------------------------------------------------------------------------

void the_same_seed_gives_the_same_sequence() {
    // Everything reproducible rests on this. Exact equality is deliberate:
    // the sequences must be bit-identical, not merely close.
    Rng first(42);
    Rng second(42);

    for (int i = 0; i < 100; ++i) {
        CHECK_EQ(first.uniform01(), second.uniform01());
    }
}

// ---------------------------------------------------------------------------
// ChargerPool: three chargers, and the queue that forms behind them
// ---------------------------------------------------------------------------

void requests_beyond_capacity_join_the_queue() {
    ChargerPool pool(3);

    CHECK(pool.request(10));
    CHECK(pool.request(11));
    CHECK(pool.request(12));
    CHECK_EQ(pool.inUse(), std::size_t{3});

    // The fourth arrival is the entire problem.
    CHECK(!pool.request(13));
    CHECK_EQ(pool.queueLength(), std::size_t{1});
    CHECK(pool.isWaiting(13));
    CHECK(!pool.isCharging(13));
}

void releasing_hands_the_charger_to_the_longest_waiter() {
    ChargerPool pool(1);

    pool.request(100);  // charging
    pool.request(101);  // queued first
    pool.request(102);
    pool.request(103);

    // One charger makes the handover order unambiguous. They must come off
    // the queue in arrival order.
    VehicleId current = 100;
    for (VehicleId want : {101, 102, 103}) {
        const auto next = pool.release(current);

        CHECK(next.has_value());
        CHECK_EQ(*next, want);

        // The charger changed hands rather than going idle.
        CHECK_EQ(pool.inUse(), std::size_t{1});
        current = want;
    }

    CHECK(!pool.release(current).has_value());
    CHECK_EQ(pool.inUse(), std::size_t{0});
}

void releasing_with_an_empty_queue_leaves_a_charger_idle() {
    ChargerPool pool(3);

    pool.request(10);
    pool.request(11);

    const auto next = pool.release(10);

    CHECK(!next.has_value());
    CHECK_EQ(pool.inUse(), std::size_t{1});
    CHECK_EQ(pool.queueLength(), std::size_t{0});
}

void capacity_is_never_exceeded_under_churn() {
    // The charger limit is the one constraint that fails silently. Breaking
    // it wouldn't crash anything, it would just report a fleet that flew more
    // than it could have.
    constexpr int kFleet = 20;
    constexpr std::size_t kChargers = kDefaultChargerCount;

    ChargerPool pool(kChargers);

    for (int id = 0; id < kFleet; ++id) {
        pool.request(id);
    }

    std::deque<VehicleId> holders;
    for (int id = 0; id < static_cast<int>(kChargers); ++id) {
        holders.push_back(id);
    }

    for (int step = 0; step < 200; ++step) {
        const VehicleId finished = holders.front();
        holders.pop_front();

        const auto next = pool.release(finished);

        // With 20 vehicles and 3 chargers the queue can never empty.
        CHECK(next.has_value());
        if (next.has_value()) {
            holders.push_back(*next);
        }

        pool.request(finished);  // straight back in line

        CHECK(pool.inUse() <= pool.capacity());
        CHECK_EQ(pool.inUse() + pool.queueLength(), std::size_t{kFleet});
    }
}

// ---------------------------------------------------------------------------
// Vehicle: the state machine and what it counts
// ---------------------------------------------------------------------------

void only_flight_queue_charge_transitions_are_legal() {
    using S = VehicleState;

    // Walking the whole table matters because the illegal cases are otherwise
    // reachable only by an assert firing, and this harness cannot catch an
    // abort.
    CHECK(!Vehicle::isLegalTransition(S::InFlight, S::InFlight));
    CHECK(Vehicle::isLegalTransition(S::InFlight, S::Queued));
    CHECK(Vehicle::isLegalTransition(S::InFlight, S::Charging));

    // A flat battery cannot take off, so there is no way back to flight from
    // the queue.
    CHECK(!Vehicle::isLegalTransition(S::Queued, S::InFlight));
    CHECK(!Vehicle::isLegalTransition(S::Queued, S::Queued));
    CHECK(Vehicle::isLegalTransition(S::Queued, S::Charging));

    // Charging always runs to completion.
    CHECK(Vehicle::isLegalTransition(S::Charging, S::InFlight));
    CHECK(!Vehicle::isLegalTransition(S::Charging, S::Queued));
    CHECK(!Vehicle::isLegalTransition(S::Charging, S::Charging));
}

void unfinished_periods_are_excluded_from_totals() {
    Vehicle vehicle(0, alpha());

    // Vehicles start airborne on a full battery, as the problem specifies.
    CHECK(vehicle.state() == VehicleState::InFlight);

    // An hour in, nothing has finished. Counting the flight here would drag
    // the average below the aircraft's real per-flight figure - this is the
    // truncation rule, arrived at by the totals simply not being updated.
    CHECK_NEAR(vehicle.totalFlightHours(), 0.0, kTolerance);
    CHECK_EQ(vehicle.completedFlights(), 0);

    // The miles are still claimable for passenger-miles. 120 mph for an hour.
    CHECK_NEAR(vehicle.milesFlownInCurrentFlight(1.0), 120.0, kTolerance);

    vehicle.startQueueing(1.0);
    CHECK_NEAR(vehicle.totalFlightHours(), 1.0, kTolerance);
    CHECK_EQ(vehicle.completedFlights(), 1);
    CHECK_NEAR(vehicle.milesFlownInCurrentFlight(1.5), 0.0, kTolerance);

    // Same rule for a charge still running.
    vehicle.startCharging(1.5);
    CHECK_EQ(vehicle.completedCharges(), 0);

    vehicle.startFlight(2.1);
    CHECK_EQ(vehicle.completedCharges(), 1);
    CHECK_NEAR(vehicle.totalChargeHours(), 0.6, kTolerance);
    CHECK_NEAR(vehicle.totalQueueHours(), 0.5, kTolerance);

    // Distance is measured from the current take-off, not from time zero.
    CHECK_NEAR(vehicle.milesFlownInCurrentFlight(2.6), 60.0, kTolerance);
}

// ---------------------------------------------------------------------------
// FaultModel: a Poisson process, tested two different ways
// ---------------------------------------------------------------------------

void the_interval_matches_the_inverse_transform_formula() {
    // Two generators on the same seed produce the same sequence, so the
    // second reveals exactly which u the fault model was handed. That turns a
    // random result into an exact expectation.
    Rng modelRng(20250810);
    Rng mirrorRng(20250810);

    FaultModel faults(modelRng);
    const double lambda = 0.25;  // Alpha

    for (int draw = 0; draw < 50; ++draw) {
        const Hours actual = faults.timeToNextFault(lambda);

        const double u = mirrorRng.uniform01();
        const Hours expected = -std::log(1.0 - u) / lambda;

        CHECK_NEAR(actual, expected, 1e-12);

        // uniform01 can return exactly zero, and ln(0) is negative infinity.
        // The 1 - u in the formula is what prevents that.
        CHECK(std::isfinite(actual));
        CHECK(actual >= 0.0);
    }
}

void a_zero_fault_rate_never_faults() {
    Rng rng(1);
    FaultModel faults(rng);

    // Not in the problem, but a perfectly reliable aircraft is a reasonable
    // thing to model and dividing by zero is not a reasonable way to do it.
    CHECK(std::isinf(faults.timeToNextFault(0.0)));
}

void the_mean_interval_converges_on_one_over_lambda() {
    // The defining property of an exponential gap. No single run is remotely
    // a large enough sample, so this draws its own.
    Rng rng(5150);
    FaultModel faults(rng);

    for (double lambda : {0.05, 0.25, 0.61}) {  // Charlie, Alpha, Echo
        constexpr int kSamples = 40000;

        double total = 0.0;
        for (int i = 0; i < kSamples; ++i) {
            total += faults.timeToNextFault(lambda);
        }

        const double expectedMean = 1.0 / lambda;

        // Standard error over 40k samples is about 0.5% of the mean, so 2% is
        // loose enough never to flake and tight enough to catch a formula
        // wrong by any interesting amount.
        CHECK_NEAR(total / kSamples, expectedMean, expectedMean * 0.02);
    }
}

void faults_scale_with_flight_hours_not_wall_clock() {
    // The assumption under test: faults accrue only in flight. The fleet
    // spends a large share of the run queued or charging, so if faults were
    // accruing against wall-clock time this count would come out far higher.
    const Run run = simulate(8675309, 300.0);

    double expected = 0.0;
    double observed = 0.0;
    double flightHours = 0.0;

    for (const Vehicle& v : run.fleet) {
        expected += v.type().faultsPerHour * v.totalFlightHours();
        observed += v.faults();
        flightHours += v.totalFlightHours();
    }

    CHECK(observed > 0.0);
    CHECK_NEAR(observed, expected, expected * 0.10);

    // If the fleet were airborne nearly all the time this test would prove
    // very little, so assert that it isn't.
    CHECK(flightHours < run.config.duration * run.fleet.size() * 0.85);
}

// ---------------------------------------------------------------------------
// Simulation: the event loop
// ---------------------------------------------------------------------------

void simultaneous_events_come_out_in_the_order_they_were_scheduled() {
    // Events landing on the same instant are routine: a vehicle finishing its
    // charge and the queued vehicle taking that charger happen at the same
    // time, every time.
    //
    // This tests the comparator directly, because a run on one machine is
    // deterministic whatever the tie-break does. The tie-break is what stops
    // the same seed producing different output on a different standard
    // library.
    //
    // The count matters. With three simultaneous events libstdc++ happens to
    // pop them in insertion order anyway, so a test that size would pass with
    // the tie-break deleted. Reordering starts at four, so this uses six.
    std::priority_queue<Event, std::vector<Event>, EventIsLater> queue;

    queue.push(Event{2.0, EventType::FlightComplete, 90, 0});

    for (std::uint64_t i = 1; i <= 6; ++i) {
        queue.push(Event{1.0, EventType::ChargeComplete, static_cast<VehicleId>(i), i});
    }

    for (std::uint64_t want : {1, 2, 3, 4, 5, 6, 0}) {
        CHECK(!queue.empty());
        CHECK_EQ(queue.top().sequence, want);
        queue.pop();
    }

    CHECK(queue.empty());
}

void the_fleet_always_totals_twenty() {
    // The split across the five types is random. The total is not.
    for (std::uint32_t seed = 1; seed <= 3; ++seed) {
        CHECK_EQ(simulate(seed).fleet.size(), std::size_t{20});
    }
}

void every_hour_of_every_vehicle_is_accounted_for() {
    // Each vehicle is in exactly one state at every instant, so its finished
    // periods plus its current one must add up to the length of the run. A
    // gap means a transition dropped time; an excess means something was
    // counted twice.
    for (std::uint32_t seed = 1; seed <= 3; ++seed) {
        const Run run = simulate(seed);

        for (const Vehicle& v : run.fleet) {
            const Hours accounted = v.totalFlightHours() + v.totalQueueHours() +
                                    v.totalChargeHours() +
                                    (run.config.duration - v.stateEnteredAt());

            CHECK_NEAR(accounted, run.config.duration, kTolerance);
        }
    }
}

void contention_appears_only_when_the_fleet_outnumbers_the_chargers() {
    // One vehicle, nothing to compete with: it should never wait.
    const Run alone = simulate(9, 12.0, 1);
    CHECK_NEAR(alone.fleet.front().totalQueueHours(), 0.0, kTolerance);
    CHECK(alone.fleet.front().completedFlights() > 0);

    // Twenty on three: queueing must happen, and all three chargers must
    // saturate. If neither were true the pool would be doing nothing.
    const Run crowd = simulate(11);

    Hours waiting = 0.0;
    for (const Vehicle& v : crowd.fleet) {
        waiting += v.totalQueueHours();
    }

    CHECK(waiting > 0.0);
    CHECK_EQ(crowd.peakChargers, kDefaultChargerCount);
}

void alpha_completes_exactly_one_flight_in_three_hours() {
    // Alpha flies 1h40m, so a second flight cannot finish inside the window
    // however the chargers behave. The README says so in prose; this checks
    // the running simulation agrees.
    const Run run = simulate(3);

    bool sawAnAlpha = false;
    for (const Vehicle& v : run.fleet) {
        if (v.type().company != Company::Alpha) {
            continue;
        }

        sawAnAlpha = true;
        CHECK_EQ(v.completedFlights(), 1);
    }

    // If seed 3 ever stops drawing an Alpha this test would pass by doing
    // nothing, so say so out loud.
    CHECK(sawAnAlpha);
}

// ---------------------------------------------------------------------------
// Statistics: the five reported figures
// ---------------------------------------------------------------------------

void averages_equal_the_type_specification() {
    // The strongest assertion available here. Every vehicle flies until its
    // battery is flat, so every completed flight of a type is identical - the
    // averages must come out exactly equal to endurance, range and charge
    // time. Anything that mixes types, double counts a flight, or lets a
    // truncated flight into an average breaks this immediately.
    for (std::uint32_t seed = 1; seed <= 3; ++seed) {
        const Run run = simulate(seed, 8.0);

        for (const TypeStatistics& row : run.stats) {
            const AircraftSpec& spec = aircraftFor(row.company);

            if (row.completedFlights > 0) {
                CHECK_NEAR(*row.averageFlightTime(), spec.enduranceHours(), kTolerance);
                CHECK_NEAR(*row.averageDistancePerFlight(), spec.rangeMiles(), kTolerance);
            }

            if (row.completedCharges > 0) {
                CHECK_NEAR(*row.averageChargeTime(), spec.chargeTimeHours, kTolerance);
            }
        }
    }
}

void an_absent_type_reports_nothing_rather_than_zero() {
    // One vehicle leaves four types empty. Reporting zero would look like a
    // measurement; an empty optional says there was nothing to measure.
    const Run run = simulate(12, 3.0, 1);

    int empty = 0;
    for (const TypeStatistics& row : run.stats) {
        if (row.vehicleCount > 0) {
            continue;
        }

        ++empty;
        CHECK(!row.averageFlightTime().has_value());
        CHECK(!row.averageDistancePerFlight().has_value());
        CHECK(!row.averageChargeTime().has_value());
        CHECK_NEAR(row.totalPassengerMiles, 0.0, kTolerance);
    }

    CHECK_EQ(empty, 4);
}

void a_flight_in_the_air_is_excluded_from_averages_but_not_from_miles() {
    // Alpha flies 1h40m, so at the one hour mark every Alpha is airborne with
    // nothing finished. The averages must have nothing to report - and the
    // miles already flown must still be counted.
    const Run run = simulate(3, 1.0);
    const TypeStatistics& alphaRow = run.stats[static_cast<std::size_t>(Company::Alpha)];

    CHECK(alphaRow.vehicleCount > 0);
    CHECK_EQ(alphaRow.completedFlights, 0);
    CHECK(!alphaRow.averageFlightTime().has_value());
    CHECK(!alphaRow.averageDistancePerFlight().has_value());

    // One hour at 120 mph with 4 seats, times however many Alphas there are.
    const double expected = 1.0 * alpha().cruiseSpeedMph * alpha().passengerCount *
                            alphaRow.vehicleCount;

    CHECK(alphaRow.totalPassengerMiles > 0.0);
    CHECK_NEAR(alphaRow.totalPassengerMiles, expected, 1e-6);
}

void passenger_miles_reproduce_the_worked_example() {
    // From the problem statement: two vehicles carrying four passengers,
    // cruising one hour at 100 mph, is 800 passenger miles.
    //
    // Built by hand rather than from a run, so the arithmetic is checked
    // against the stated answer with nothing else in the way. Battery equals
    // drain rate, so this aircraft flies exactly one hour.
    const AircraftSpec example{
        Company::Bravo, "Example",
        100.0,  // mph
        150.0,  // kWh - equal to the drain rate, so it flies exactly one hour
        0.5,    // charge hours, unused here
        1.5,    // kWh/mile
        4,      // passengers
        0.0     // faults per hour
    };

    std::vector<Vehicle> pair;
    pair.emplace_back(0, example, 0.0);
    pair.emplace_back(1, example, 0.0);

    pair[0].startQueueing(1.0);
    pair[1].startQueueing(1.0);

    const auto stats = collectStatistics(pair, 1.0);
    const TypeStatistics& row = stats[static_cast<std::size_t>(Company::Bravo)];

    CHECK_EQ(row.completedFlights, 2);
    CHECK_NEAR(row.totalPassengerMiles, 800.0, kTolerance);
    CHECK_NEAR(*row.averageDistancePerFlight(), 100.0, kTolerance);
}

void queue_hours_include_waits_still_in_progress() {
    // Queue hours are a total with no average attached, so unlike flights and
    // charges a wait still running at the end belongs in the figure. At three
    // hours most of the fleet is queued, so excluding them would report a
    // fraction of the real wait - which is what this originally did.
    const Run run = simulate(42);

    Hours finished = 0.0;
    Hours stillWaiting = 0.0;
    int queuedAtEnd = 0;

    for (const Vehicle& v : run.fleet) {
        finished += v.totalQueueHours();
        stillWaiting += v.hoursWaitingInCurrentQueue(run.config.duration);

        if (v.state() == VehicleState::Queued) {
            ++queuedAtEnd;
        }
    }

    Hours reported = 0.0;
    for (const TypeStatistics& row : run.stats) {
        reported += row.totalQueueHours;
    }

    CHECK_NEAR(reported, finished + stillWaiting, kTolerance);

    // The in-progress part has to be a serious share of the answer, or this
    // test proves very little. Measured across seeds it runs 40-56% of the
    // total; a quarter is a floor that holds without pinning one fleet.
    CHECK(queuedAtEnd > 0);
    CHECK(stillWaiting > 0.25 * (finished + stillWaiting));
}

void per_type_totals_sum_to_the_fleet_totals() {
    const Run run = simulate(2718);

    int vehicles = 0;
    int flights = 0;
    int faults = 0;

    for (const TypeStatistics& row : run.stats) {
        vehicles += row.vehicleCount;
        flights += row.completedFlights;
        faults += row.totalFaults;
    }

    int expectedFlights = 0;
    int expectedFaults = 0;
    for (const Vehicle& v : run.fleet) {
        expectedFlights += v.completedFlights();
        expectedFaults += v.faults();
    }

    // Nothing lost and nothing counted twice on the way through the grouping.
    CHECK_EQ(vehicles, static_cast<int>(run.fleet.size()));
    CHECK_EQ(flights, expectedFlights);
    CHECK_EQ(faults, expectedFaults);
}

// ---------------------------------------------------------------------------
// Reporter
// ---------------------------------------------------------------------------

void an_absent_type_prints_a_dash_not_a_zero() {
    const Run run = simulate(12, 3.0, 1);
    const std::string text = formatReport(run.stats, run.config, 12);

    for (const TypeStatistics& row : run.stats) {
        // Every type gets a row even with no vehicles. Dropping empty rows
        // would leave a reader wondering whether the type was absent or the
        // program forgot it.
        const std::string line = rowFor(text, companyName(row.company));
        CHECK(!line.empty());

        if (row.vehicleCount == 0) {
            // A dash reads as "nothing to report". A 0.0000 in an average
            // column reads as a measurement, and would be a lie.
            CHECK(contains(line, "-"));
            CHECK(!contains(line, "0.0000"));
        }
    }
}

void the_report_is_identical_for_the_same_seed() {
    auto text = [](std::uint32_t seed) {
        const Run run = simulate(seed);
        return formatReport(run.stats, run.config, seed);
    };

    // End-to-end reproducibility: not just matching totals but byte-identical
    // output. This is what lets the sample run in the README be checked by
    // anyone who clones the repo.
    CHECK(text(2024) == text(2024));
    CHECK(text(2024) != text(2025));

    // And the seed has to be in there, or the run is a claim nobody can test.
    CHECK(contains(text(31337), "--seed 31337"));
}

}  // namespace
}  // namespace evtol::test

// ---------------------------------------------------------------------------

using namespace evtol::test;

#define RUN(fn) runTest(#fn, fn)

int main() {
    std::printf("running tests\n\n");

    RUN(specs_match_the_problem_statement_table);
    RUN(endurance_and_range_match_hand_calculation);
    RUN(lookup_returns_the_matching_company);

    RUN(the_same_seed_gives_the_same_sequence);

    RUN(requests_beyond_capacity_join_the_queue);
    RUN(releasing_hands_the_charger_to_the_longest_waiter);
    RUN(releasing_with_an_empty_queue_leaves_a_charger_idle);
    RUN(capacity_is_never_exceeded_under_churn);

    RUN(only_flight_queue_charge_transitions_are_legal);
    RUN(unfinished_periods_are_excluded_from_totals);

    RUN(the_interval_matches_the_inverse_transform_formula);
    RUN(a_zero_fault_rate_never_faults);
    RUN(the_mean_interval_converges_on_one_over_lambda);
    RUN(faults_scale_with_flight_hours_not_wall_clock);

    RUN(simultaneous_events_come_out_in_the_order_they_were_scheduled);
    RUN(the_fleet_always_totals_twenty);
    RUN(every_hour_of_every_vehicle_is_accounted_for);
    RUN(contention_appears_only_when_the_fleet_outnumbers_the_chargers);
    RUN(alpha_completes_exactly_one_flight_in_three_hours);

    RUN(averages_equal_the_type_specification);
    RUN(an_absent_type_reports_nothing_rather_than_zero);
    RUN(a_flight_in_the_air_is_excluded_from_averages_but_not_from_miles);
    RUN(passenger_miles_reproduce_the_worked_example);
    RUN(queue_hours_include_waits_still_in_progress);
    RUN(per_type_totals_sum_to_the_fleet_totals);

    RUN(an_absent_type_prints_a_dash_not_a_zero);
    RUN(the_report_is_identical_for_the_same_seed);

    return summary();
}
