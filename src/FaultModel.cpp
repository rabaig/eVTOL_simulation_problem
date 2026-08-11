#include "evtol/FaultModel.h"

#include <cassert>
#include <cmath>
#include <limits>

namespace evtol {

Hours FaultModel::timeToNextFault(double faultsPerHour) {
    assert(faultsPerHour >= 0.0 && "a negative fault rate is meaningless");

    // None of the five types has a rate of zero, but a perfectly reliable
    // aircraft is a reasonable thing to want to simulate and dividing by zero
    // is not a reasonable way to handle it. Infinity is the honest answer:
    // the next fault never arrives.
    if (faultsPerHour <= 0.0) {
        return std::numeric_limits<Hours>::infinity();
    }

    const double u = rng_->uniform01();

    return -std::log(1.0 - u) / faultsPerHour;
}

}  // namespace evtol
