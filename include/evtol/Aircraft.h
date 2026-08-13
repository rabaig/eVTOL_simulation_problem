#ifndef EVTOL_AIRCRAFT_H
#define EVTOL_AIRCRAFT_H

#include <array>
#include <cstddef>

namespace evtol {

/// The five manufacturers from the problem statement.
enum class Company {
    Alpha,
    Bravo,
    Charlie,
    Delta,
    Echo,

    Count  ///< Not a manufacturer. Keeps the count next to the list.
};

constexpr std::size_t kCompanyCount = static_cast<std::size_t>(Company::Count);

/// One manufacturer's design: the six properties from the problem table, plus
/// the numbers that follow from them.
///
/// A specification, not an aircraft in the air. Anything that changes during
/// the run belongs to Vehicle.
///
/// Units are in the member names because this problem mixes time and distance
/// in every calculation, and a unit mixup gives a plausible wrong answer
/// rather than an obvious one.
struct AircraftSpec {
    Company company;
    const char* name;

    double cruiseSpeedMph;
    double batteryCapacityKwh;
    double chargeTimeHours;
    double energyPerMileKwh;
    int passengerCount;
    double faultsPerHour;

    /// How fast the battery drains in the air: kWh per mile x miles per hour.
    double powerDrawKwhPerHour() const {
        return energyPerMileKwh * cruiseSpeedMph;
    }

    /// How long it stays up on a full battery.
    ///
    /// The problem says a vehicle flies until the battery is empty, so this is
    /// also the length of every flight it makes. Which is why the reported
    /// average flight time has to come out equal to this exactly.
    double enduranceHours() const {
        return batteryCapacityKwh / powerDrawKwhPerHour();
    }

    /// How far it gets on that battery.
    double rangeMiles() const { return enduranceHours() * cruiseSpeedMph; }

    /// Range times seats: the problem's passenger-miles figure for one flight.
    double passengerMilesPerFlight() const { return rangeMiles() * passengerCount; }
};

/// The five specifications, in Company order.
const std::array<AircraftSpec, kCompanyCount>& allAircraft();

/// Look up one type. Company is a closed enum, so this always finds something.
const AircraftSpec& aircraftFor(Company company);

/// Printable company name, for the results table.
const char* companyName(Company company);

}  // namespace evtol

#endif  // EVTOL_AIRCRAFT_H
