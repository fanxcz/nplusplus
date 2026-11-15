cmake_minimum_required(VERSION 3.20)

# ============================================================================
# Compiler selection - ONLY CHANGE THESE IF NEEDED
# ============================================================================
# The compiler must be selected before project(). Forcing it afterwards leaves
# the platform rules of the default compiler in place, which on Windows mixes
# MinGW link flags with an MSVC-targeting clang and breaks the link step.
# Override via command line: -DNPP_C_COMPILER=clang -DNPP_CXX_COMPILER=clang++
# Or set in CMakePresets.json / CMakeUserPresets.json
option(NPP_USE_CLANG "Use Clang compiler (auto-detected on non-Windows)" ON)

# Allow manual override of compiler paths
set(NPP_C_COMPILER   "" CACHE STRING "Path to C compiler (clang/gcc/clang-cl)")
set(NPP_CXX_COMPILER "" CACHE STRING "Path to C++ compiler (clang++/g++/clang-cl)")
set(NPP_LINKER       "" CACHE STRING "Linker to use (lld/mold/gold/default)")

if (NPP_C_COMPILER)
  set(CMAKE_C_COMPILER ${NPP_C_COMPILER} CACHE STRING "C compiler" FORCE)
endif()
if (NPP_CXX_COMPILER)
  set(CMAKE_CXX_COMPILER ${NPP_CXX_COMPILER} CACHE STRING "C++ compiler" FORCE)
endif()

# Auto-detect Clang on Windows independently for C and C++, while
# preserving every compiler path explicitly supplied by the user.
if (WIN32 AND NPP_USE_CLANG)
  set(LLVM_PATHS
    "C:/Program Files/LLVM"
    "C:/Program Files (x86)/LLVM"
    "C:/tools/llvm"
    "$ENV{USERPROFILE}/scoop/apps/llvm/current"
    "$ENV{LOCALAPPDATA}/Programs/LLVM"
  )
  foreach(path ${LLVM_PATHS})
    if (NOT NPP_C_COMPILER AND NOT CMAKE_C_COMPILER AND EXISTS "${path}/bin/clang.exe")
      set(CMAKE_C_COMPILER "${path}/bin/clang.exe" CACHE STRING "C compiler" FORCE)
      set(NPP_C_COMPILER "${path}/bin/clang.exe")
      message(STATUS "Found Clang C compiler at: ${path}")
    endif()
    if (NOT NPP_CXX_COMPILER AND NOT CMAKE_CXX_COMPILER AND EXISTS "${path}/bin/clang++.exe")
      set(CMAKE_CXX_COMPILER "${path}/bin/clang++.exe" CACHE STRING "C++ compiler" FORCE)
      set(NPP_CXX_COMPILER "${path}/bin/clang++.exe")
      message(STATUS "Found Clang C++ compiler at: ${path}")
    endif()
endforeach()
endif()

project(npp VERSION 1.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Select a fast linker through the compiler driver instead of overriding
# CMAKE_LINKER. Clang and GCC both understand -fuse-ld=<flavor>, which keeps
# CMake's platform link rules intact.
set(NPP_LINK_FLAG "")
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
  if (NPP_LINKER)
    set(NPP_LINK_FLAG "-fuse-ld=${NPP_LINKER}")
    message(STATUS "Using linker flavor: ${NPP_LINKER}")
  else()
    find_program(MOLD mold)
    find_program(LLD lld lld-link)
    if (MOLD)
      set(NPP_LINK_FLAG "-fuse-ld=mold")
      message(STATUS "Using mold linker")
    elseif (LLD)
      set(NPP_LINK_FLAG "-fuse-ld=lld")
      message(STATUS "Using lld linker")
    endif()
  endif()
endif()

option(NPP_BUILD_TESTS "Build N++ tests" ON)
option(NPP_BUILD_LIBFUZZER "Build opt-in Clang/libFuzzer frontend targets" OFF)

# ============================================================================
# Find dependencies
# ============================================================================
# SDL2 - try vcpkg first, then pkg-config, then system paths
find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
  pkg_check_modules(SDL2 QUIET sdl2)
  pkg_check_modules(SDL2_IMAGE QUIET SDL2_image)
  pkg_check_modules(SDL2_MIXER QUIET SDL2_mixer)
  pkg_check_modules(SDL2_TTF QUIET SDL2_ttf)
endif()

# If not found via pkg-config, try find_package (works with vcpkg)
if (NOT SDL2_FOUND)
  find_package(SDL2 QUIET)
  find_package(SDL2_image QUIET)
  find_package(SDL2_mixer QUIET)
  find_package(SDL2_ttf QUIET)
endif()

if (NOT SDL2_FOUND)
  message(WARNING "SDL2 not found! Graphics runtime will not work.")
  message(WARNING "Install SDL2 development libraries:")
  message(WARNING "  Ubuntu/Debian: sudo apt-get install libsdl2-dev libsdl2-image-dev libsdl2-mixer-dev libsdl2-ttf-dev")
  message(WARNING "  Arch: sudo pacman -S sdl2 sdl2_image sdl2_mixer sdl2_ttf")
  message(WARNING "  Fedora: sudo dnf install SDL2-devel SDL2_image-devel SDL2_mixer-devel SDL2_ttf-devel")
  message(WARNING "  Windows (vcpkg): vcpkg install sdl2 sdl2-image sdl2-mixer sdl2-ttf --triplet x64-windows")
  message(WARNING "  Windows (choco): choco install sdl2")
  message(WARNING "  macOS: brew install sdl2 sdl2_image sdl2_mixer sdl2_ttf")
endif()

# ============================================================================
# Compiler flags
# ============================================================================
if (MSVC OR CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(NPP_COMPILE_OPTIONS /W4 /permissive- /utf-8)
else()
  set(NPP_COMPILE_OPTIONS
    -Wall -Wextra -Wpedantic -Wconversion
    -Wno-misleading-indentation -Wno-shadow
    -Wno-unused-parameter
  )
endif()

# ============================================================================
# Sources
# ============================================================================
set(NPP_DRIVER_SOURCES
    src/compiler/driver/driver.cpp
    src/compiler/driver/cli/usage.cpp
    src/compiler/driver/commands/doctor.cpp
    src/compiler/driver/commands/export.cpp
    src/compiler/driver/commands/format.cpp
    src/compiler/driver/commands/info.cpp
    src/compiler/driver/commands/quality.cpp
    src/compiler/driver/lsp/server.cpp
    src/compiler/driver/lsp/protocol.cpp
    src/compiler/driver/lsp/text.cpp
    src/compiler/driver/lsp/uri.cpp
    src/compiler/driver/lsp/diagnostics.cpp
    src/compiler/driver/package/command.cpp
    src/compiler/driver/package/lockfile.cpp
    src/compiler/driver/package/metadata.cpp
    src/compiler/driver/package/vendor.cpp
    src/compiler/driver/project/config.cpp
    src/compiler/driver/project/modules.cpp
    src/compiler/driver/project/templates.cpp
    src/compiler/driver/repl/command.cpp
    src/compiler/driver/repl/input.cpp
    src/compiler/driver/repl/repl_classify.cpp
    src/compiler/driver/repl/repl_history.cpp
    src/compiler/driver/support/diagnostics.cpp
    src/compiler/driver/support/manifest.cpp
    src/compiler/driver/support/process.cpp
    src/compiler/driver/support/text.cpp
    src/compiler/driver/support/toml.cpp
    src/compiler/driver/toolchain/compile.cpp
    src/compiler/driver/toolchain/exec.cpp
    src/compiler/driver/toolchain/paths.cpp
    src/compiler/driver/toolchain/runtime.cpp
)

set(NPP_CODEGEN_SOURCES
    src/compiler/backend/codegen/llvm_ir.cpp
    src/compiler/backend/codegen/llvm_cg_core.cpp
    src/compiler/backend/codegen/llvm_cg_expr.cpp
    src/compiler/backend/codegen/llvm_cg_stmt.cpp
    src/compiler/backend/codegen/llvm_cg_program.cpp
)

set(NPP_NATIVE_SOURCES
    src/compiler/backend/native/native.cpp
    src/compiler/backend/native/native_subset.cpp
    src/compiler/backend/native/native_gen_core.cpp
    src/compiler/backend/native/native_gen_stmt.cpp
    src/compiler/backend/native/native_gen_runtime.cpp
    src/compiler/backend/native/target.cpp
    src/compiler/backend/native/pe_elf.cpp
)

set(NPP_LEXER_SOURCES
    src/compiler/frontend/lexer/lexer.cpp
    src/compiler/frontend/lexer/lexer_token.cpp
    src/compiler/frontend/lexer/lexer_literal.cpp
    src/compiler/frontend/lexer/lexer_scan.cpp
)

set(NPP_PARSER_SOURCES
    src/compiler/frontend/parser/parser.cpp
    src/compiler/frontend/parser/parser_support.cpp
    src/compiler/frontend/parser/parser_stmt.cpp
    src/compiler/frontend/parser/parser_expr.cpp
)

set(NPP_SEMANTIC_SOURCES
    src/compiler/semantic/semantic.cpp
    src/compiler/semantic/semantic_expr.cpp
    src/compiler/semantic/semantic_stmt.cpp
)

set(NPP_SOURCES
    src/cli/main.cpp
    src/compiler/format/formatter.cpp
    ${NPP_LEXER_SOURCES}
    ${NPP_PARSER_SOURCES}
    ${NPP_SEMANTIC_SOURCES}
    ${NPP_CODEGEN_SOURCES}
    ${NPP_NATIVE_SOURCES}
    ${NPP_DRIVER_SOURCES}
)

if (NPP_BUILD_LIBFUZZER)
  if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR "NPP_BUILD_LIBFUZZER requires a Clang compiler with libFuzzer support")
  endif()

  add_executable(npp_fuzz_lexer
      tests/fuzz/lexer_fuzz.cpp
      ${NPP_LEXER_SOURCES}
  )
  add_executable(npp_fuzz_parser
      tests/fuzz/parser_fuzz.cpp
      ${NPP_LEXER_SOURCES}
      ${NPP_PARSER_SOURCES}
  )

  foreach(target IN ITEMS npp_fuzz_lexer npp_fuzz_parser)
    target_include_directories(${target} PRIVATE
        src/compiler
        src/compiler/frontend
    )
    target_compile_features(${target} PRIVATE cxx_std_20)
    target_compile_options(${target} PRIVATE
        -fsanitize=fuzzer,address,undefined
        -fno-omit-frame-pointer
        -g
        ${NPP_COMPILE_OPTIONS}
    )
    target_link_options(${target} PRIVATE
        -fsanitize=fuzzer,address,undefined
    )
  endforeach()
endif()

add_executable(npp ${NPP_SOURCES})
# The official Windows portable package must run without an installed
# Visual C++ redistributable. Statically link the MSVC CRT into the compiler.
if (WIN32)
  # Applies to MSVC-ABI compilers (including compatible Clang builds).
  # Harmless for toolchains that use a different runtime model.
  set_property(TARGET npp PROPERTY
      MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()
target_include_directories(npp PRIVATE
    src/compiler
    src/compiler/frontend
    src/compiler/backend)
target_compile_features(npp PRIVATE cxx_std_20)
target_compile_definitions(npp PRIVATE NPP_SOURCE_ROOT="${CMAKE_SOURCE_DIR}" NPP_VERSION="${PROJECT_VERSION}")
target_compile_options(npp PRIVATE ${NPP_COMPILE_OPTIONS})
if (NPP_LINK_FLAG)
  target_link_options(npp PRIVATE ${NPP_LINK_FLAG})
endif()

# Link SDL2 if found
if (SDL2_FOUND)
  target_link_libraries(npp PRIVATE ${SDL2_LIBRARIES})
  target_include_directories(npp PRIVATE ${SDL2_INCLUDE_DIRS})
  if (SDL2_IMAGE_FOUND)
    target_link_libraries(npp PRIVATE ${SDL2_IMAGE_LIBRARIES})
  endif()
  if (SDL2_MIXER_FOUND)
    target_link_libraries(npp PRIVATE ${SDL2_MIXER_LIBRARIES})
  endif()
  if (SDL2_TTF_FOUND)
    target_link_libraries(npp PRIVATE ${SDL2_TTF_LIBRARIES})
  endif()
endif()

# ============================================================================
# Install
# ============================================================================
include(GNUInstallDirs)
target_compile_definitions(npp PRIVATE NPP_INSTALL_RUNTIME_DIR="${CMAKE_INSTALL_FULL_DATADIR}/npp/runtime")
install(TARGETS npp RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
# The runtime is one translation unit assembled from topic files, so the
# whole directory (runtime.cpp/runtime.c plus every *.inc part) must ship.
install(DIRECTORY src/runtime/
        DESTINATION ${CMAKE_INSTALL_DATADIR}/npp/runtime
        FILES_MATCHING PATTERN "*.cpp" PATTERN "*.c" PATTERN "*.inc" PATTERN "*.hpp")
install(DIRECTORY stdlib/ DESTINATION ${CMAKE_INSTALL_DATADIR}/npp/stdlib OPTIONAL)
install(DIRECTORY examples/templates/ DESTINATION ${CMAKE_INSTALL_DATADIR}/npp/project-templates OPTIONAL)
install(DIRECTORY examples/assets/ DESTINATION ${CMAKE_INSTALL_DATADIR}/npp/assets OPTIONAL)

# ============================================================================
# Tests
# ============================================================================
if (NPP_BUILD_TESTS)
  enable_testing()
  add_executable(npp_tests
      tests/test_main.cpp
      src/compiler/format/formatter.cpp
      ${NPP_LEXER_SOURCES}
      ${NPP_PARSER_SOURCES}
      ${NPP_SEMANTIC_SOURCES}
      ${NPP_CODEGEN_SOURCES}
      ${NPP_NATIVE_SOURCES}
      ${NPP_DRIVER_SOURCES}
  )
  target_include_directories(npp_tests PRIVATE
      src/compiler
      src/compiler/frontend
      src/compiler/backend)
  target_compile_features(npp_tests PRIVATE cxx_std_20)
  target_compile_definitions(npp_tests PRIVATE
      NPP_SOURCE_ROOT="${CMAKE_SOURCE_DIR}"
      NPP_INSTALL_RUNTIME_DIR="${CMAKE_INSTALL_FULL_DATADIR}/npp/runtime"
      NPP_VERSION="${PROJECT_VERSION}"
  )
  target_compile_options(npp_tests PRIVATE ${NPP_COMPILE_OPTIONS})
  if (SDL2_FOUND)
    target_link_libraries(npp_tests PRIVATE ${SDL2_LIBRARIES})
  endif()

  find_package(Threads REQUIRED)
  add_executable(npp_runtime_memory_test
      tests/runtime_memory_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(npp_runtime_memory_test PRIVATE cxx_std_20)
  target_compile_definitions(npp_runtime_memory_test PRIVATE NPP_RUNTIME_TESTING)
  target_link_libraries(npp_runtime_memory_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME npp_runtime_memory_test COMMAND npp_runtime_memory_test)
  
  add_executable(npp_runtime_geometry_test
      tests/runtime_geometry_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(npp_runtime_geometry_test PRIVATE cxx_std_20)
  target_link_libraries(npp_runtime_geometry_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME npp_runtime_geometry_test COMMAND npp_runtime_geometry_test)

  add_executable(npp_runtime_numeric_test
      tests/runtime_numeric_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(npp_runtime_numeric_test PRIVATE cxx_std_20)
  target_link_libraries(npp_runtime_numeric_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME npp_runtime_numeric_test COMMAND npp_runtime_numeric_test)

  add_executable(npp_runtime_args_test
      tests/runtime_args_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(npp_runtime_args_test PRIVATE cxx_std_20)
  target_link_libraries(npp_runtime_args_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME npp_runtime_args_test
      COMMAND npp_runtime_args_test alpha "beta gamma" omega)

  add_executable(npp_runtime_json_test
      tests/runtime_json_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(npp_runtime_json_test PRIVATE cxx_std_20)
  target_link_libraries(npp_runtime_json_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME npp_runtime_json_test COMMAND npp_runtime_json_test)

  add_executable(npp_runtime_http_test
      tests/runtime_http_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(npp_runtime_http_test PRIVATE cxx_std_20)
  target_link_libraries(npp_runtime_http_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME npp_runtime_http_test COMMAND npp_runtime_http_test)

  add_executable(npp_runtime_scene_test
      tests/runtime_scene_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(npp_runtime_scene_test PRIVATE cxx_std_20)
  target_link_libraries(npp_runtime_scene_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME npp_runtime_scene_test COMMAND npp_runtime_scene_test)
  set_tests_properties(npp_runtime_scene_test PROPERTIES TIMEOUT 30)

  add_executable(npp_runtime_files_test
      tests/runtime_files_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(npp_runtime_files_test PRIVATE cxx_std_20)
  target_link_libraries(npp_runtime_files_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME npp_runtime_files_test COMMAND npp_runtime_files_test)
  set_tests_properties(npp_runtime_files_test PROPERTIES TIMEOUT 30)

  add_executable(npp_runtime_backend_state_test
      tests/runtime_backend_state_test.cpp
      src/runtime/runtime.cpp
  )
  target_compile_features(npp_runtime_backend_state_test PRIVATE cxx_std_20)
  target_link_libraries(npp_runtime_backend_state_test PRIVATE ${CMAKE_DL_LIBS} Threads::Threads)
  add_test(NAME npp_runtime_backend_state_test COMMAND npp_runtime_backend_state_test)
  set_tests_properties(npp_runtime_backend_state_test PROPERTIES TIMEOUT 30)

  add_executable(npp_frontend_fuzz_smoke
      tests/frontend_fuzz_smoke.cpp
      ${NPP_LEXER_SOURCES}
      ${NPP_PARSER_SOURCES}
  )
  target_include_directories(npp_frontend_fuzz_smoke PRIVATE
      src/compiler
      src/compiler/frontend)
  target_compile_features(npp_frontend_fuzz_smoke PRIVATE cxx_std_20)
  target_compile_options(npp_frontend_fuzz_smoke PRIVATE ${NPP_COMPILE_OPTIONS})
  add_test(NAME npp_frontend_fuzz_smoke COMMAND npp_frontend_fuzz_smoke)
  set_tests_properties(npp_frontend_fuzz_smoke PROPERTIES TIMEOUT 30)

  add_test(NAME npp_tests COMMAND npp_tests)
  add_test(NAME npp_version_smoke COMMAND $<TARGET_FILE:npp> --version)
  set_tests_properties(npp_version_smoke PROPERTIES PASS_REGULAR_EXPRESSION "N\\+\\+ ${PROJECT_VERSION}")
  add_test(NAME npp_repl_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DREPL_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/repl_smoke.cmake)
  add_test(NAME npp_formatter_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -P ${CMAKE_SOURCE_DIR}/tests/formatter_smoke.cmake)
  add_test(NAME npp_self_test COMMAND $<TARGET_FILE:npp> self-test)
  set_tests_properties(npp_self_test PROPERTIES PASS_REGULAR_EXPRESSION "self-test: PASS")
  if(WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    set(NPP_NATIVE_DIVISION_TEST ON)
  else()
    set(NPP_NATIVE_DIVISION_TEST OFF)
  endif()
  add_test(NAME npp_division_zero_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
          -DTEST_NATIVE_BACKEND=${NPP_NATIVE_DIVISION_TEST}
          -P ${CMAKE_SOURCE_DIR}/tests/division_zero_smoke.cmake)
  add_test(NAME npp_pointer_lifetime_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/pointer_lifetime_smoke.cmake)
  add_test(NAME npp_managed_allocation_lifetime_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/managed_allocation_lifetime_smoke.cmake)
  add_test(NAME npp_doctor_json_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -P ${CMAKE_SOURCE_DIR}/tests/doctor_json_smoke.cmake)
  add_test(NAME npp_cli_arguments_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/cli_arguments_smoke.cmake)

  add_test(NAME npp_vscode_extension_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/vscode_extension_smoke.cmake)
  add_test(NAME npp_backend_explain_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/backend_explain_smoke.cmake)
  add_test(NAME npp_project_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/project_smoke.cmake)
  add_test(NAME npp_project_paths_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -P ${CMAKE_SOURCE_DIR}/tests/project_paths_smoke.cmake)
  add_test(NAME npp_corrupt_inputs_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -P ${CMAKE_SOURCE_DIR}/tests/corrupt_inputs_smoke.cmake)
  add_test(NAME npp_templates_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/templates_smoke.cmake)
  add_test(NAME npp_manifest_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -P ${CMAKE_SOURCE_DIR}/tests/manifest_smoke.cmake)
  add_test(NAME npp_lint_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -P ${CMAKE_SOURCE_DIR}/tests/lint_smoke.cmake)
  add_test(NAME npp_examples_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/examples_smoke.cmake)
  add_test(NAME npp_paint_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -P ${CMAKE_SOURCE_DIR}/tests/paint_smoke.cmake)
  add_test(NAME npp_diagnostics_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -P ${CMAKE_SOURCE_DIR}/tests/diagnostics_smoke.cmake)
  add_test(NAME npp_lsp_smoke
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -P ${CMAKE_SOURCE_DIR}/tests/lsp_smoke.cmake)
  find_package(Python3 COMPONENTS Interpreter QUIET)
  if (Python3_Interpreter_FOUND)
    add_test(NAME npp_lsp_watched_files_smoke
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/lsp_watched_files_smoke.py
            --npp $<TARGET_FILE:npp>)
    set_tests_properties(npp_lsp_watched_files_smoke PROPERTIES TIMEOUT 45)
    add_test(NAME npp_repository_sources
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/tests/repository_sources_smoke.py
            --npp $<TARGET_FILE:npp>
            --root ${CMAKE_SOURCE_DIR})
    set_tests_properties(npp_repository_sources PROPERTIES TIMEOUT 300)
    add_test(NAME npp_check_versions
        COMMAND ${Python3_EXECUTABLE}
            ${CMAKE_SOURCE_DIR}/scripts/check_versions.py
            --root ${CMAKE_SOURCE_DIR})
    if(UNIX)
      add_test(NAME npp_repl_tty_smoke
          COMMAND ${Python3_EXECUTABLE}
              ${CMAKE_SOURCE_DIR}/tests/repl_tty_smoke.py
              --npp $<TARGET_FILE:npp>)
      set_tests_properties(npp_repl_tty_smoke PROPERTIES TIMEOUT 20)
    endif()
  endif()
  add_test(NAME npp_regression_check
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/regression_check.cmake)
  add_test(NAME npp_game2d_platformer_check
      COMMAND $<TARGET_FILE:npp> check ${CMAKE_SOURCE_DIR}/examples/game2d_platformer.npp)
  set_tests_properties(npp_game2d_platformer_check PROPERTIES
      PASS_REGULAR_EXPRESSION "check: OK")
  add_test(NAME npp_game2d_starter_check
      COMMAND $<TARGET_FILE:npp> check ${CMAKE_SOURCE_DIR}/examples/game2d_starter.npp)
  set_tests_properties(npp_game2d_starter_check PROPERTIES
      PASS_REGULAR_EXPRESSION "check: OK")
  add_test(NAME npp_ui_playground_check
      COMMAND $<TARGET_FILE:npp> check ${CMAKE_SOURCE_DIR}/examples/ui_playground.npp)
  set_tests_properties(npp_ui_playground_check PROPERTIES
      PASS_REGULAR_EXPRESSION "check: OK")
  # The built-in executable image emitter is only runnable on x86-64
  # Linux and Windows. Keep its end-to-end executable tests off macOS.
  if (WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    add_test(NAME npp_native_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/native_smoke.cmake)
  endif()

  # Self-hosting: the N++-written lexer's test suite and self-lexing smoke.
  # The .ll pipeline needs a clang-compatible driver, so plain g++ is skipped.
  find_program(NPP_SELFHOST_CXX NAMES clang++ clang++-18 clang++-17)
  if (NPP_SELFHOST_CXX)
    # The Paint project uses the LLVM backend, which needs a clang-compatible
    # driver. Reuse the driver found for the self-hosting tests.
    set_tests_properties(npp_paint_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}")
    # The fixture reads a relative file, so the native backend must link the
    # runtime through the same clang-compatible driver as the self-host tests.
    add_test(NAME npp_run_working_directory_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -P ${CMAKE_SOURCE_DIR}/tests/run_working_directory_smoke.cmake)
    set_tests_properties(npp_run_working_directory_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}")
    add_test(NAME npp_selfhost_lexer
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/selfhost_lexer.cmake)
    set_tests_properties(npp_selfhost_lexer PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}")
    add_test(NAME npp_match_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/match_smoke.cmake)
    set_tests_properties(npp_match_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}")

    add_test(NAME npp_compound_assign_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/compound_assign_smoke.cmake)
    set_tests_properties(npp_compound_assign_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_for_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/for_smoke.cmake)
    set_tests_properties(npp_for_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}")
    add_test(NAME npp_utf8_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/utf8_smoke.cmake)
    set_tests_properties(npp_utf8_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}")
    add_test(NAME npp_math_helpers_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/math_helpers_smoke.cmake)
    set_tests_properties(npp_math_helpers_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}")
    add_test(NAME npp_math_trig_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/math_trig_smoke.cmake)
    set_tests_properties(npp_math_trig_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}")
    add_test(NAME npp_unicode_escape_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/unicode_escape_smoke.cmake)
    set_tests_properties(npp_unicode_escape_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_tilemap_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/tilemap_smoke.cmake)
    set_tests_properties(npp_tilemap_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_game_libraries_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/game_libraries_smoke.cmake)
    set_tests_properties(npp_game_libraries_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_game_primitives_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/game_primitives_smoke.cmake)
    set_tests_properties(npp_game_primitives_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_enum_payloads_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/enum_payloads_smoke.cmake)
    set_tests_properties(npp_enum_payloads_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_fixed_width_types_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DNPP_NATIVE_BACKEND_SUPPORTED=${NPP_NATIVE_DIVISION_TEST}
            -P ${CMAKE_SOURCE_DIR}/tests/fixed_width_types_smoke.cmake)
    set_tests_properties(npp_fixed_width_types_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 60)
    add_test(NAME npp_slice_llvm_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
            -DNPP_NATIVE_BACKEND_SUPPORTED=${NPP_NATIVE_DIVISION_TEST}
            -P ${CMAKE_SOURCE_DIR}/tests/slice_llvm_smoke.cmake)
    set_tests_properties(npp_slice_llvm_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 60)
    add_test(NAME npp_mutable_slice_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
            -DNPP_NATIVE_BACKEND_SUPPORTED=${NPP_NATIVE_DIVISION_TEST}
            -P ${CMAKE_SOURCE_DIR}/tests/mutable_slice_smoke.cmake)
    set_tests_properties(npp_mutable_slice_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 60)
    add_test(NAME npp_stdlib_error_helpers_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/stdlib_error_helpers_smoke.cmake)
    set_tests_properties(npp_stdlib_error_helpers_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_ui_library_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/ui_library_smoke.cmake)
    set_tests_properties(npp_ui_library_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_app_game_llvm_examples_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/llvm_examples_smoke.cmake)
    set_tests_properties(npp_app_game_llvm_examples_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 120)
    add_test(NAME npp_string_iteration_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/string_iteration_smoke.cmake)
    set_tests_properties(npp_string_iteration_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)

    add_test(NAME npp_defer_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/defer_smoke.cmake)
    set_tests_properties(npp_defer_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_defer_try_propagation_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/defer_try_propagation_smoke.cmake)
    set_tests_properties(npp_defer_try_propagation_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_defer_mem_free_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/defer_mem_free_smoke.cmake)
    set_tests_properties(npp_defer_mem_free_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)

    add_test(NAME npp_checked_file_read_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/checked_file_read_smoke.cmake)
    set_tests_properties(npp_checked_file_read_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)

    add_test(NAME npp_collections_smoke
        COMMAND ${CMAKE_COMMAND}
            -DNPP_BIN=$<TARGET_FILE:npp>
            -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
            -P ${CMAKE_SOURCE_DIR}/tests/collections_smoke.cmake)
    set_tests_properties(npp_collections_smoke PROPERTIES
        ENVIRONMENT "NPP_CXX=${NPP_SELFHOST_CXX}"
        TIMEOUT 45)
    add_test(NAME npp_engine2d_api_smoke
        COMMAND $<TARGET_FILE:npp> check ${CMAKE_SOURCE_DIR}/tests/regression_programs/engine2d_api.npp)
    set_tests_properties(npp_engine2d_api_smoke PROPERTIES
        PASS_REGULAR_EXPRESSION "check: OK")
  endif()

  # The built-in emitters currently target x86-64 Linux and Windows.
  if (WIN32 OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    find_program(NPP_PARITY_CXX NAMES clang++ clang++-18 g++ c++)
    if (NPP_SELFHOST_CXX)
      # Prefer the clang-compatible driver already used by the self-hosting
      # tests. A plain g++ cannot consume textual LLVM IR.
      set(NPP_PARITY_CXX ${NPP_SELFHOST_CXX})
    endif()
    if (NPP_PARITY_CXX)
      add_test(NAME npp_backend_parity
          COMMAND ${CMAKE_COMMAND}
              -DNPP_BIN=$<TARGET_FILE:npp>
              -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
              -P ${CMAKE_SOURCE_DIR}/tests/backend_parity.cmake)
      set_tests_properties(npp_backend_parity PROPERTIES
          ENVIRONMENT "NPP_CXX=${NPP_PARITY_CXX}")
      add_test(NAME npp_differential_smoke
          COMMAND ${CMAKE_COMMAND}
              -DNPP_BIN=$<TARGET_FILE:npp>
              -DNPP_SOURCE_DIR=${CMAKE_SOURCE_DIR}
              -P ${CMAKE_SOURCE_DIR}/tests/differential/run.cmake)
      set_tests_properties(npp_differential_smoke PROPERTIES
          ENVIRONMENT "NPP_CXX=${NPP_PARITY_CXX}"
          TIMEOUT 180)
    endif()
  endif()

  add_test(NAME npp_negative_tests
      COMMAND ${CMAKE_COMMAND}
          -DNPP_BIN=$<TARGET_FILE:npp>
          -DTEST_SOURCE_DIR=${CMAKE_SOURCE_DIR}/tests/negative
          -DTEST_BINARY_DIR=${CMAKE_BINARY_DIR}
          -P ${CMAKE_SOURCE_DIR}/tests/negative/run.cmake)
  set_tests_properties(npp_negative_tests PROPERTIES TIMEOUT 30)

  if (NPP_BUILD_LIBFUZZER)
    file(COPY ${CMAKE_SOURCE_DIR}/tests/fuzz/corpus/lexer
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/fuzz-corpus)
    file(COPY ${CMAKE_SOURCE_DIR}/tests/fuzz/corpus/parser
         DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/fuzz-corpus)
    add_test(NAME npp_fuzz_lexer_smoke
        COMMAND $<TARGET_FILE:npp_fuzz_lexer>
            -runs=1000 -max_len=4096 ${CMAKE_CURRENT_BINARY_DIR}/fuzz-corpus/lexer)
    add_test(NAME npp_fuzz_parser_smoke
        COMMAND $<TARGET_FILE:npp_fuzz_parser>
            -runs=1000 -max_len=4096 ${CMAKE_CURRENT_BINARY_DIR}/fuzz-corpus/parser)
    set_tests_properties(npp_fuzz_lexer_smoke npp_fuzz_parser_smoke
        PROPERTIES TIMEOUT 30)
  endif()

endif()

# ============================================================================
# Summary
# ============================================================================
message(STATUS "==========================================")
message(STATUS "N++ ${PROJECT_VERSION} Configuration Summary")
message(STATUS "==========================================")
message(STATUS "C Compiler:   ${CMAKE_C_COMPILER}")
message(STATUS "C++ Compiler: ${CMAKE_CXX_COMPILER}")
message(STATUS "Linker:       ${NPP_LINK_FLAG}")
message(STATUS "Build Type:   ${CMAKE_BUILD_TYPE}")
message(STATUS "SDL2 Found:   ${SDL2_FOUND}")
if (SDL2_FOUND)
  message(STATUS "SDL2 Version: ${SDL2_VERSION}")
endif()
message(STATUS "Tests:        ${NPP_BUILD_TESTS}")
message(STATUS "==========================================")