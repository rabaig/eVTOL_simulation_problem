#include <cstdio>

#include "evtol/MersenneRng.h"

// Entry point. There is nothing to simulate yet: the engine arrives in
// EVTOL-5 and argument handling in EVTOL-8. For now this exists so the
// scaffold is demonstrably wired together, and so the library is linked by
// something other than the tests.

int main() {
    evtol::MersenneRng rng = evtol::MersenneRng::fromEntropy();

    std::printf("eVTOL simulation (scaffold)\n");
    std::printf("seed %u, first sample %.6f\n", rng.seed(), rng.uniform01());

    return 0;
}
