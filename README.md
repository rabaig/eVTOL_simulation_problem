# eVTOL Fleet Simulation

C++ simulation of a fleet of electric air taxis sharing a small number of chargers.

## What this is

Picture an airfield with 20 air taxis parked on it and only 3 charging stations. The aircraft come from five different manufacturers, and the designs are all over the place: one flies for over an hour and a half, another barely manages 26 miles before it's empty.

Everything starts fully charged and takes off at once. After that each aircraft just loops:

fly until the battery dies, land, get in line for a charger, wait your turn, charge, take off again.

The whole problem is really about that queue. Twenty aircraft, three chargers, so at any given moment a chunk of the fleet is sitting on the ground doing nothing. We run this for 3 hours of simulated time and then report how each manufacturer's aircraft did.

## The aircraft

| | Alpha | Bravo | Charlie | Delta | Echo |
|---|---:|---:|---:|---:|---:|
| Cruise speed (mph) | 120 | 100 | 160 | 90 | 30 |
| Battery (kWh) | 320 | 100 | 220 | 120 | 150 |
| Charge time (hr) | 0.60 | 0.20 | 0.80 | 0.62 | 0.30 |
| Energy at cruise (kWh/mile) | 1.6 | 1.5 | 2.2 | 0.8 | 5.8 |
| Passengers | 4 | 5 | 3 | 2 | 2 |
| Fault probability per hour | 0.25 | 0.10 | 0.05 | 0.22 | 0.61 |

## Working out how long they fly

Only one calculation matters here, and everything else falls out of it. Multiply energy per mile by cruise speed and you get how fast the battery drains. Divide the battery by that and you get flight time.

Alpha, for example, burns 1.6 kWh every mile and covers 120 miles an hour, so it's pulling 192 kWh/hr. With a 320 kWh battery that's 1.67 hours in the air, or 200 miles.

Doing that for all five:

| | Alpha | Bravo | Charlie | Delta | Echo |
|---|---:|---:|---:|---:|---:|
| Drain (kWh/hr) | 192 | 150 | 352 | 72 | 174 |
| Flight time | 1h 40m | 40m | 37m | 1h 40m | 52m |
| Distance | 200 mi | 67 mi | 100 mi | 150 mi | 26 mi |
| Charge time | 36m | 12m | 48m | 37m | 18m |
| Passenger-miles per flight | 800 | 333 | 300 | 300 | 52 |

A few things jump out once you see them side by side.

Alpha does most of the heavy lifting. 200 miles with 4 people on board is 800 passenger-miles a flight, more than twice anything else in the fleet.

Bravo is the opposite. It only stays up 40 minutes, but it's back off the charger in 12, so it cycles constantly and hits the queue far more often than anyone else.

Charlie is the fastest aircraft at 160 mph and it doesn't help much, because it needs 48 minutes of charging for 37 minutes of flying. It spends more of the day plugged in than airborne.

Delta gets the same endurance as Alpha out of a battery a third the size. Slow, but nothing else in the fleet uses power that efficiently.

Echo is rough. 5.8 kWh/mile at 30 mph works out to 26 miles a flight, and it faults 61% of the time per hour on top of that.

One consequence worth flagging: Alpha and Delta both fly for 1h 40m, so inside a 3 hour window they only ever finish one flight. The second one doesn't complete before the clock stops. That's expected, not a bug in the numbers.

## What gets reported

Per manufacturer, not per aircraft:

- average flight time per flight
- average distance per flight
- average charging time per session
- total faults
- total passenger miles

Passenger miles is just miles flown times seats. Two aircraft with 4 seats each flying an hour at 100 mph gives 2 × 4 × 1 × 100 = 800.

## Assumptions

Some of these came with the problem, some I had to decide myself.

Given to us: everyone starts fully charged, aircraft hit cruise speed instantly with no climb or descent, they fly until the battery is flat and then queue immediately, a charger handles one aircraft at a time, and charging always takes the listed time no matter how empty the battery is. The fleet is 20 aircraft split randomly across the five types.

The rest I decided on:

**Faults use a Poisson process.** "Probability of fault per hour" reads like a coin flip once an hour, but almost none of these flights are a whole number of hours, so that doesn't work cleanly. I treat it as a continuous process with rate λ and sample the next fault time as `-ln(U)/λ`. Handles partial hours properly and drops straight into the event queue.

**Faults only happen in flight.** An aircraft sitting on a charger isn't flying, so it isn't accumulating risk. If faults ticked over during all wall clock time instead, every type would end up with roughly the same count regardless of how much it actually flew, which would make the statistic useless.

**A fault gets counted but the aircraft keeps going.** The problem only asks for a count. Grounding an aircraft would mean deciding what kinds of failure exist and how bad each one is, and none of that was specified. Left as a TODO.

**Flights still running at the 3 hour mark don't count toward the averages.** Some aircraft are mid-flight when the clock stops. If I counted those truncated flights, the average flight time would come out lower than what the aircraft can actually do, which is misleading. So they're excluded from the flight time and distance averages. The passenger miles they flew do still count, because those miles happened. Same rule for charging sessions in progress.

**Charger queue is FIFO.** First come first served. Nothing in the problem suggests otherwise and it's the easiest policy to defend.

**A type can have zero aircraft.** Random distribution across 5 types means one might not show up at all. Those get reported as N/A instead of dividing by zero.

## How it's built

This is a discrete event simulation rather than a fixed timestep one.

The reason is that nothing in this system happens at an unpredictable time. When an aircraft takes off you already know exactly when it'll run out of battery. When it starts charging you know exactly when it'll finish. So instead of ticking the clock forward in small steps and checking what changed, I keep a priority queue of upcoming events and jump straight to the next one. No timestep to tune, no rounding error, and it runs in milliseconds.

The pieces:

- `Aircraft` — the spec for one manufacturer's design, plus the derived numbers (endurance, range, drain rate). Immutable.
- `Vehicle` — one actual aircraft and its state: flying, queued, or charging.
- `ChargerPool` — the 3 chargers and the line waiting for them.
- `FaultModel` — interface, with `PoissonFaultModel` as the real one.
- `Rng` — interface over random numbers, seedable.
- `Simulation` — owns the clock and the event queue, runs the whole thing.
- `StatsCollector` — accumulates the five statistics per type.
- `Reporter` — prints the results table.

`FaultModel` and `Rng` are interfaces mainly so tests can swap in deterministic versions. Once the randomness is stubbed out, every test becomes an exact equality check instead of a statistical one, which is much nicer to work with.

## Testing

What I'm checking:

- the derived numbers (endurance, range, drain) against hand calculations from the spec table
- charger contention: 4 aircraft and 3 chargers means the fourth waits, and the queue stays in order
- passenger miles reproducing the worked example from the problem statement exactly
- fault timing being deterministic when the RNG is stubbed
- a fixed seed producing identical output every run
- edge cases, mainly zero aircraft of a type not blowing up the averages, and a fleet smaller than 3 never queueing at all

## Running it

```bash
git clone https://github.com/rabaig/eVTOL_simulation_problem.git
cd eVTOL_simulation_problem
cmake -S . -B build
cmake --build build

./build/bin/evtol_sim              # random seed
./build/bin/evtol_sim --seed 42    # reproducible
./build/bin/tests
```

Needs CMake 3.16 or newer and a compiler with C++17. No external dependencies, nothing to fetch, no submodules.

Layout:

```
include/evtol/   public headers
src/             implementation
tests/           unit tests and the harness they run in
docs/            problem statement and development plan
```


## Sample run

Coming once the simulation is actually implemented.

## TODO

- Faults don't do anything right now beyond incrementing a counter. Splitting them into minor and major, where a major fault forces a landing and pulls the aircraft out of service, would be more realistic.
- FIFO is fair but it's not the best use of the chargers. Letting Bravo (12 minute charge) cut ahead of Charlie (48 minutes) would almost certainly push total passenger miles up. Would be interesting to simulate both and compare.
- The aircraft specs are hardcoded. Reading them from a config file would let you try new designs without a rebuild.
- A single 3 hour run with a random fleet split is noisy. Running it a few hundred times and reporting mean and standard deviation would say a lot more than one run does.
- Charging is a fixed duration per the problem statement, but real chargers deliver a rate. Modelling kW throughput would let an aircraft top up partway when the line is short.
