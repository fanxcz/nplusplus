# Compile recently maintained interactive examples through LLVM codegen.
# They open real windows, so this smoke test compiles but does not run them.
if(NOT DEFINED NPP_BIN OR NOT DEFINED NPP_SOURCE_DIR)
  message(FATAL_ERROR "NPP_BIN and NPP_SOURCE_DIR are required")
endif()

set(SCRATCH "${CMAKE_BINARY_DIR}/llvm-example-smoke")
file(REMOVE_RECURSE "${SCRATCH}")
file(MAKE_DIRECTORY "${SCRATCH}")

foreach(EXAMPLE IN ITEMS "ui_playground.npp" "game2d_starter.npp")
  set(SOURCE "${NPP_SOURCE_DIR}/examples/${EXAMPLE}")
  get_filename_component(NAME "${EXAMPLE}" NAME_WE)
  set(EXE "${SCRATCH}/${NAME}")
  execute_process(
    COMMAND "${NPP_BIN}" build "${SOURCE}" -o "${EXE}" --backend llvm
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "LLVM compilation failed for ${EXAMPLE}:\n${out}\n${err}")
  endif()
  message(STATUS "PASS LLVM example ${EXAMPLE}")
endforeach()

message(STATUS "N++ interactive LLVM example smoke test: PASS")
