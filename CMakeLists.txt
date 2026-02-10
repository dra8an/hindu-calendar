cmake_minimum_required(VERSION 3.20)
project(hindu-calendar C)
set(CMAKE_C_STANDARD 99)

# Swiss Ephemeris (vendored) — explicit list matching the Makefile
add_library(swisseph STATIC
    lib/swisseph/swecl.c
    lib/swisseph/swedate.c
    lib/swisseph/sweephe4.c
    lib/swisseph/swehel.c
    lib/swisseph/swehouse.c
    lib/swisseph/swejpl.c
    lib/swisseph/swemmoon.c
    lib/swisseph/swemplan.c
    lib/swisseph/sweph.c
    lib/swisseph/swephlib.c
)
target_include_directories(swisseph PUBLIC lib/swisseph)

# Application library (everything except main.c)
add_library(hindu_cal_lib STATIC
    src/astro.c
    src/date_utils.c
    src/tithi.c
    src/masa.c
    src/panchang.c
)
target_include_directories(hindu_cal_lib PUBLIC src lib/swisseph)
target_link_libraries(hindu_cal_lib swisseph m)

# Main executable
add_executable(hindu-calendar src/main.c)
target_link_libraries(hindu-calendar hindu_cal_lib)

# Tests
enable_testing()
foreach(test_name test_astro test_tithi test_masa test_validation test_csv_regression test_adhika_kshaya)
    add_executable(${test_name} tests/${test_name}.c)
    target_link_libraries(${test_name} hindu_cal_lib)
    add_test(NAME ${test_name} COMMAND ${test_name})
endforeach()
