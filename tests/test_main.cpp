#include <cstdio>

#include "TestHarness.h"
#include "test_aircraft.h"
#include "test_charger_pool.h"
#include "test_rng.h"
#include "test_simulation.h"
#include "test_vehicle.h"

// Every test is listed here by hand. Adding one means writing the function,
// declaring it in the matching header, and adding a line below. Slightly more
// typing than a framework, and in exchange there's no magic to explain.

using namespace evtol::test;

#define RUN(fn) runTest(#fn, fn)

int main() {
    std::printf("running tests\n\n");

    RUN(uniform01_stays_between_zero_and_one);
    RUN(same_seed_gives_the_same_sequence);
    RUN(different_seeds_give_different_sequences);
    RUN(uniformInt_can_return_both_ends_of_the_range);
    RUN(uniformInt_handles_a_range_of_one);
    RUN(seed_survives_construction);

    RUN(specs_match_the_problem_statement_table);
    RUN(power_draw_is_energy_per_mile_times_cruise_speed);
    RUN(flight_time_matches_hand_calculation);
    RUN(range_matches_hand_calculation);
    RUN(passenger_miles_matches_the_worked_example);
    RUN(lookup_returns_the_matching_company);
    RUN(alpha_and_delta_cannot_finish_two_flights_in_three_hours);
    RUN(charlie_spends_longer_charging_than_flying);

    RUN(chargers_are_granted_while_any_are_free);
    RUN(requests_beyond_capacity_join_the_queue);
    RUN(releasing_hands_the_charger_to_the_longest_waiter);
    RUN(releasing_with_an_empty_queue_leaves_a_charger_idle);
    RUN(queue_preserves_arrival_order);
    RUN(a_fleet_smaller_than_the_pool_never_queues);
    RUN(a_vehicle_can_come_back_for_another_charge);
    RUN(capacity_is_never_exceeded_under_churn);

    RUN(vehicles_start_airborne_on_a_full_battery);
    RUN(the_transition_table_allows_only_the_real_cycle);
    RUN(a_full_cycle_returns_to_flight);
    RUN(a_free_charger_skips_the_queue);
    RUN(totals_only_count_completed_periods);
    RUN(several_cycles_accumulate);
    RUN(partial_flight_distance_is_available_for_passenger_miles);
    RUN(faults_accumulate_across_flights);

    RUN(simultaneous_events_come_out_in_the_order_they_were_scheduled);
    RUN(the_fleet_always_totals_the_configured_size);
    RUN(the_same_seed_reproduces_the_run);
    RUN(the_charger_limit_is_never_exceeded);
    RUN(every_hour_of_every_vehicle_is_accounted_for);
    RUN(a_lone_vehicle_never_waits);
    RUN(twenty_vehicles_on_three_chargers_do_wait);
    RUN(alpha_completes_exactly_one_flight_in_three_hours);
    RUN(a_longer_run_produces_more_flights);

    return summary();
}
