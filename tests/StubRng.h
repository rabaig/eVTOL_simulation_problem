#ifndef EVTOL_STUB_RNG_H
#define EVTOL_STUB_RNG_H

#include <cassert>
#include <vector>

#include "evtol/Rng.h"

namespace evtol::test {

/// An Rng that hands back a sequence you wrote yourself.
///
/// This is the whole reason Rng is an interface. With one of these in place a
/// test can say "the first fault lands at exactly 0.5 hours" instead of
/// "faults happen at roughly the right rate over a large sample", and a single
/// 3-hour run is nowhere near a large enough sample for the second kind of
/// assertion to mean anything.
///
/// The sequences cycle once exhausted. That's deliberate: a fault test may
/// draw hundreds of times over a long run, and scripting hundreds of values to
/// test one behaviour would bury the point of the test.
class StubRng final : public Rng {
public:
    StubRng(std::vector<double> reals, std::vector<int> ints = {})
        : reals_(std::move(reals)), ints_(std::move(ints)) {}

    double uniform01() override {
        assert(!reals_.empty() && "StubRng asked for a real with none scripted");
        return reals_[realCalls_++ % reals_.size()];
    }

    int uniformInt(int lo, int hi) override {
        assert(!ints_.empty() && "StubRng asked for an int with none scripted");
        const int value = ints_[intCalls_++ % ints_.size()];

        // A scripted value outside the requested range means the test and the
        // code under test disagree about what's being asked for. Worth failing
        // on rather than clamping.
        assert(value >= lo && value <= hi && "scripted int is outside the range");
        (void)lo;
        (void)hi;

        return value;
    }

    std::size_t realCalls() const { return realCalls_; }
    std::size_t intCalls() const { return intCalls_; }

private:
    std::vector<double> reals_;
    std::vector<int> ints_;
    std::size_t realCalls_ = 0;
    std::size_t intCalls_ = 0;
};

}  // namespace evtol::test

#endif  // EVTOL_STUB_RNG_H
