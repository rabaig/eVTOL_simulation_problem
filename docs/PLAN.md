# Development Plan

How I'm breaking this problem up and working through it.

The whole thing could be written in one sitting as a single file, but that isn't how I'd build it at work and it wouldn't show much. So I've split it into stages. Each one is a ticket, each ticket gets its own branch, and each branch ends in a pull request with a description of what changed and why. The git history should read as a record of how the design came together.

## Working method

Trunk-based. `main` always builds; every ticket is a short-lived branch off it, merged back through a PR with `--no-ff` so each ticket stays a visible cluster rather than being flattened into a straight line. Squash-merging would give a tidier log and throw away the sequence of decisions inside each ticket, which is the part worth reading.

Commit subjects carry the ticket ID; commit bodies carry the reasoning, not a restatement of the diff. If a commit involved a judgement call, the body says what the alternatives were.

The rest of this file is the tickets. `git log --graph` shows how it actually went.

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

Added after a review pass over the finished code:

| Ticket | Branch | What |
|---|---|---|
| EVTOL-10 | `fix/EVTOL-10-correctness-defects` | Four defects, including UB in a Release build and a test assertion that could never fail |
| EVTOL-11 | `chore/EVTOL-11-trim-tests` | 59 tests down to 26, in one file |
| EVTOL-12 | `refactor/EVTOL-12-simplify-code` | `AircraftSpec` becomes a struct; the names that needed comments to explain them |
| EVTOL-13 | `docs/EVTOL-13-trim-documentation` | This file and the README, cut to what earns its place |

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

`AircraftSpec` holds the six given properties and derives drain rate, endurance, range and passenger-miles per flight. The five specs live in one table. This is where the maths gets pinned down, and it's the easiest ticket to test properly because every expected value can be worked out by hand.

Done when: endurance, range and drain rate are verified against hand calculations for all five types.

> **Changed later.** This shipped as a class with an eight parameter constructor, twelve getters and the derived values cached in members. EVTOL-12 made it a plain struct computing them on demand. The caching was justified in a comment claiming the simulation reads them constantly; it reads them about sixty times in a whole run.

---

### EVTOL-3 — Charger pool

Three chargers and the queue for them, built and tested in isolation with no aircraft involved.

`ChargerPool` tracks how many chargers are free and holds a FIFO queue of waiting vehicles. Two operations: request a charger (either granted immediately or you join the queue) and release one (the head of the queue takes it, if anyone is waiting).

Done when: requesting with a free charger is granted immediately, the fourth simultaneous request queues, releasing hands the charger to the longest waiter, and releasing with an empty queue just frees the charger.

---

### EVTOL-4 — Vehicle state machine

One aircraft and the states it moves through: in flight, waiting, charging.

`Vehicle` owns a pointer to its `AircraftSpec` plus its current state and the timestamps of when the current state began. It knows the legal transitions and rejects the illegal ones. It does not own a clock and does not schedule anything itself, it just responds to being told that something happened.

Done when: a vehicle starts in flight fully charged, the flight-to-queue-to-charging-to-flight cycle works, illegal transitions are rejected, and time spent in each state is tracked correctly.

---

### EVTOL-5 — Simulation engine

The event loop, and the point where the previous three tickets start behaving like a fleet.

A priority queue of events ordered by time, a clock that jumps from one event to the next rather than ticking in fixed steps, and the handlers for flight-complete and charge-complete. Fleet generation also lives here: 20 vehicles split randomly across the five types using the injected `Rng`. The loop runs until the clock passes 3 hours.

Done when: a seeded run is reproducible, the fleet always totals 20, no more than 3 vehicles are ever charging at once, and the clock never runs backwards. The charger constraint gets an assertion rather than just a test, since violating it silently would invalidate every statistic.

---

### EVTOL-6 — Fault model

Faults, as a continuous process rather than an hourly coin flip.

`FaultModel` samples the gap to the next fault as `-ln(1 - u)/λ`, with λ taken from the type's hourly probability. Faults are scheduled as events like anything else and only accrue while a vehicle is airborne. A fault increments a counter and the flight continues.

The `1 - u` is not cosmetic: `uniform01()` returns `[0, 1)`, so zero can come back and `ln(0)` is negative infinity.

Testing this without a stub takes some care. A fixed seed catches regressions but doesn't prove correctness; the argument for correctness is the long-run check, where the observed rate has to converge on λ over far more hours than the real run.

Done when: a seeded run reproduces its fault counts exactly, faults don't fire while charging or queued, and a long-run rate check lands near the expected value.

---

### EVTOL-7 — Statistics

`collectStatistics()` folds a finished fleet into the five required figures, grouped by manufacturer rather than by aircraft.

> **Changed during the ticket.** This was planned as a `StatsCollector` class subscribing to the events the simulation emits. It ended up as a plain function, because by this point vehicles already accumulate their own totals as transitions happen — so there is no state left for a collector to keep and nothing to subscribe to. An observer would have been machinery wrapped around a `for` loop.

Done when: passenger-miles reproduces the worked example from the problem statement exactly, truncated flights are excluded from averages but their miles are counted, and an absent type doesn't crash the report.

---

### EVTOL-8 — Reporting, sample run and final docs

Output, and everything needed to hand it over.

`Reporter` formats the per-type results as an aligned table. Command-line handling for `--seed` so a run can be reproduced. A committed sample run in the README, which the problem asks for explicitly.

Done when: output is readable and aligned, `--seed 42` gives identical output on repeated runs, and the README contains a real run.

---

### EVTOL-9 — Continuous integration

Every push and pull request builds on Linux and macOS, in Debug and Release, and runs the tests.

Both build types on purpose: `assert` compiles away in Release, so the two configurations are running genuinely different code and a Release-only failure would otherwise sit unnoticed. Two platforms because gcc and clang disagree about enough warnings to be worth catching.

Added later than it should have been. It belongs next to EVTOL-1 and would have caught a broken build before a reviewer did rather than after.

Done when: the badge is green on `main` and every PR shows its own result.

---

## Looking back

`v1.0` is tagged on `main`. Four things are worth recording, since the point of writing a plan down is being able to see afterwards where it was wrong.

**Two designs got simpler than planned.** `Rng` and `FaultModel` were both specified as interfaces with test doubles behind them — the textbook approach, and it does buy exact assertions. Both were dropped for concrete classes and fixed seeds. The trade is real and named in each section above: tests now largely catch change rather than proving correctness, and the gap is covered by long-run convergence checks and by a second generator on the same seed recovering the exact draw where the result is a pure function of one.

**CI arrived far too late.** EVTOL-9 should have been EVTOL-1.5. Every PR before it merged without anything checking it built on a second compiler.

**The tests that mattered most were the ones written to fail.** Every ticket was checked by deliberately breaking the code, and twice that found problems the passing tests could not have: a harness that discarded its output when a test aborted, and a tie-break test that passed with the tie-break deleted, because libstdc++ happens not to reorder three equal elements. Both would have shipped looking tested.

**Too much of everything.** A review pass over the finished code (EVTOL-10 to 13) cut 59 tests to 26 and around 250 lines from the source without losing any coverage — verified by re-running every mutation from the earlier tickets against the smaller suite. It also found four real defects, including undefined behaviour that only existed in Release builds and a test assertion that could never fail. The lesson isn't that the extra material was wrong; it's that volume was hiding the parts that mattered, and a review pass caught what writing more never would have.
