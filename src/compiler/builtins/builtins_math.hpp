#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace npp {

    inline std::optional<BuiltinSignature> builtinMathSignature(const std::string& name) {

        using T = Type;

        if (name == "sqrt")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "npp_sqrt"};

        // Core floating-point math used by 2D/3D games, animation and simulation.

        if (name == "sin")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "npp_sin"};

        if (name == "cos")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "npp_cos"};

        if (name == "tan")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "npp_tan"};

        if (name == "atan2")
            return BuiltinSignature{name, {T::f64(), T::f64()}, T::f64(), "npp_atan2"};

        if (name == "floor")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "npp_floor"};

        if (name == "ceil")
            return BuiltinSignature{name, {T::f64()}, T::f64(), "npp_ceil"};

        if (name == "pow")
            return BuiltinSignature{name, {T::f64(), T::f64()}, T::f64(), "npp_pow"};

        return std::nullopt;
    }

} // namespace npp
