# End-to-end test for for loops over ranges and arrays.
if(NOT DEFINED NPP_BIN OR NOT DEFINED NPP_SOURCE_DIR)
  message(FATAL_ERROR "NPP_BIN and NPP_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/for-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NPP_SOURCE_DIR}/tests/regression_programs/for_loops.npp")
set(EXE "${SCRATCH}/for_loops.exe")

execute_process(
  COMMAND "${NPP_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to compile for_loops.npp:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "for_loops.exe exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
set(expected "0\n1\n2\n10\n20\n30\n8\n0\n10\n20\n7\n8\n99")
if(NOT run_out STREQUAL expected)
  message(FATAL_ERROR "Unexpected for loop output:\n--- actual ---\n${run_out}\n--- expected ---\n${expected}")
endif()

message(STATUS "NPP for loop smoke test: PASS")
