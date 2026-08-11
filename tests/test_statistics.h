#ifndef EVTOL_TEST_STATISTICS_H
#define EVTOL_TEST_STATISTICS_H

namespace evtol::test {

void an_absent_type_reports_nothing_rather_than_zero();
void average_flight_time_equals_the_type_endurance();
void average_distance_equals_the_type_range();
void average_charge_time_equals_the_type_charge_time();
void averages_ignore_a_flight_still_in_the_air();
void passenger_miles_credit_a_flight_still_in_the_air();
void passenger_miles_reproduce_the_worked_example();
void totals_add_back_up_to_the_whole_fleet();

}  // namespace evtol::test

#endif  // EVTOL_TEST_STATISTICS_H
