# End-to-end tests for the builtin native backend (no external toolchain).
# Each program is compiled with --backend native, executed, and its stdout
# (and exit code) is compared against the expected text below.
if(NOT DEFINED NPP_BIN)
  message(FATAL_ERROR "NPP_BIN is required")
endif()
if(NOT DEFINED NPP_SOURCE_DIR)
  message(FATAL_ERROR "NPP_SOURCE_DIR is required")
endif()

set(PROGRAM_DIR "${NPP_SOURCE_DIR}/tests/native_programs")
set(SCRATCH "${CMAKE_BINARY_DIR}/native-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

macro(check_native name expected)
  set(SRC "${PROGRAM_DIR}/${name}.npp")
  if(WIN32)
    set(EXE "${SCRATCH}/${name}.exe")
  else()
    set(EXE "${SCRATCH}/${name}")
  endif()
  execute_process(
    COMMAND "${NPP_BIN}" build "${SRC}" -o "${EXE}" --backend native
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "native build failed for ${name}: ${out}\n${err}")
  endif()
  if(NOT EXISTS "${EXE}")
    message(FATAL_ERROR "native build produced no executable for ${name}")
  endif()
  execute_process(
    COMMAND "${EXE}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE got
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "native executable failed for ${name} (exit ${rc}): ${err}")
  endif()
  # Normalize trailing newlines for comparison.
  string(REGEX REPLACE "\r\n" "\n" got "${got}")
  string(REGEX REPLACE "\n+$" "" got "${got}")
  set(want "${expected}")
  string(REGEX REPLACE "\n+$" "" want "${want}")
  if(NOT got STREQUAL want)
    message(FATAL_ERROR "native output mismatch for ${name}:\n--- got ---\n${got}\n--- want ---\n${want}")
  endif()
  message(STATUS "  PASS ${name}.npp")
endmacro()

check_native(arith "14\n20\n42\n3\n2\n-3\n-2\n-5\n-42\ntrue\nfalse\ntrue\ntrue\nfalse\nfalse\nfalse\ntrue\nfalse\nfalse\ntrue")
check_native(calls "55\ntrue\nfalse\n21")
check_native(loops "64\n8\n5")
check_native(for_ranges "10\n3\n6\n0")
check_native(wide_frame "21\n210\n-9223372036854775807\n0")
check_native(print_types "literal line\ntrue\nfalse\ntrue\n42")
check_native(strings_full "\nabc\nHello, N++!\ntrue\nfalse\nfalse\ntrue\n-12345\n0\nanswer: 42")
check_native(string_edges "string.len: 5\nstring.byte_at: 72\nstring.byte_at OOB: -1\nstring.byte_at neg: -1")
check_native(short_circuit "false\ntrue\nrhs\ntrue\nrhs\ntrue\nguard\n3")
check_native(statement_lines "2")
check_native(bitwise "8\n15\n6\n32\n16\n-4\n20\n1\n-4\n1")
check_native(sys_io "true\nfalse\nfalse\ntrue")

# input.npp reads one line from stdin; pipe a fixed name in.
if(WIN32)
  set(INPUT_EXE "${SCRATCH}/input.exe")
else()
  set(INPUT_EXE "${SCRATCH}/input")
endif()
file(WRITE "${SCRATCH}/input.txt" "Alice\n")
execute_process(
  COMMAND "${NPP_BIN}" build "${PROGRAM_DIR}/input.npp" -o "${INPUT_EXE}" --backend native
  RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native build failed for input")
endif()
execute_process(
  COMMAND "${INPUT_EXE}"
  INPUT_FILE "${SCRATCH}/input.txt"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE got
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native input executable failed (exit ${rc})")
endif()
string(REGEX REPLACE "\r\n" "\n" got "${got}")
if(NOT got STREQUAL "Your name: Hello, Alice!\ntrue\n")
  message(FATAL_ERROR "native input output mismatch:\n--- got ---\n${got}")
endif()
message(STATUS "  PASS input.npp")

# exit_code.npp prints nothing and returns 45 from main.
if(WIN32)
  set(EXIT_EXE "${SCRATCH}/exit_code.exe")
else()
  set(EXIT_EXE "${SCRATCH}/exit_code")
endif()
execute_process(
  COMMAND "${NPP_BIN}" build "${PROGRAM_DIR}/exit_code.npp" -o "${EXIT_EXE}" --backend native
  RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native build failed for exit_code")
endif()
execute_process(COMMAND "${EXIT_EXE}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 45)
  message(FATAL_ERROR "native exit code mismatch: got ${rc}, want 45")
endif()
message(STATUS "  PASS exit_code.npp (exit 45)")

# exit_builtin.npp calls exit(7) mid-program; "after" must never print.
if(WIN32)
  set(EXITB_EXE "${SCRATCH}/exit_builtin.exe")
else()
  set(EXITB_EXE "${SCRATCH}/exit_builtin")
endif()
execute_process(
  COMMAND "${NPP_BIN}" build "${PROGRAM_DIR}/exit_builtin.npp" -o "${EXITB_EXE}" --backend native
  RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native build failed for exit_builtin")
endif()
execute_process(COMMAND "${EXITB_EXE}" RESULT_VARIABLE rc OUTPUT_VARIABLE got)
if(NOT rc EQUAL 7)
  message(FATAL_ERROR "native exit() builtin mismatch: got ${rc}, want 7")
endif()
string(REGEX REPLACE "\r\n" "\n" got "${got}")
string(REGEX REPLACE "\n+$" "" got "${got}")
if(NOT got STREQUAL "before")
  message(FATAL_ERROR "native exit() builtin printed unexpected output: ${got}")
endif()
message(STATUS "  PASS exit_builtin.npp (exit 7)")

check_native(f64 "0\n1.5\n-2.75\n3.14159\n100\n999.99\n12345.678\n-0.001\n0.0001\n0.0001\n123456789\n1e15\n1e16\n25000000000\n-1.5e20\n9.99e30\n1e18\n1e99\n1e100\n1e-05\n1e-12\n1e-15\n3.75\n2.5\n9\ntrue\ntrue\n3\n42\n-7\n2.5")

# input_i64.npp prompts, skips invalid lines on stderr, parses "  -17 ".
if(WIN32)
  set(IN64_EXE "${SCRATCH}/input_i64.exe")
else()
  set(IN64_EXE "${SCRATCH}/input_i64")
endif()
file(WRITE "${SCRATCH}/input_i64.txt" "abc\n  -17 \n")
execute_process(
  COMMAND "${NPP_BIN}" build "${PROGRAM_DIR}/input_i64.npp" -o "${IN64_EXE}" --backend native
  RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native build failed for input_i64")
endif()
execute_process(
  COMMAND "${IN64_EXE}"
  INPUT_FILE "${SCRATCH}/input_i64.txt"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE got
  ERROR_VARIABLE goterr
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native input_i64 executable failed (exit ${rc})")
endif()
string(REGEX REPLACE "\r\n" "\n" got "${got}")
string(REGEX REPLACE "\n+$" "" got "${got}")
if(NOT got STREQUAL "Number: Number: -17")
  message(FATAL_ERROR "native input_i64 output mismatch:\n--- got ---\n${got}")
endif()
if(NOT "${goterr}" MATCHES "Invalid integer")
  message(FATAL_ERROR "native input_i64 did not reject 'abc' on stderr: ${goterr}")
endif()
message(STATUS "  PASS input_i64.npp")

# input_f64.npp prompts and parses decimal/scientific floats from stdin.
if(WIN32)
  set(INF64_EXE "${SCRATCH}/input_f64.exe")
else()
  set(INF64_EXE "${SCRATCH}/input_f64")
endif()
file(WRITE "${SCRATCH}/input_f64.txt" "3.25\n-1e3\n")
execute_process(
  COMMAND "${NPP_BIN}" build "${PROGRAM_DIR}/input_f64.npp" -o "${INF64_EXE}" --backend native
  RESULT_VARIABLE rc
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native build failed for input_f64")
endif()
execute_process(
  COMMAND "${INF64_EXE}"
  INPUT_FILE "${SCRATCH}/input_f64.txt"
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE got
)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "native input_f64 executable failed (exit ${rc})")
endif()
string(REGEX REPLACE "\r\n" "\n" got "${got}")
string(REGEX REPLACE "\n+$" "" got "${got}")
if(NOT got STREQUAL "first: 3.25\n6.5\nsecond: -1000")
  message(FATAL_ERROR "native input_f64 output mismatch:\n--- got ---\n${got}")
endif()
message(STATUS "  PASS input_f64.npp")

# Programs outside the subset must fail with an actionable error.
execute_process(
  COMMAND "${NPP_BIN}" build "${NPP_SOURCE_DIR}/tests/regression_programs/types.npp"
    -o "${SCRATCH}/rejected" --backend native
  RESULT_VARIABLE rc
  OUTPUT_VARIABLE out
  ERROR_VARIABLE err
)
if(rc EQUAL 0)
  message(FATAL_ERROR "--backend native unexpectedly accepted types.npp")
endif()
if(NOT "${out}${err}" MATCHES "outside the builtin native v0 subset")
  message(FATAL_ERROR "--backend native rejection lacks an actionable reason: ${out}\n${err}")
endif()
message(STATUS "  PASS subset rejection is actionable")

message(STATUS "N++ builtin native backend smoke test: PASS")
