#include "evtol/MersenneRng.h"

#include <cassert>

namespace evtol {

MersenneRng::MersenneRng(std::uint32_t seed)
    : seed_(seed), engine_(seed) {}

MersenneRng MersenneRng::fromEntropy() {
    std::random_device device;
    return MersenneRng(device());
}

double MersenneRng::uniform01() {
    return unit_(engine_);
}

int MersenneRng::uniformInt(int lo, int hi) {
    // An inverted range is a caller bug, not something to paper over by
    // swapping the bounds. uniform_int_distribution is undefined behaviour
    // here rather than an error, so catch it in debug builds.
    assert(lo <= hi && "uniformInt called with lo > hi");

    std::uniform_int_distribution<int> dist(lo, hi);
    return dist(engine_);
}

}  // namespace evtol
