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
///
/// Calibration is a judgement call worth naming. The table's figure is used
/// directly as lambda, so 0.61 for Echo means 0.61 faults per airborne hour
/// on average - which puts the chance of at least one fault in a given hour
/// at 1 - e^-0.61 = 0.457, not 0.61. Reading the table as that probability
/// instead would mean lambda = -ln(1 - p) = 0.94, and about a third more
/// Echo faults.
///
/// Rate was chosen because 'per hour' alongside a continuous process reads
/// as a rate, and because a probability cannot exceed 1 while these figures
/// are clearly meant to scale. It is the kind of thing worth one question
/// to the customer rather than one paragraph of guessing.
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
