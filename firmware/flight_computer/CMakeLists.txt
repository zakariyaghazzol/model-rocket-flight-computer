cmake_minimum_required(VERSION 3.16)
project(rocket_flight_computer LANGUAGES CXX)

add_library(rocket_flight_core
  src/filter.cpp
  src/flight_computer.cpp
  src/logger.cpp
  src/state_machine.cpp
  src/telemetry.cpp
  src/types.cpp
)
target_include_directories(rocket_flight_core PUBLIC include)
target_compile_features(rocket_flight_core PUBLIC cxx_std_17)
if(MSVC)
  target_compile_options(rocket_flight_core PRIVATE /W4 /permissive-)
else()
  target_compile_options(rocket_flight_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

add_executable(synthetic_flight ../../simulations/synthetic_flight.cpp)
target_link_libraries(synthetic_flight PRIVATE rocket_flight_core)

option(ROCKET_BUILD_TESTS "Build host-side tests" ON)
if(ROCKET_BUILD_TESTS)
  enable_testing()
  add_executable(rocket_flight_tests ../../tests/flight_core_tests.cpp)
  target_link_libraries(rocket_flight_tests PRIVATE rocket_flight_core)
  add_test(NAME rocket_flight_tests COMMAND rocket_flight_tests)
  add_test(NAME synthetic_flight_smoke COMMAND synthetic_flight)
endif()
