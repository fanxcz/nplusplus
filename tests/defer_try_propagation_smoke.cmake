# Ensure ? propagation runs all active defer calls before returning the error enum.
if(NOT DEFINED NPP_BIN OR NOT DEFINED NPP_SOURCE_DIR)
  message(FATAL_ERROR "NPP_BIN and NPP_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/defer-try-propagation-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NPP_SOURCE_DIR}/tests/regression_programs/defer_try_propagation.npp")
set(EXE "${SCRATCH}/defer_try_propagation")
execute_process(
  COMMAND "${NPP_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to compile defer_try_propagation.npp:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "defer_try_propagation exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
set(expected [=[inner
outer:second
outer:first
ok=42
inner
outer:second
outer:first
error=source failed]=])
if(NOT run_out STREQUAL expected)
  message(FATAL_ERROR "Unexpected defer/? output:\n--- actual ---\n${run_out}\n--- expected ---\n${expected}\n${run_err}")
endif()

message(STATUS "N++ defer + ? propagation smoke test: PASS")
