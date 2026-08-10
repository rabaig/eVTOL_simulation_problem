#ifndef EVTOL_TYPES_H
#define EVTOL_TYPES_H

namespace evtol {

/// Identifies one vehicle. Just an index into the simulation's fleet.
using VehicleId = int;

/// Simulated time, in hours.
///
/// Hours throughout, everywhere, with no minutes or seconds anywhere in the
/// code. The problem states charge times in hours and speeds in miles per
/// hour, so anything else would mean converting at every boundary and getting
/// it wrong somewhere. Minutes appear only in printed output.
using Hours = double;

}  // namespace evtol

#endif  // EVTOL_TYPES_H
