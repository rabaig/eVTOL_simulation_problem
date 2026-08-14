#include "evtol/ChargerPool.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>

namespace evtol {

ChargerPool::ChargerPool(std::size_t chargerCount)
    : capacity_(chargerCount) {
    // A pool with no chargers deadlocks the fleet. In Release the assert
    // vanished and the run reported every flight completing and nobody ever
    // charging, which looks like a result.
    if (chargerCount == 0) {
        throw std::invalid_argument("a charger pool needs at least one charger");
    }

    charging_.reserve(chargerCount);
}

bool ChargerPool::request(VehicleId vehicle) {
    // Asking twice means the caller has lost track of its own state, which
    // would show up later as a vehicle charging on two chargers or sitting in
    // the queue twice. Cheaper to catch here.
    assert(!isCharging(vehicle) && "vehicle is already on a charger");
    assert(!isWaiting(vehicle) && "vehicle is already in the queue");

    if (charging_.size() < capacity_) {
        charging_.push_back(vehicle);
        return true;
    }

    queue_.push_back(vehicle);
    return false;
}

std::optional<VehicleId> ChargerPool::release(VehicleId vehicle) {
    const auto held = std::find(charging_.begin(), charging_.end(), vehicle);
    assert(held != charging_.end() && "vehicle is not on a charger");

    // The assert says this is a caller bug, but assert compiles away under
    // NDEBUG and erase(end()) is undefined behaviour, not a no-op. A Release
    // build would corrupt the pool rather than complain. Cheap to guard, and
    // returning "nobody took a charger" is the truthful answer when no
    // charger was actually given up.
    if (held == charging_.end()) {
        return std::nullopt;
    }

    charging_.erase(held);

    if (queue_.empty()) {
        return std::nullopt;
    }

    // The charger never sits idle while someone is waiting, so the handover
    // happens here rather than being left for the caller to remember.
    const VehicleId next = queue_.front();
    queue_.pop_front();
    charging_.push_back(next);

    return next;
}

bool ChargerPool::isCharging(VehicleId vehicle) const {
    return std::find(charging_.begin(), charging_.end(), vehicle) != charging_.end();
}

bool ChargerPool::isWaiting(VehicleId vehicle) const {
    return std::find(queue_.begin(), queue_.end(), vehicle) != queue_.end();
}

}  // namespace evtol
