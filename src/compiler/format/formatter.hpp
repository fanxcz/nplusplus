#pragma once

#include <string>

namespace npp {

    // Format N++ source using the conservative indentation and whitespace rules
    // shared by the CLI and Language Server Protocol implementation.
    std::string formatNppSource(const std::string& source);

} // namespace npp
