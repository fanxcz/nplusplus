#pragma once

#include "builtins/builtin_signature.hpp"

#include <optional>
#include <string>

namespace npp {

    inline std::optional<BuiltinSignature> builtinDataSignature(const std::string& name) {

        using T = Type;

        if (name == "http_get")
            return BuiltinSignature{name, {T::string()}, T::string(), "npp_http_get"};

        if (name == "json_get")
            return BuiltinSignature{
                name, {T::string(), T::string()}, T::string(), "npp_json_get"};

        if (name == "json_valid")
            return BuiltinSignature{name, {T::string()}, T::boolean(), "npp_json_valid"};

        if (name == "json_type")
            return BuiltinSignature{
                name, {T::string(), T::string()}, T::string(), "npp_json_type"};

        if (name == "json_array_len")
            return BuiltinSignature{
                name, {T::string(), T::string()}, T::i64(), "npp_json_array_len"};

        if (name == "json_array_get")
            return BuiltinSignature{
                name, {T::string(), T::string(), T::i64()}, T::string(), "npp_json_array_get"};

        if (name == "sqlite_open")
            return BuiltinSignature{name, {T::string()}, T::i64(), "npp_sqlite_open"};

        if (name == "sqlite_exec")
            return BuiltinSignature{name, {T::i64(), T::string()}, T::i64(), "npp_sqlite_exec"};

        if (name == "sqlite_close")
            return BuiltinSignature{name, {T::i64()}, T::void_(), "npp_sqlite_close"};

        if (name == "sha256")
            return BuiltinSignature{name, {T::string()}, T::string(), "npp_sha256"};

        return std::nullopt;
    }

} // namespace npp
