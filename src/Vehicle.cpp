#include "evtol/Vehicle.h"

#include <cassert>

namespace evtol {

bool Vehicle::isLegalTransition(VehicleState from, VehicleState to) {
    switch (from) {
        // Battery runs out, so either a charger was free or the queue is next.
        case VehicleState::InFlight:
            return to == VehicleState::Queued || to == VehicleState::Charging;

        // Only way out of the queue is onto a charger. A vehicle with a flat
        // battery cannot go flying again.
        case VehicleState::Queued:
            return to == VehicleState::Charging;

        // Charging always runs to completion, so the only exit is take-off.
        case VehicleState::Charging:
            return to == VehicleState::InFlight;
    }

    return false;
}

Vehicle::Vehicle(VehicleId id, const AircraftSpec& type, Hours startTime)
    : id_(id),
      type_(&type),
      state_(VehicleState::InFlight),
      stateEnteredAt_(startTime) {}

void Vehicle::leaveCurrentState(Hours at) {
    // Events come off a priority queue ordered by time, so a transition
    // scheduled before the current state began means the queue ordering is
    // broken. Every duration computed after that would be negative.
    assert(at >= stateEnteredAt_ && "simulated time moved backwards");

    const Hours elapsed = at - stateEnteredAt_;

    switch (state_) {
        case VehicleState::InFlight:
            totalFlightHours_ += elapsed;
            ++completedFlights_;
            break;

        case VehicleState::Queued:
            // Waiting is measured but isn't one of the five reported figures.
            // It's the most direct read on how much the charger shortage
            // actually costs, so it's worth having.
            totalQueueHours_ += elapsed;
            break;

        case VehicleState::Charging:
            totalChargeHours_ += elapsed;
            ++completedCharges_;
            break;
    }

    stateEnteredAt_ = at;
}

void Vehicle::startQueueing(Hours at) {
    assert(isLegalTransition(state_, VehicleState::Queued));

    leaveCurrentState(at);
    state_ = VehicleState::Queued;
}

void Vehicle::startCharging(Hours at) {
    assert(isLegalTransition(state_, VehicleState::Charging));

    leaveCurrentState(at);
    state_ = VehicleState::Charging;
}

void Vehicle::startFlight(Hours at) {
    assert(isLegalTransition(state_, VehicleState::InFlight));

    leaveCurrentState(at);
    state_ = VehicleState::InFlight;
}

void Vehicle::recordFault() {
    assert(state_ == VehicleState::InFlight &&
           "faults only accrue in flight, not on a charger or in the queue");

    ++faults_;
}

double Vehicle::milesFlownInCurrentFlight(Hours now) const {
    if (state_ != VehicleState::InFlight) {
        return 0.0;
    }

    assert(now >= stateEnteredAt_ && "asked about a time before the flight began");

    return (now - stateEnteredAt_) * type_->cruiseSpeedMph;
}

}  // namespace evtol
