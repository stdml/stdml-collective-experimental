#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <stdexcept>

#include <stdml/bits/collective/dtype.hpp>

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

template <typename T>
struct std_min {
    T operator()(const T &x, const T &y) const
    {
        return std::min(x, y);
    }
};

template <typename T>
struct std_max {
    T operator()(const T &x, const T &y) const
    {
        return std::max(x, y);
    }
};

template <typename T>
struct std_xor {
    T operator()(const T &x, const T &y) const
    {
        return x ^ y;
    }
};

template <>
struct std_xor<float> {
    using T = float;
    T operator()(const T &x, const T &y) const
    {
        throw std::invalid_argument("xor does't support float");
    }
};

template <>
struct std_xor<double> {
    using T = double;
    T operator()(const T &x, const T &y) const
    {
        throw std::invalid_argument("xor does't support double");
    }
};

struct __workspace {
    const void *input1;
    const void *input2;
    void *output;

    template <typename T>
    void call_as(const size_t n, const reduce_op o) const
    {
        const T *x = reinterpret_cast<const T *>(input1);
        const T *y = reinterpret_cast<const T *>(input2);
        T *z = reinterpret_cast<T *>(output);
        switch (o) {
        case sum:
            std::transform(x, x + n, y, z, std::plus<T>());
            break;
        case min:
            std::transform(x, x + n, y, z, std_min<T>());
            break;
        case max:
            std::transform(x, x + n, y, z, std_max<T>());
            break;
        case prod:
            std::transform(x, x + n, y, z, std::multiplies<T>());
            break;
        case bit_xor:
            std::transform(x, x + n, y, z, std_xor<T>());
            break;
        default:
            throw std::invalid_argument("unknown reduce op");
        }
    }

    // void call_as_f16(const int n, const reduce_op o) const
    // {
    //     switch (o) {
    //     case sum:
    //         float16_sum(output, input1, input2, n);
    //         break;
    //     default:
    //         exit(1);
    //     }
    // }
};

void reduce(void *x, const void *y, const void *z, size_t count, dtype dt,
            reduce_op op)
{
    const __workspace w = {
        .input1 = y,
        .input2 = z,
        .output = x,
    };

#define CASE(T, t)                                                             \
    case t:                                                                    \
        w.call_as<T>(count, op);                                               \
        break

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

        // case bf16:
        //     w.call_as_f16(n, o);
        //     break;

        // case f16:
        //     w.call_as_f16(n, o);
        //     break;

    default:
        exit(1);
    };

#undef CASE
}
}  // namespace stdml::collective
