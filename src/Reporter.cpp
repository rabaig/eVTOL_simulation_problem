#include "evtol/Reporter.h"

#include <iomanip>
#include <optional>
#include <sstream>

namespace evtol {
namespace {

// Column widths, kept together so the header and the rows can't drift apart.
constexpr int kCompanyWidth = 11;
constexpr int kCountWidth = 9;
constexpr int kFlightWidth = 8;
constexpr int kAverageWidth = 14;
constexpr int kDistanceWidth = 13;
constexpr int kChargeWidth = 9;
constexpr int kFaultWidth = 8;
constexpr int kPassengerWidth = 17;

/// Prints an optional number, or a dash where there is nothing to report.
///
/// A type with no vehicles, or none that finished a flight, has no average.
/// Printing 0.00 there would read as a measurement rather than an absence,
/// which is exactly the wrong impression.
void writeOptional(std::ostringstream& out, const std::optional<double>& value,
                   int width, int precision) {
    if (!value.has_value()) {
        out << std::setw(width) << "-";
        return;
    }

    out << std::setw(width) << std::fixed << std::setprecision(precision) << *value;
}

}  // namespace

std::string formatReport(const std::array<TypeStatistics, kCompanyCount>& stats,
                         const SimulationConfig& config,
                         std::uint32_t seed) {
    std::ostringstream out;

    out << "eVTOL fleet simulation\n";
    out << "  " << config.fleetSize << " vehicles, "
        << config.chargerCount << " chargers, "
        << std::fixed << std::setprecision(2) << config.duration << " hours\n";

    // The seed is the whole reason a run in a README is worth anything. Print
    // it whether it was chosen or drawn from entropy, so any result can be
    // reproduced with --seed.
    out << "  seed " << seed << " (re-run with --seed " << seed << ")\n\n";

    out << std::left << std::setw(kCompanyWidth) << "Company" << std::right
        << std::setw(kCountWidth) << "Vehicles"
        << std::setw(kFlightWidth) << "Flights"
        << std::setw(kAverageWidth) << "Avg flight/h"
        << std::setw(kDistanceWidth) << "Avg dist/mi"
        << std::setw(kChargeWidth) << "Charges"
        << std::setw(kAverageWidth) << "Avg charge/h"
        << std::setw(kFaultWidth) << "Faults"
        << std::setw(kPassengerWidth) << "Passenger-miles"
        << "\n";

    const int totalWidth = kCompanyWidth + kCountWidth + kFlightWidth +
                           kAverageWidth * 2 + kDistanceWidth + kChargeWidth +
                           kFaultWidth + kPassengerWidth;
    out << std::string(static_cast<std::size_t>(totalWidth), '-') << "\n";

    int totalVehicles = 0;
    int totalFlights = 0;
    int totalCharges = 0;
    int totalFaults = 0;
    double totalPassengerMiles = 0.0;

    for (const TypeStatistics& row : stats) {
        out << std::left << std::setw(kCompanyWidth) << companyName(row.company)
            << std::right
            << std::setw(kCountWidth) << row.vehicleCount
            << std::setw(kFlightWidth) << row.completedFlights;

        writeOptional(out, row.averageFlightTime(), kAverageWidth, 4);
        writeOptional(out, row.averageDistancePerFlight(), kDistanceWidth, 2);

        out << std::setw(kChargeWidth) << row.completedCharges;

        writeOptional(out, row.averageChargeTime(), kAverageWidth, 4);

        out << std::setw(kFaultWidth) << row.totalFaults
            << std::setw(kPassengerWidth) << std::fixed << std::setprecision(1)
            << row.totalPassengerMiles << "\n";

        totalVehicles += row.vehicleCount;
        totalFlights += row.completedFlights;
        totalCharges += row.completedCharges;
        totalFaults += row.totalFaults;
        totalPassengerMiles += row.totalPassengerMiles;
    }

    out << std::string(static_cast<std::size_t>(totalWidth), '-') << "\n";

    out << std::left << std::setw(kCompanyWidth) << "Total" << std::right
        << std::setw(kCountWidth) << totalVehicles
        << std::setw(kFlightWidth) << totalFlights
        << std::setw(kAverageWidth) << "-"
        << std::setw(kDistanceWidth) << "-"
        << std::setw(kChargeWidth) << totalCharges
        << std::setw(kAverageWidth) << "-"
        << std::setw(kFaultWidth) << totalFaults
        << std::setw(kPassengerWidth) << std::fixed << std::setprecision(1)
        << totalPassengerMiles << "\n";

    // Averages are deliberately absent from the total row. An average across
    // types would weight Alpha's 1h40m flights against Charlie's 37 minutes
    // and mean nothing.

    out << "\nA dash means there was nothing to report: no vehicles of that "
           "type, or\nnone that finished a flight or a charge before the run "
           "ended.\n";

    out << "\nFlights and charges still in progress at " << std::fixed
        << std::setprecision(2) << config.duration
        << " hours are excluded from the\naverages, but the miles already "
           "flown on them are included in passenger-miles.\n";

    return out.str();
}

}  // namespace evtol
