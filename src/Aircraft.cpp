#include "evtol/Aircraft.h"

#include <cassert>

namespace evtol {

const std::array<AircraftSpec, kCompanyCount>& allAircraft() {
    // Straight from the table in the problem statement. Order matches the
    // Company enum, which aircraftFor() relies on.
    //
    // A function-local static rather than a namespace-scope constant: every
    // Vehicle holds a pointer into this array, so it has to outlive them all,
    // and this sidesteps static initialisation order entirely.
    static const std::array<AircraftSpec, kCompanyCount> specs{{
        //       company           name       speed  battery  charge  kWh/mi  seats  faults/hr
        {Company::Alpha,   "Alpha",     120.0,   320.0,   0.60,    1.6,     4,      0.25},
        {Company::Bravo,   "Bravo",     100.0,   100.0,   0.20,    1.5,     5,      0.10},
        {Company::Charlie, "Charlie",   160.0,   220.0,   0.80,    2.2,     3,      0.05},
        {Company::Delta,   "Delta",      90.0,   120.0,   0.62,    0.8,     2,      0.22},
        {Company::Echo,    "Echo",       30.0,   150.0,   0.30,    5.8,     2,      0.61},
    }};

    return specs;
}

const AircraftSpec& aircraftFor(Company company) {
    const auto index = static_cast<std::size_t>(company);
    assert(index < kCompanyCount);

    return allAircraft()[index];
}

const char* companyName(Company company) {
    return aircraftFor(company).name;
}

}  // namespace evtol
