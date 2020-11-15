#include <iostream>
#include <numeric>
#include <vector>

#include <stdml/collective>

template <typename T>
void test_all_reduce(stdml::collective::session &sess, size_t count,
                     stdml::collective::dtype dt, const char *dt_name)
{
    std::vector<T> x(count);
    std::vector<T> y(count);
    std::vector<T> z(count);

    T value = sess.rank();
    T result = sess.size() * (sess.size() - 1) / 2;
    std::fill(x.begin(), x.end(), value);
    std::fill(z.begin(), z.end(), result);
    std::iota(y.begin(), y.end(), -0xfffff);
    sess.all_reduce(x.data(), y.data(), count, dt, stdml::collective::sum);
    if (!std::equal(y.begin(), y.end(), z.begin())) {
        std::cerr << "failed" << std::endl;
        std::cerr << "want: " << result << ", got: " << y[0] << std::endl;
        // exit(1);
    } else {
        std::cout << "OK all_reduce(dtype=" << dt_name << ", count=" << count
                  << ")" << std::endl;
    }
}

void test_all_reduce_all(stdml::collective::session &sess, size_t count)
{
    test_all_reduce<int8_t>(sess, count, stdml::collective::i8, "i8");
    test_all_reduce<int16_t>(sess, count, stdml::collective::i16, "i16");
    test_all_reduce<int32_t>(sess, count, stdml::collective::i32, "i32");
    test_all_reduce<int64_t>(sess, count, stdml::collective::i64, "i64");

    test_all_reduce<uint8_t>(sess, count, stdml::collective::u8, "u8");
    test_all_reduce<uint16_t>(sess, count, stdml::collective::u16, "u16");
    test_all_reduce<uint32_t>(sess, count, stdml::collective::u32, "u32");
    test_all_reduce<uint64_t>(sess, count, stdml::collective::u64, "u64");

    test_all_reduce<float>(sess, count, stdml::collective::f32, "f32");
    test_all_reduce<double>(sess, count, stdml::collective::f64, "f64");
}

int main()
{
    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session sess = peer.join();
    auto counts = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100, 1024,
    };
    for (auto count : counts) { test_all_reduce_all(sess, count); }
    return 0;
}
