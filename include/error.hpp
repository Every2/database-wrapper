#include <iostream>
#include <string_view>

class Error {

public:
    Error(std::string_view message) : errorMessage(message) {};

    void die();
    
private:
    std::string_view errorMessage {};
};