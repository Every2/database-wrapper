#include <iostream>
#include <string>

class Error {

public:
    Error(const std::string& message) : errorMessage(message) {};

    void die();
    
private:
    std::string errorMessage {};
};

enum class Errors {
    unknown_error = 1,
    too_big_error = 2,
    type_error = 3,
    argument_error = 4,
};