#ifndef EVTOL_FAULT_MODEL_H
#define EVTOL_FAULT_MODEL_H

#include "evtol/Rng.h"
#include "evtol/Types.h"

namespace evtol {

/// Decides when the next fault happens.
///
/// The problem gives a "probability of fault per hour". Read literally that
/// is a coin flip once an hour, but almost no flight here is a whole number
/// of hours — Charlie's is 37 minutes — so that reading has nothing sensible
/// to say about the remainder.
///
/// Treating it as a Poisson process with rate lambda handles any duration,
/// puts faults at any instant rather than on hour boundaries, and drops
/// straight into an event queue. The gap between faults is exponential, and
/// sampling it by inverse transform gives
///
///     t = -ln(1 - u) / lambda,    u uniform in [0, 1)
///
/// Note the 1 - u rather than u. uniform01() can return exactly zero, and
/// ln(0) is negative infinity; 1 - u lands in (0, 1] instead, where the
/// logarithm is always finite.
class FaultModel {
public:
    explicit FaultModel(Rng& rng) : rng_(&rng) {}

    /// Hours from now until this aircraft's next fault.
    ///
    /// Memoryless, so "now" can be a take-off or the moment of the previous
    /// fault and the answer is drawn the same way either time.
    Hours timeToNextFault(double faultsPerHour);

private:
    Rng* rng_;
};

}  // namespace evtol

#endif  // EVTOL_FAULT_MODEL_H
