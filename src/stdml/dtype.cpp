#include <stdexcept>

#include <stdml/bits/dtype.hpp>

namespace stdml::collective
{
size_t dtype_size(dtype dt)
{
#define CASE(T, t)                                                             \
    case t:                                                                    \
        return sizeof(T);

    switch (dt) {
        CASE(int8_t, i8);
        CASE(int16_t, i16);
        CASE(int32_t, i32);
        CASE(int64_t, i64);

        CASE(uint8_t, u8);
        CASE(uint16_t, u16);
        CASE(uint32_t, u32);
        CASE(uint64_t, u64);

        CASE(float, f32);
        CASE(double, f64);

    default:
        throw std::invalid_argument("");
    }

#undef CASE
}
}  // namespace stdml::collective
