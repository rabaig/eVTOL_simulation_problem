#ifndef EVTOL_REPORTER_H
#define EVTOL_REPORTER_H

#include <array>
#include <cstdint>
#include <string>

#include "evtol/Simulation.h"
#include "evtol/Statistics.h"

namespace evtol {

/// Renders the results table.
///
/// Returns a string rather than printing. Printing directly would make this
/// the one part of the program a test can only check by capturing stdout,
/// and there is no reason for the formatting to be harder to verify than the
/// arithmetic behind it.
std::string formatReport(const std::array<TypeStatistics, kCompanyCount>& stats,
                         const SimulationConfig& config,
                         std::uint32_t seed);

}  // namespace evtol

#endif  // EVTOL_REPORTER_H
