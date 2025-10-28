#include <fstream>
#include <iostream>


class FileIO {
public:
  FileIO() = default;

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
