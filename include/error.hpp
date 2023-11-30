#include <iostream>
#include <string_view>

class Error {

public:
    void message(std::string_view errorMessage);
    void die(std::string_view errorMessage);
};

enum class Errors {
    unknown_error = 1,
    too_big_error = 2,
    type_error = 3,
    argument_error = 4,
};