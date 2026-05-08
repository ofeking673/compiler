#include <fstream>
#include <iostream>


// Minimal file reader used by the compiler frontend.
class FileIO {
public:
  FileIO() = default;

  // Summary: Reads full text content of a file into memory.
  // Input: path to the source file.
  // Output: file contents as a single string, or empty string on failure.
  std::string readFile(std::string filename) {
    std::fstream fileToRead(filename);
    
    if(!fileToRead.is_open()) {
      std::cerr << "Could not open file " << filename << ", Aborting" << std::endl;
      return "";
    }
    
    std::string line, final = "";
    while(std::getline(fileToRead, line))
    {
      final += line + "\n";
    }

    fileToRead.close();
    return final;
  }
};
