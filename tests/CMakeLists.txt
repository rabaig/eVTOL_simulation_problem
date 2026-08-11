# Sources are listed rather than globbed. A glob won't re-run when a file is
# added, so a new test file silently doesn't get built until someone
# reconfigures and wonders why their test never ran.
add_executable(tests
    test_main.cpp
    TestHarness.cpp
    test_aircraft.cpp
    test_charger_pool.cpp
    test_fault_model.cpp
    test_rng.cpp
    test_simulation.cpp
    test_statistics.cpp
    test_vehicle.cpp
)

target_link_libraries(tests PRIVATE evtol_core)

# So test helpers can be included by name: "TestHarness.h", "test_rng.h".
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME unit_tests COMMAND tests)
