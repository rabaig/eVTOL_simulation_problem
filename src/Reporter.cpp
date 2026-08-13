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
constexpr int kQueueWidth = 11;
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

std::string headerRow() {
    std::ostringstream out;

    out << std::left << std::setw(kCompanyWidth) << "Company" << std::right
        << std::setw(kCountWidth) << "Vehicles"
        << std::setw(kFlightWidth) << "Flights"
        << std::setw(kAverageWidth) << "Avg flight/h"
        << std::setw(kDistanceWidth) << "Avg dist/mi"
        << std::setw(kChargeWidth) << "Charges"
        << std::setw(kAverageWidth) << "Avg charge/h"
        << std::setw(kQueueWidth) << "Queued/h"
        << std::setw(kFaultWidth) << "Faults"
        << std::setw(kPassengerWidth) << "Passenger-miles";

    return out.str();
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

    // The seed is printed whether it was chosen or drawn from entropy. A run
    // nobody can reproduce is not evidence of anything.
    out << "  seed " << seed << " (re-run with --seed " << seed << ")\n\n";

    // The rule is drawn from the header's own length rather than by adding the
    // widths up by hand, which is an expression that can silently disagree
    // with the line it is supposed to underline.
    const std::string header = headerRow();
    const std::string rule(header.size(), '-');

    out << header << "\n" << rule << "\n";

    int totalVehicles = 0;
    int totalFlights = 0;
    int totalCharges = 0;
    int totalFaults = 0;
    Hours totalQueued = 0.0;
    double totalPassengerMiles = 0.0;

    for (const TypeStatistics& row : stats) {
        // std::fixed and setprecision are sticky on the stream, so each
        // numeric column sets its own. The integer columns don't care.
        out << std::left << std::setw(kCompanyWidth) << companyName(row.company)
            << std::right
            << std::setw(kCountWidth) << row.vehicleCount
            << std::setw(kFlightWidth) << row.completedFlights;

        writeOptional(out, row.averageFlightTime(), kAverageWidth, 4);
        writeOptional(out, row.averageDistancePerFlight(), kDistanceWidth, 2);

        out << std::setw(kChargeWidth) << row.completedCharges;

        writeOptional(out, row.averageChargeTime(), kAverageWidth, 4);

        out << std::setw(kQueueWidth) << std::fixed << std::setprecision(2)
            << row.totalQueueHours
            << std::setw(kFaultWidth) << row.totalFaults
            << std::setw(kPassengerWidth) << std::fixed << std::setprecision(1)
            << row.totalPassengerMiles << "\n";

        totalVehicles += row.vehicleCount;
        totalFlights += row.completedFlights;
        totalCharges += row.completedCharges;
        totalFaults += row.totalFaults;
        totalQueued += row.totalQueueHours;
        totalPassengerMiles += row.totalPassengerMiles;
    }

    out << rule << "\n";

    // Averages are deliberately absent from the total row. An average across
    // types would weigh Alpha's 1h40m flights against Charlie's 37 minutes and
    // mean nothing.
    out << std::left << std::setw(kCompanyWidth) << "Total" << std::right
        << std::setw(kCountWidth) << totalVehicles
        << std::setw(kFlightWidth) << totalFlights
        << std::setw(kAverageWidth) << "-"
        << std::setw(kDistanceWidth) << "-"
        << std::setw(kChargeWidth) << totalCharges
        << std::setw(kAverageWidth) << "-"
        << std::setw(kQueueWidth) << std::fixed << std::setprecision(2) << totalQueued
        << std::setw(kFaultWidth) << totalFaults
        << std::setw(kPassengerWidth) << std::fixed << std::setprecision(1)
        << totalPassengerMiles << "\n";

    out << "\nA dash means there was nothing to report: no vehicles of that "
           "type, or\nnone that finished a flight or a charge before the run "
           "ended.\n";

    out << "\nQueued/h is time spent waiting for a charger. It isn't one of "
           "the figures the\nproblem asks for, but with twenty vehicles and "
           "three chargers it is the most\ndirect measure of what the shortage "
           "costs.\n";

    out << "\nFlights and charges still in progress at " << std::fixed
        << std::setprecision(2) << config.duration
        << " hours are excluded from the\naverages, but the miles already "
           "flown on them are included in passenger-miles.\n";

    return out.str();
}

}  // namespace evtol
