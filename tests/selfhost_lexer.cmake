# Self-hosting step 1: run the N++-written lexer's own test suite and the
# self-lexing smoke through the LLVM backend. Requires a clang-compatible
# NPP_CXX (the .ll path cannot be linked by plain g++).
if(NOT DEFINED NPP_BIN)
  message(FATAL_ERROR "NPP_BIN is required")
endif()
if(NOT DEFINED NPP_SOURCE_DIR)
  message(FATAL_ERROR "NPP_SOURCE_DIR is required")
endif()

set(SELFCASES
    "${NPP_SOURCE_DIR}/selfhost/lexer_test.npp"
    "${NPP_SOURCE_DIR}/selfhost/lex_self.npp"
    "${NPP_SOURCE_DIR}/selfhost/parser_test.npp"
    "${NPP_SOURCE_DIR}/selfhost/typecheck_test.npp"
    "${NPP_SOURCE_DIR}/selfhost/codegen_test.npp")

foreach(CASE ${SELFCASES})
  get_filename_component(NAME "${CASE}" NAME)
  execute_process(
    COMMAND "${NPP_BIN}" run "${CASE}" --backend llvm
    WORKING_DIRECTORY "${NPP_SOURCE_DIR}"
    RESULT_VARIABLE rc
    OUTPUT_VARIABLE out
    ERROR_VARIABLE err
    TIMEOUT 300
  )
  if(NOT rc EQUAL 0)
    message(FATAL_ERROR "selfhost lexer case ${NAME} failed (rc=${rc}):\n${out}\n${err}")
  endif()
  if(NOT out MATCHES "(all lexer tests passed|all parser tests passed|all typecheck tests passed|all codegen tests passed|self-lex ok)")
    message(FATAL_ERROR "selfhost lexer case ${NAME} produced unexpected output:\n${out}\n${err}")
  endif()
endforeach()
message(STATUS "selfhost lexer tests passed")
