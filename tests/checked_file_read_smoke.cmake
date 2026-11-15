# Test that checked reads distinguish a valid empty file from a failed open.
if(NOT DEFINED NPP_BIN OR NOT DEFINED NPP_SOURCE_DIR)
  message(FATAL_ERROR "NPP_BIN and NPP_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/checked-file-read-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

set(SOURCE "${NPP_SOURCE_DIR}/tests/regression_programs/checked_io.npp")
set(EXE "${SCRATCH}/checked-io")
execute_process(
  COMMAND "${NPP_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
  RESULT_VARIABLE build_rc
  OUTPUT_VARIABLE build_out
  ERROR_VARIABLE build_err
)
if(NOT build_rc EQUAL 0)
  message(FATAL_ERROR "Failed to build checked_io.npp:\n${build_out}\n${build_err}")
endif()

execute_process(
  COMMAND "${EXE}"
  WORKING_DIRECTORY "${SCRATCH}"
  RESULT_VARIABLE run_rc
  OUTPUT_VARIABLE run_out
  ERROR_VARIABLE run_err
)
if(NOT run_rc EQUAL 0)
  message(FATAL_ERROR "checked file-read smoke test exited ${run_rc}:\n${run_err}")
endif()

string(REPLACE "\r\n" "\n" run_out "${run_out}")
string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
if(NOT run_out STREQUAL "N++ checked file read: PASS")
  message(FATAL_ERROR "Unexpected checked file-read output: [${run_out}]\n${run_err}")
endif()
message(STATUS "N++ checked file-read smoke test: PASS")
