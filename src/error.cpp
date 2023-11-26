#include "../include/error.hpp"

void Error::die() {
    std::cerr << errorMessage << '\n';
    std::abort();
}