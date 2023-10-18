#include "gctex.h"

#include <iostream>

int main() {
  std::string version = gctex::get_version();
  std::cout << "gctex version: " << version << std::endl;
  return 0;
}
