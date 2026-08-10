# Development Plan

How I'm breaking this problem up and working through it.

The whole thing could be written in one sitting as a single file, but that isn't how I'd build it at work and it wouldn't show much. So I've split it into eight stages. Each one is a ticket, each ticket gets its own branch, and each branch ends in a pull request with a description of what changed and why. The git history should read as a record of how the design came together.

## Branching

Trunk-based. `main` is always in a working state, and every piece of work happens on a short-lived branch off it.

```
feature/EVTOL-<n>-<short-description>
```

so `feature/EVTOL-3-charger-pool`, `feature/EVTOL-6-fault-model`, and so on.

Branches are merged back with `--no-ff` even when a fast-forward is possible. That's deliberate: it keeps each ticket visible as a distinct group of commits in the history rather than flattening everything into a straight line. Squash-merging would give a tidier log but would throw away the sequence of decisions inside each ticket, which is the interesting part here.

The README and this plan are on `main` directly, since they're what the branches get planned from. Everything after that goes through a branch and a PR, even though I'm the only one reviewing.

## Commits

Imperative subject line, kept under 72 characters, with the ticket ID at the front:

```
EVTOL-3: Add FIFO queue to ChargerPool

A charger serves one aircraft at a time and the problem gives no
guidance on ordering, so arrivals are served first come first served.
Documented as an assumption in the README along with a note about
why a shortest-charge-first policy would likely score better.
```

The body is for the reasoning, not a restatement of the diff. If a commit involved a judgement call, the body is where I say what the alternatives were.

## The stages

| Ticket | Branch | Depends on |
|---|---|---|
| EVTOL-1 | `feature/EVTOL-1-project-scaffold` | — |
| EVTOL-2 | `feature/EVTOL-2-aircraft-spec` | 1 |
| EVTOL-3 | `feature/EVTOL-3-charger-pool` | 1 |
| EVTOL-4 | `feature/EVTOL-4-vehicle-state-machine` | 2, 3 |
| EVTOL-5 | `feature/EVTOL-5-simulation-engine` | 4 |
| EVTOL-6 | `feature/EVTOL-6-fault-model` | 5 |
| EVTOL-7 | `feature/EVTOL-7-statistics` | 5 |
| EVTOL-8 | `feature/EVTOL-8-reporting-and-docs` | 6, 7 |
| EVTOL-9 | `chore/EVTOL-9-continuous-integration` | 1 |

---

### EVTOL-1 — Project scaffold and test harness

Get a project that builds and runs tests before writing any simulation logic, so every ticket after this one has somewhere to put its tests on day one.

CMake with two targets, the simulation binary and the test binary. Directory layout is `src/`, `include/`, `tests/`, `docs/`. Tests use a small hand-rolled assertion harness rather than pulling in GoogleTest, to keep the repo free of submodules and dependency setup. The problem says a framework isn't required.

This ticket also introduces `Rng`, a seedable wrapper over `std::mt19937`. It belongs here rather than with the fault model because two separate things need randomness (fleet composition and faults) and both should share one seed. Seeding is what makes a run reproducible, which is what makes the sample output in the README worth committing.

Done when: `cmake --build build` produces both binaries, `./build/bin/tests` runs and passes, and the same seed demonstrably produces the same sequence.

> **Changed during the ticket.** `Rng` started as an interface with a `StubRng` test double behind it, so that fault tests could assert exact values against a scripted sequence. That's the textbook approach and it works, but it costs a class, a virtual call and a concept that has to be explained, and this project is small enough that fixed seeds cover the same ground. Dropped it in the second commit on the branch. The trade is real and worth naming: tests now assert on recorded output rather than hand-computed values, so they catch change rather than proving correctness.
>
> The same reasoning removed the planned `FaultModel` interface in EVTOL-6.

---

### EVTOL-2 — Aircraft specification and derived performance

The five aircraft types and the numbers that fall out of them.

`Aircraft` holds the six given properties and exposes the derived ones: drain rate in kWh/hr, endurance in hours, range in miles, passenger-miles per flight. Immutable once constructed. The five company specs live in one place as named constants.

This is the ticket where the maths gets pinned down, and it's the easiest one to test properly because the expected values can be worked out by hand from the specification table.

Done when: endurance, range and drain rate are verified against hand calculations for all five types, and the derived values are computed once rather than recalculated on every call.

---

### EVTOL-3 — Charger pool

Three chargers and the queue for them, built and tested in isolation with no aircraft involved.

`ChargerPool` tracks how many chargers are free and holds a FIFO queue of waiting vehicles. Two operations: request a charger (either granted immediately or you join the queue) and release one (the head of the queue takes it, if anyone is waiting).

Keeping this independent of `Vehicle` is what makes it testable on its own. The tests can push integers through it instead of constructing aircraft.

Done when: requesting with a free charger is granted immediately, the fourth simultaneous request queues, releasing hands the charger to the longest waiter, and releasing with an empty queue just frees the charger.

---

### EVTOL-4 — Vehicle state machine

One aircraft and the states it moves through: in flight, waiting, charging.

`Vehicle` owns a pointer to its `Aircraft` spec plus its current state and the timestamps of when the current state began. It knows the legal transitions and rejects the illegal ones. It does not own a clock and does not schedule anything itself, it just responds to being told that something happened.

The reason for keeping it passive is that the simulation engine in the next ticket owns time. If `Vehicle` also tried to reason about time you'd have the clock in two places.

Done when: a vehicle starts in flight fully charged, the flight-to-queue-to-charging-to-flight cycle works, illegal transitions are rejected, and time spent in each state is tracked correctly.

---

### EVTOL-5 — Simulation engine

The event loop, and the point where the previous three tickets start behaving like a fleet.

A priority queue of events ordered by time, a clock that jumps from one event to the next rather than ticking in fixed steps, and the handlers for flight-complete and charge-complete. Fleet generation also lives here: 20 vehicles split randomly across the five types using the injected `Rng`. The loop runs until the clock passes 3 hours.

Discrete-event rather than fixed-timestep because every state change in this system happens at a time you can calculate in advance. There's no timestep to tune and no rounding error, and the whole run finishes in milliseconds.

Done when: a seeded run is reproducible, the fleet always totals 20, no more than 3 vehicles are ever charging at once, and the clock never runs backwards. The charger constraint gets an assertion rather than just a test, since violating it silently would invalidate every statistic.

---

### EVTOL-6 — Fault model

Faults, as a continuous process rather than an hourly coin flip.

`FaultModel` samples the next fault time as `-ln(U)/λ`, with λ taken from the type's hourly probability. Faults are scheduled as events like anything else and only accrue while a vehicle is airborne. A fault increments a counter and the flight continues.

`uniform01()` returns values in `[0, 1)`, so zero can come back and the logarithm has to be fed `1 - u` rather than `u` directly. Small detail, but it's an infinity if it's missed.

Testing this without a stub takes a bit more care. Fault counts come from a fixed seed and are asserted against recorded output, which catches regressions but doesn't prove correctness on its own. The correctness argument is the long-run check: over enough simulated hours the observed rate has to converge on λ, and that test is doing the real work.

Done when: a seeded run reproduces its fault counts exactly, faults don't fire while charging or queued, and a long-run rate check lands near the expected value.

---

### EVTOL-7 — Statistics

Collecting the five required numbers, per type rather than per aircraft.

`StatsCollector` accumulates completed flights, completed charge sessions, faults and passenger-miles for each type. It listens to the same events the simulation already emits rather than having the simulation call it explicitly, so adding a statistic later doesn't mean editing the event handlers.

Two things need care here. Flights and charge sessions still in progress when the clock stops are excluded from the averages, because counting a truncated flight would drag the average below what the aircraft can actually do. The miles flown during those partial flights still count, since they happened. And a type with zero vehicles has to report as not-applicable rather than dividing by zero, which the random fleet split makes a real possibility.

Done when: passenger-miles reproduces the worked example from the problem statement exactly, truncated flights are excluded from averages but their miles are counted, and an absent type doesn't crash the report.

---

### EVTOL-8 — Reporting, sample run and final docs

Output, and everything needed to hand it over.

`Reporter` formats the per-type results as an aligned table. Command-line handling for `--seed` so a run can be reproduced. A committed sample run in the README, which the problem asks for explicitly.

The last part of this ticket is a pass back over the README to make sure the assumptions section matches what the code actually does, and that the TODOs are the ones still outstanding rather than ones already handled along the way.

Done when: output is readable and aligned, `--seed 42` gives identical output on repeated runs, and the README contains a real run.

---

### EVTOL-9 — Continuous integration

Every push and pull request builds on Linux and macOS, in Debug and Release, and runs the tests.

Both build types on purpose: `assert` compiles away in Release, so the two configurations are running genuinely different code and a Release-only failure would otherwise sit unnoticed. Two platforms because gcc and clang disagree about enough warnings to be worth catching.

Added later than it should have been. It belongs next to EVTOL-1 and would have caught a broken build before a reviewer did rather than after.

Done when: the badge is green on `main` and every PR shows its own result.

---

## Finishing up

Tag `v1.0` on `main` once EVTOL-8 merges.

The history at that point should show eight merge commits on `main`, each one a ticket, with the work inside each visible underneath. Anyone reading it should be able to follow the order the design was built in without reading the code first.
