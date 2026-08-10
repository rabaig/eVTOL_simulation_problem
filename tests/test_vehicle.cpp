#include "test_vehicle.h"

#include "TestHarness.h"
#include "evtol/Vehicle.h"

// Times here are round numbers rather than the real endurance figures. The
// flight maths already has its own tests in test_aircraft.cpp; what's being
// checked here is that the state machine closes off the right period and adds
// it to the right total, and that reads better against 1.0 and 2.5 than
// against 1.6666666666666667.

namespace evtol::test {
namespace {

constexpr double kTolerance = 1e-9;

const Aircraft& alpha() { return aircraftFor(Company::Alpha); }

}  // namespace

void vehicles_start_airborne_on_a_full_battery() {
    const Vehicle vehicle(0, alpha());

    // The problem says every vehicle begins fully charged and is airborne for
    // the full use of the battery, so there is no idle state to start from.
    CHECK(vehicle.state() == VehicleState::InFlight);
    CHECK_NEAR(vehicle.stateEnteredAt(), 0.0, kTolerance);

    CHECK_EQ(vehicle.completedFlights(), 0);
    CHECK_EQ(vehicle.faults(), 0);
    CHECK_NEAR(vehicle.totalFlightHours(), 0.0, kTolerance);
}

void the_transition_table_allows_only_the_real_cycle() {
    using S = VehicleState;

    // Walking the whole table matters because the illegal cases are the ones
    // that would otherwise be caught only by an assert firing in a run, and
    // this harness can't catch an abort.
    CHECK(!Vehicle::isLegalTransition(S::InFlight, S::InFlight));
    CHECK(Vehicle::isLegalTransition(S::InFlight, S::Queued));
    CHECK(Vehicle::isLegalTransition(S::InFlight, S::Charging));

    // A flat battery can't take off again, so there is no way back to flight
    // from the queue.
    CHECK(!Vehicle::isLegalTransition(S::Queued, S::InFlight));
    CHECK(!Vehicle::isLegalTransition(S::Queued, S::Queued));
    CHECK(Vehicle::isLegalTransition(S::Queued, S::Charging));

    // Charging always runs to completion. Nothing interrupts it back into
    // the queue.
    CHECK(Vehicle::isLegalTransition(S::Charging, S::InFlight));
    CHECK(!Vehicle::isLegalTransition(S::Charging, S::Queued));
    CHECK(!Vehicle::isLegalTransition(S::Charging, S::Charging));
}

void a_full_cycle_returns_to_flight() {
    Vehicle vehicle(0, alpha());

    vehicle.startQueueing(1.0);
    CHECK(vehicle.state() == VehicleState::Queued);
    CHECK_EQ(vehicle.completedFlights(), 1);
    CHECK_NEAR(vehicle.totalFlightHours(), 1.0, kTolerance);

    vehicle.startCharging(1.5);
    CHECK(vehicle.state() == VehicleState::Charging);
    CHECK_NEAR(vehicle.totalQueueHours(), 0.5, kTolerance);

    // Still charging, so nothing has been added to the charge total yet.
    CHECK_EQ(vehicle.completedCharges(), 0);
    CHECK_NEAR(vehicle.totalChargeHours(), 0.0, kTolerance);

    vehicle.startFlight(2.1);
    CHECK(vehicle.state() == VehicleState::InFlight);
    CHECK_EQ(vehicle.completedCharges(), 1);
    CHECK_NEAR(vehicle.totalChargeHours(), 0.6, kTolerance);

    // One flight and one charge behind it, and the clock on the second
    // flight starts where the charge ended.
    CHECK_EQ(vehicle.completedFlights(), 1);
    CHECK_NEAR(vehicle.stateEnteredAt(), 2.1, kTolerance);
}

void a_free_charger_skips_the_queue() {
    Vehicle vehicle(0, alpha());

    // Early in a run, before the fleet has bunched up, a vehicle can land on
    // a free charger and never queue at all.
    vehicle.startCharging(1.0);

    CHECK(vehicle.state() == VehicleState::Charging);
    CHECK_EQ(vehicle.completedFlights(), 1);
    CHECK_NEAR(vehicle.totalFlightHours(), 1.0, kTolerance);
    CHECK_NEAR(vehicle.totalQueueHours(), 0.0, kTolerance);
}

void totals_only_count_completed_periods() {
    Vehicle vehicle(0, alpha());

    // This is the truncation rule from the README. Three hours in, a vehicle
    // still airborne has flown for three hours, but that flight isn't over
    // and counting it would pull the average below the aircraft's real
    // per-flight figure.
    CHECK_NEAR(vehicle.totalFlightHours(), 0.0, kTolerance);
    CHECK_EQ(vehicle.completedFlights(), 0);

    // The miles are still there to be claimed for passenger miles.
    CHECK_NEAR(vehicle.milesFlownInCurrentFlight(1.0), 120.0, kTolerance);

    vehicle.startQueueing(1.0);

    // Once it lands the flight counts, and there is no flight in progress.
    CHECK_NEAR(vehicle.totalFlightHours(), 1.0, kTolerance);
    CHECK_EQ(vehicle.completedFlights(), 1);
    CHECK_NEAR(vehicle.milesFlownInCurrentFlight(1.5), 0.0, kTolerance);

    // Same rule for a charge session still running.
    vehicle.startCharging(1.5);
    CHECK_EQ(vehicle.completedCharges(), 0);
    CHECK_NEAR(vehicle.totalChargeHours(), 0.0, kTolerance);
}

void several_cycles_accumulate() {
    Vehicle vehicle(0, alpha());

    // Three identical cycles: fly 1.0, wait 0.25, charge 0.5.
    Hours clock = 0.0;
    for (int cycle = 0; cycle < 3; ++cycle) {
        vehicle.startQueueing(clock + 1.0);
        vehicle.startCharging(clock + 1.25);
        vehicle.startFlight(clock + 1.75);
        clock += 1.75;
    }

    CHECK_EQ(vehicle.completedFlights(), 3);
    CHECK_EQ(vehicle.completedCharges(), 3);

    CHECK_NEAR(vehicle.totalFlightHours(), 3.0, kTolerance);
    CHECK_NEAR(vehicle.totalQueueHours(), 0.75, kTolerance);
    CHECK_NEAR(vehicle.totalChargeHours(), 1.5, kTolerance);

    // Every hour is accounted for exactly once, with no gaps or double
    // counting between the states.
    const Hours tracked = vehicle.totalFlightHours() +
                          vehicle.totalQueueHours() +
                          vehicle.totalChargeHours();
    CHECK_NEAR(tracked, clock, kTolerance);
}

void partial_flight_distance_is_available_for_passenger_miles() {
    Vehicle vehicle(0, alpha());

    // Alpha cruises at 120 mph, so half an hour is 60 miles.
    CHECK_NEAR(vehicle.milesFlownInCurrentFlight(0.5), 60.0, kTolerance);
    CHECK_NEAR(vehicle.milesFlownInCurrentFlight(1.0), 120.0, kTolerance);

    // Measured from the start of the current flight, not from time zero.
    vehicle.startQueueing(1.0);
    vehicle.startCharging(1.2);
    vehicle.startFlight(1.8);

    CHECK_NEAR(vehicle.milesFlownInCurrentFlight(2.3), 60.0, kTolerance);
}

void faults_accumulate_across_flights() {
    Vehicle vehicle(0, alpha());

    vehicle.recordFault();
    vehicle.recordFault();
    CHECK_EQ(vehicle.faults(), 2);

    // Faults survive the trip through the charger and keep adding up. The
    // total is reported per type over the whole run, not per flight.
    vehicle.startQueueing(1.0);
    vehicle.startCharging(1.2);
    vehicle.startFlight(1.8);

    vehicle.recordFault();
    CHECK_EQ(vehicle.faults(), 3);
}

}  // namespace evtol::test
