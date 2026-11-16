#pragma once

#include "native/native.hpp"

#include <map>
#include <string>
#include <vector>

namespace npp::native {
    namespace detail {

        // Codegen-facing kind of a value slot. All values occupy one 8-byte
        // slot; aggregates (arrays/structs) travel as pointers to arena
        // blocks of 8-byte slots, so they share Kind::Ptr.
        enum class Kind { I64, Bool, Void, Str, F64, Ptr };

        Kind kindOf(const Type& t);

        struct FnSig {
            std::vector<Type> params;
            Type ret = Type::void_();
        };

        // Returns the expression's type, or TypeKind::Unknown with `reason`
        // set when the expression is outside the native subset.
        Type checkExpr(const Expr* e,
                       const std::map<std::string, Type>& env,
                       const std::map<std::string, FnSig>& fns,
                       const std::map<std::string, const StructDecl*>& structs,
                       const std::map<std::string, const EnumDecl*>& enums,
                       std::string& reason);
        void checkBlock(const Block* b,
                        std::map<std::string, Type>& env,
                        const std::map<std::string, FnSig>& fns,
                        const std::map<std::string, const StructDecl*>& structs,
                        const std::map<std::string, const EnumDecl*>& enums,
                        bool inLoop,
                        bool fnScope,
                        std::string& reason);

        // True when `t` is representable by the native backend (scalars plus
        // fixed arrays and structs built from supported types).
        bool supportedType(const Type& t,
                           const std::map<std::string, const StructDecl*>& structs,
                           std::string& reason,
                           std::size_t depth = 0);

        // Number of 8-byte slots an aggregate value occupies.
        std::size_t aggregateSlots(const Type& t,
                                   const std::map<std::string, const StructDecl*>& structs);

    } // namespace detail
} // namespace npp::native
