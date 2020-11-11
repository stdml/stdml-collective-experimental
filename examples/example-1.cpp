#include <vector>

#include <stdml/collective>

int main()
{
    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session session = peer.join();

    const int n = 1 << 10;
    std::vector<float> x(n);
    std::vector<float> y(n);
    session.all_reduce(x.data(), x.data() + x.size(), y.data());
    return 0;
}
