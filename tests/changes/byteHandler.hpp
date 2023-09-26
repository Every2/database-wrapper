#include <string>
#include <cstdint>
#include <array>

template<size_t S>
class ByteHandler {

public:
    std::array<uint8_t, S> stringToByteArray(const std::string& sourceStr);
    std::string byteArrayToString(std::array<uint8_t, S>& sourceArray);
};


