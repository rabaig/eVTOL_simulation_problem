#ifndef EVTOL_VEHICLE_H
#define EVTOL_VEHICLE_H

#include "evtol/Aircraft.h"
#include "evtol/Types.h"

namespace evtol {

/// Where a vehicle is in its cycle.
enum class VehicleState {
    InFlight,
    Queued,
    Charging
};

const char* stateName(VehicleState state);

/// One aircraft in the fleet, and what it's doing right now.
///
/// Vehicle is passive. It owns no clock and schedules nothing: every
/// transition is told the time it happened at. The simulation owns time, and
/// having a second class reason about it independently is how the two end up
/// disagreeing.
///
/// It does keep its own running totals. Flight hours, charge hours and flight
/// counts are accumulated as transitions happen, which means EVTOL-7 can add
/// up finished vehicles rather than subscribing to a stream of events.
class Vehicle {
public:
    /// Vehicles start airborne on a full battery, as the problem specifies.
    Vehicle(VehicleId id, const Aircraft& type, Hours startTime = 0.0);

    VehicleId id() const { return id_; }
    const Aircraft& type() const { return *type_; }
    VehicleState state() const { return state_; }
    Hours stateEnteredAt() const { return stateEnteredAt_; }

    // --- transitions ---
    //
    // Each takes the time it occurs at, closes off the state being left, and
    // adds its duration to the relevant total.

    /// Battery empty. Chargers were all busy, so join the queue.
    void startQueueing(Hours at);

    /// A charger was free. Reachable straight from flight, or after waiting.
    void startCharging(Hours at);

    /// Charge finished, back in the air.
    void startFlight(Hours at);

    /// Count a fault. Only legal while airborne.
    ///
    /// The rule that faults accrue only in flight is enforced here rather
    /// than left to the caller. It's a documented assumption, and an
    /// assumption nothing checks tends to stop being true.
    void recordFault();

    /// Which transitions the state machine allows.
    ///
    /// Public and static so the tests can walk the whole table rather than
    /// relying on an assert firing, which this test harness can't catch.
    static bool isLegalTransition(VehicleState from, VehicleState to);

    // --- totals, covering completed periods only ---
    //
    // A flight or charge still in progress contributes nothing until it ends.
    // That is what keeps a run truncated at three hours from dragging the
    // per-flight averages below what the aircraft can actually do.

    int completedFlights() const { return completedFlights_; }
    int completedCharges() const { return completedCharges_; }

    Hours totalFlightHours() const { return totalFlightHours_; }
    Hours totalChargeHours() const { return totalChargeHours_; }
    Hours totalQueueHours() const { return totalQueueHours_; }

    int faults() const { return faults_; }

    /// Miles covered so far by a flight that hasn't finished.
    ///
    /// Zero unless airborne. Those miles were genuinely flown, so they count
    /// toward passenger miles even though the flight is excluded from the
    /// per-flight averages.
    double milesFlownInCurrentFlight(Hours now) const;

private:
    void leaveCurrentState(Hours at);

    VehicleId id_;

    // A pointer, not a reference, purely so Vehicle stays assignable and can
    // live in a vector. The Aircraft it points at is a function-local static
    // that outlives every vehicle.
    const Aircraft* type_;

    VehicleState state_;
    Hours stateEnteredAt_;

    int completedFlights_ = 0;
    int completedCharges_ = 0;

    Hours totalFlightHours_ = 0.0;
    Hours totalChargeHours_ = 0.0;
    Hours totalQueueHours_ = 0.0;

    int faults_ = 0;
};

}  // namespace evtol

#endif  // EVTOL_VEHICLE_H
