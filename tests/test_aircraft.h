#ifndef EVTOL_TEST_AIRCRAFT_H
#define EVTOL_TEST_AIRCRAFT_H

namespace evtol::test {

void specs_match_the_problem_statement_table();
void power_draw_is_energy_per_mile_times_cruise_speed();
void flight_time_matches_hand_calculation();
void range_matches_hand_calculation();
void passenger_miles_matches_the_worked_example();
void lookup_returns_the_matching_company();
void alpha_and_delta_cannot_finish_two_flights_in_three_hours();
void charlie_spends_longer_charging_than_flying();

}  // namespace evtol::test

#endif  // EVTOL_TEST_AIRCRAFT_H
