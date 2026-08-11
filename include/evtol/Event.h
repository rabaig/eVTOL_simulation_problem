#ifndef EVTOL_EVENT_H
#define EVTOL_EVENT_H

#include <cstdint>

#include "evtol/Types.h"

namespace evtol {

/// Something that happens to one vehicle at a known time.
enum class EventType {
    FlightComplete,  ///< Battery empty. Vehicle wants a charger.
    ChargeComplete,  ///< Charge finished. Vehicle takes off again.
    Fault            ///< Something went wrong in the air. Counted, not acted on.
};

/// A scheduled event.
///
/// Every state change in this problem happens at a time that can be worked
/// out the moment the previous one does: a flight ends exactly
/// flightTimeHours after take-off, a charge exactly chargeTimeHours after it
/// starts. So the simulation keeps these in a queue ordered by time and jumps
/// straight from one to the next, rather than stepping a clock forward and
/// checking what changed. No timestep to tune, no rounding, and three hours
/// of flying resolves in microseconds.
struct Event {
    Hours time;
    EventType type;
    VehicleId vehicle;

    /// Insertion order, used only to break ties.
    ///
    /// Two events landing on the same instant is not a corner case here: a
    /// vehicle finishing its charge and the queued vehicle taking that
    /// charger happen at the same time, every time. A heap gives no ordering
    /// guarantee between equal elements, so without this the same seed could
    /// produce different output on a different compiler.
    std::uint64_t sequence;
};

/// Orders the queue: earliest time first, and among equal times, whichever
/// was scheduled first.
struct EventIsLater {
    bool operator()(const Event& a, const Event& b) const {
        if (a.time != b.time) {
            return a.time > b.time;
        }

        return a.sequence > b.sequence;
    }
};

}  // namespace evtol

#endif  // EVTOL_EVENT_H
