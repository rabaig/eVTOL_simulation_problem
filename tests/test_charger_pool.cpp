#include "test_charger_pool.h"

#include <deque>

#include "TestHarness.h"
#include "evtol/ChargerPool.h"

// The pool is tested on its own with plain integers for vehicle IDs. It never
// needs an Aircraft, which is the reason it was built before Vehicle: the
// queueing behaviour can be pinned down without any of the flight maths in
// the way.

namespace evtol::test {

void chargers_are_granted_while_any_are_free() {
    ChargerPool pool(3);

    CHECK_EQ(pool.capacity(), std::size_t{3});
    CHECK_EQ(pool.inUse(), std::size_t{0});

    CHECK(pool.request(10));
    CHECK(pool.request(11));
    CHECK(pool.request(12));

    CHECK_EQ(pool.inUse(), std::size_t{3});
    CHECK_EQ(pool.queueLength(), std::size_t{0});

    CHECK(pool.isCharging(10));
    CHECK(pool.isCharging(12));
    CHECK(!pool.isWaiting(10));
}

void requests_beyond_capacity_join_the_queue() {
    ChargerPool pool(3);

    pool.request(10);
    pool.request(11);
    pool.request(12);

    // The fourth arrival is the whole point of the problem.
    CHECK(!pool.request(13));

    CHECK_EQ(pool.inUse(), std::size_t{3});
    CHECK_EQ(pool.queueLength(), std::size_t{1});

    CHECK(pool.isWaiting(13));
    CHECK(!pool.isCharging(13));
}

void releasing_hands_the_charger_to_the_longest_waiter() {
    ChargerPool pool(3);

    pool.request(10);
    pool.request(11);
    pool.request(12);
    pool.request(13);  // queued first
    pool.request(14);  // queued second

    // 11 finishes. 13 has been waiting longer than 14, so 13 goes on.
    const auto next = pool.release(11);

    CHECK(next.has_value());
    CHECK_EQ(*next, 13);

    CHECK(pool.isCharging(13));
    CHECK(pool.isWaiting(14));
    CHECK(!pool.isCharging(11));

    // The charger changed hands rather than sitting idle, so occupancy is
    // unchanged and the queue is one shorter.
    CHECK_EQ(pool.inUse(), std::size_t{3});
    CHECK_EQ(pool.queueLength(), std::size_t{1});
}

void releasing_with_an_empty_queue_leaves_a_charger_idle() {
    ChargerPool pool(3);

    pool.request(10);
    pool.request(11);

    const auto next = pool.release(10);

    // Nobody waiting, so nothing to hand over and a charger goes free.
    CHECK(!next.has_value());
    CHECK_EQ(pool.inUse(), std::size_t{1});
    CHECK_EQ(pool.queueLength(), std::size_t{0});
}

void queue_preserves_arrival_order() {
    ChargerPool pool(1);

    pool.request(100);  // charging

    // Four more arrive in a known order.
    pool.request(101);
    pool.request(102);
    pool.request(103);
    pool.request(104);

    CHECK_EQ(pool.queueLength(), std::size_t{4});

    // Drain the pool one at a time and check they come out in arrival order.
    // With a single charger the handover sequence is unambiguous, which is
    // why this test uses one rather than three.
    VehicleId current = 100;
    const VehicleId expected[] = {101, 102, 103, 104};

    for (VehicleId want : expected) {
        const auto next = pool.release(current);
        CHECK(next.has_value());
        CHECK_EQ(*next, want);
        current = want;
    }

    // Last one finishes with nobody behind it.
    CHECK(!pool.release(current).has_value());
    CHECK_EQ(pool.inUse(), std::size_t{0});
}

void a_fleet_smaller_than_the_pool_never_queues() {
    ChargerPool pool(3);

    // Two vehicles, three chargers. There is no contention to model and the
    // queue should stay empty however they cycle.
    for (int round = 0; round < 10; ++round) {
        CHECK(pool.request(1));
        CHECK(pool.request(2));
        CHECK_EQ(pool.queueLength(), std::size_t{0});

        CHECK(!pool.release(1).has_value());
        CHECK(!pool.release(2).has_value());
    }

    CHECK_EQ(pool.inUse(), std::size_t{0});
}

void a_vehicle_can_come_back_for_another_charge() {
    ChargerPool pool(1);

    CHECK(pool.request(7));
    CHECK(!pool.release(7).has_value());

    // Over three hours a vehicle charges several times. Releasing has to
    // leave the pool willing to take it again.
    CHECK(pool.request(7));
    CHECK(pool.isCharging(7));
}

void capacity_is_never_exceeded_under_churn() {
    // The charger limit is the one constraint that would invalidate every
    // statistic if it were violated, and it would do so silently. This runs
    // the full fleet through a long sequence of handovers and checks the
    // invariant holds at every step.
    constexpr int kFleet = 20;
    constexpr std::size_t kChargers = kDefaultChargerCount;

    ChargerPool pool(kChargers);

    for (int id = 0; id < kFleet; ++id) {
        pool.request(id);
    }

    CHECK_EQ(pool.inUse(), kChargers);
    CHECK_EQ(pool.queueLength(), std::size_t{kFleet} - kChargers);

    // Track who holds a charger, oldest grant first, so the test knows who to
    // release next without the pool having to expose its internals.
    std::deque<VehicleId> holders;
    for (int id = 0; id < static_cast<int>(kChargers); ++id) {
        holders.push_back(id);
    }

    for (int step = 0; step < 200; ++step) {
        const VehicleId finished = holders.front();
        holders.pop_front();

        const auto next = pool.release(finished);

        // With 20 vehicles and 3 chargers the queue can never empty, so a
        // release must always hand over to somebody.
        CHECK(next.has_value());
        if (next.has_value()) {
            holders.push_back(*next);
        }

        // Straight back in line, as the problem describes.
        pool.request(finished);

        CHECK(pool.inUse() <= pool.capacity());
        CHECK_EQ(pool.inUse() + pool.queueLength(), std::size_t{kFleet});
    }
}

}  // namespace evtol::test
