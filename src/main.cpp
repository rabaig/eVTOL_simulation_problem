#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <limits>
#include <optional>

#include "evtol/Reporter.h"
#include "evtol/Simulation.h"
#include "evtol/Statistics.h"

// Runs the simulation the problem describes: 20 vehicles, 3 chargers, 3 hours.
//
// The only option is --seed. An earlier version also took --hours, --vehicles
// and --chargers, which was seventy-odd lines of argument handling in the
// first file anyone opens, for flags nothing else in the project used. Tests
// that need a different fleet or a longer run build a SimulationConfig
// directly, which is a better way to do it anyway.

namespace {

/// Reads a seed from the command line.
///
/// Returns false on anything malformed. Parsed as an integer rather than
/// through a double: a seed is a uint32_t, and going via floating point means
/// 1.5 is silently truncated and 5000000000 is an out-of-range conversion,
/// which is undefined rather than merely wrong.
bool parseSeed(const char* text, std::uint32_t& seed) {
    char* end = nullptr;
    errno = 0;

    const unsigned long long parsed = std::strtoull(text, &end, 10);

    if (end == text || *end != '\0' || errno == ERANGE ||
        parsed > std::numeric_limits<std::uint32_t>::max()) {
        return false;
    }

    seed = static_cast<std::uint32_t>(parsed);
    return true;
}

}  // namespace

int main(int argc, char** argv) {
    std::optional<std::uint32_t> requestedSeed;

    if (argc == 3 && std::strcmp(argv[1], "--seed") == 0) {
        std::uint32_t seed = 0;

        if (!parseSeed(argv[2], seed)) {
            std::fprintf(stderr, "error: --seed must be a whole number from 0 to %u\n",
                         std::numeric_limits<std::uint32_t>::max());
            return 1;
        }

        requestedSeed = seed;
    } else if (argc != 1) {
        std::fprintf(stderr, "usage: evtol_sim [--seed N]\n");
        return 1;
    }

    evtol::Rng rng = requestedSeed.has_value() ? evtol::Rng(*requestedSeed)
                                               : evtol::Rng::fromEntropy();

    // Captured before the run, because the report prints it and the whole
    // point is that someone can feed it back in with --seed.
    const std::uint32_t seed = rng.seed();

    const evtol::SimulationConfig config;

    evtol::Simulation simulation(config, rng);
    simulation.run();

    const auto stats = evtol::collectStatistics(simulation.fleet(), config.duration);

    std::fputs(evtol::formatReport(stats, config, seed).c_str(), stdout);

    return 0;
}
