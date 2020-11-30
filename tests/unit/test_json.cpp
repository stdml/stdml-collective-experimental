#include <experimental/net>
#include <iostream>
#include <string>

#include <stdml/bits/collective/address.hpp>

int main()
{
    auto rs = stdml::collective::peer_list::gen(10);
    auto ws = stdml::collective::peer_list::gen(10);
    stdml::collective::cluster_config config(rs, ws);
    std::cout << config.json() << std::endl;
    return 0;
}
