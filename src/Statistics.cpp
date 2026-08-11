#include "evtol/Statistics.h"

#include <cassert>

namespace evtol {

std::optional<Hours> TypeStatistics::averageFlightTime() const {
    if (completedFlights == 0) {
        return std::nullopt;
    }

    return totalFlightHours / completedFlights;
}

std::optional<double> TypeStatistics::averageDistancePerFlight() const {
    if (completedFlights == 0) {
        return std::nullopt;
    }

    // Deliberately not totalPassengerMiles divided by anything. That figure
    // includes miles from a flight still in the air, which would drag this
    // average below the distance a completed flight actually covers.
    return completedFlightMiles / completedFlights;
}

std::optional<Hours> TypeStatistics::averageChargeTime() const {
    if (completedCharges == 0) {
        return std::nullopt;
    }

    return totalChargeHours / completedCharges;
}

std::array<TypeStatistics, kCompanyCount> collectStatistics(
    const std::vector<Vehicle>& fleet, Hours endOfRun) {
    std::array<TypeStatistics, kCompanyCount> stats{};

    for (std::size_t i = 0; i < kCompanyCount; ++i) {
        stats[i].company = static_cast<Company>(i);
    }

    for (const Vehicle& vehicle : fleet) {
        const Aircraft& spec = vehicle.type();
        TypeStatistics& row = stats[static_cast<std::size_t>(spec.company())];

        ++row.vehicleCount;

        row.completedFlights += vehicle.completedFlights();
        row.completedCharges += vehicle.completedCharges();
        row.totalFaults += vehicle.faults();

        row.totalFlightHours += vehicle.totalFlightHours();
        row.totalChargeHours += vehicle.totalChargeHours();
        row.totalQueueHours += vehicle.totalQueueHours();

        // Every flight runs the battery flat, so distance is simply the hours
        // flown at cruise speed. No integration required.
        const double completedMiles = vehicle.totalFlightHours() * spec.cruiseSpeedMph();
        row.completedFlightMiles += completedMiles;

        // A vehicle still airborne at the end has covered ground that hasn't
        // been credited anywhere yet. Those miles carried passengers, so they
        // belong in the passenger-mile total even though the flight itself is
        // excluded from the averages above.
        const double airborneMiles = vehicle.milesFlownInCurrentFlight(endOfRun);

        row.totalPassengerMiles +=
            (completedMiles + airborneMiles) * spec.passengerCount();
    }

    return stats;
}

}  // namespace evtol
