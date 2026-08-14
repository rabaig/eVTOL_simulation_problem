#include "evtol/Rng.h"

#include <cassert>

namespace evtol {
namespace {

// mt19937 produces values across the whole 32-bit range.
constexpr std::uint64_t kEngineRange = 0x1'0000'0000ull;

}  // namespace

Rng::Rng(std::uint32_t seed)
    : seed_(seed), engine_(seed) {}

Rng Rng::fromEntropy() {
    std::random_device device;
    return Rng(device());
}

double Rng::uniform01() {
    // Deliberately not std::uniform_real_distribution.
    //
    // The standard fixes the engine but leaves the distribution algorithms
    // unspecified ([rand.dist.general]), so the same seed produces different
    // numbers on libstdc++, libc++ and MSVC. This project claims a run is a
    // pure function of its seed, and that claim is only worth anything if it
    // survives crossing a compiler.
    //
    // This is the standard 53-bit construction: 27 bits from one draw and 26
    // from the next, giving a uniform multiple of 2^-53 in [0, 1).
    // Casts are explicit because 53 bits is exactly what a double holds, and
    // it is worth being visible that the arithmetic is at the limit rather
    // than comfortably inside it.
    const double high = static_cast<double>(engine_() >> 5);  // 27 bits
    const double low = static_cast<double>(engine_() >> 6);   // 26 bits

    return (high * 67108864.0 + low) / 9007199254740992.0;  // 2^26, 2^53
}

int Rng::uniformInt(int lo, int hi) {
    // An inverted range is a caller bug, not something to quietly fix by
    // swapping the bounds.
    assert(lo <= hi && "uniformInt called with lo > hi");

    const std::uint64_t range = static_cast<std::uint64_t>(hi) -
                                static_cast<std::uint64_t>(lo) + 1;

    if (range == 1) {
        return lo;
    }

    // Rejection sampling, for the same portability reason as above. Taking
    // the remainder alone would bias the low end of the range whenever the
    // range doesn't divide 2^32; discarding the short final block removes it.
    const std::uint64_t limit = kEngineRange - (kEngineRange % range);

    std::uint64_t draw = 0;
    do {
        draw = engine_();
    } while (draw >= limit);

    return static_cast<int>(static_cast<std::uint64_t>(lo) + (draw % range));
}

}  // namespace evtol
