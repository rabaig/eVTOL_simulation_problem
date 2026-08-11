#include "test_simulation.h"

#include <vector>

#include "TestHarness.h"
#include "evtol/Simulation.h"

namespace evtol::test {
namespace {

constexpr double kTolerance = 1e-9;

/// Time a vehicle has accounted for: everything it has finished, plus the
/// period it is still in when the clock stops.
Hours accountedTime(const Vehicle& v, Hours endOfRun) {
    return v.totalFlightHours() + v.totalQueueHours() + v.totalChargeHours() +
           (endOfRun - v.stateEnteredAt());
}

}  // namespace

void simultaneous_events_come_out_in_the_order_they_were_scheduled() {
    // Events landing on the same instant are routine here: a vehicle
    // finishing its charge and the queued vehicle taking that charger happen
    // at the same time, every time.
    //
    // This tests the comparator directly rather than through a run, because a
    // run on one machine is deterministic whatever the tie-break does — the
    // heap behaves the same way twice in a row regardless. The tie-break is
    // what stops the same seed producing different output on a different
    // standard library.
    //
    // The count matters. With three simultaneous events libstdc++ happens to
    // pop them in insertion order anyway, so a test that size would pass with
    // the tie-break deleted. From four onward the heap reorders them, so this
    // uses six.
    std::priority_queue<Event, std::vector<Event>, EventIsLater> queue;

    queue.push(Event{2.0, EventType::FlightComplete, 90, 0});

    for (std::uint64_t i = 1; i <= 6; ++i) {
        queue.push(Event{1.0, EventType::ChargeComplete, static_cast<VehicleId>(i), i});
    }

    // Earliest time first, and among the six at t = 1, insertion order.
    const std::uint64_t expected[] = {1, 2, 3, 4, 5, 6, 0};

    for (std::uint64_t want : expected) {
        CHECK(!queue.empty());
        CHECK_EQ(queue.top().sequence, want);
        queue.pop();
    }

    CHECK(queue.empty());
}

void the_fleet_always_totals_the_configured_size() {
    // The split across the five types is random, but the total is not.
    for (std::uint32_t seed = 1; seed <= 20; ++seed) {
        Rng rng(seed);
        Simulation sim(SimulationConfig{}, rng);

        CHECK_EQ(sim.fleet().size(), std::size_t{20});
    }
}

void the_same_seed_reproduces_the_run() {
    // Without this the sample output in the README would be worthless: nobody
    // could re-run it and get the same numbers. It also depends on the event
    // queue breaking ties deterministically, which a heap does not do on its
    // own.
    Rng firstRng(4242);
    Rng secondRng(4242);

    Simulation first(SimulationConfig{}, firstRng);
    Simulation second(SimulationConfig{}, secondRng);

    first.run();
    second.run();

    CHECK_EQ(first.eventsProcessed(), second.eventsProcessed());

    for (std::size_t i = 0; i < first.fleet().size(); ++i) {
        const Vehicle& a = first.fleet()[i];
        const Vehicle& b = second.fleet()[i];

        CHECK(a.type().company() == b.type().company());
        CHECK_EQ(a.completedFlights(), b.completedFlights());
        CHECK_EQ(a.completedCharges(), b.completedCharges());
        CHECK_NEAR(a.totalFlightHours(), b.totalFlightHours(), kTolerance);
        CHECK_NEAR(a.totalQueueHours(), b.totalQueueHours(), kTolerance);
        CHECK_NEAR(a.totalChargeHours(), b.totalChargeHours(), kTolerance);
    }
}

void the_charger_limit_is_never_exceeded() {
    // Three chargers is the constraint the whole problem turns on. Breaking
    // it wouldn't crash anything, it would just quietly produce a fleet that
    // flies more than it could have.
    for (std::uint32_t seed = 1; seed <= 25; ++seed) {
        Rng rng(seed);
        Simulation sim(SimulationConfig{}, rng);
        sim.run();

        CHECK(sim.peakChargersInUse() <= kDefaultChargerCount);
    }
}

void every_hour_of_every_vehicle_is_accounted_for() {
    // Each vehicle is in exactly one state at every instant, so its finished
    // periods plus its current one must add up to the length of the run. Any
    // gap means a transition dropped time on the floor; any excess means
    // something got counted twice.
    for (std::uint32_t seed = 1; seed <= 15; ++seed) {
        Rng rng(seed);
        SimulationConfig config;
        Simulation sim(config, rng);
        sim.run();

        for (const Vehicle& v : sim.fleet()) {
            CHECK_NEAR(accountedTime(v, config.duration), config.duration, 1e-9);
        }
    }
}

void a_lone_vehicle_never_waits() {
    SimulationConfig config;
    config.fleetSize = 1;
    config.duration = 12.0;

    Rng rng(9);
    Simulation sim(config, rng);
    sim.run();

    const Vehicle& only = sim.fleet().front();

    // Nobody to compete with, so it flies and charges and never queues.
    CHECK_NEAR(only.totalQueueHours(), 0.0, kTolerance);
    CHECK(only.completedFlights() > 0);
    CHECK(sim.peakChargersInUse() <= 1);
}

void twenty_vehicles_on_three_chargers_do_wait() {
    Rng rng(11);
    Simulation sim(SimulationConfig{}, rng);
    sim.run();

    // The opposite of the test above. If nothing ever queued, the charger
    // pool would not be doing anything and the problem would be trivial.
    Hours totalWaiting = 0.0;
    for (const Vehicle& v : sim.fleet()) {
        totalWaiting += v.totalQueueHours();
    }

    CHECK(totalWaiting > 0.0);
    CHECK_EQ(sim.peakChargersInUse(), kDefaultChargerCount);
}

void alpha_completes_exactly_one_flight_in_three_hours() {
    // Alpha flies 1h40m, so a second flight cannot finish inside the window
    // however the chargers behave. The README says so in prose and
    // test_aircraft.cpp checks the arithmetic; this checks the running
    // simulation agrees.
    SimulationConfig config;
    config.fleetSize = 20;

    Rng rng(3);
    Simulation sim(config, rng);
    sim.run();

    bool sawAnAlpha = false;
    for (const Vehicle& v : sim.fleet()) {
        if (v.type().company() != Company::Alpha) {
            continue;
        }

        sawAnAlpha = true;
        CHECK_EQ(v.completedFlights(), 1);
    }

    // Seed 3 happens to include at least one Alpha. If that ever stops being
    // true the test would pass by doing nothing, so say so out loud.
    CHECK(sawAnAlpha);
}

void a_longer_run_produces_more_flights() {
    auto flightsOver = [](Hours duration) {
        SimulationConfig config;
        config.duration = duration;

        Rng rng(77);
        Simulation sim(config, rng);
        sim.run();

        int flights = 0;
        for (const Vehicle& v : sim.fleet()) {
            flights += v.completedFlights();
        }
        return flights;
    };

    // Not a deep property, but it would catch a loop that terminates on the
    // first event or ignores the duration entirely.
    CHECK(flightsOver(3.0) > 0);
    CHECK(flightsOver(12.0) > flightsOver(3.0));
}

}  // namespace evtol::test
