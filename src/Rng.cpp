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
    // Built per call, as uniformInt does below. An earlier version held this
    // as a member with a comment about distributions carrying state between
    // draws — but uniformInt was constructing a fresh one every time anyway,
    // so the two contradicted each other and only one could be right.
    // Neither of these distributions is stateful in practice.
    std::uniform_real_distribution<double> unit(0.0, 1.0);

    return unit(engine_);
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
