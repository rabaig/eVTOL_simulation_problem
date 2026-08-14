#ifndef EVTOL_SIMULATION_H
#define EVTOL_SIMULATION_H

#include <cstddef>
#include <queue>
#include <vector>

#include "evtol/ChargerPool.h"
#include "evtol/Event.h"
#include "evtol/FaultModel.h"
#include "evtol/Rng.h"
#include "evtol/Types.h"
#include "evtol/Vehicle.h"

namespace evtol {

/// Everything the problem fixes, in one place so tests can vary it.
struct SimulationConfig {
    int fleetSize = 20;
    std::size_t chargerCount = kDefaultChargerCount;
    Hours duration = 3.0;
};

/// The event loop, and the only thing here that owns a clock.
///
/// Builds a random fleet, puts every vehicle in the air on a full battery,
/// and works through the queue of scheduled events until the clock passes the
/// end of the run. Vehicles and the charger pool react; this decides when.
class Simulation {
public:
    Simulation(const SimulationConfig& config, Rng& rng);

    /// Runs to completion. Callable once.
    void run();

    /// The fleet after the run, each vehicle holding its own totals.
    ///
    /// Vehicles are deliberately left mid-flight or mid-charge at the end
    /// rather than being tidied up. An unfinished period contributes nothing
    /// to the totals, which is exactly the truncation rule the README states.
    const std::vector<Vehicle>& fleet() const { return fleet_; }

    /// Most vehicles seen charging simultaneously.
    ///
    /// Tracked so the charger limit can be tested rather than assumed. It is
    /// the one constraint that would invalidate every reported statistic, and
    /// it would do so silently.
    std::size_t peakChargersInUse() const { return peakChargersInUse_; }

private:
    void buildFleet(Rng& rng);
    void schedule(Hours at, EventType type, VehicleId vehicle);

    /// Schedules the end of a flight and the first fault within it.
    ///
    /// Only schedules - it does not transition the vehicle, which is why
    /// this is not symmetric with beginCharging below. Callers put the
    /// vehicle in the air themselves, and run() relies on Vehicle already
    /// being constructed InFlight.
    void scheduleFlightEvents(VehicleId vehicle, Hours takeoff);

    /// Draws the next fault and schedules it only if it lands before the
    /// battery runs out. A fault that would fall after the flight simply
    /// doesn't happen on this flight.
    void scheduleNextFault(VehicleId vehicle, Hours from, Hours flightEnds);

    void beginCharging(VehicleId vehicle, Hours at);
    void handleFlightComplete(VehicleId vehicle, Hours at);
    void handleChargeComplete(VehicleId vehicle, Hours at);
    void handleFault(VehicleId vehicle, Hours at);

    SimulationConfig config_;

    std::vector<Vehicle> fleet_;
    ChargerPool chargers_;
    FaultModel faultModel_;

    std::priority_queue<Event, std::vector<Event>, EventIsLater> pending_;

    Hours clock_ = 0.0;
    std::uint64_t nextSequence_ = 0;
    std::size_t peakChargersInUse_ = 0;
    bool hasRun_ = false;
};

}  // namespace evtol

#endif  // EVTOL_SIMULATION_H
