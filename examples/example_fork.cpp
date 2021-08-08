#include <numeric>
#include <vector>

#include <stdml/collective>

void f(stdml::collective::session &sess)
{
    std::cout << "rank: " << sess.rank() << " size: " << sess.size()
              << std::endl;

    std::vector<int> x(10);
    std::vector<int> y(x.size());

    std::iota(x.begin(), x.end(), 1);
    std::fill(y.begin(), y.end(), 0);
    sess.all_reduce(x.data(), x.data() + x.size(), y.data());
    std::cout << y[0] << std::endl;
}

int main()
{
    stdml::collective::prun(4, f);
    return 0;
}
