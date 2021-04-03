#include <future>
#include <iostream>
#include <numeric>
#include <sstream>
#include <vector>

#include <stdml/collective>

#include <cxxabi.h>
#include <std/ranges>

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

struct test_data_1 {
    size_t rank;
    size_t size;

    test_data_1(size_t rank, size_t size) : rank(rank), size(size)
    {
    }

    template <typename T>
    void operator()(std::vector<T> &x, std::vector<T> &y, std::vector<T> &z)
    {
        T value = rank;
        T result = size * (size - 1) / 2;
        std::fill(x.begin(), x.end(), value);
        std::fill(z.begin(), z.end(), result);
        std::iota(y.begin(), y.end(), -0xfffff);
    }
};

struct test_data_2 {
    size_t rank;
    size_t size;

    test_data_2(size_t rank, size_t size) : rank(rank), size(size)
    {
    }

    template <typename T>
    void operator()(std::vector<T> &x, std::vector<T> &y, std::vector<T> &z)
    {
        T value = 1 << rank;
        T result = (1 << size) - 1;
        std::fill(x.begin(), x.end(), value);
        std::fill(z.begin(), z.end(), result);
        std::iota(y.begin(), y.end(), -0xfffff);
    }
};

template <typename T, typename Init>
int test_all_reduce(stdml::collective::session &sess, size_t count, Init &init)
{
    const stdml::collective::dtype dt = stdml::collective::type<T>();
    const std::string dt_name = type_name<T>();
    std::stringstream name;
    name << type_name<Init>() << "::all_reduce(count=" << count << ", "
         << "dtype=" << dt_name << ")";

    std::vector<T> x(count);
    std::vector<T> y(count);
    std::vector<T> z(count);
    init(x, y, z);

    sess.all_reduce(x.data(), y.data(), count, dt, stdml::collective::sum,
                    name.str());
    if (!std::equal(y.begin(), y.end(), z.begin())) {
        std::stringstream msg;
        msg << "Failed " << name.str()  //
            << " want: " << show_value(z[0]) << ", got: " << show_value(y[0]);
        std::cerr << msg.str() << std::endl;
        return 1;
    } else {
        // std::cout << "OK " << name.str() << std::endl;
        return 0;
    }
}

template <typename Init>
int test_all_reduce_all(stdml::collective::session &sess, size_t count)
{
    Init init(sess.rank(), sess.size());
    int f = 0;
    f += test_all_reduce<int8_t>(sess, count, init);
    f += test_all_reduce<int16_t>(sess, count, init);
    f += test_all_reduce<int32_t>(sess, count, init);
    f += test_all_reduce<int64_t>(sess, count, init);

    f += test_all_reduce<uint8_t>(sess, count, init);
    f += test_all_reduce<uint16_t>(sess, count, init);
    f += test_all_reduce<uint32_t>(sess, count, init);
    f += test_all_reduce<uint64_t>(sess, count, init);

    f += test_all_reduce<float>(sess, count, init);
    f += test_all_reduce<double>(sess, count, init);
    return f;
}

template <typename T>
std::future<T> make_ready_future(T x)
{
    std::promise<T> p;
    std::future<T> f = p.get_future();
    p.set_value(std::move(x));
    return f;
}

template <typename T, typename Init>
int test_group_all_reduce(stdml::collective::session &sess,
                          const std::vector<size_t> &counts, Init &init)
{
    const stdml::collective::dtype dt = stdml::collective::type<T>();
    const std::string dt_name = type_name<T>();
    std::stringstream name;
    name << type_name<Init>() << "::group_all_reduce(n=" << counts.size()
         << ", "
         << "dtype=" << dt_name << ")";

    using Tensor = std::vector<T>;
    std::vector<Tensor> xs;
    std::vector<Tensor> ys;
    std::vector<Tensor> zs;
    for (auto i : std::views::iota((size_t)0, counts.size())) {
        xs.emplace_back(counts[i]);
        ys.emplace_back(counts[i]);
        zs.emplace_back(counts[i]);
        init(xs[i], ys[i], zs[i]);
    }
    std::vector<std::future<stdml::collective::workspace>> fs;
    for (auto i : std::views::iota((size_t)0, counts.size())) {
        stdml::collective::workspace w = {
            .send = xs[i].data(),
            .recv = ys[i].data(),
            .count = counts[i],
            .dt = dt,
            .op = stdml::collective::sum,
            .name = name.str() + std::to_string(i),
        };
        fs.push_back(make_ready_future(w));
    }
    sess.group_all_reduce(std::move(fs));
    if (ys != zs) {
        std::stringstream msg;
        msg << "Failed " << name.str();
        std::cerr << msg.str() << std::endl;
        return 1;
    }
    return 0;
}

template <typename Init>
int test_group_all_reduce_all(stdml::collective::session &sess,
                              const std::vector<size_t> &counts)
{
    Init init(sess.rank(), sess.size());
    int f = 0;
    f += test_group_all_reduce<int8_t>(sess, counts, init);
    f += test_group_all_reduce<int16_t>(sess, counts, init);
    f += test_group_all_reduce<int32_t>(sess, counts, init);
    f += test_group_all_reduce<int64_t>(sess, counts, init);

    f += test_group_all_reduce<uint8_t>(sess, counts, init);
    f += test_group_all_reduce<uint16_t>(sess, counts, init);
    f += test_group_all_reduce<uint32_t>(sess, counts, init);
    f += test_group_all_reduce<uint64_t>(sess, counts, init);

    f += test_group_all_reduce<float>(sess, counts, init);
    f += test_group_all_reduce<double>(sess, counts, init);
    return f;
}

int main()
{
    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session sess = peer.join();
    const std::vector<size_t> counts({
        1,
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        9,
        10,
        100,
        1024,
    });
    int failed = 0;
    for (auto count : counts) {
        failed += test_all_reduce_all<test_data_1>(sess, count);
        failed += test_all_reduce_all<test_data_2>(sess, count);
    }
    failed += test_group_all_reduce_all<test_data_1>(sess, counts);
    failed += test_group_all_reduce_all<test_data_2>(sess, counts);

    if (failed) {
        std::cerr << failed << " failed!" << std::endl;
        return 1;
    } else {
        std::cout << "all tests passed." << std::endl;
    }
    return 0;
}
