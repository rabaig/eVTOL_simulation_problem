#ifndef EVTOL_MERSENNE_RNG_H
#define EVTOL_MERSENNE_RNG_H

#include <cstdint>
#include <random>

#include "evtol/Rng.h"

namespace evtol {

/// The generator used for real runs.
///
/// mt19937 is more than this problem needs, but it's in the standard library,
/// it's seedable, and its sequence is reproducible across platforms. That last
/// part matters: a run someone else can't reproduce isn't much use as evidence.
class MersenneRng final : public Rng {
public:
    explicit MersenneRng(std::uint32_t seed);

    /// Picks a seed from the system entropy source. The seed is kept so it can
    /// be printed, which is what lets you re-run an interesting random result.
    static MersenneRng fromEntropy();

    double uniform01() override;
    int uniformInt(int lo, int hi) override;

    std::uint32_t seed() const { return seed_; }

private:
    std::uint32_t seed_;
    std::mt19937 engine_;

    // Held as a member rather than constructed per call. Distributions are
    // allowed to carry state between draws, and rebuilding one each time
    // would quietly discard it.
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
};

}  // namespace evtol

#endif  // EVTOL_MERSENNE_RNG_H
