#include "evtol/Simulation.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace evtol {

Simulation::Simulation(const SimulationConfig& config, Rng& rng)
    : config_(config), chargers_(config.chargerCount), faultModel_(rng) {
    // Thrown, not asserted. An assert vanishes under NDEBUG, and Release is
    // the default build - a negative duration would then run quietly and
    // report negative passenger miles rather than stopping. Same standard as
    // ChargerPool::release: a precondition on public API gets a real check.
    if (config.fleetSize <= 0) {
        throw std::invalid_argument("fleetSize must be at least 1");
    }

    if (config.duration <= 0.0) {
        throw std::invalid_argument("duration must be above zero");
    }

    buildFleet(rng);
}

void Simulation::buildFleet(Rng& rng) {
    fleet_.reserve(static_cast<std::size_t>(config_.fleetSize));

    // "A random number of each type of vehicle should be used, with the total
    // between all five types being 20."
    //
    // Rolling a type per vehicle is the simplest reading and satisfies both
    // halves: the counts are random and they sum to the fleet size by
    // construction. It can produce zero of a type, which is why the report
    // has to cope with an empty one.
    for (int id = 0; id < config_.fleetSize; ++id) {
        const int roll = rng.uniformInt(0, static_cast<int>(kCompanyCount) - 1);
        const Company company = static_cast<Company>(roll);

        fleet_.emplace_back(id, aircraftFor(company), 0.0);
    }
}

void Simulation::schedule(Hours at, EventType type, VehicleId vehicle) {
    pending_.push(Event{at, type, vehicle, nextSequence_++});
}

void Simulation::scheduleNextFault(VehicleId vehicle, Hours from, Hours flightEnds) {
    const Vehicle& v = fleet_[static_cast<std::size_t>(vehicle)];
    const Hours at = from + faultModel_.timeToNextFault(v.type().faultsPerHour);

    // Faults accrue only in flight, so a draw landing past the end of this
    // one simply doesn't happen. Restricting the process to the flight
    // interval is what keeps queueing and charging fault-free without any
    // bookkeeping about which flight a stale event belonged to.
    //
    // Strictly less than, not less than or equal: at exactly the end of the
    // flight, FlightComplete was scheduled first and so wins the tie-break,
    // and the fault would arrive to find the vehicle already on the ground.
    if (at < flightEnds) {
        schedule(at, EventType::Fault, vehicle);
    }
}

void Simulation::scheduleFlightEvents(VehicleId vehicle, Hours takeoff) {
    const Vehicle& v = fleet_[static_cast<std::size_t>(vehicle)];
    const Hours flightEnds = takeoff + v.type().enduranceHours();

    schedule(flightEnds, EventType::FlightComplete, vehicle);
    scheduleNextFault(vehicle, takeoff, flightEnds);
}

void Simulation::handleFault(VehicleId vehicle, Hours at) {
    Vehicle& v = fleet_[static_cast<std::size_t>(vehicle)];

    // Guaranteed by scheduleNextFault only ever scheduling inside a flight.
    assert(v.state() == VehicleState::InFlight);

    v.recordFault();

    // A Poisson process is memoryless, so the next gap is drawn from the
    // moment of this fault exactly as it was from take-off.
    scheduleNextFault(vehicle, at, v.stateEnteredAt() + v.type().enduranceHours());
}

void Simulation::beginCharging(VehicleId vehicle, Hours at) {
    Vehicle& v = fleet_[static_cast<std::size_t>(vehicle)];

    v.startCharging(at);
    schedule(at + v.type().chargeTimeHours, EventType::ChargeComplete, vehicle);

    peakChargersInUse_ = std::max(peakChargersInUse_, chargers_.inUse());
}

void Simulation::handleFlightComplete(VehicleId vehicle, Hours at) {
    // The problem says a vehicle is immediately in line for the charger after
    // running out of power, so there is no idle state between the two.
    if (chargers_.request(vehicle)) {
        beginCharging(vehicle, at);
        return;
    }

    fleet_[static_cast<std::size_t>(vehicle)].startQueueing(at);
}

void Simulation::handleChargeComplete(VehicleId vehicle, Hours at) {
    // The pool tracks vehicles by ID and knows nothing about their state, so
    // releasing before or after the take-off transition makes no difference.
    // Released first only because the handover reads better that way.
    const auto next = chargers_.release(vehicle);

    fleet_[static_cast<std::size_t>(vehicle)].startFlight(at);
    scheduleFlightEvents(vehicle, at);

    if (next.has_value()) {
        beginCharging(*next, at);
    }
}

void Simulation::run() {
    // Also thrown rather than asserted. Running twice re-seeds every flight
    // at t=0 while the clock is already at the end, so durations come out
    // negative and most of the fleet ends up with corrupted totals - a
    // plausible-looking table that is simply wrong.
    if (hasRun_) {
        throw std::logic_error("run() has already been called on this Simulation");
    }

    hasRun_ = true;

    // Everyone starts airborne on a full battery. Take the time from the
    // vehicle rather than repeating the 0.0 that buildFleet() passed to the
    // constructor - two copies of one fact, and if they disagreed the fault
    // window and the flight would be bounded differently.
    for (const Vehicle& v : fleet_) {
        scheduleFlightEvents(v.id(), v.stateEnteredAt());
    }

    while (!pending_.empty()) {
        const Event event = pending_.top();

        // Anything past the end of the run never happens. Vehicles are left
        // in whatever state they were in, mid-flight or mid-charge.
        if (event.time > config_.duration) {
            break;
        }

        pending_.pop();

        // The queue is ordered by time, so this can only trip if the ordering
        // itself is broken. Every duration computed afterwards would be
        // negative, and the totals would be quietly wrong rather than absent.
        assert(event.time >= clock_ && "event queue produced an out-of-order time");
        clock_ = event.time;

        switch (event.type) {
            case EventType::FlightComplete:
                handleFlightComplete(event.vehicle, event.time);
                break;

            case EventType::ChargeComplete:
                handleChargeComplete(event.vehicle, event.time);
                break;

            case EventType::Fault:
                handleFault(event.vehicle, event.time);
                break;
        }

        assert(chargers_.inUse() <= chargers_.capacity());
    }
}

}  // namespace evtol
