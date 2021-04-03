#include <stdml/collective>

int test_1(stdml::collective::session &sess)
{
    int x = sess.rank();
    x = sess.broadcast(x);
    if (x != 0) {
        return 1;
    }
    return 0;
}

int main()
{
    {
        auto peer = stdml::collective::peer::from_env();
        stdml::collective::session sess = peer.join();
        int failed = test_1(sess);
        if (failed) {
            return 1;
        }
    }
    std::cout << "all test passed." << std::endl;
    return 0;
}
