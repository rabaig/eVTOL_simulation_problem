# eVTOL Fleet Simulation

[![build and test](https://github.com/rabaig/eVTOL_simulation_problem/actions/workflows/ci.yml/badge.svg)](https://github.com/rabaig/eVTOL_simulation_problem/actions/workflows/ci.yml)

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

**Faults use a Poisson process.** "Probability of fault per hour" reads like a coin flip once an hour, but almost none of these flights are a whole number of hours — Charlie's is 37 minutes — so that doesn't work cleanly. I treat it as a continuous process with rate λ and sample the next gap as `-ln(1 - u)/λ`, with `u` uniform in `[0, 1)`. Handles partial hours properly and drops straight into the event queue.

The `1 - u` rather than `u` matters: `u` can come back as exactly zero, and `ln(0)` is negative infinity.

**Faults only happen in flight.** An aircraft sitting on a charger isn't flying, so it isn't accumulating risk. If faults ticked over during all wall clock time instead, every type would end up with roughly the same count regardless of how much it actually flew, which would make the statistic useless.

**A fault gets counted but the aircraft keeps going.** The problem only asks for a count. Grounding an aircraft would mean deciding what kinds of failure exist and how bad each one is, and none of that was specified. Left as a TODO.

**Flights still running at the 3 hour mark don't count toward the averages.** Some aircraft are mid-flight when the clock stops. If I counted those truncated flights, the average flight time would come out lower than what the aircraft can actually do, which is misleading. So they're excluded from the flight time and distance averages. The passenger miles they flew do still count, because those miles happened. Same rule for charging sessions in progress.

**Charger queue is FIFO.** First come first served. Nothing in the problem suggests otherwise and it's the easiest policy to defend.

**A type can have zero aircraft.** The fleet is built by rolling a type per aircraft, which makes the counts random and the total exactly 20 by construction. It also means a type might not show up at all. Those report a dash rather than dividing by zero — and a dash rather than a `0.00`, which would read as a measurement instead of an absence.

## How it's built

This is a discrete event simulation rather than a fixed timestep one.

The reason is that nothing in this system happens at an unpredictable time. When an aircraft takes off you already know exactly when it'll run out of battery. When it starts charging you know exactly when it'll finish. So instead of ticking the clock forward in small steps and checking what changed, I keep a priority queue of upcoming events and jump straight to the next one. No timestep to tune, no rounding error, and it runs in milliseconds.

The pieces:

- `AircraftSpec` — one manufacturer's design, plus the numbers derived from it (endurance, range, drain rate). A specification, not an aircraft in the air.
- `Vehicle` — one actual aircraft and its state: flying, queued, or charging.
- `ChargerPool` — the 3 chargers and the line waiting for them.
- `FaultModel` — draws the gap to the next fault.
- `Rng` — seedable wrapper over `std::mt19937`.
- `Simulation` — owns the clock and the event queue, runs the whole thing.
- `Statistics` — folds a finished fleet into the five figures per type.
- `Reporter` — formats the results table.

Only `Simulation` owns a clock. Everything else is told what time it is. Two classes reasoning about time independently is how they end up disagreeing, and every duration in the output depends on them not doing that.

The dependencies only point one way. `AircraftSpec` has never heard of `Vehicle`; `ChargerPool` has never heard of either, and tracks vehicles as bare integers. That's what let each piece be finished and tested before the one above it existed.

## Testing

26 tests, no framework. The problem asked for "just a few examples", so these are the ones worth defending: each covers something that could plausibly be wrong and that no other test would catch. Tests that only exercised the standard library, or restated a fact from the specification table, were left out.

What they cover:

- derived numbers (endurance, range, drain rate) against hand calculations from the specification table
- charger contention: a fourth aircraft waits, the queue stays in order, and capacity holds across 200 handovers
- the vehicle transition table, all nine combinations, including the six that must be rejected
- every hour of every vehicle adding up to exactly the length of the run — no gaps, nothing counted twice
- passenger miles reproducing the problem's worked example exactly
- fault intervals against `-ln(1 - u)/λ` computed by hand, using a second generator on the same seed to recover the `u`
- faults converging on λ multiplied by hours *actually flown*, over a 3000-hour run
- byte-for-byte identical output from the same seed
- edge cases: zero aircraft of a type, a fleet smaller than the charger count, a zero fault rate

Several of those are exact rather than approximate, for the reason described under the sample run below — which makes them unusually strong assertions for a simulation.

Each component was also checked by deliberately breaking it and confirming the tests noticed: reversing the charger queue to LIFO, letting partial flights into the averages, swapping `1 - u` for `u` in the fault draw, grouping every vehicle under one type. Two of those runs found real problems that the passing tests had missed — a harness that discarded its output on abort, and a tie-break test that passed with the tie-break deleted.

## Building and running

### What you need

CMake 3.16 or newer, and a C++17 compiler. That's the whole list — no external libraries, nothing downloaded during the build, no submodules.

<details>
<summary>Installing those, if you don't have them</summary>

**macOS** — the compiler comes with Apple's command line tools:

```bash
xcode-select --install
brew install cmake
```

**Ubuntu / Debian:**

```bash
sudo apt update && sudo apt install -y build-essential cmake
```

**Windows** — install Visual Studio 2019 or newer with the "Desktop development with C++" workload, which includes CMake. Run the commands below from a *Developer Command Prompt*.

Check both are present:

```bash
cmake --version     # 3.16 or higher
c++ --version
```

</details>

### Build

```bash
git clone https://github.com/rabaig/eVTOL_simulation_problem.git
cd eVTOL_simulation_problem

cmake -S . -B build      # configure: reads CMakeLists.txt, writes build/
cmake --build build      # compile
```

Takes a few seconds. It produces two programs in `build/bin/`: `evtol_sim` and `tests`.

If you'd rather use an IDE, this is a standard CMake project — CLion, Visual Studio and Qt Creator all open `CMakeLists.txt` directly and handle the two commands above themselves.

### Run

```bash
./build/bin/evtol_sim
```

That runs the simulation the problem describes: 20 vehicles, 3 chargers, 3 hours. It prints the results table shown further down, and it picks a random seed each time, so the numbers change from run to run.

To get the same numbers every time, give it a seed:

```bash
./build/bin/evtol_sim --seed 42
```

That reproduces the sample run in this README exactly. Every run prints the seed it used — including the random ones — so any result you find interesting can be repeated.

`--seed` is the only option. Fleet size, charger count and duration are fixed at what the problem specifies; the tests build a `SimulationConfig` directly when they need something different.

### Run the tests

```bash
./build/bin/tests
```

26 tests, about ten milliseconds. Every test prints its name, a failure prints the file and line it came from, and the program exits non-zero if anything failed. `ctest --test-dir build` works too, if you prefer to drive it that way.

### Debug builds

The default build is optimised, with `assert` compiled out. To get the assertions — worth doing if you change anything, since several invariants are enforced that way rather than by tests:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

Both configurations are built and tested on Linux and macOS by CI on every push.

### If the build fails

**`cmake: command not found`** — see the install section above.

**`No CMAKE_CXX_COMPILER could be found`** — CMake is installed but a compiler isn't. On macOS run `xcode-select --install`.

**A cache error mentioning a different directory** — the `build/` folder was configured somewhere else, usually because the project was moved or copied. Delete it and start again; nothing in there is precious:

```bash
rm -rf build
cmake -S . -B build
```

### Where things are

```
include/evtol/   public headers
src/             implementation
tests/           the tests and the harness they run in
docs/            problem statement and development plan
```

Start with `docs/PLAN.md` for how the project was built, or `src/Simulation.cpp` for the event loop everything else hangs off.


## Sample run

`./build/bin/evtol_sim --seed 42`

```
eVTOL fleet simulation
  20 vehicles, 3 chargers, 3.00 hours
  seed 42 (re-run with --seed 42)

Company     Vehicles Flights  Avg flight/h  Avg dist/mi  Charges  Avg charge/h   Queued/h  Faults  Passenger-miles
------------------------------------------------------------------------------------------------------------------
Alpha              6       6        1.6667       200.00        1        0.6000       2.31       3           5028.0
Bravo              2       4        0.6667        66.67        2        0.2000       1.52       1           1333.3
Charlie            4       8        0.6250       100.00        4        0.8000       0.80       2           2400.0
Delta              6       6        1.6667       150.00        2        0.6200       1.67       1           1909.8
Echo               2       4        0.8621        25.86        2        0.3000       1.53       3            206.9
------------------------------------------------------------------------------------------------------------------
Total             20      28             -            -       11             -       7.83      10          10878.0

A dash means there was nothing to report: no vehicles of that type, or
none that finished a flight or a charge before the run ended.

Queued/h is time spent waiting for a charger. It isn't one of the figures the
problem asks for, but with twenty vehicles and three chargers it is the most
direct measure of what the shortage costs.

Flights and charges still in progress at 3.00 hours are excluded from the
averages, but the miles already flown on them are included in passenger-miles.
```

Run that command yourself and you'll get this table back. That's the whole reason the seed is printed.

Three things to read out of it. The averages are exact rather than approximate — Alpha's 1.6667 hours and 200.00 miles are precisely its endurance and range, because every completed flight of a type is identical. Only 11 charges finished across 28 flights, the rest of the fleet still queued or charging when the clock stopped. And `Queued/h` totals 7.83 hours of waiting inside a three-hour run, which is the clearest measure of what three chargers for twenty aircraft actually costs.

The total row carries no averages on purpose: one across types would weigh Alpha's 1h40m flights against Charlie's 37 minutes and mean nothing.

## TODO

- **One run is noisy.** The sample happened to draw six Alphas; a different seed tells a noticeably different story. A few hundred trials reporting mean and standard deviation would say far more. The code is already set up for it — the run is a pure function of the seed.
- **FIFO is fair but wasteful.** Letting Bravo (12 minute charge) cut ahead of Charlie (48 minutes) would almost certainly raise total passenger miles. Worth simulating both and comparing.
- **The specification table is still eight positional values per row**, so transposing two doubles compiles cleanly and only one test would catch it. C++20 designated initializers (`.cruiseSpeedMph = 120.0`) would make it a compile error — possible now that `AircraftSpec` is an aggregate.
