#include <vector>

#include <stdml/collective>

int main()
{
    stdml::collective::peer peer;
    stdml::collective::session session = peer.join();

    const int n = 1 << 10;
    std::vector<float> x(n);
    std::vector<float> y(n);
    session.all_reduce_sum_f32(x.data(), x.data() + x.size(), y.data());
    return 0;
}
