# `npp run <file>` must start the program in the directory the command was
# invoked from. Building a file inside a project writes the executable under the
# project's build/ directory, so launching it from the executable's directory
# would silently break relative asset and data paths.
if(NOT DEFINED NPP_BIN)
  message(FATAL_ERROR "NPP_BIN is required")
endif()

set(SMOKE_DIR "${CMAKE_BINARY_DIR}/run-cwd-smoke")
file(REMOVE_RECURSE "${SMOKE_DIR}")
file(MAKE_DIRECTORY "${SMOKE_DIR}/src")

file(WRITE "${SMOKE_DIR}/npp.toml" [==[
[package]
name = "run_cwd_fixture"
version = "1.0.0"

[build]
entry = "src/main.npp"
artifact = "run_cwd_fixture"
]==])

file(WRITE "${SMOKE_DIR}/src/main.npp" [==[
import std.fs

fn main() {
    print(fs_read("marker.txt"))
}
]==])

file(WRITE "${SMOKE_DIR}/marker.txt" "NPP_RUN_CWD_OK")

execute_process(
  COMMAND "${NPP_BIN}" run src/main.npp
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0 OR NOT out MATCHES "NPP_RUN_CWD_OK")
  message(FATAL_ERROR
    "npp run did not start the program in the invocation directory: rc=${rc}\n${out}\n${err}")
endif()

execute_process(
  COMMAND "${NPP_BIN}" build src/main.npp
  WORKING_DIRECTORY "${SMOKE_DIR}"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "npp build rejected a file inside a project: rc=${rc}\n${out}\n${err}")
endif()

file(REMOVE_RECURSE "${SMOKE_DIR}")
message(STATUS "N++ run working-directory smoke test: PASS")
