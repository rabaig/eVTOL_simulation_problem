#include "evtol/Rng.h"

#include <cassert>

namespace evtol {

Rng::Rng(std::uint32_t seed)
    : seed_(seed), engine_(seed) {}

Rng Rng::fromEntropy() {
    std::random_device device;
    return Rng(device());
}

double Rng::uniform01() {
    return unit_(engine_);
}

int Rng::uniformInt(int lo, int hi) {
    // An inverted range is a caller bug, not something to quietly fix by
    // swapping the bounds. The standard library calls this undefined rather
    // than an error, so catch it here in debug builds.
    assert(lo <= hi && "uniformInt called with lo > hi");

    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(engine_);
}

}  // namespace evtol
