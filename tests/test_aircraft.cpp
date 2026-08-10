#include "test_aircraft.h"

#include <string>

#include "TestHarness.h"
#include "evtol/Aircraft.h"

// Expected values here are worked out by hand from the table in the problem
// statement, not copied from a run. That's the point of this file: if the
// derived maths is wrong, these fail, and the failure says what the answer
// should have been.
//
// Tolerance is loose enough to ignore floating point noise and tight enough
// that a real mistake can't hide under it.

namespace evtol::test {
namespace {

constexpr double kTolerance = 1e-9;

}  // namespace

void specs_match_the_problem_statement_table() {
    const Aircraft& alpha = aircraftFor(Company::Alpha);
    CHECK_NEAR(alpha.cruiseSpeedMph(), 120.0, kTolerance);
    CHECK_NEAR(alpha.batteryCapacityKwh(), 320.0, kTolerance);
    CHECK_NEAR(alpha.chargeTimeHours(), 0.60, kTolerance);
    CHECK_NEAR(alpha.energyPerMileKwh(), 1.6, kTolerance);
    CHECK_EQ(alpha.passengerCount(), 4);
    CHECK_NEAR(alpha.faultsPerHour(), 0.25, kTolerance);

    // Echo is the other end of the range, so between the two most of the
    // typing mistakes in the table would show up.
    const Aircraft& echo = aircraftFor(Company::Echo);
    CHECK_NEAR(echo.cruiseSpeedMph(), 30.0, kTolerance);
    CHECK_NEAR(echo.batteryCapacityKwh(), 150.0, kTolerance);
    CHECK_NEAR(echo.chargeTimeHours(), 0.30, kTolerance);
    CHECK_NEAR(echo.energyPerMileKwh(), 5.8, kTolerance);
    CHECK_EQ(echo.passengerCount(), 2);
    CHECK_NEAR(echo.faultsPerHour(), 0.61, kTolerance);

    CHECK_EQ(allAircraft().size(), kCompanyCount);
}

void power_draw_is_energy_per_mile_times_cruise_speed() {
    // Alpha:   1.6 x 120 = 192
    // Bravo:   1.5 x 100 = 150
    // Charlie: 2.2 x 160 = 352
    // Delta:   0.8 x  90 =  72
    // Echo:    5.8 x  30 = 174
    CHECK_NEAR(aircraftFor(Company::Alpha).powerDrawKwhPerHour(), 192.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Bravo).powerDrawKwhPerHour(), 150.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Charlie).powerDrawKwhPerHour(), 352.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Delta).powerDrawKwhPerHour(), 72.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Echo).powerDrawKwhPerHour(), 174.0, kTolerance);
}

void flight_time_matches_hand_calculation() {
    // battery / drain rate, in hours.
    //
    // Alpha:   320 / 192 = 1.6667   (1h 40m)
    // Bravo:   100 / 150 = 0.6667   (40m)
    // Charlie: 220 / 352 = 0.625    (37m 30s)
    // Delta:   120 /  72 = 1.6667   (1h 40m)
    // Echo:    150 / 174 = 0.8621   (51m 43s)
    CHECK_NEAR(aircraftFor(Company::Alpha).flightTimeHours(), 320.0 / 192.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Bravo).flightTimeHours(), 100.0 / 150.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Charlie).flightTimeHours(), 0.625, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Delta).flightTimeHours(), 120.0 / 72.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Echo).flightTimeHours(), 150.0 / 174.0, kTolerance);

    // Alpha and Delta land on the same endurance from very different
    // batteries, which is worth pinning down because it looks like a bug.
    CHECK_NEAR(aircraftFor(Company::Alpha).flightTimeHours(),
               aircraftFor(Company::Delta).flightTimeHours(), kTolerance);
}

void range_matches_hand_calculation() {
    // Three of these come out round, which makes them easy to check by eye.
    //
    // Alpha:   1.6667 x 120 = 200
    // Charlie: 0.625  x 160 = 100
    // Delta:   1.6667 x  90 = 150
    CHECK_NEAR(aircraftFor(Company::Alpha).rangeMiles(), 200.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Charlie).rangeMiles(), 100.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Delta).rangeMiles(), 150.0, kTolerance);

    CHECK_NEAR(aircraftFor(Company::Bravo).rangeMiles(), 200.0 / 3.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Echo).rangeMiles(), 750.0 / 29.0, kTolerance);

    // Echo covers less ground than anything else despite carrying the second
    // largest battery in the fleet.
    for (const Aircraft& other : allAircraft()) {
        if (other.company() != Company::Echo) {
            CHECK(aircraftFor(Company::Echo).rangeMiles() < other.rangeMiles());
        }
    }
}

void passenger_miles_matches_the_worked_example() {
    // The problem gives this example: 2 vehicles carrying 4 passengers,
    // cruising 1 hour at 100 mph, is 800 passenger miles.
    //
    // Build a type that flies exactly one hour: battery equals drain rate,
    // so 1.5 kWh/mile at 100 mph draws 150 kWh/hr on a 150 kWh battery.
    const Aircraft example(Company::Alpha, "Example",
                           100.0,   // mph
                           150.0,   // kWh
                           0.5,     // charge hours, not used here
                           1.5,     // kWh/mile
                           4,       // passengers
                           0.0);    // faults

    CHECK_NEAR(example.flightTimeHours(), 1.0, kTolerance);
    CHECK_NEAR(example.rangeMiles(), 100.0, kTolerance);
    CHECK_NEAR(example.passengerMilesPerFlight(), 400.0, kTolerance);

    // Two of them, as in the example.
    CHECK_NEAR(2.0 * example.passengerMilesPerFlight(), 800.0, kTolerance);

    // And the real fleet. Alpha does more than twice the passenger miles of
    // anything else per flight, which is why it dominates the totals.
    CHECK_NEAR(aircraftFor(Company::Alpha).passengerMilesPerFlight(), 800.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Charlie).passengerMilesPerFlight(), 300.0, kTolerance);
    CHECK_NEAR(aircraftFor(Company::Delta).passengerMilesPerFlight(), 300.0, kTolerance);
}

void lookup_returns_the_matching_company() {
    // aircraftFor indexes the array with the enum value, so the array order
    // and the enum order have to agree. If someone reorders one of them this
    // is what catches it.
    for (const Aircraft& spec : allAircraft()) {
        CHECK(aircraftFor(spec.company()).name() == spec.name());
    }

    CHECK(std::string(companyName(Company::Alpha)) == "Alpha");
    CHECK(std::string(companyName(Company::Echo)) == "Echo");
}

void alpha_and_delta_cannot_finish_two_flights_in_three_hours() {
    // The README claims this, so it should be a test rather than a sentence
    // someone has to take on trust. Both fly 1h 40m, so a second flight would
    // end past the 3 hour mark even with instant charging.
    constexpr double kSimulationHours = 3.0;

    CHECK(2.0 * aircraftFor(Company::Alpha).flightTimeHours() > kSimulationHours);
    CHECK(2.0 * aircraftFor(Company::Delta).flightTimeHours() > kSimulationHours);

    // Bravo is the counter-example: short flights and a short charge, so it
    // gets through several cycles in the same window.
    const Aircraft& bravo = aircraftFor(Company::Bravo);
    const double bravoCycle = bravo.flightTimeHours() + bravo.chargeTimeHours();
    CHECK(3.0 * bravoCycle < kSimulationHours);
}

void charlie_spends_longer_charging_than_flying() {
    // Also a README claim. Charlie is the only type where this holds, and it
    // is the reason being the fastest aircraft doesn't help it much.
    const Aircraft& charlie = aircraftFor(Company::Charlie);
    CHECK(charlie.chargeTimeHours() > charlie.flightTimeHours());

    for (const Aircraft& other : allAircraft()) {
        if (other.company() != Company::Charlie) {
            CHECK(other.chargeTimeHours() < other.flightTimeHours());
        }
    }
}

}  // namespace evtol::test
