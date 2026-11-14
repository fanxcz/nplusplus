if(NOT DEFINED NPP_BIN)
  message(FATAL_ERROR "NPP_BIN is required")
endif()
if(NOT DEFINED NPP_SOURCE_DIR)
  message(FATAL_ERROR "NPP_SOURCE_DIR is required")
endif()

# Compile-check every current standalone example. The v080_* samples are
# explicitly legacy snapshots from an older compiler/runtime API; do not
# silently treat them as current examples.
file(GLOB EXAMPLE_SOURCES "${NPP_SOURCE_DIR}/examples/*.npp")
list(SORT EXAMPLE_SOURCES)
foreach(SOURCE_FILE IN LISTS EXAMPLE_SOURCES)
  get_filename_component(SOURCE_NAME "${SOURCE_FILE}" NAME)
  if(SOURCE_NAME MATCHES "^v080_")
    message(STATUS "SKIP legacy example ${SOURCE_NAME}")
    continue()
  endif()

  execute_process(
    COMMAND "${NPP_BIN}" check "${SOURCE_FILE}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
    message(FATAL_ERROR "NPP example failed semantic check: ${SOURCE_FILE}\n${out}\n${err}")
  endif()
  message(STATUS "PASS example ${SOURCE_NAME}")
endforeach()

# Check every checked-in source template, including editor.npp (which is an
# example but is not accepted by npp new --template).
file(GLOB TEMPLATE_SOURCES "${NPP_SOURCE_DIR}/examples/templates/*.npp")
list(SORT TEMPLATE_SOURCES)
foreach(SOURCE_FILE IN LISTS TEMPLATE_SOURCES)
  get_filename_component(SOURCE_NAME "${SOURCE_FILE}" NAME)
  execute_process(
    COMMAND "${NPP_BIN}" check "${SOURCE_FILE}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
    message(FATAL_ERROR "NPP template source failed semantic check: ${SOURCE_FILE}\n${out}\n${err}")
  endif()
  message(STATUS "PASS template source ${SOURCE_NAME}")
endforeach()

# Also validate project generation for every template the current CLI accepts.
set(TEMPLATE_ROOT "${CMAKE_BINARY_DIR}/template-check-smoke")
file(REMOVE_RECURSE "${TEMPLATE_ROOT}")
file(MAKE_DIRECTORY "${TEMPLATE_ROOT}")
foreach(TEMPLATE_NAME IN ITEMS cli app game 3d paint server library)
  set(PROJECT_DIR "${TEMPLATE_ROOT}/${TEMPLATE_NAME}-project")
  execute_process(
    COMMAND "${NPP_BIN}" new "${PROJECT_DIR}" --template "${TEMPLATE_NAME}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "npp new failed for template ${TEMPLATE_NAME}:\n${out}\n${err}")
  endif()
  execute_process(
    COMMAND "${NPP_BIN}" check
    WORKING_DIRECTORY "${PROJECT_DIR}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
  )
  if(NOT rc EQUAL 0 OR NOT out MATCHES "check: OK")
    message(FATAL_ERROR "Generated ${TEMPLATE_NAME} project failed semantic check:\n${out}\n${err}")
  endif()
  message(STATUS "PASS generated template ${TEMPLATE_NAME}")
endforeach()

message(STATUS "NPP example and template smoke tests: PASS")
