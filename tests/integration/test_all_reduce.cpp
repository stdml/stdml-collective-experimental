#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>

#include <stdml/collective>

#include <cxxabi.h>

template <typename T>
std::string type_name()
{
    int status = 0;
    return abi::__cxa_demangle(typeid(T).name(), 0, 0, &status);
}

template <typename T>
struct lift {
    using type = T;
};

template <>
struct lift<int8_t> {
    using type = int32_t;
};

template <>
struct lift<uint8_t> {
    using type = uint32_t;
};

template <typename T>
std::string show_value(T x)
{
    using U = typename lift<T>::type;
    return std::to_string(static_cast<U>(x));
}

template <typename T>
int test_all_reduce(stdml::collective::session &sess, size_t count)
{
    const stdml::collective::dtype dt = stdml::collective::type<T>();
    const std::string dt_name = type_name<T>();
    std::stringstream name;
    name << "all_reduce(count=" << count << ", "
         << "dtype=" << dt_name << ")";

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
        std::cerr << "Failed " << name.str()  //
                  << " want: " << show_value(result)
                  << ", got: " << show_value(y[0]) << std::endl;
        return 1;
    } else {
        std::cout << "OK " << name.str() << std::endl;
        return 0;
    }
}

int test_all_reduce_all(stdml::collective::session &sess, size_t count)
{
    int f = 0;
    f += test_all_reduce<int8_t>(sess, count);
    f += test_all_reduce<int16_t>(sess, count);
    f += test_all_reduce<int32_t>(sess, count);
    f += test_all_reduce<int64_t>(sess, count);

    f += test_all_reduce<uint8_t>(sess, count);
    f += test_all_reduce<uint16_t>(sess, count);
    f += test_all_reduce<uint32_t>(sess, count);
    f += test_all_reduce<uint64_t>(sess, count);

    f += test_all_reduce<float>(sess, count);
    f += test_all_reduce<double>(sess, count);
    return f;
}

int main()
{
    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session sess = peer.join();
    auto counts = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100, 1024,
    };
    int failed = 0;
    for (auto count : counts) { failed += test_all_reduce_all(sess, count); }
    if (failed) {
        std::cerr << failed << " failed!" << std::endl;
        return 1;
    } else {
        std::cout << "all tests passed." << std::endl;
    }
    return 0;
}
