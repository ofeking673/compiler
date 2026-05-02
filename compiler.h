#pragma once
#include "QbeCodeGen.h"
#include <iostream>
#include <set>

class compiler {
public:
    static void runOrFail(const std::string& cmd) {
        int rc = std::system(cmd.c_str());
        if (rc != 0) {
            std::cerr << "command failed: " << cmd << "\n";
            std::exit(1);
        }
    }

    static void processFile(const std::string& filePath, bool buildLib, QbeCodeGen& gen)
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

        std::set<std::string> imports = findImports(gen.output.str());

        std::string exePath = dir + "/" + base;
        std::string linkCmd = "gcc -o \"" + exePath + "\" \"" + objPath + "\"";

        for (const auto& imp : imports) {
            std::string lib = resolveLocalLibOrFail(filePath, imp);
            linkCmd += " \"" + lib + "\"";
        }

        runOrFail(linkCmd);
        std::cout << "Built executable: " << exePath << "\n";
    }

    static std::set<std::string> findImports(const std::string& source) {
        std::set<std::string> out;

        size_t pos = 0;
        while (pos < source.size()) {
            size_t end = source.find('\n', pos);
            if (end == std::string::npos) end = source.size();
            std::string line = source.substr(pos, end - pos);

            size_t i = 0;
            while (i < line.size() && (line[i] == ' ' || line[i] == '\t' || line[i] == '\r')) i++;

            const std::string kw = "import ";
            if (line.compare(i, kw.size(), kw) == 0) {
                i += kw.size();
                size_t j = i;
                while (j < line.size() && (std::isalnum((unsigned char)line[j]) || line[j] == '_')) j++;
                if (j > i) out.insert(line.substr(i, j - i));
            }

            pos = (end == source.size()) ? source.size() : end + 1;
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