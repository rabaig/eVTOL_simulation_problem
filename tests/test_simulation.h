#ifndef EVTOL_TEST_SIMULATION_H
#define EVTOL_TEST_SIMULATION_H

namespace evtol::test {

void simultaneous_events_come_out_in_the_order_they_were_scheduled();
void the_fleet_always_totals_the_configured_size();
void the_same_seed_reproduces_the_run();
void the_charger_limit_is_never_exceeded();
void every_hour_of_every_vehicle_is_accounted_for();
void a_lone_vehicle_never_waits();
void twenty_vehicles_on_three_chargers_do_wait();
void alpha_completes_exactly_one_flight_in_three_hours();
void a_longer_run_produces_more_flights();

}  // namespace evtol::test

#endif  // EVTOL_TEST_SIMULATION_H
