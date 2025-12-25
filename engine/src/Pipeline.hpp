#pragma once

#include <string>
#include <vector>

namespace engine {
  class Pipeline {
  public:
    Pipeline(const std::string &vertPath, const std::string &fragPath);

  private:
    static std::vector<char> readFile(const std::string &path);

    void createGraphicsPipeline(const std::string &vertPath, const std::string &fragPath);
  };
}
