#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <optional>
#include <string>

#include "evtol/Reporter.h"
#include "evtol/Simulation.h"
#include "evtol/Statistics.h"

namespace {

void printUsage() {
    std::printf(
        "eVTOL fleet simulation\n"
        "\n"
        "Usage: evtol_sim [options]\n"
        "\n"
        "  --seed N        Seed the generator so the run can be reproduced.\n"
        "                  Without it a seed is drawn from the system and\n"
        "                  printed, so any result can be repeated.\n"
        "  --hours H       Length of the run. Defaults to 3, as the problem\n"
        "                  specifies.\n"
        "  --vehicles N    Fleet size. Defaults to 20.\n"
        "  --chargers N    Number of chargers. Defaults to 3.\n"
        "  --help          This text.\n");
}

/// Reads the value following an option.
///
/// Returns nothing on a missing or malformed value rather than letting atoi
/// turn "--hours banana" into zero and running a simulation of nothing.
std::optional<double> readValue(int argc, char** argv, int& index) {
    if (index + 1 >= argc) {
        std::fprintf(stderr, "error: %s needs a value\n", argv[index]);
        return std::nullopt;
    }

    ++index;

    try {
        std::size_t consumed = 0;
        const std::string text = argv[index];
        const double value = std::stod(text, &consumed);

        if (consumed != text.size()) {
            std::fprintf(stderr, "error: '%s' is not a number\n", argv[index]);
            return std::nullopt;
        }

        return value;
    } catch (const std::exception&) {
        std::fprintf(stderr, "error: '%s' is not a number\n", argv[index]);
        return std::nullopt;
    }
}

}  // namespace

int main(int argc, char** argv) {
    evtol::SimulationConfig config;
    std::optional<std::uint32_t> requestedSeed;

    for (int i = 1; i < argc; ++i) {
        const char* arg = argv[i];

        if (std::strcmp(arg, "--help") == 0) {
            printUsage();
            return 0;
        }

        std::optional<double> value;

        if (std::strcmp(arg, "--seed") == 0) {
            value = readValue(argc, argv, i);
            if (!value) return 1;
            requestedSeed = static_cast<std::uint32_t>(*value);
        } else if (std::strcmp(arg, "--hours") == 0) {
            value = readValue(argc, argv, i);
            if (!value) return 1;
            config.duration = *value;
        } else if (std::strcmp(arg, "--vehicles") == 0) {
            value = readValue(argc, argv, i);
            if (!value) return 1;
            config.fleetSize = static_cast<int>(*value);
        } else if (std::strcmp(arg, "--chargers") == 0) {
            value = readValue(argc, argv, i);
            if (!value) return 1;
            config.chargerCount = static_cast<std::size_t>(*value);
        } else {
            std::fprintf(stderr, "error: unknown option '%s'\n\n", arg);
            printUsage();
            return 1;
        }
    }

    // Checked here rather than left to an assert, because these come from a
    // user and a Release build would sail straight past the assert.
    if (config.fleetSize <= 0 || config.chargerCount == 0 || config.duration <= 0.0) {
        std::fprintf(stderr,
                     "error: vehicles, chargers and hours must all be above zero\n");
        return 1;
    }

    evtol::Rng rng = requestedSeed.has_value() ? evtol::Rng(*requestedSeed)
                                               : evtol::Rng::fromEntropy();

    // Captured before the run, because the report prints it and the whole
    // point is that someone can feed it back in with --seed.
    const std::uint32_t seed = rng.seed();

    evtol::Simulation simulation(config, rng);
    simulation.run();

    const auto stats = evtol::collectStatistics(simulation.fleet(), config.duration);

    std::fputs(evtol::formatReport(stats, config, seed).c_str(), stdout);

    return 0;
}
