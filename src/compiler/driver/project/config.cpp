#include "driver/driver_internal.hpp"

namespace npp {
    namespace driver {

        // Project commands and package commands share one manifest parser, so a
        // file accepted by one is accepted by the other.

        fs::path findProjectRoot(fs::path start) {

            std::error_code ec;

            start = fs::weakly_canonical(start, ec);

            if (ec)
                start = fs::absolute(start, ec);

            if (fs::is_regular_file(start, ec))
                start = start.parent_path();

            for (;;) {

                if (fs::exists(start / "npp.toml"))
                    return start;

                const auto parent = start.parent_path();

                if (parent == start)
                    break;

                start = parent;
            }
            return {};
        }

        ProjectConfig loadProject(const fs::path& requestedRoot) {

            const fs::path root = findProjectRoot(requestedRoot);

            if (root.empty())
                throw std::runtime_error(
                    "npp.toml not found; run `npp init` or pass a .npp file");

            const std::string data = readFile(root / "npp.toml");

            (void)parseManifestEntries(data, (root / "npp.toml").string());

            ProjectConfig cfg;

            cfg.root = root;

            cfg.name = tomlString(data, "package", "name");

            cfg.version = tomlString(data, "package", "version");

            cfg.entry = tomlString(data, "build", "entry");

            cfg.artifact = tomlString(data, "build", "artifact");

            cfg.target = tomlString(data, "build", "target");

            cfg.windowWidth = tomlInt(data, "window", "width", 1280);

            cfg.windowHeight = tomlInt(data, "window", "height", 720);

            cfg.windowTitle = tomlString(data, "window", "title");

            if (cfg.name.empty())
                cfg.name = root.filename().string();

            if (cfg.version.empty())
                cfg.version = kVersion;

            if (cfg.entry.empty())
                cfg.entry = "src/main.npp";

            if (cfg.artifact.empty())
                cfg.artifact = cfg.name;

            if (cfg.windowTitle.empty())
                cfg.windowTitle = cfg.name;

            return cfg;
        }

    } // namespace driver
} // namespace npp
