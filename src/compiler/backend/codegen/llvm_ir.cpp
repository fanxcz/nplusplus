#include "codegen/llvm_ir.hpp"

#include "codegen/llvm_cg.hpp"

namespace npp {

    std::string LLVMIRGenerator::generate(const Program& p) {
        return CG(options_).generate(p);
    }

} // namespace npp
