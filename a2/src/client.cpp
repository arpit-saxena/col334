#include <iostream>

#include "config.hpp"

int main(int argc, char *argv[]) {
  if (argc < 2) {
    std::cout << argv[0] << " Version " << ChatApp_VERSION_MAJOR << "."
              << ChatApp_VERSION_MINOR << std::endl;
  }
}