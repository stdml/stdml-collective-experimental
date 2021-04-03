#pragma once
#include <execution>
#if __has_include(<execution>) && !defined(__APPLE__)
    // FIXME: use system <execution> on APPLE
    #include <execution>
#else

namespace std::execution
{
struct parallel_policy {
};

inline parallel_policy par;

struct sequenced_policy {
};

inline sequenced_policy seq;
}  // namespace std::execution
#endif
