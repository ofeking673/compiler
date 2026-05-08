#pragma once
#include "QbeCodeGen.h"
#include <iostream>
#include <set>

// Build orchestration helpers that turn emitted SSA into a library or executable.
class compiler {
public:
    static void runOrFail(const std::string& cmd) {
        int rc = std::system(cmd.c_str());
        if (rc != 0) {
            std::cerr << "command failed: " << cmd << "\n";
            std::exit(1);
        }
    }

    static void processFile(const std::string& filePath, bool buildLib, QbeCodeGen& gen, std::set<std::string>& imports)
    {
        std::string dir  = dirOf(filePath);
        std::string base = baseNoExtOf(filePath);

        std::string ssaPath = dir + "/" + base + ".ssa";
        std::string asmPath = dir + "/" + base + ".s";
        std::string objPath = dir + "/" + base + ".o";


        gen.produceFinalFile(ssaPath);

        runOrFail("qbe -o \"" + asmPath + "\" \"" + ssaPath + "\"");
        runOrFail("gcc -c -o \"" + objPath + "\" \"" + asmPath + "\"");

        if (buildLib) {
			std::string libPath = dir + "/" + base + ".a";
			runOrFail("ar rcs \"" + libPath + "\" \"" + objPath + "\"");
            runOrFail("ranlib \"" + libPath + "\"");
            std::cout << "Built library: " << libPath << "\n";
            return;
		}


        std::string exePath = dir + "/" + base;
        std::string linkCmd = "gcc -o \"" + exePath + "\" \"" + objPath + "\"";

        for (const auto& imp : imports) {
            std::string lib = resolveLocalLibOrFail(filePath, imp);
            linkCmd += " \"" + lib + "\"";
        }

        runOrFail(linkCmd);
        std::cout << "Built executable: " << exePath << "\n";
    }
    
    static std::set<std::string> findImports(Program* program) {
        std::set<std::string> out;

        for (const auto& stmt : program->stmt) {
            if (auto importStmt = dynamic_cast<ImportStmt*>(stmt.get())) {
                out.insert(importStmt->moduleName);
            }
        }

        return out;
    }

    // Return directory of a path (Linux-style). If no '/', return "."
    static std::string dirOf(const std::string& path) {
        size_t p = path.find_last_of('/');
        if (p == std::string::npos) return ".";
        if (p == 0) return "/";              // "/src.fg" case
        return path.substr(0, p);
    }


    static std::string baseNoExtOf(const std::string& path) {
        size_t slash = path.find_last_of('/');
        std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
        size_t dot = file.find_last_of('.');
        if (dot == std::string::npos) return file;
        return file.substr(0, dot);
    }

    static bool fileExists(const std::string& path) {
        std::ifstream f(path);
        return f.good();
    }

    static std::string resolveLocalLibOrFail(const std::string& fgPath, const std::string& name) {
        std::string dir = dirOf(fgPath);

        std::string a1 = dir + "/" + name + ".a";
        if (fileExists(a1)) return a1;

        std::string a2 = dir + "/lib" + name + ".a";
        if (fileExists(a2)) return a2;

        std::cerr << "error: import '" << name << "' not found in " << dir << "\n"
            << "expected: " << a1 << " (or " << a2 << ")\n";
        std::exit(1);
    }
};
