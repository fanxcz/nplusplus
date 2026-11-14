if(NOT DEFINED NPP_BIN)
  message(FATAL_ERROR "NPP_BIN is required")
endif()

execute_process(
  COMMAND "${NPP_BIN}" doctor --json
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "npp doctor --json failed: ${err}")
endif()

string(JSON version ERROR_VARIABLE json_error GET "${out}" version)
if(json_error)
  message(FATAL_ERROR "doctor output is not valid JSON or has no version field: ${json_error}\n${out}")
endif()
if(NOT version MATCHES "^[0-9]+\\.[0-9]+\\.[0-9]+")
  message(FATAL_ERROR "doctor returned unexpected version: ${version}")
endif()

foreach(key IN ITEMS native_backend llvm_backend compiler_found runtime stdlib templates project)
  string(JSON field_type ERROR_VARIABLE json_error TYPE "${out}" ${key})
  if(json_error)
    message(FATAL_ERROR "doctor JSON is missing '${key}': ${json_error}\n${out}")
  endif()
  if(NOT field_type STREQUAL "BOOLEAN")
    message(FATAL_ERROR "doctor JSON field '${key}' must be a boolean, got ${field_type}")
  endif()
endforeach()

message(STATUS "NPP doctor JSON smoke test: PASS")
