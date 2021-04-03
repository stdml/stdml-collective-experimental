#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <numeric>
#include <string>
#include <thread>
#include <vector>

#include <stdml/collective>

template <typename T>
void pprint(const std::vector<T> &xs)
{
    int i = 0;
    for (const T &x : xs) {
        if (i++) {
            std::cout << ", ";
        }
        std::cout << x;
    }
    std::cout << std::endl;
}

void example_1()
{
    auto peer = stdml::collective::peer::from_env();
    std::cout << "peer created" << std::endl;

    stdml::collective::session session = peer.join();
    std::cout << "session joined, rank: " << session.rank()
              << ", size: " << session.size() << std::endl;
    int np = session.size();
    int answer = np * (np - 1) / 2;
    const int n = 10;
    std::vector<int> x(n);
    std::vector<int> y(n);
    std::fill(x.begin(), x.end(), session.rank());
    std::fill(y.begin(), y.end(), -10);
    session.all_reduce(x.data(), x.data() + x.size(), y.data());
    if (y[0] != answer) {
        printf("wrong answer: %d, expect: %d\n", y[0], answer);
        exit(1);
    }
    printf("answer is correct\n");
}

int main()
{
    example_1();
    std::cout << "example finished" << std::endl;
    return 0;
}
