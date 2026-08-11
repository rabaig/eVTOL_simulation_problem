#include "evtol/Simulation.h"

#include <algorithm>
#include <cassert>

namespace evtol {

Simulation::Simulation(const SimulationConfig& config, Rng& rng)
    : config_(config), chargers_(config.chargerCount) {
    assert(config.fleetSize > 0);
    assert(config.duration > 0.0);

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

void Simulation::beginCharging(VehicleId vehicle, Hours at) {
    Vehicle& v = fleet_[static_cast<std::size_t>(vehicle)];

    v.startCharging(at);
    schedule(at + v.type().chargeTimeHours(), EventType::ChargeComplete, vehicle);

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

    Vehicle& v = fleet_[static_cast<std::size_t>(vehicle)];
    v.startFlight(at);
    schedule(at + v.type().flightTimeHours(), EventType::FlightComplete, vehicle);

    if (next.has_value()) {
        beginCharging(*next, at);
    }
}

void Simulation::run() {
    assert(!hasRun_ && "run() is not re-entrant; build another Simulation");
    hasRun_ = true;

    // Everyone starts airborne on a full battery.
    for (const Vehicle& v : fleet_) {
        schedule(v.type().flightTimeHours(), EventType::FlightComplete, v.id());
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
        ++eventsProcessed_;

        switch (event.type) {
            case EventType::FlightComplete:
                handleFlightComplete(event.vehicle, event.time);
                break;

            case EventType::ChargeComplete:
                handleChargeComplete(event.vehicle, event.time);
                break;
        }

        assert(chargers_.inUse() <= chargers_.capacity());
    }

    clock_ = config_.duration;
}

}  // namespace evtol
