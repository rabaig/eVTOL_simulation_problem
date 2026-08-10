#include "evtol/Aircraft.h"

#include <cassert>

namespace evtol {

Aircraft::Aircraft(Company company,
                   const char* name,
                   double cruiseSpeedMph,
                   double batteryCapacityKwh,
                   double chargeTimeHours,
                   double energyPerMileKwh,
                   int passengerCount,
                   double faultsPerHour)
    : company_(company),
      name_(name),
      cruiseSpeedMph_(cruiseSpeedMph),
      batteryCapacityKwh_(batteryCapacityKwh),
      chargeTimeHours_(chargeTimeHours),
      energyPerMileKwh_(energyPerMileKwh),
      passengerCount_(passengerCount),
      faultsPerHour_(faultsPerHour) {
    // A zero or negative speed or energy figure would divide by zero below and
    // hand back an infinite flight time, which the simulation would happily
    // run with. Better to stop here than to explain the results later.
    assert(cruiseSpeedMph > 0.0);
    assert(batteryCapacityKwh > 0.0);
    assert(chargeTimeHours > 0.0);
    assert(energyPerMileKwh > 0.0);
    assert(passengerCount > 0);
    assert(faultsPerHour >= 0.0);

    // kWh/mile x miles/hour leaves kWh/hour.
    powerDrawKwhPerHour_ = energyPerMileKwh_ * cruiseSpeedMph_;

    // kWh / (kWh/hour) leaves hours.
    flightTimeHours_ = batteryCapacityKwh_ / powerDrawKwhPerHour_;

    rangeMiles_ = flightTimeHours_ * cruiseSpeedMph_;
    passengerMilesPerFlight_ = rangeMiles_ * passengerCount_;
}

const std::array<Aircraft, kCompanyCount>& allAircraft() {
    // Straight from the table in the problem statement. Built once on first
    // use and handed out by reference after that.
    //
    // Order matches the Company enum, which aircraftFor() relies on.
    static const std::array<Aircraft, kCompanyCount> specs{{
        //        company           name       speed  battery  charge  kWh/mi  seats  faults/hr
        Aircraft{Company::Alpha,   "Alpha",     120.0,   320.0,   0.60,    1.6,     4,      0.25},
        Aircraft{Company::Bravo,   "Bravo",     100.0,   100.0,   0.20,    1.5,     5,      0.10},
        Aircraft{Company::Charlie, "Charlie",   160.0,   220.0,   0.80,    2.2,     3,      0.05},
        Aircraft{Company::Delta,   "Delta",      90.0,   120.0,   0.62,    0.8,     2,      0.22},
        Aircraft{Company::Echo,    "Echo",       30.0,   150.0,   0.30,    5.8,     2,      0.61},
    }};

    return specs;
}

const Aircraft& aircraftFor(Company company) {
    const auto index = static_cast<std::size_t>(company);
    assert(index < kCompanyCount);

    return allAircraft()[index];
}

const char* companyName(Company company) {
    return aircraftFor(company).name();
}

}  // namespace evtol
