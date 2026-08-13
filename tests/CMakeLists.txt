add_executable(tests tests.cpp)

target_link_libraries(tests PRIVATE evtol_core)

# So TestHarness.h can be included by name.
target_include_directories(tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

add_test(NAME unit_tests COMMAND tests)
