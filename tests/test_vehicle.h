#ifndef EVTOL_TEST_VEHICLE_H
#define EVTOL_TEST_VEHICLE_H

namespace evtol::test {

void vehicles_start_airborne_on_a_full_battery();
void the_transition_table_allows_only_the_real_cycle();
void a_full_cycle_returns_to_flight();
void a_free_charger_skips_the_queue();
void totals_only_count_completed_periods();
void several_cycles_accumulate();
void partial_flight_distance_is_available_for_passenger_miles();
void faults_accumulate_across_flights();

}  // namespace evtol::test

#endif  // EVTOL_TEST_VEHICLE_H
