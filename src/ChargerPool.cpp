#include "evtol/ChargerPool.h"

#include <algorithm>
#include <cassert>

namespace evtol {

ChargerPool::ChargerPool(std::size_t chargerCount)
    : capacity_(chargerCount) {
    assert(chargerCount > 0 && "a pool with no chargers would deadlock the fleet");

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
