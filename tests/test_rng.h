#ifndef EVTOL_TEST_RNG_H
#define EVTOL_TEST_RNG_H

// Tests are plain functions. They're declared here so test_main.cpp can call
// them, which is the whole registration mechanism.

namespace evtol::test {

void uniform01_stays_between_zero_and_one();
void same_seed_gives_the_same_sequence();
void different_seeds_give_different_sequences();
void uniformInt_can_return_both_ends_of_the_range();
void uniformInt_handles_a_range_of_one();
void seed_survives_construction();

}  // namespace evtol::test

#endif  // EVTOL_TEST_RNG_H
