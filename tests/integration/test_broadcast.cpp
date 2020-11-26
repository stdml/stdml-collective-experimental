#include <stdml/collective>

void test_1(stdml::collective::session &sess)
{
    int x = sess.rank();
    std::cout << "before: " << x << std::endl;
    sess.broadcast(x);
    std::cout << "after: " << x << std::endl;
}

int main()
{
    auto peer = stdml::collective::peer::from_env();
    stdml::collective::session sess = peer.join();
    test_1(sess);
    return 0;
}
