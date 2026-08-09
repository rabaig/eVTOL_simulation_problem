#ifndef EVTOL_RNG_H
#define EVTOL_RNG_H

namespace evtol {

/// Source of randomness for the simulation.
///
/// This is an interface rather than a concrete generator because two separate
/// things in this problem are random: how the 20 aircraft get split across the
/// five types, and when faults occur. Neither can be tested for an exact value
/// unless the randomness can be pinned down from outside the class using it.
///
/// Tests substitute a stub that hands back a scripted sequence. See
/// tests/StubRng.h.
class Rng {
public:
    virtual ~Rng() = default;

    /// Uniform sample in [0, 1).
    ///
    /// Half-open, so zero is possible and one is not. The fault model feeds
    /// this into a logarithm, so it has to work around the zero rather than
    /// assume it never shows up.
    virtual double uniform01() = 0;

    /// Uniform integer in [lo, hi]. Both ends included.
    virtual int uniformInt(int lo, int hi) = 0;
};

}  // namespace evtol

#endif  // EVTOL_RNG_H
