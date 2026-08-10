#ifndef EVTOL_CHARGER_POOL_H
#define EVTOL_CHARGER_POOL_H

#include <cstddef>
#include <deque>
#include <optional>
#include <vector>

namespace evtol {

/// Identifies one vehicle. Just an index into the simulation's fleet.
///
/// The pool deliberately knows nothing about vehicles beyond this number. It
/// doesn't need their speed or battery, and keeping it that way means it can
/// be tested with plain integers.
using VehicleId = int;

/// The problem gives three chargers for the whole fleet of twenty.
constexpr std::size_t kDefaultChargerCount = 3;

/// The chargers and the queue waiting for them.
///
/// This is where the whole problem bottlenecks. Twenty aircraft, three
/// chargers, so most of the fleet spends part of the run parked in a line.
///
/// The pool has no notion of time. It answers "who is charging now" and "who
/// goes next", and the simulation decides when those things happen. Two
/// places tracking the clock is one too many.
class ChargerPool {
public:
    /// Count is a parameter rather than a hardcoded three so tests can set up
    /// contention with two vehicles instead of four.
    explicit ChargerPool(std::size_t chargerCount = kDefaultChargerCount);

    /// Ask for a charger.
    ///
    /// Returns true if one was free and the vehicle is now charging, false if
    /// it went to the back of the queue instead. Either way the pool is now
    /// tracking it, so the caller doesn't need to remember which happened.
    bool request(VehicleId vehicle);

    /// Give up a charger.
    ///
    /// Returns the vehicle that immediately took it, or nothing if the queue
    /// was empty. The caller needs that ID: a vehicle starting to charge is
    /// the moment its charge-complete event gets scheduled, and without the
    /// return value the simulation would have to poll to notice.
    std::optional<VehicleId> release(VehicleId vehicle);

    std::size_t capacity() const { return capacity_; }
    std::size_t inUse() const { return charging_.size(); }
    std::size_t queueLength() const { return queue_.size(); }

    bool isCharging(VehicleId vehicle) const;
    bool isWaiting(VehicleId vehicle) const;

private:
    std::size_t capacity_;

    // A vector searched linearly rather than a set. There are three chargers.
    // A linear scan over three elements beats hashing, and the code reads as
    // what it is.
    std::vector<VehicleId> charging_;

    // Front is the longest waiter. First come, first served: the problem says
    // nothing about ordering, and anything cleverer needs justifying.
    std::deque<VehicleId> queue_;
};

}  // namespace evtol

#endif  // EVTOL_CHARGER_POOL_H
