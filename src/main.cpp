#include <cstdio>

#include "evtol/Rng.h"

// Entry point. There is nothing to simulate yet: the engine arrives in
// EVTOL-5 and command line handling in EVTOL-8. This exists so the scaffold
// is demonstrably wired together and the library is linked by something
// other than the tests.

int main() {
    evtol::Rng rng = evtol::Rng::fromEntropy();

    std::printf("eVTOL simulation (scaffold)\n");
    std::printf("seed %u, first sample %.6f\n", rng.seed(), rng.uniform01());

    return 0;
}
