#include <numeric>
#include <vector>

#include <stdml/collective>

int main()
{
    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session sess = peer.join();

    if (sess.size() > 1) {
        std::vector<int> a(10);
        std::fill(a.begin(), a.end(), 0);
        if (sess.rank() == 0) {
            std::iota(a.begin(), a.end(), 0);
            sess.send(a.data(), a.size(), stdml::collective::type<int>(), 1);
        } else if (sess.rank() == 1) {
            sess.recv(a.data(), a.size(), stdml::collective::type<int>(), 0);
            for (auto i : a) {
                std::cout << i << std::endl;
            }
        }
    }
    return 0;
}
