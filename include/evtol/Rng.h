#ifndef EVTOL_RNG_H
#define EVTOL_RNG_H

#include <cstdint>
#include <random>

namespace evtol {

/// Random number generator for the simulation.
///
/// Two things here are random: how the 20 aircraft are split across the five
/// types, and when faults happen. Both go through this class so there is one
/// seed for the whole run.
///
/// The seed is the point. Same seed in, same run out, so a result worth
/// showing someone can be reproduced instead of described.
class Rng {
public:
    explicit Rng(std::uint32_t seed);

    /// Picks a seed from the system. Use seed() afterwards to find out which
    /// one, so an interesting run can be repeated.
    static Rng fromEntropy();

    /// A number in [0, 1). Zero can come back, one cannot.
    double uniform01();

    /// A whole number in [lo, hi], both ends included.
    int uniformInt(int lo, int hi);

    std::uint32_t seed() const { return seed_; }

private:
    std::uint32_t seed_;
    std::mt19937 engine_;

    // Kept as a member instead of built per call. Distributions are allowed
    // to hold state between draws, and rebuilding one each time would throw
    // that away.
    std::uniform_real_distribution<double> unit_{0.0, 1.0};
};

}  // namespace evtol

#endif  // EVTOL_RNG_H
