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

template <>
struct _type<int8_t> {
    static constexpr dtype value = i8;
};

template <>
struct _type<int32_t> {
    static constexpr dtype value = i32;
};

template <>
struct _type<float> {
    static constexpr dtype value = f32;
};

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
