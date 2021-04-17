#include <stdml/collective>

#include <std/ranges>

int test_1(stdml::collective::session &sess, int count)
{
    std::vector<int> x(count);
    std::vector<int> y(x.size() * sess.size());
    std::vector<int> z(y.size());

    std::fill(x.begin(), x.end(), sess.rank());
    std::fill(y.begin(), y.end(), -1);
    for (auto i : std::views::iota((size_t)0, z.size())) {
        z[i] = i / x.size();
    }

    sess.all_gather(x.data(), y.data(), x.size());
    if (y != z) {
        return 1;
    }
    return 0;
}

int main()
{
    {
        auto peer = stdml::collective::peer::from_env();
        stdml::collective::session sess = peer.join();
        int failed = 0;
        failed += test_1(sess, 1);
        failed += test_1(sess, 2);
        failed += test_1(sess, 10);
        if (failed) {
            return 1;
        }
    }
    std::cout << "all test passed." << std::endl;
    return 0;
}
