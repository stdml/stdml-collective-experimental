#pragma once
#include <cstddef>
#include <cstdint>

namespace stdml::collective
{
enum dtype {
    i8,
    i16,
    i32,
    i64,

    u8,
    u16,
    u32,
    u64,

    bf16,
    f16,
    f32,
    f64,
};

template <typename T>
struct _type;

#define DEFINE_DTYPE(T, t)                                                     \
    template <>                                                                \
    struct _type<T> {                                                          \
        static constexpr dtype value = t;                                      \
    };

DEFINE_DTYPE(int8_t, i8);
DEFINE_DTYPE(int16_t, i16);
DEFINE_DTYPE(int32_t, i32);
DEFINE_DTYPE(int64_t, i64);

DEFINE_DTYPE(uint8_t, u8);
DEFINE_DTYPE(uint16_t, u16);
DEFINE_DTYPE(uint32_t, u32);
DEFINE_DTYPE(uint64_t, u64);

DEFINE_DTYPE(float, f32);
DEFINE_DTYPE(double, f64);

#undef DEFINE_DTYPE

template <typename T>
constexpr dtype type()
{
    return _type<T>::value;
}

size_t dtype_size(dtype dt);

enum reduce_op {
    sum,
    min,
    max,
    prod,
};
}  // namespace stdml::collective
