#ifndef EVTOL_STATISTICS_H
#define EVTOL_STATISTICS_H

#include <array>
#include <optional>
#include <vector>

#include "evtol/Aircraft.h"
#include "evtol/Types.h"
#include "evtol/Vehicle.h"

namespace evtol {

/// The reported figures for one manufacturer.
///
/// Everything is per type, not per vehicle, which is what the problem asks
/// for. Averages are optional because the fleet split is random and can
/// produce no vehicles of a type at all — or vehicles that never finish a
/// flight inside the window. An empty optional says "nothing to report",
/// which the caller has to deal with; a zero would quietly look like an
/// answer.
struct TypeStatistics {
    Company company = Company::Alpha;

    int vehicleCount = 0;
    int completedFlights = 0;
    int completedCharges = 0;
    int totalFaults = 0;

    /// Miles from completed flights only. Feeds the distance average.
    double completedFlightMiles = 0.0;

    /// Miles flown times seats, including flights still in the air when the
    /// clock stopped. Those miles were covered, so they count here even
    /// though the unfinished flight is left out of the averages.
    double totalPassengerMiles = 0.0;

    Hours totalFlightHours = 0.0;
    Hours totalChargeHours = 0.0;

    /// Not one of the five required figures, but the most direct measure of
    /// what the charger shortage costs.
    ///
    /// Unlike the flight and charge hours above, this includes waits still
    /// in progress when the clock stops. Those two feed averages, where a
    /// truncated period would drag the figure below what the aircraft can
    /// actually do; this is a plain total, and at three hours most of the
    /// fleet is still queued - excluding them would report a fraction of
    /// the real wait. Same reasoning as passenger miles below.
    Hours totalQueueHours = 0.0;

    // --- the three averages the problem asks for ---
    //
    // The other two figures it wants, total faults and total passenger
    // miles, are the plain members above.

    std::optional<Hours> averageFlightTime() const;
    std::optional<double> averageDistancePerFlight() const;
    std::optional<Hours> averageChargeTime() const;

};

/// Adds up a finished fleet, grouped by manufacturer.
///
/// A plain function rather than a class because there is no state to keep.
/// Vehicles accumulate their own totals as they go, so this is a fold over
/// the fleet at the end rather than something that has to watch the run
/// happen.
///
/// endOfRun is needed to work out how far any still-airborne vehicle has
/// flown since take-off.
std::array<TypeStatistics, kCompanyCount> collectStatistics(
    const std::vector<Vehicle>& fleet, Hours endOfRun);

}  // namespace evtol

#endif  // EVTOL_STATISTICS_H
