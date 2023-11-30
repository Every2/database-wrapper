#include <cstdint>
#include <string>
#include <vector>

class Container {
public:
    template<typename T, typename MemberType>
    static T* container_of(MemberType& object, T MemberType::*member) {
        return &object - offsetof(T, *member);
    }
};

class HashUtil {

public:
    static uint64_t string_hash(const std::vector<uint8_t>& data) {
        uint32_t hash {0x811C9DC5};
        for (uint8_t byte : data) {
            hash = (hash + byte) * 0x01000193;
        }

        return hash;
    }
};

enum {
    SERIALIZE_NIL = 0,
    SERIALIZE_ERR = 1,
    SERIALIZE_STR = 2,
    SERIALIZE_INT = 3,
    SERIALIZE_DBL = 4,
    SERIALIZE_ARR = 5,
};