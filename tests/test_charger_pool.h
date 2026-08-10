#ifndef EVTOL_TEST_CHARGER_POOL_H
#define EVTOL_TEST_CHARGER_POOL_H

namespace evtol::test {

void chargers_are_granted_while_any_are_free();
void requests_beyond_capacity_join_the_queue();
void releasing_hands_the_charger_to_the_longest_waiter();
void releasing_with_an_empty_queue_leaves_a_charger_idle();
void queue_preserves_arrival_order();
void a_fleet_smaller_than_the_pool_never_queues();
void a_vehicle_can_come_back_for_another_charge();
void capacity_is_never_exceeded_under_churn();

}  // namespace evtol::test

#endif  // EVTOL_TEST_CHARGER_POOL_H
