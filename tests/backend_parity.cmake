if(NOT DEFINED NPP_BIN OR NOT DEFINED NPP_SOURCE_DIR)
  message(FATAL_ERROR "NPP_BIN and NPP_SOURCE_DIR are required")
endif()

set(PROGRAM_DIR "${NPP_SOURCE_DIR}/tests/native_programs")
set(SCRATCH "${CMAKE_BINARY_DIR}/backend-parity")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

if(WIN32)
  set(EXE_SUFFIX ".exe")
else()
  set(EXE_SUFFIX "")
endif()

foreach(program arith calls loops for_ranges wide_frame print_types strings_full short_circuit bitwise)
  set(native_output "")
  set(llvm_output "")
  foreach(backend native llvm)
    set(src "${PROGRAM_DIR}/${program}.npp")
    set(exe "${SCRATCH}/${program}-${backend}${EXE_SUFFIX}")
    execute_process(
      COMMAND "${NPP_BIN}" build "${src}" -o "${exe}" --backend "${backend}"
      RESULT_VARIABLE build_rc
      OUTPUT_VARIABLE build_out
      ERROR_VARIABLE build_err
    )
    if(NOT build_rc EQUAL 0)
      message(FATAL_ERROR
        "Failed to compile ${program}.npp using ${backend}:\n${build_out}\n${build_err}")
    endif()

    execute_process(
      COMMAND "${exe}"
      RESULT_VARIABLE run_rc
      OUTPUT_VARIABLE run_out
      ERROR_VARIABLE run_err
    )
    if(NOT run_rc EQUAL 0)
      message(FATAL_ERROR
        "${program}.npp using ${backend} exited ${run_rc}:\n${run_err}")
    endif()
    string(REPLACE "\r\n" "\n" run_out "${run_out}")
    string(REGEX REPLACE "\n+$" "" run_out "${run_out}")
    if(backend STREQUAL "native")
      set(native_output "${run_out}")
    else()
      set(llvm_output "${run_out}")
    endif()
  endforeach()

  if(NOT native_output STREQUAL llvm_output)
    message(FATAL_ERROR
      "Backend output mismatch for ${program}.npp:\n--- native ---\n${native_output}\n--- LLVM ---\n${llvm_output}")
  endif()
  if(program STREQUAL "bitwise")
    set(expected_bitwise "8;15;6;32;16;-4;20;1;-4;1")
    string(REPLACE ";" "\n" expected_bitwise "${expected_bitwise}")
    if(NOT native_output STREQUAL expected_bitwise OR
       NOT llvm_output STREQUAL expected_bitwise)
      message(FATAL_ERROR
        "Unexpected bitwise/shift result. Expected:\n${expected_bitwise}\n--- native ---\n${native_output}\n--- LLVM ---\n${llvm_output}")
    endif()
  endif()
  message(STATUS "  PASS backend parity: ${program}.npp")
endforeach()

message(STATUS "NPP native/LLVM backend parity: PASS")
