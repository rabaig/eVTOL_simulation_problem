#ifndef EVTOL_TEST_FAULT_MODEL_H
#define EVTOL_TEST_FAULT_MODEL_H

namespace evtol::test {

void the_interval_matches_the_inverse_transform_formula();
void a_zero_fault_rate_never_faults();
void intervals_are_never_negative_or_infinite();
void the_long_run_mean_interval_converges_on_one_over_lambda();
void a_higher_rate_produces_shorter_gaps();
void faults_scale_with_flight_hours_not_wall_clock();
void fault_counts_are_reproducible_from_a_seed();

}  // namespace evtol::test

#endif  // EVTOL_TEST_FAULT_MODEL_H
