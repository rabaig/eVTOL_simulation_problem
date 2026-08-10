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
    Echo
};

constexpr std::size_t kCompanyCount = 5;

/// One manufacturer's design.
///
/// Holds the six properties given in the problem plus the handful of numbers
/// that follow from them. Nothing here changes once it's built: an Aircraft
/// describes a type, not a particular vehicle. State belongs to Vehicle.
///
/// Units are in the member names on purpose. Mixing hours and minutes, or
/// miles and kilometres, is the kind of mistake that produces plausible
/// wrong answers rather than obvious ones.
class Aircraft {
public:
    Aircraft(Company company,
             const char* name,
             double cruiseSpeedMph,
             double batteryCapacityKwh,
             double chargeTimeHours,
             double energyPerMileKwh,
             int passengerCount,
             double faultsPerHour);

    // --- given by the problem ---

    Company company() const { return company_; }
    const char* name() const { return name_; }

    double cruiseSpeedMph() const { return cruiseSpeedMph_; }
    double batteryCapacityKwh() const { return batteryCapacityKwh_; }
    double chargeTimeHours() const { return chargeTimeHours_; }
    double energyPerMileKwh() const { return energyPerMileKwh_; }
    int passengerCount() const { return passengerCount_; }
    double faultsPerHour() const { return faultsPerHour_; }

    // --- derived ---
    //
    // Worked out once in the constructor rather than on every call. They can
    // never change, and the simulation asks for them constantly.

    /// How fast the battery drains in the air: kWh per mile x miles per hour.
    double powerDrawKwhPerHour() const { return powerDrawKwhPerHour_; }

    /// How long the aircraft stays up on a full battery, in hours.
    ///
    /// The problem says a vehicle flies until the battery is empty, so this
    /// is also the length of every flight it makes.
    double flightTimeHours() const { return flightTimeHours_; }

    /// How far it gets on that battery, in miles.
    double rangeMiles() const { return rangeMiles_; }

    /// Range times seats. The problem's passenger-miles figure for one flight.
    double passengerMilesPerFlight() const { return passengerMilesPerFlight_; }

private:
    Company company_;
    const char* name_;

    double cruiseSpeedMph_;
    double batteryCapacityKwh_;
    double chargeTimeHours_;
    double energyPerMileKwh_;
    int passengerCount_;
    double faultsPerHour_;

    double powerDrawKwhPerHour_;
    double flightTimeHours_;
    double rangeMiles_;
    double passengerMilesPerFlight_;
};

/// The five types, in Company enum order.
const std::array<Aircraft, kCompanyCount>& allAircraft();

/// Look up one type. Company is a closed enum, so this always finds something.
const Aircraft& aircraftFor(Company company);

/// Printable company name, for the results table.
const char* companyName(Company company);

}  // namespace evtol

#endif  // EVTOL_AIRCRAFT_H
