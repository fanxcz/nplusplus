# End-to-end test for std.math gameplay helpers.
if(NOT DEFINED NPP_BIN OR NOT DEFINED NPP_SOURCE_DIR)
  message(FATAL_ERROR "NPP_BIN and NPP_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/math-helpers-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NPP_SOURCE_DIR}/tests/regression_programs/math_helpers.npp")
set(EXE "${SCRATCH}/math_helpers")

execute_process(
  COMMAND "${NPP_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to compile math_helpers.npp:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "math_helpers exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
if(NOT run_out STREQUAL "N++ math helpers: PASS")
  message(FATAL_ERROR "Unexpected math helper output: [${run_out}]")
endif()

message(STATUS "N++ math helper smoke test: PASS")
