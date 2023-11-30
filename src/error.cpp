#include "../include/error.hpp"

void Error::die(std::string_view errorMessage) {
    std::cerr << errorMessage << '\n';
    std::abort();
}

void Error::message(std::string_view errorMessage) {
    std::cerr << errorMessage << '\n';
}