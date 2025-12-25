#include "Pipeline.hpp"

//std
#include <fstream> //file input/output
#include <iostream>
#include <stdexcept>


namespace engine {
  Pipeline::Pipeline(const std::string &vertPath, const std::string &fragPath) {
    createGraphicsPipeline(vertPath, fragPath);
  }

  // Read an entire file into memory and return its contents as a vector of chars
  std::vector<char> Pipeline::readFile(const std::string &path) {
    // Open the file, seek to the end immediately, and read raw bytes to avoid text conversion
    std::ifstream file{path, std::ios::ate | std::ios::binary};

    if (!file.is_open()) {
      throw std::runtime_error{"Failed to open file \"" + path + "\"."};
    }

    // tellg() returns current read position (which we said should be the last character)
    size_t fileSize = static_cast<size_t>(file.tellg());
    std::vector<char> buffer(fileSize); // Create a buffer big enough to hold the whole file

    file.seekg(0); // Return to the first position in the file
    // Read fileSize bytes into the buffer
    file.read(buffer.data(), fileSize); // buffer.data() returns a raw pointer to the vector's storage

    file.close();
    return buffer;
  }

  void Pipeline::createGraphicsPipeline(const std::string &vertPath, const std::string &fragPath) {
    auto vertCode = readFile(vertPath);
    auto fragCode = readFile(fragPath);

    std::cout << "Vertex Shader Code Size: " << vertCode.size() << std::endl;
    std::cout << "Fragment Shader Code Size: " << fragCode.size() << std::endl;
  }
}
