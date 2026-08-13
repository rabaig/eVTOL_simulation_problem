# Sources are listed rather than globbed. file(GLOB CONFIGURE_DEPENDS) would
# pick up new files correctly on any CMake this project supports, so that is
# not the reason - an explicit list just makes it obvious what the test binary
# is built from, and a file that isn't meant to be compiled can't drift in.
add_executable(tests
    test_main.cpp
    TestHarness.cpp
    test_aircraft.cpp
    test_charger_pool.cpp
    test_fault_model.cpp
    test_reporter.cpp
    test_rng.cpp
    test_simulation.cpp
    test_statistics.cpp
    test_vehicle.cpp
)

target_link_libraries(tests PRIVATE evtol_core)

# So test helpers can be included by name: "TestHarness.h", "test_rng.h".
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME unit_tests COMMAND tests)
