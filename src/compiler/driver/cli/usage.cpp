#include "driver/driver_internal.hpp"

namespace npp {
    namespace driver {

        void usage() {

            std::cout
                << "N++ " << kVersion << "\n"
                << "Usage: npp <command> [options]\n\n"
                << "Interactive shell:\n"
                << "  repl                 evaluate expressions (also starts by running npp with "
                   "no arguments)\n\n"
                << "Project commands:\n"
                << "  new <project> [--template cli|app|game|3d|paint|server|library]\n"
                << "  init [--template cli|app|game|3d|paint|server|library]\n"
                << "  info\n"
                << "  assets\n"
                << "  clean\n"
                << "  doctor [--json]\n"
                << "  self-test\n"
                << "  explain-backend <file.npp> [--target TRIPLE]\n"
                << "  editor [--gui]\n\n"
                << "Build commands:\n"
                << "  build [file.npp] [-o output] [--target TRIPLE] [--backend auto|native|llvm] "
                   "[--emit-ir] [--release]\n"
                << "  run [file.npp] [--target TRIPLE] [--backend auto|native|llvm] [--release]\n"
                << "  export [--target TRIPLE] [--backend auto|native|llvm] [-o output]\n"
                << "  check [file.npp]\n"
                << "  targets\n"
                << "  fmt [file.npp] [--check]\n"
                << "  lint [file.npp]\n"
                << "  doc\n"
                << "  bench [file.npp] [--runs N]\n"
                << "  test\n"
                << "  pkg <init|add|remove|list|update|build|publish> [args]\n"
                << "  lsp\n"
                << "  version\n"
                << "  help\n\n"
                << "Backends:\n"
                << "  auto     use the builtin native backend when the program fits\n"
                << "           its subset, otherwise the LLVM backend (default)\n"
                << "  native   builtin direct machine-code backend, no external\n"
                << "           compiler or linker (x86-64 Windows/Linux subset)\n"
                << "  llvm     LLVM IR + external C++ compiler (full language)\n\n"
                << "Examples:\n"
                << "  npp new NeonGame --template game\n"
                << "  cd NeonGame && npp build && npp run\n"
                << "  npp check src/main.npp\n"
                << "  npp build src/main.npp -o app --target x86_64-linux\n";
        }

    } // namespace driver
} // namespace npp
