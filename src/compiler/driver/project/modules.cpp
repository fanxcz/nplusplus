#include "driver/driver_internal.hpp"

namespace npp {
    namespace driver {


        void appendProgram(Program& dst, Program src) {

            dst.imports.insert(dst.imports.end(), src.imports.begin(), src.imports.end());

            for (auto& s : src.structs)
                dst.structs.push_back(std::move(s));

            for (auto& e : src.enums)
                dst.enums.push_back(std::move(e));

            for (auto& f : src.functions)
                dst.functions.push_back(std::move(f));
        }


        Program loadProgram(const fs::path& root, ModuleLoadContext& context) {

            const auto canon = fs::weakly_canonical(root);

            if (context.active.contains(canon)) {
                std::ostringstream cycle;
                cycle << "circular import detected: ";
                const auto begin = std::find(context.stack.begin(), context.stack.end(), canon);
                for (auto it = begin; it != context.stack.end(); ++it) {
                    if (it != begin)
                        cycle << " -> ";
                    cycle << it->generic_string();
                }
                if (begin != context.stack.end())
                    cycle << " -> ";
                cycle << canon.generic_string();
                throw std::runtime_error(cycle.str());
            }

            if (context.loaded.contains(canon))
                return {};

            context.active.insert(canon);
            context.stack.push_back(canon);
            struct ModuleLoadGuard {
                ModuleLoadContext& context;
                fs::path path;
                ~ModuleLoadGuard() {
                    context.active.erase(path);
                    if (!context.stack.empty())
                        context.stack.pop_back();
                }
            } guard{context, canon};

            gDiagnosticSource = canon;

            const auto src = readFile(canon);
            gDiagnosticText = src;
            gDiagnosticSources[canon.generic_string()] = src;

            auto prog = Parser(Lexer(src).tokenize(), canon.generic_string()).parseProgram();

            Program all;

            for (const auto& imp : prog.imports) {

                fs::path child;

                if (imp.rfind("std.", 0) == 0) {

                    const std::string mod = imp.substr(4);

                    std::vector<fs::path> candidates;

                    candidates.push_back(canon.parent_path() / "stdlib" / (mod + ".npp"));

                    if (const char* stdDir = std::getenv("NPP_STD_DIR")) {
                        candidates.push_back(fs::path(stdDir) / (mod + ".npp"));
                    }

#ifdef NPP_SOURCE_ROOT
                    candidates.push_back(fs::path(NPP_SOURCE_ROOT) / "stdlib" / (mod + ".npp"));

#endif
                    if (!gExecutable.empty()) {

                        const auto bin = fs::weakly_canonical(gExecutable).parent_path();

                        // Portable distribution places standard modules next to npp.exe.
                        candidates.push_back(bin / "stdlib" / (mod + ".npp"));
                        candidates.push_back(bin / "../stdlib" / (mod + ".npp"));
                        candidates.push_back(bin / "../share/npp/stdlib" / (mod + ".npp"));
                        candidates.push_back(bin / "../../stdlib" / (mod + ".npp"));
                    }
                    for (const auto& c : candidates)
                        if (fs::exists(c)) {
                            child = c;
                            break;
                        }
                    if (child.empty())
                        throw std::runtime_error("standard module not found: " + imp);

                } else {

                    child = imp;

                    if (child.extension().empty())
                        child += ".npp";

                    if (child.is_relative()) {

                        const auto local = canon.parent_path() / child;

                        const auto vendorName = child.stem();

                        std::vector<fs::path> candidates;

                        candidates.push_back(local);

                        const auto pkgRoot = findProjectRoot(canon);

                        if (!pkgRoot.empty()) {

                            candidates.push_back(pkgRoot / "vendor" / vendorName / "src/main.npp");

                            candidates.push_back(pkgRoot / "vendor" / child);
                        }
                        candidates.push_back(canon.parent_path() / "vendor" / vendorName /
                                             "src/main.npp");

                        candidates.push_back(canon.parent_path() / "vendor" / child);

                        for (const auto& c : candidates)
                            if (fs::exists(c)) {
                                child = c;
                                break;
                            }
                    }
                }
                appendProgram(all, loadProgram(child, context));
            }
            appendProgram(all, std::move(prog));
            context.loaded.insert(canon);

            return all;
        }


        fs::path resolveInput(const std::string& arg) {

            if (!arg.empty()) {

                fs::path p = arg;

                if (p.is_relative())
                    p = fs::absolute(p);

                return p;
            }
            const ProjectConfig cfg = loadProject();

            return cfg.root / cfg.entry;
        }


        fs::path templateSource(const std::string& templ) {
            std::vector<fs::path> candidates;
            const fs::path templateName = templ + ".npp";
            const fs::path exampleName = (templ == "game" ? "neon_survivor.npp" : templ + ".npp");
            if (!gExecutable.empty()) {
                const fs::path bin = fs::weakly_canonical(gExecutable).parent_path();
                candidates.push_back(bin / "../project-templates" / templateName);
                candidates.push_back(bin / "../share/npp/project-templates" / templateName);
                candidates.push_back(bin / "../share/npp/examples" / exampleName);
                candidates.push_back(bin / "../../examples/templates" / templateName);
                candidates.push_back(bin / "../../examples" / exampleName);
#ifdef NPP_SOURCE_ROOT
                candidates.push_back(fs::path(NPP_SOURCE_ROOT) / "examples" / "templates" /
                                     templateName);
                candidates.push_back(fs::path(NPP_SOURCE_ROOT) / "examples" / exampleName);
#endif
            }
            candidates.push_back(fs::current_path() / "examples" / "templates" / templateName);
            candidates.push_back(fs::current_path() / "examples" / exampleName);
            for (const auto& candidate : candidates) {
                std::error_code ec;
                if (fs::is_regular_file(candidate, ec))
                    return fs::weakly_canonical(candidate, ec);
            }
            return {};
        }

    } // namespace driver
} // namespace npp
