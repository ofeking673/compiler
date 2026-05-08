#pragma once
#include "QbeCodeGen.h"
#include <iostream>
#include <set>

// Build orchestration helpers that turn emitted SSA into a library or executable.
class compiler {
public:
    // Summary: Executes a shell command and aborts on non-zero exit.
    // Input: command string to run.
    // Output: no return value; process exits on failure.
    static void runOrFail(const std::string& cmd) {
        int rc = std::system(cmd.c_str());
        if (rc != 0) {
            std::cerr << "command failed: " << cmd << "\n";
            std::exit(1);
        }
    }

    // Summary: Writes SSA, invokes qbe/gcc, and produces either .a or executable output.
    // Input: source file path, lib/exe mode, codegen buffer, and imported module names.
    // Output: generated artifacts on disk for the input source.
    static void processFile(const std::string& filePath, bool buildLib, QbeCodeGen& gen, std::set<std::string>& imports)
    {
        std::string dir  = dirOf(filePath);
        std::string base = baseNoExtOf(filePath);

        std::string ssaPath = dir + "/" + base + ".ssa";
        std::string asmPath = dir + "/" + base + ".s";
        std::string objPath = dir + "/" + base + ".o";


        gen.produceFinalFile(ssaPath);

        // Summary: Lower SSA to assembly with qbe.
        // Input: generated .ssa path.
        // Output: architecture assembly file (.s).
        runOrFail("qbe -o \"" + asmPath + "\" \"" + ssaPath + "\"");
        // Summary: Assemble/compile assembly into a relocatable object.
        // Input: generated .s path.
        // Output: object file (.o).
        runOrFail("gcc -c -o \"" + objPath + "\" \"" + asmPath + "\"");

        if (buildLib) {
			std::string libPath = dir + "/" + base + ".a";
            // Summary: Create static archive from object file.
            // Input: object file (.o).
            // Output: static library archive (.a).
			runOrFail("ar rcs \"" + libPath + "\" \"" + objPath + "\"");
            // Summary: Refresh archive index for linker symbol lookup.
            // Input: static library archive (.a).
            // Output: indexed archive ready for linking.
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

        // Summary: Link object file and imported static libraries into an executable.
        // Input: main object file plus resolved `.a` libraries.
        // Output: runnable executable.
        runOrFail(linkCmd);
        std::cout << "Built executable: " << exePath << "\n";
    }
    
    // Summary: Extracts imported module names from top-level program statements.
    // Input: parsed Program AST.
    // Output: unique set of import names.
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
    // Summary: Gets parent directory for a file path.
    // Input: file path string.
    // Output: directory portion of the path.
    static std::string dirOf(const std::string& path) {
        size_t p = path.find_last_of('/');
        if (p == std::string::npos) return ".";
        if (p == 0) return "/";              // "/src.fg" case
        return path.substr(0, p);
    }


    // Summary: Returns filename without extension.
    // Input: file path string.
    // Output: basename minus final extension.
    static std::string baseNoExtOf(const std::string& path) {
        size_t slash = path.find_last_of('/');
        std::string file = (slash == std::string::npos) ? path : path.substr(slash + 1);
        size_t dot = file.find_last_of('.');
        if (dot == std::string::npos) return file;
        return file.substr(0, dot);
    }

    // Summary: Checks whether a path exists and is readable as a file.
    // Input: file path string.
    // Output: true if file can be opened; false otherwise.
    static bool fileExists(const std::string& path) {
        std::ifstream f(path);
        return f.good();
    }

    // Summary: Resolves import name to a local static library path.
    // Input: current .fg path and import module name.
    // Output: resolved `.a` path; process exits if not found.
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
