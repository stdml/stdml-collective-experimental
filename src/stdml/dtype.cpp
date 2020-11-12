#include <stdexcept>

#include <stdml/bits/dtype.hpp>

namespace stdml::collective
{
size_t dtype_size(dtype dt)
{
    switch (dt) {
    case _type<int8_t>::value:
        return sizeof(int8_t);
    case _type<int32_t>::value:
        return sizeof(int32_t);
    case _type<float>::value:
        return sizeof(float);
    default:
        throw std::invalid_argument("");
    }
}
}  // namespace stdml::collective
