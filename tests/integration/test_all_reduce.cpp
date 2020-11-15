#include <iostream>
#include <numeric>
#include <vector>

#include <stdml/collective>

void test_all_reduce(stdml::collective::session &sess, size_t n)
{
    std::vector<float> x(n);
    std::vector<float> y(n);
    std::vector<float> z(n);

    float value = sess.rank();
    float result = sess.size() * (sess.size() - 1) / 2;
    std::fill(x.begin(), x.end(), value);
    std::fill(z.begin(), z.end(), result);
    std::iota(y.begin(), y.end(), -0xfffff);
    sess.all_reduce(x.data(), y.data(), n, stdml::collective::f32,
                    stdml::collective::sum);
    if (!std::equal(y.begin(), y.end(), z.begin())) {
        std::cerr << "failed" << std::endl;
        std::cerr << "want:" << result << ", got: " << y[0] << std::endl;
        exit(1);
    }
    std::cout << "OK all_reduce(dtype="
              << "float"
              << ", count=" << n << ")" << std::endl;
}

int main()
{
    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session sess = peer.join();
    auto counts = {
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 100, 1024,
    };
    for (auto count : counts) { test_all_reduce(sess, count); }
    return 0;
}
